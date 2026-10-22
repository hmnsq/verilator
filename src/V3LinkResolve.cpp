// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Resolve module/signal name references
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// LinkResolve TRANSFORMATIONS:
//      Top-down traversal
//          Extracts:
//              Add SUB so that we subtract off the "base 0-start" of the array
//          SelBit: Convert to ArraySel
//              Add SUB so that we subtract off the "base 0-start" of the array
//          File operations
//              Convert normal var to FILE* type
//          SenItems: Convert pos/negedge of non-simple signals to temporaries
//*************************************************************************

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkResolve.h"

#അന്തര്ഭാവ "V3String.h"
#അന്തര്ഭാവ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

വർഗ്ഗം LinkResolveVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    പൂർണ്ണസംഖ്യ m_senitem_Cvt_അക്കം = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    തർക്കം m_underGenerate = അസത്യം;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        എങ്കിൽ (nodep->dead()) തിരികെ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_അക്കം);
        m_modp = nodep;
        m_senitem_Cvt_അക്കം = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        എങ്കിൽ (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(AstVar* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_ftaskp) nodep->funcLocal(സത്യം);
        എങ്കിൽ (nodep->isSigModPublic()) {
            nodep->sigModPublic(അസത്യം);  // We're done with this attribute
            m_modp->modPublic(സത്യം);  // Avoid flattening if signals are exposed
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(സത്യം);
            // Look for where genvar is valid
            തർക്കം ok = അസത്യം;
            // cppcheck-suppress constVariablePointer
            വേണ്ടി (AstGenFor* സ്ഥിരം forp : m_underGenFors) {
                എങ്കിൽ (ok) നിർത്തുക;
                എങ്കിൽ (forp->initsp())
                    forp->initsp()->foreach([&](സ്ഥിരം AstVarRef* refp) {  //
                        എങ്കിൽ (refp->varp() == nodep->varp()) ok = സത്യം;
                    });
            }
            എങ്കിൽ (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        എങ്കിൽ (m_underGenerate) nodep->underGenerate(സത്യം);
        // Remember the existing symbol table scope
        എങ്കിൽ (m_classp) nodep->classMethod(സത്യം);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(അസത്യം);
        എങ്കിൽ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        എങ്കിൽ (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* സ്ഥിരം fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_കണ്ണി* സ്ഥിരം crstp = പുതിയ AstAssign{
                fvarp->fileline(), പുതിയ AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                പുതിയ AstCReset{fvarp->fileline(), fvarp, അസത്യം}};
            fvarp->noCReset(സത്യം);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        എങ്കിൽ (nodep->dpiExport()) nodep->scopeNamep(പുതിയ AstScopeName{nodep->fileline(), അസത്യം});
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            എങ്കിൽ (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }
            letp->user2(സത്യം);
            എങ്കിൽ (VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം letStmtp = VN_AS(letp->stmtsp(), Stmt_വ്യഞ്ജകം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = letStmtp->exprp()->clone_വൃക്ഷം(അസത്യം);
            സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<സ്ഥിരം AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*> portToExprs;
            വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
                സ്ഥിരം AstVar* സ്ഥിരം portp = tconnect.first;
                സ്ഥിരം AstArg* സ്ഥിരം argp = tconnect.second;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinp = argp->exprp();
                എങ്കിൽ (!pinp) തുടരുക;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                സ്ഥിരം auto it = portToExprs.find(refp->varp());
                എങ്കിൽ (it != portToExprs.end()) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_വൃക്ഷം(അസത്യം));
                    VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(അസത്യം);
            തിരികെ;
        }
    }

    ശൂന്യം visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstStmtPragma* nodep) override {
        എങ്കിൽ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            എങ്കിൽ (!v3Global.opt.coverage_വരി() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstPragma* nodep) override {
        എങ്കിൽ (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(സത്യം);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(സത്യം);
            m_modp->modPublic(സത്യം);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(സത്യം);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_കണ്ണി* nodep, സ്ഥിരം string& format, Ast_കണ്ണി* argp, തർക്കം isScan) {
        // Check display arguments, return new format string
        string newFormat;
        തർക്കം inPct = അസത്യം;
        തർക്കം inIgnore = അസത്യം;
        string fmt;
        string parseFormat = format;
        ആകുമ്പോൾ (!parseFormat.empty() || argp) {
            വേണ്ടി (സ്ഥിരം അക്ഷരം ch : parseFormat) {
                എങ്കിൽ (!inPct && ch == '%') {
                    inPct = സത്യം;
                    inIgnore = അസത്യം;
                    fmt = ch;
                } അല്ലെങ്കിൽ എങ്കിൽ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } അല്ലെങ്കിൽ എങ്കിൽ (inPct) {
                    inPct = അസത്യം;
                    fmt += ch;
                    തിരഞ്ഞെടുപ്പ് (std::tolower(ch)) {
                    അവസ്ഥ '%':  // %% - just output a %
                        നിർത്തുക;
                    അവസ്ഥ '*':
                        inPct = സത്യം;
                        inIgnore = സത്യം;
                        നിർത്തുക;
                    അവസ്ഥ 'm':  // %m - auto insert "name"
                        എങ്കിൽ (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        നിർത്തുക;
                    അവസ്ഥ 'l':  // %l - auto insert "library"
                        എങ്കിൽ (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        എങ്കിൽ (m_modp)
                            fmt = Ast_കണ്ണി::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        നിർത്തുക;
                    സ്ഥിരസ്ഥിതി:  // Most operators, just move to next argument
                        എങ്കിൽ (!V3_സംഖ്യ::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } അല്ലെങ്കിൽ എങ്കിൽ (!inIgnore) {
                            എങ്കിൽ (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } അല്ലെങ്കിൽ {
                                argp = argp->nextp();
                            }
                        }
                        നിർത്തുക;
                    }  // switch
                    newFormat += fmt;
                } അല്ലെങ്കിൽ {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            എങ്കിൽ (isScan) നിർത്തുക;
            ആകുമ്പോൾ (argp) {
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(argp, Const);
                സ്ഥിരം തർക്കം isFromString = (constp) ? constp->അക്കം().isFromString() : അസത്യം;
                എങ്കിൽ (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } അല്ലെങ്കിൽ {  // New format string
                    parseFormat += constp->അക്കം().toString();
                    Ast_കണ്ണി* സ്ഥിരം nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(തള്ളുക_Deletep(argp), argp);
                    argp = nextp;
                    നിർത്തുക;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        തിരികെ newFormat;
    }

    സ്ഥിതം ശൂന്യം expectDescriptor(Ast_കണ്ണി* /*nodep*/, സ്ഥിരം Ast_കണ്ണി_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        എങ്കിൽ (filep && filep->varp()) filep->varp()->attrFileDescr(സത്യം);
    }

    ശൂന്യം visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), കണ്ണി_Var_Ref));
    }
    ശൂന്യം visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), കണ്ണി_Var_Ref));
    }
    ശൂന്യം visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), കണ്ണി_Var_Ref));
    }
    ശൂന്യം visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), കണ്ണി_Var_Ref));
    }
    ശൂന്യം visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), സത്യം);
    }
    ശൂന്യം visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), സത്യം);
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        എങ്കിൽ (nodep->user2SetOnce()) തിരികെ;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        എങ്കിൽ (nodep->വ്യഞ്ജകം_Format_2()) {
            എങ്കിൽ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->അക്കം().isFromString()) {
                AstConst* സ്ഥിരം fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->അക്കം().toString());
                nodep->വ്യഞ്ജകം_Format_2(അസത്യം);
                VL_DO_DANGLING(തള്ളുക_Deletep(fmtp), fmtp);
            }
        }
        എങ്കിൽ (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(അസത്യം);
            nodep->വ്യഞ്ജകം_Format_2(അസത്യം);
        }
        എങ്കിൽ ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(പുതിയ AstScopeName{nodep->fileline(), സത്യം});
        }
        എങ്കിൽ (!nodep->വ്യഞ്ജകം_Format_2()) {
            സ്ഥിരം string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), അസത്യം);
            nodep->text(newFormat);
        }
    }

    ശൂന്യം visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        എങ്കിൽ (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } അല്ലെങ്കിൽ {
            // Massive hack, just tie off all outputs so our analysis can proceed
            സ്ഥിരം AstVar* varoutp = nullptr;
            വേണ്ടി (Ast_കണ്ണി* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                    എങ്കിൽ (varp->isReadOnly()) {
                    } അല്ലെങ്കിൽ എങ്കിൽ (varp->isWritable()) {
                        എങ്കിൽ (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* സ്ഥിരം ap
                            = പുതിയ AstAssignW{varp->fileline(),
                                             പുതിയ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             പുതിയ AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(പുതിയ AstAlways{ap});
                    } അല്ലെങ്കിൽ {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    ശൂന്യം visit(AstSystemCSection* nodep) override {
        തിരഞ്ഞെടുപ്പ് (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        അവസ്ഥ VSystemCSectionType::CTOR:
        അവസ്ഥ VSystemCSectionType::DTOR:
        അവസ്ഥ VSystemCSectionType::INT: m_modp->modPublic(സത്യം); നിർത്തുക;
        സ്ഥിരസ്ഥിതി: നിർത്തുക;
        }
        // Has no children
    }

    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    ശൂന്യം visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = സത്യം;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.എടുക്കുക_back();
    }
    ശൂന്യം visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = സത്യം;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

വർഗ്ഗം LinkBotupVisitor final : പൊതു VNVisitorConst {
    // STATE
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        എങ്കിൽ (nodep->modp()->modPublic()) m_modp->modPublic(സത്യം);
        //** No iteration for speed
    }
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// V3LinkResolve class functions

ശൂന്യം V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        സ്ഥിരം LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("linkresolve", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
