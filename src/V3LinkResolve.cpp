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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkResolve.h"

#ضمن "V3String.h"
#ضمن "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

صنف LinkResolveVisitor final : عام VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    ثابت VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_عقدة_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_عقدة_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    صحيح m_senitem_Cvt_رقم = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    منطقي m_underGenerate = خطأ;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    فراغ visit(Ast_عقدة_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        إذا (nodep->dead()) إرجاع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_رقم);
        m_modp = nodep;
        m_senitem_Cvt_رقم = 0;
        iterateChildren(nodep);
    }
    فراغ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        لكل (Ast_عقدة* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        إذا (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }
    }
    فراغ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_ftaskp) nodep->funcLocal(true);
        إذا (nodep->isSigModPublic()) {
            nodep->sigModPublic(خطأ);  // We're done with this attribute
            m_modp->modPublic(true);  // Avoid flattening if signals are exposed
        }
    }

    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        إذا (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(true);
            // Look for where genvar is valid
            منطقي ok = خطأ;
            // cppcheck-suppress constVariablePointer
            لكل (AstGenFor* ثابت forp : m_underGenFors) {
                إذا (ok) اكسر;
                إذا (forp->initsp())
                    forp->initsp()->foreach([&](ثابت AstVarRef* refp) {  //
                        إذا (refp->varp() == nodep->varp()) ok = true;
                    });
            }
            إذا (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    فراغ visit(Ast_عقدة_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        إذا (m_underGenerate) nodep->underGenerate(true);
        // Remember the existing symbol table scope
        إذا (m_classp) nodep->classMethod(true);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(خطأ);
        إذا (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_شجرة(), nodep);
            إرجاع;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        إذا (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ثابت fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_عقدة* ثابت crstp = جديد AstAssign{
                fvarp->fileline(), جديد AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                جديد AstCReset{fvarp->fileline(), fvarp, خطأ}};
            fvarp->noCReset(true);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        إذا (nodep->dpiExport()) nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), خطأ});
    }
    فراغ visit(Ast_عقدة_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        إذا (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            إذا (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                إرجاع;
            }
            letp->user2(true);
            إذا (VN_IS(nodep->backp(), Stmt_تعبير)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_تعبير* ثابت letStmtp = VN_AS(letp->stmtsp(), Stmt_تعبير);
            Ast_عقدة_تعبير* ثابت newp = letStmtp->exprp()->clone_شجرة(خطأ);
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ثابت AstVar*, Ast_عقدة_تعبير*> portToExprs;
            لكل (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                Ast_عقدة_تعبير* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ثابت auto it = portToExprs.find(refp->varp());
                إذا (it != portToExprs.end()) {
                    Ast_عقدة_تعبير* ثابت pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_شجرة(خطأ));
                    VL_DO_DANGLING(ادفع_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(خطأ);
            إرجاع;
        }
    }

    فراغ visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    فراغ visit(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            إذا (!v3Global.opt.coverage_سطر() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
                إرجاع;
            }
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(true);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(true);
            m_modp->modPublic(true);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(true);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_عقدة* nodep, ثابت string& format, Ast_عقدة* argp, منطقي isScan) {
        // Check display arguments, return new format string
        string newFormat;
        منطقي inPct = خطأ;
        منطقي inIgnore = خطأ;
        string fmt;
        string parseFormat = format;
        بينما (!parseFormat.empty() || argp) {
            لكل (ثابت حرف ch : parseFormat) {
                إذا (!inPct && ch == '%') {
                    inPct = true;
                    inIgnore = خطأ;
                    fmt = ch;
                } وإلا إذا (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وإلا إذا (inPct) {
                    inPct = خطأ;
                    fmt += ch;
                    بدل (std::tolower(ch)) {
                    حالة '%':  // %% - just output a %
                        اكسر;
                    حالة '*':
                        inPct = true;
                        inIgnore = true;
                        اكسر;
                    حالة 'm':  // %m - auto insert "name"
                        إذا (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        اكسر;
                    حالة 'l':  // %l - auto insert "library"
                        إذا (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        إذا (m_modp)
                            fmt = Ast_عقدة::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        اكسر;
                    افتراضي:  // Most operators, just move to next argument
                        إذا (!V3_العدد::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } وإلا إذا (!inIgnore) {
                            إذا (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } وإلا {
                                argp = argp->nextp();
                            }
                        }
                        اكسر;
                    }  // switch
                    newFormat += fmt;
                } وإلا {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            إذا (isScan) اكسر;
            بينما (argp) {
                ثابت AstConst* ثابت constp = VN_CAST(argp, Const);
                ثابت منطقي isFromString = (constp) ? constp->رقم().isFromString() : خطأ;
                إذا (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } وإلا {  // New format string
                    parseFormat += constp->رقم().toString();
                    Ast_عقدة* ثابت nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(ادفع_Deletep(argp), argp);
                    argp = nextp;
                    اكسر;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        إرجاع newFormat;
    }

    ساكن فراغ expectDescriptor(Ast_عقدة* /*nodep*/, ثابت Ast_عقدة_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        إذا (filep && filep->varp()) filep->varp()->attrFileDescr(true);
    }

    فراغ visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), عقدة_Var_Ref));
    }
    فراغ visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), عقدة_Var_Ref));
    }
    فراغ visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), عقدة_Var_Ref));
    }
    فراغ visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), عقدة_Var_Ref));
    }
    فراغ visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), true);
    }
    فراغ visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), true);
    }
    فراغ visit(AstSFormatF* nodep) override {
        إذا (nodep->user2SetOnce()) إرجاع;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        إذا (nodep->تعبير_Format_2()) {
            إذا (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->رقم().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->رقم().toString());
                nodep->تعبير_Format_2(خطأ);
                VL_DO_DANGLING(ادفع_Deletep(fmtp), fmtp);
            }
        }
        إذا (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(خطأ);
            nodep->تعبير_Format_2(خطأ);
        }
        إذا ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), true});
        }
        إذا (!nodep->تعبير_Format_2()) {
            ثابت string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), خطأ);
            nodep->text(newFormat);
        }
    }

    فراغ visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        إذا (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } وإلا {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ثابت AstVar* varoutp = nullptr;
            لكل (Ast_عقدة* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    إذا (varp->isReadOnly()) {
                    } وإلا إذا (varp->isWritable()) {
                        إذا (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* ثابت ap
                            = جديد AstAssignW{varp->fileline(),
                                             جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             جديد AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(جديد AstAlways{ap});
                    } وإلا {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }
    }

    فراغ visit(AstSystemCSection* nodep) override {
        بدل (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        حالة VSystemCSectionType::CTOR:
        حالة VSystemCSectionType::DTOR:
        حالة VSystemCSectionType::INT: m_modp->modPublic(true); اكسر;
        افتراضي: اكسر;
        }
        // Has no children
    }

    فراغ visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    فراغ visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = true;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.اسحب_back();
    }
    فراغ visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = true;
        iterateChildren(nodep);
    }

    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = افتراضي;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

صنف LinkBotupVisitor final : عام VNVisitorConst {
    // STATE
    Ast_عقدة_Module* m_modp = nullptr;  // Current module

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        إذا (nodep->modp()->modPublic()) m_modp->modPublic(true);
        //** No iteration for speed
    }
    فراغ visit(Ast_عقدة_تعبير*) override {}  // Accelerate
    فراغ visit(Ast_عقدة* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = افتراضي;
};

//######################################################################
// V3LinkResolve class functions

فراغ V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ثابت LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("linkresolve", 0, dump_شجرة_Either_Level() >= 6);
}
