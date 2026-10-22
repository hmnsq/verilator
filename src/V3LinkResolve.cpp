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
    AstNodeModule* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    AstNodeFTask* m_ftaskp = nullptr;  // Function or task we're inside
    صحيح m_senitemCvtNum = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    منطقي m_underGenerate = ܕܓܠܐ;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    فراغ visit(AstNodeModule* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        ܐܢ (nodep->dead()) ܦܢܐ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitemCvtNum);
        m_modp = nodep;
        m_senitemCvtNum = 0;
        iterateChildren(nodep);
    }
    فراغ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        لكل (AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ܐܢ (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                ܐܢ (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        ܐܢ (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    فراغ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        ܐܢ (m_ftaskp) nodep->funcLocal(ܫܪܪܐ);
        ܐܢ (nodep->isSigModPublic()) {
            nodep->sigModPublic(ܕܓܠܐ);  // We're done with this attribute
            m_modp->modPublic(ܫܪܪܐ);  // Avoid flattening if signals are exposed
        }
    }

    فراغ visit(AstNodeVarRef* nodep) override {
        ܐܢ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(ܫܪܪܐ);
            // Look for where genvar is valid
            منطقي ok = ܕܓܠܐ;
            // cppcheck-suppress constVariablePointer
            لكل (AstGenFor* ثابت forp : m_underGenFors) {
                ܐܢ (ok) اكسر;
                ܐܢ (forp->initsp())
                    forp->initsp()->foreach([&](ثابت AstVarRef* refp) {  //
                        ܐܢ (refp->varp() == nodep->varp()) ok = ܫܪܪܐ;
                    });
            }
            ܐܢ (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstNodeFTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        ܐܢ (m_underGenerate) nodep->underGenerate(ܫܪܪܐ);
        // Remember the existing symbol table scope
        ܐܢ (m_classp) nodep->classMethod(ܫܪܪܐ);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ܕܓܠܐ);
        ܐܢ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            ܦܢܐ;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        ܐܢ (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ثابت fvarp = VN_AS(nodep->fvarp(), Var);
            AstNode* ثابت crstp = جديد AstAssign{
                fvarp->fileline(), جديد AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                جديد AstCReset{fvarp->fileline(), fvarp, ܕܓܠܐ}};
            fvarp->noCReset(ܫܪܪܐ);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        ܐܢ (nodep->dpiExport()) nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), ܕܓܠܐ});
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        iterateChildren(nodep);
        ܐܢ (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            ܐܢ (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ܦܢܐ;
            }
            letp->user2(ܫܪܪܐ);
            ܐܢ (VN_IS(nodep->backp(), StmtExpr)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            AstStmtExpr* ثابت letStmtp = VN_AS(letp->stmtsp(), StmtExpr);
            AstNodeExpr* ثابت newp = letStmtp->exprp()->cloneTree(ܕܓܠܐ);
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ثابت AstVar*, AstNodeExpr*> portToExprs;
            لكل (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                AstNodeExpr* ثابت pinp = argp->exprp();
                ܐܢ (!pinp) استمر;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ثابت auto it = portToExprs.find(refp->varp());
                ܐܢ (it != portToExprs.end()) {
                    AstNodeExpr* ثابت pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->cloneTree(ܕܓܠܐ));
                    VL_DO_DANGLING(pushDeletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(ܕܓܠܐ);
            ܦܢܐ;
        }
    }

    فراغ visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstStmtPragma* nodep) override {
        ܐܢ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            ܐܢ (!v3Global.opt.coverageLine() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                ܦܢܐ;
            }
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstPragma* nodep) override {
        ܐܢ (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } ܐܚܪܢܐ ܐܢ (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(ܫܪܪܐ);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } ܐܚܪܢܐ ܐܢ (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(ܫܪܪܐ);
            m_modp->modPublic(ܫܪܪܐ);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } ܐܚܪܢܐ ܐܢ (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(ܫܪܪܐ);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } ܐܚܪܢܐ {
            iterateChildren(nodep);
        }
    }

    string expectFormat(AstNode* nodep, ثابت string& format, AstNode* argp, منطقي isScan) {
        // Check display arguments, return new format string
        string newFormat;
        منطقي inPct = ܕܓܠܐ;
        منطقي inIgnore = ܕܓܠܐ;
        string fmt;
        string parseFormat = format;
        ܟܡܐ (!parseFormat.empty() || argp) {
            لكل (ثابت حرف ch : parseFormat) {
                ܐܢ (!inPct && ch == '%') {
                    inPct = ܫܪܪܐ;
                    inIgnore = ܕܓܠܐ;
                    fmt = ch;
                } ܐܚܪܢܐ ܐܢ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ܐܚܪܢܐ ܐܢ (inPct) {
                    inPct = ܕܓܠܐ;
                    fmt += ch;
                    بدل (std::tolower(ch)) {
                    حالة '%':  // %% - just output a %
                        اكسر;
                    حالة '*':
                        inPct = ܫܪܪܐ;
                        inIgnore = ܫܪܪܐ;
                        اكسر;
                    حالة 'm':  // %m - auto insert "name"
                        ܐܢ (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        اكسر;
                    حالة 'l':  // %l - auto insert "library"
                        ܐܢ (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        ܐܢ (m_modp)
                            fmt = AstNode::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        اكسر;
                    افتراضي:  // Most operators, just move to next argument
                        ܐܢ (!V3Number::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } ܐܚܪܢܐ ܐܢ (!inIgnore) {
                            ܐܢ (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } ܐܚܪܢܐ {
                                argp = argp->nextp();
                            }
                        }
                        اكسر;
                    }  // switch
                    newFormat += fmt;
                } ܐܚܪܢܐ {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            ܐܢ (isScan) اكسر;
            ܟܡܐ (argp) {
                ثابت AstConst* ثابت constp = VN_CAST(argp, Const);
                ثابت منطقي isFromString = (constp) ? constp->num().isFromString() : ܕܓܠܐ;
                ܐܢ (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } ܐܚܪܢܐ {  // New format string
                    parseFormat += constp->num().toString();
                    AstNode* ثابت nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(pushDeletep(argp), argp);
                    argp = nextp;
                    اكسر;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        ܦܢܐ newFormat;
    }

    ساكن فراغ expectDescriptor(AstNode* /*nodep*/, ثابت AstNodeVarRef* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        ܐܢ (filep && filep->varp()) filep->varp()->attrFileDescr(ܫܪܪܐ);
    }

    فراغ visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), NodeVarRef));
    }
    فراغ visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), NodeVarRef));
    }
    فراغ visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), NodeVarRef));
    }
    فراغ visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), NodeVarRef));
    }
    فراغ visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), ܫܪܪܐ);
    }
    فراغ visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), ܫܪܪܐ);
    }
    فراغ visit(AstSFormatF* nodep) override {
        ܐܢ (nodep->user2SetOnce()) ܦܢܐ;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        ܐܢ (nodep->exprFormat()) {
            ܐܢ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->num().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->num().toString());
                nodep->exprFormat(ܕܓܠܐ);
                VL_DO_DANGLING(pushDeletep(fmtp), fmtp);
            }
        }
        ܐܢ (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(ܕܓܠܐ);
            nodep->exprFormat(ܕܓܠܐ);
        }
        ܐܢ ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), ܫܪܪܐ});
        }
        ܐܢ (!nodep->exprFormat()) {
            ثابت string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), ܕܓܠܐ);
            nodep->text(newFormat);
        }
    }

    فراغ visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        ܐܢ (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } ܐܚܪܢܐ {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ثابت AstVar* varoutp = nullptr;
            لكل (AstNode* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ܐܢ (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    ܐܢ (varp->isReadOnly()) {
                    } ܐܚܪܢܐ ܐܢ (varp->isWritable()) {
                        ܐܢ (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* ثابت ap
                            = جديد AstAssignW{varp->fileline(),
                                             جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             جديد AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(جديد AstAlways{ap});
                    } ܐܚܪܢܐ {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }

    فراغ visit(AstSystemCSection* nodep) override {
        بدل (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        حالة VSystemCSectionType::CTOR:
        حالة VSystemCSectionType::DTOR:
        حالة VSystemCSectionType::INT: m_modp->modPublic(ܫܪܪܐ); اكسر;
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
        m_underGenerate = ܫܪܪܐ;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.pop_back();
    }
    فراغ visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ܫܪܪܐ;
        iterateChildren(nodep);
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

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
    AstNodeModule* m_modp = nullptr;  // Current module

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        ܐܢ (nodep->modp()->modPublic()) m_modp->modPublic(ܫܪܪܐ);
        //** No iteration for speed
    }
    فراغ visit(AstNodeExpr*) override {}  // Accelerate
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

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
    V3Global::dumpCheckGlobalTree("linkresolve", 0, dumpTreeEitherLevel() >= 6);
}
