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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkResolve.h"

#ଅନ୍ତର୍ଭାଵ "V3String.h"
#ଅନ୍ତର୍ଭାଵ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ଶ୍ରେଣୀ LinkResolveVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_senitem_Cvt_ଅଙ୍କ = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    ତାର୍କିକ m_underGenerate = ମିଥ୍ୟା;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        ଯଦି (nodep->dead()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_ଅଙ୍କ);
        m_modp = nodep;
        m_senitem_Cvt_ଅଙ୍କ = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        ଯଦି (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_ftaskp) nodep->funcLocal(ସତ୍ୟ);
        ଯଦି (nodep->isSigModPublic()) {
            nodep->sigModPublic(ମିଥ୍ୟା);  // We're done with this attribute
            m_modp->modPublic(ସତ୍ୟ);  // Avoid flattening if signals are exposed
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(ସତ୍ୟ);
            // Look for where genvar is valid
            ତାର୍କିକ ok = ମିଥ୍ୟା;
            // cppcheck-suppress constVariablePointer
            ପାଇଁ (AstGenFor* ଅଚଳ forp : m_underGenFors) {
                ଯଦି (ok) ଭାଙ୍ଗ;
                ଯଦି (forp->initsp())
                    forp->initsp()->foreach([&](ଅଚଳ AstVarRef* refp) {  //
                        ଯଦି (refp->varp() == nodep->varp()) ok = ସତ୍ୟ;
                    });
            }
            ଯଦି (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        ଯଦି (m_underGenerate) nodep->underGenerate(ସତ୍ୟ);
        // Remember the existing symbol table scope
        ଯଦି (m_classp) nodep->classMethod(ସତ୍ୟ);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ମିଥ୍ୟା);
        ଯଦି (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        ଯଦି (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ଅଚଳ fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_ଗଣ୍ଠି* ଅଚଳ crstp = ନୂତନ AstAssign{
                fvarp->fileline(), ନୂତନ AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                ନୂତନ AstCReset{fvarp->fileline(), fvarp, ମିଥ୍ୟା}};
            fvarp->noCReset(ସତ୍ୟ);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        ଯଦି (nodep->dpiExport()) nodep->scopeNamep(ନୂତନ AstScopeName{nodep->fileline(), ମିଥ୍ୟା});
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ଯଦି (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            ଯଦି (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
            letp->user2(ସତ୍ୟ);
            ଯଦି (VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ letStmtp = VN_AS(letp->stmtsp(), Stmt_ପଦସମଷ୍ଟି);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = letStmtp->exprp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ଅଚଳ AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> portToExprs;
            ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
                ଅଚଳ AstVar* ଅଚଳ portp = tconnect.first;
                ଅଚଳ AstArg* ଅଚଳ argp = tconnect.second;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ pinp = argp->exprp();
                ଯଦି (!pinp) ଜାରି;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ଅଚଳ auto it = portToExprs.find(refp->varp());
                ଯଦି (it != portToExprs.end()) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(ମିଥ୍ୟା);
            ଫେରନ୍ତୁ;
        }
    }

    ଶୂନ୍ୟ visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstStmtPragma* nodep) override {
        ଯଦି (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            ଯଦି (!v3Global.opt.coverage_ଧାଡ଼ି() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstPragma* nodep) override {
        ଯଦି (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(ସତ୍ୟ);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(ସତ୍ୟ);
            m_modp->modPublic(ସତ୍ୟ);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(ସତ୍ୟ);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& format, Ast_ଗଣ୍ଠି* argp, ତାର୍କିକ isScan) {
        // Check display arguments, return new format string
        string newFormat;
        ତାର୍କିକ inPct = ମିଥ୍ୟା;
        ତାର୍କିକ inIgnore = ମିଥ୍ୟା;
        string fmt;
        string parseFormat = format;
        ଯେତେବେଳେ (!parseFormat.empty() || argp) {
            ପାଇଁ (ଅଚଳ ଅକ୍ଷର ch : parseFormat) {
                ଯଦି (!inPct && ch == '%') {
                    inPct = ସତ୍ୟ;
                    inIgnore = ମିଥ୍ୟା;
                    fmt = ch;
                } ଅନ୍ୟଥା ଯଦି (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ଅନ୍ୟଥା ଯଦି (inPct) {
                    inPct = ମିଥ୍ୟା;
                    fmt += ch;
                    ବାଛ (std::tolower(ch)) {
                    ସ୍ଥିତି '%':  // %% - just output a %
                        ଭାଙ୍ଗ;
                    ସ୍ଥିତି '*':
                        inPct = ସତ୍ୟ;
                        inIgnore = ସତ୍ୟ;
                        ଭାଙ୍ଗ;
                    ସ୍ଥିତି 'm':  // %m - auto insert "name"
                        ଯଦି (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        ଭାଙ୍ଗ;
                    ସ୍ଥିତି 'l':  // %l - auto insert "library"
                        ଯଦି (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        ଯଦି (m_modp)
                            fmt = Ast_ଗଣ୍ଠି::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        ଭାଙ୍ଗ;
                    ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // Most operators, just move to next argument
                        ଯଦି (!V3_ସଂଖ୍ୟା::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } ଅନ୍ୟଥା ଯଦି (!inIgnore) {
                            ଯଦି (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } ଅନ୍ୟଥା {
                                argp = argp->nextp();
                            }
                        }
                        ଭାଙ୍ଗ;
                    }  // switch
                    newFormat += fmt;
                } ଅନ୍ୟଥା {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            ଯଦି (isScan) ଭାଙ୍ଗ;
            ଯେତେବେଳେ (argp) {
                ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(argp, Const);
                ଅଚଳ ତାର୍କିକ isFromString = (constp) ? constp->ଅଙ୍କ().isFromString() : ମିଥ୍ୟା;
                ଯଦି (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } ଅନ୍ୟଥା {  // New format string
                    parseFormat += constp->ଅଙ୍କ().toString();
                    Ast_ଗଣ୍ଠି* ଅଚଳ nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(ଠେଲ_Deletep(argp), argp);
                    argp = nextp;
                    ଭାଙ୍ଗ;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        ଫେରନ୍ତୁ newFormat;
    }

    ସ୍ଥିର ଶୂନ୍ୟ expectDescriptor(Ast_ଗଣ୍ଠି* /*nodep*/, ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        ଯଦି (filep && filep->varp()) filep->varp()->attrFileDescr(ସତ୍ୟ);
    }

    ଶୂନ୍ୟ visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ଗଣ୍ଠି_Var_Ref));
    }
    ଶୂନ୍ୟ visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ଗଣ୍ଠି_Var_Ref));
    }
    ଶୂନ୍ୟ visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ଗଣ୍ଠି_Var_Ref));
    }
    ଶୂନ୍ୟ visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ଗଣ୍ଠି_Var_Ref));
    }
    ଶୂନ୍ୟ visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        ଯଦି (nodep->user2SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        ଯଦି (nodep->ପଦସମଷ୍ଟି_Format_2()) {
            ଯଦି (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->ଅଙ୍କ().isFromString()) {
                AstConst* ଅଚଳ fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->ଅଙ୍କ().toString());
                nodep->ପଦସମଷ୍ଟି_Format_2(ମିଥ୍ୟା);
                VL_DO_DANGLING(ଠେଲ_Deletep(fmtp), fmtp);
            }
        }
        ଯଦି (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(ମିଥ୍ୟା);
            nodep->ପଦସମଷ୍ଟି_Format_2(ମିଥ୍ୟା);
        }
        ଯଦି ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(ନୂତନ AstScopeName{nodep->fileline(), ସତ୍ୟ});
        }
        ଯଦି (!nodep->ପଦସମଷ୍ଟି_Format_2()) {
            ଅଚଳ string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), ମିଥ୍ୟା);
            nodep->text(newFormat);
        }
    }

    ଶୂନ୍ୟ visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        ଯଦି (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } ଅନ୍ୟଥା {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ଅଚଳ AstVar* varoutp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                    ଯଦି (varp->isReadOnly()) {
                    } ଅନ୍ୟଥା ଯଦି (varp->isWritable()) {
                        ଯଦି (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* ଅଚଳ ap
                            = ନୂତନ AstAssignW{varp->fileline(),
                                             ନୂତନ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             ନୂତନ AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(ନୂତନ AstAlways{ap});
                    } ଅନ୍ୟଥା {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstSystemCSection* nodep) override {
        ବାଛ (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        ସ୍ଥିତି VSystemCSectionType::CTOR:
        ସ୍ଥିତି VSystemCSectionType::DTOR:
        ସ୍ଥିତି VSystemCSectionType::INT: m_modp->modPublic(ସତ୍ୟ); ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଭାଙ୍ଗ;
        }
        // Has no children
    }

    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    ଶୂନ୍ୟ visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ସତ୍ୟ;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.କାଢ଼_back();
    }
    ଶୂନ୍ୟ visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ସତ୍ୟ;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

ଶ୍ରେଣୀ LinkBotupVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        ଯଦି (nodep->modp()->modPublic()) m_modp->modPublic(ସତ୍ୟ);
        //** No iteration for speed
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// V3LinkResolve class functions

ଶୂନ୍ୟ V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ଅଚଳ LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linkresolve", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
