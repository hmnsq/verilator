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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkResolve.h"

#অন্তর্ভাব "V3String.h"
#অন্তর্ভাব "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

শ্রেণী LinkResolveVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    ধ্রুবক VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    পূর্ণসংখ্যা m_senitem_Cvt_অঙ্ক = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    যৌক্তিক m_underGenerate = মিথ্যা;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        যদি (nodep->dead()) ফেরত;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_অঙ্ক);
        m_modp = nodep;
        m_senitem_Cvt_অঙ্ক = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        জন্য (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                যদি (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        যদি (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(AstVar* nodep) override {
        iterateChildren(nodep);
        যদি (m_ftaskp) nodep->funcLocal(সত্য);
        যদি (nodep->isSigModPublic()) {
            nodep->sigModPublic(মিথ্যা);  // We're done with this attribute
            m_modp->modPublic(সত্য);  // Avoid flattening if signals are exposed
        }
    }

    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(সত্য);
            // Look for where genvar is valid
            যৌক্তিক ok = মিথ্যা;
            // cppcheck-suppress constVariablePointer
            জন্য (AstGenFor* ধ্রুবক forp : m_underGenFors) {
                যদি (ok) বিরতি;
                যদি (forp->initsp())
                    forp->initsp()->foreach([&](ধ্রুবক AstVarRef* refp) {  //
                        যদি (refp->varp() == nodep->varp()) ok = সত্য;
                    });
            }
            যদি (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        যদি (m_underGenerate) nodep->underGenerate(সত্য);
        // Remember the existing symbol table scope
        যদি (m_classp) nodep->classMethod(সত্য);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(মিথ্যা);
        যদি (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        যদি (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ধ্রুবক fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_গ্রন্থি* ধ্রুবক crstp = নতুন AstAssign{
                fvarp->fileline(), নতুন AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                নতুন AstCReset{fvarp->fileline(), fvarp, মিথ্যা}};
            fvarp->noCReset(সত্য);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        যদি (nodep->dpiExport()) nodep->scopeNamep(নতুন AstScopeName{nodep->fileline(), মিথ্যা});
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        যদি (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            যদি (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            }
            letp->user2(সত্য);
            যদি (VN_IS(nodep->backp(), Stmt_রাশিমালা)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_রাশিমালা* ধ্রুবক letStmtp = VN_AS(letp->stmtsp(), Stmt_রাশিমালা);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = letStmtp->exprp()->clone_বৃক্ষ(মিথ্যা);
            ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ধ্রুবক AstVar*, Ast_গ্রন্থি_রাশিমালা*> portToExprs;
            জন্য (ধ্রুবক auto& tconnect : tconnects) {
                ধ্রুবক AstVar* ধ্রুবক portp = tconnect.first;
                ধ্রুবক AstArg* ধ্রুবক argp = tconnect.second;
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক pinp = argp->exprp();
                যদি (!pinp) চালু;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ধ্রুবক auto it = portToExprs.find(refp->varp());
                যদি (it != portToExprs.end()) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_বৃক্ষ(মিথ্যা));
                    VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(মিথ্যা);
            ফেরত;
        }
    }

    শূন্য visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstStmtPragma* nodep) override {
        যদি (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            যদি (!v3Global.opt.coverage_সারি() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                ফেরত;
            }
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstPragma* nodep) override {
        যদি (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(সত্য);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(সত্য);
            m_modp->modPublic(সত্য);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(সত্য);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_গ্রন্থি* nodep, ধ্রুবক string& format, Ast_গ্রন্থি* argp, যৌক্তিক isScan) {
        // Check display arguments, return new format string
        string newFormat;
        যৌক্তিক inPct = মিথ্যা;
        যৌক্তিক inIgnore = মিথ্যা;
        string fmt;
        string parseFormat = format;
        যখন (!parseFormat.empty() || argp) {
            জন্য (ধ্রুবক অক্ষর ch : parseFormat) {
                যদি (!inPct && ch == '%') {
                    inPct = সত্য;
                    inIgnore = মিথ্যা;
                    fmt = ch;
                } অন্যথা যদি (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } অন্যথা যদি (inPct) {
                    inPct = মিথ্যা;
                    fmt += ch;
                    নির্বাচন (std::tolower(ch)) {
                    ক্ষেত্র '%':  // %% - just output a %
                        বিরতি;
                    ক্ষেত্র '*':
                        inPct = সত্য;
                        inIgnore = সত্য;
                        বিরতি;
                    ক্ষেত্র 'm':  // %m - auto insert "name"
                        যদি (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        বিরতি;
                    ক্ষেত্র 'l':  // %l - auto insert "library"
                        যদি (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        যদি (m_modp)
                            fmt = Ast_গ্রন্থি::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        বিরতি;
                    পূর্বনির্ধারিত:  // Most operators, just move to next argument
                        যদি (!V3_সংখ্যা::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } অন্যথা যদি (!inIgnore) {
                            যদি (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } অন্যথা {
                                argp = argp->nextp();
                            }
                        }
                        বিরতি;
                    }  // switch
                    newFormat += fmt;
                } অন্যথা {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            যদি (isScan) বিরতি;
            যখন (argp) {
                ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(argp, Const);
                ধ্রুবক যৌক্তিক isFromString = (constp) ? constp->অঙ্ক().isFromString() : মিথ্যা;
                যদি (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } অন্যথা {  // New format string
                    parseFormat += constp->অঙ্ক().toString();
                    Ast_গ্রন্থি* ধ্রুবক nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(ঠেলো_Deletep(argp), argp);
                    argp = nextp;
                    বিরতি;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        ফেরত newFormat;
    }

    স্থির শূন্য expectDescriptor(Ast_গ্রন্থি* /*nodep*/, ধ্রুবক Ast_গ্রন্থি_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        যদি (filep && filep->varp()) filep->varp()->attrFileDescr(সত্য);
    }

    শূন্য visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), গ্রন্থি_Var_Ref));
    }
    শূন্য visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), গ্রন্থি_Var_Ref));
    }
    শূন্য visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), গ্রন্থি_Var_Ref));
    }
    শূন্য visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), গ্রন্থি_Var_Ref));
    }
    শূন্য visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), সত্য);
    }
    শূন্য visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), সত্য);
    }
    শূন্য visit(AstSFormatF* nodep) override {
        যদি (nodep->user2SetOnce()) ফেরত;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        যদি (nodep->রাশিমালা_Format_2()) {
            যদি (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->অঙ্ক().isFromString()) {
                AstConst* ধ্রুবক fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->অঙ্ক().toString());
                nodep->রাশিমালা_Format_2(মিথ্যা);
                VL_DO_DANGLING(ঠেলো_Deletep(fmtp), fmtp);
            }
        }
        যদি (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(মিথ্যা);
            nodep->রাশিমালা_Format_2(মিথ্যা);
        }
        যদি ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(নতুন AstScopeName{nodep->fileline(), সত্য});
        }
        যদি (!nodep->রাশিমালা_Format_2()) {
            ধ্রুবক string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), মিথ্যা);
            nodep->text(newFormat);
        }
    }

    শূন্য visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        যদি (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } অন্যথা {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ধ্রুবক AstVar* varoutp = nullptr;
            জন্য (Ast_গ্রন্থি* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                    যদি (varp->isReadOnly()) {
                    } অন্যথা যদি (varp->isWritable()) {
                        যদি (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* ধ্রুবক ap
                            = নতুন AstAssignW{varp->fileline(),
                                             নতুন AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             নতুন AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(নতুন AstAlways{ap});
                    } অন্যথা {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    শূন্য visit(AstSystemCSection* nodep) override {
        নির্বাচন (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        ক্ষেত্র VSystemCSectionType::CTOR:
        ক্ষেত্র VSystemCSectionType::DTOR:
        ক্ষেত্র VSystemCSectionType::INT: m_modp->modPublic(সত্য); বিরতি;
        পূর্বনির্ধারিত: বিরতি;
        }
        // Has no children
    }

    শূন্য visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    শূন্য visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = সত্য;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.বের_করো_back();
    }
    শূন্য visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = সত্য;
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

শ্রেণী LinkBotupVisitor final : সর্বজনীন VNVisitorConst {
    // STATE
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        যদি (nodep->modp()->modPublic()) m_modp->modPublic(সত্য);
        //** No iteration for speed
    }
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// V3LinkResolve class functions

শূন্য V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ধ্রুবক LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("linkresolve", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
