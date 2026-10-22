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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkResolve.h"

#અન્તર્ભાવ "V3String.h"
#અન્તર્ભાવ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

વર્ગ LinkResolveVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    અચલ VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    પૂર્ણાંક m_senitem_Cvt_અંક = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    તાર્કિક m_underGenerate = ખોટું;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        જો (nodep->dead()) પાછા;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_અંક);
        m_modp = nodep;
        m_senitem_Cvt_અંક = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        જો (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(AstVar* nodep) override {
        iterateChildren(nodep);
        જો (m_ftaskp) nodep->funcLocal(સાચું);
        જો (nodep->isSigModPublic()) {
            nodep->sigModPublic(ખોટું);  // We're done with this attribute
            m_modp->modPublic(સાચું);  // Avoid flattening if signals are exposed
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(સાચું);
            // Look for where genvar is valid
            તાર્કિક ok = ખોટું;
            // cppcheck-suppress constVariablePointer
            માટે (AstGenFor* અચલ forp : m_underGenFors) {
                જો (ok) તોડો;
                જો (forp->initsp())
                    forp->initsp()->foreach([&](અચલ AstVarRef* refp) {  //
                        જો (refp->varp() == nodep->varp()) ok = સાચું;
                    });
            }
            જો (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        જો (m_underGenerate) nodep->underGenerate(સાચું);
        // Remember the existing symbol table scope
        જો (m_classp) nodep->classMethod(સાચું);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ખોટું);
        જો (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        જો (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* અચલ fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_ગાંઠ* અચલ crstp = નવું AstAssign{
                fvarp->fileline(), નવું AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                નવું AstCReset{fvarp->fileline(), fvarp, ખોટું}};
            fvarp->noCReset(સાચું);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        જો (nodep->dpiExport()) nodep->scopeNamep(નવું AstScopeName{nodep->fileline(), ખોટું});
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        જો (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            જો (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            }
            letp->user2(સાચું);
            જો (VN_IS(nodep->backp(), Stmt_પદાવલિ)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_પદાવલિ* અચલ letStmtp = VN_AS(letp->stmtsp(), Stmt_પદાવલિ);
            Ast_ગાંઠ_પદાવલિ* અચલ newp = letStmtp->exprp()->clone_વૃક્ષ(ખોટું);
            અચલ V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<અચલ AstVar*, Ast_ગાંઠ_પદાવલિ*> portToExprs;
            માટે (અચલ auto& tconnect : tconnects) {
                અચલ AstVar* અચલ portp = tconnect.first;
                અચલ AstArg* અચલ argp = tconnect.second;
                Ast_ગાંઠ_પદાવલિ* અચલ pinp = argp->exprp();
                જો (!pinp) ચાલુ;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                અચલ auto it = portToExprs.find(refp->varp());
                જો (it != portToExprs.end()) {
                    Ast_ગાંઠ_પદાવલિ* અચલ pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_વૃક્ષ(ખોટું));
                    VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(ખોટું);
            પાછા;
        }
    }

    શૂન્ય visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstStmtPragma* nodep) override {
        જો (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            જો (!v3Global.opt.coverage_લીટી() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstPragma* nodep) override {
        જો (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર જો (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(સાચું);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર જો (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(સાચું);
            m_modp->modPublic(સાચું);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર જો (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(સાચું);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_ગાંઠ* nodep, અચલ string& format, Ast_ગાંઠ* argp, તાર્કિક isScan) {
        // Check display arguments, return new format string
        string newFormat;
        તાર્કિક inPct = ખોટું;
        તાર્કિક inIgnore = ખોટું;
        string fmt;
        string parseFormat = format;
        જ્યારે (!parseFormat.empty() || argp) {
            માટે (અચલ અક્ષર ch : parseFormat) {
                જો (!inPct && ch == '%') {
                    inPct = સાચું;
                    inIgnore = ખોટું;
                    fmt = ch;
                } નહીંતર જો (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } નહીંતર જો (inPct) {
                    inPct = ખોટું;
                    fmt += ch;
                    પસંદગી (std::tolower(ch)) {
                    સ્થિતિ '%':  // %% - just output a %
                        તોડો;
                    સ્થિતિ '*':
                        inPct = સાચું;
                        inIgnore = સાચું;
                        તોડો;
                    સ્થિતિ 'm':  // %m - auto insert "name"
                        જો (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        તોડો;
                    સ્થિતિ 'l':  // %l - auto insert "library"
                        જો (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        જો (m_modp)
                            fmt = Ast_ગાંઠ::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        તોડો;
                    મૂળભૂત:  // Most operators, just move to next argument
                        જો (!V3_સંખ્યા::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } નહીંતર જો (!inIgnore) {
                            જો (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } નહીંતર {
                                argp = argp->nextp();
                            }
                        }
                        તોડો;
                    }  // switch
                    newFormat += fmt;
                } નહીંતર {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            જો (isScan) તોડો;
            જ્યારે (argp) {
                અચલ AstConst* અચલ constp = VN_CAST(argp, Const);
                અચલ તાર્કિક isFromString = (constp) ? constp->અંક().isFromString() : ખોટું;
                જો (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } નહીંતર {  // New format string
                    parseFormat += constp->અંક().toString();
                    Ast_ગાંઠ* અચલ nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(ધકેલો_Deletep(argp), argp);
                    argp = nextp;
                    તોડો;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        પાછા newFormat;
    }

    સ્થિર શૂન્ય expectDescriptor(Ast_ગાંઠ* /*nodep*/, અચલ Ast_ગાંઠ_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        જો (filep && filep->varp()) filep->varp()->attrFileDescr(સાચું);
    }

    શૂન્ય visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ગાંઠ_Var_Ref));
    }
    શૂન્ય visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ગાંઠ_Var_Ref));
    }
    શૂન્ય visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ગાંઠ_Var_Ref));
    }
    શૂન્ય visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ગાંઠ_Var_Ref));
    }
    શૂન્ય visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), સાચું);
    }
    શૂન્ય visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), સાચું);
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        જો (nodep->user2SetOnce()) પાછા;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        જો (nodep->પદાવલિ_Format_2()) {
            જો (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->અંક().isFromString()) {
                AstConst* અચલ fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->અંક().toString());
                nodep->પદાવલિ_Format_2(ખોટું);
                VL_DO_DANGLING(ધકેલો_Deletep(fmtp), fmtp);
            }
        }
        જો (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(ખોટું);
            nodep->પદાવલિ_Format_2(ખોટું);
        }
        જો ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(નવું AstScopeName{nodep->fileline(), સાચું});
        }
        જો (!nodep->પદાવલિ_Format_2()) {
            અચલ string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), ખોટું);
            nodep->text(newFormat);
        }
    }

    શૂન્ય visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        જો (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } નહીંતર {
            // Massive hack, just tie off all outputs so our analysis can proceed
            અચલ AstVar* varoutp = nullptr;
            માટે (Ast_ગાંઠ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                    જો (varp->isReadOnly()) {
                    } નહીંતર જો (varp->isWritable()) {
                        જો (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* અચલ ap
                            = નવું AstAssignW{varp->fileline(),
                                             નવું AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             નવું AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(નવું AstAlways{ap});
                    } નહીંતર {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }

    શૂન્ય visit(AstSystemCSection* nodep) override {
        પસંદગી (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        સ્થિતિ VSystemCSectionType::CTOR:
        સ્થિતિ VSystemCSectionType::DTOR:
        સ્થિતિ VSystemCSectionType::INT: m_modp->modPublic(સાચું); તોડો;
        મૂળભૂત: તોડો;
        }
        // Has no children
    }

    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    શૂન્ય visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = સાચું;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.કાઢો_back();
    }
    શૂન્ય visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = સાચું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = મૂળભૂત;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

વર્ગ LinkBotupVisitor final : જાહેર VNVisitorConst {
    // STATE
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        જો (nodep->modp()->modPublic()) m_modp->modPublic(સાચું);
        //** No iteration for speed
    }
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = મૂળભૂત;
};

//######################################################################
// V3LinkResolve class functions

શૂન્ય V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        અચલ LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("linkresolve", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
