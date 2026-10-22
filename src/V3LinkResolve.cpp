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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkResolve.h"

#شامل "V3String.h"
#شامل "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

رده LinkResolveVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    ثابت VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_گره_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_گره_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    صحیح m_senitem_Cvt_رقم = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    منطقی m_underGenerate = نادرست;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    تهی visit(Ast_گره_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        اگر (nodep->dead()) بازگشت;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_رقم);
        m_modp = nodep;
        m_senitem_Cvt_رقم = 0;
        iterateChildren(nodep);
    }
    تهی visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    تهی visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        اگر (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(AstVar* nodep) override {
        iterateChildren(nodep);
        اگر (m_ftaskp) nodep->funcLocal(درست);
        اگر (nodep->isSigModPublic()) {
            nodep->sigModPublic(نادرست);  // We're done with this attribute
            m_modp->modPublic(درست);  // Avoid flattening if signals are exposed
        }
    }

    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(درست);
            // Look for where genvar is valid
            منطقی ok = نادرست;
            // cppcheck-suppress constVariablePointer
            برای (AstGenFor* ثابت forp : m_underGenFors) {
                اگر (ok) بشکن;
                اگر (forp->initsp())
                    forp->initsp()->foreach([&](ثابت AstVarRef* refp) {  //
                        اگر (refp->varp() == nodep->varp()) ok = درست;
                    });
            }
            اگر (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        اگر (m_underGenerate) nodep->underGenerate(درست);
        // Remember the existing symbol table scope
        اگر (m_classp) nodep->classMethod(درست);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(نادرست);
        اگر (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        اگر (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ثابت fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_گره* ثابت crstp = جدید AstAssign{
                fvarp->fileline(), جدید AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                جدید AstCReset{fvarp->fileline(), fvarp, نادرست}};
            fvarp->noCReset(درست);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        اگر (nodep->dpiExport()) nodep->scopeNamep(جدید AstScopeName{nodep->fileline(), نادرست});
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        اگر (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            اگر (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }
            letp->user2(درست);
            اگر (VN_IS(nodep->backp(), Stmt_عبارت)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_عبارت* ثابت letStmtp = VN_AS(letp->stmtsp(), Stmt_عبارت);
            Ast_گره_عبارت* ثابت newp = letStmtp->exprp()->clone_درخت(نادرست);
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ثابت AstVar*, Ast_گره_عبارت*> portToExprs;
            برای (ثابت auto& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.first;
                ثابت AstArg* ثابت argp = tconnect.second;
                Ast_گره_عبارت* ثابت pinp = argp->exprp();
                اگر (!pinp) ادامه;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ثابت auto it = portToExprs.find(refp->varp());
                اگر (it != portToExprs.end()) {
                    Ast_گره_عبارت* ثابت pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_درخت(نادرست));
                    VL_DO_DANGLING(افزودن_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(نادرست);
            بازگشت;
        }
    }

    تهی visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstStmtPragma* nodep) override {
        اگر (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            اگر (!v3Global.opt.coverage_سطر() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
        }
        iterateChildren(nodep);
    }

    تهی visit(AstPragma* nodep) override {
        اگر (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(درست);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(درست);
            m_modp->modPublic(درست);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(درست);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_گره* nodep, ثابت string& format, Ast_گره* argp, منطقی isScan) {
        // Check display arguments, return new format string
        string newFormat;
        منطقی inPct = نادرست;
        منطقی inIgnore = نادرست;
        string fmt;
        string parseFormat = format;
        وقتی (!parseFormat.empty() || argp) {
            برای (ثابت نویسه ch : parseFormat) {
                اگر (!inPct && ch == '%') {
                    inPct = درست;
                    inIgnore = نادرست;
                    fmt = ch;
                } وگرنه اگر (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وگرنه اگر (inPct) {
                    inPct = نادرست;
                    fmt += ch;
                    انتخاب (std::tolower(ch)) {
                    حالت '%':  // %% - just output a %
                        بشکن;
                    حالت '*':
                        inPct = درست;
                        inIgnore = درست;
                        بشکن;
                    حالت 'm':  // %m - auto insert "name"
                        اگر (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        بشکن;
                    حالت 'l':  // %l - auto insert "library"
                        اگر (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        اگر (m_modp)
                            fmt = Ast_گره::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        بشکن;
                    پیشفرض:  // Most operators, just move to next argument
                        اگر (!V3_عدد::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } وگرنه اگر (!inIgnore) {
                            اگر (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } وگرنه {
                                argp = argp->nextp();
                            }
                        }
                        بشکن;
                    }  // switch
                    newFormat += fmt;
                } وگرنه {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            اگر (isScan) بشکن;
            وقتی (argp) {
                ثابت AstConst* ثابت constp = VN_CAST(argp, Const);
                ثابت منطقی isFromString = (constp) ? constp->رقم().isFromString() : نادرست;
                اگر (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } وگرنه {  // New format string
                    parseFormat += constp->رقم().toString();
                    Ast_گره* ثابت nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(افزودن_Deletep(argp), argp);
                    argp = nextp;
                    بشکن;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        بازگشت newFormat;
    }

    ایستا تهی expectDescriptor(Ast_گره* /*nodep*/, ثابت Ast_گره_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        اگر (filep && filep->varp()) filep->varp()->attrFileDescr(درست);
    }

    تهی visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گره_Var_Ref));
    }
    تهی visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گره_Var_Ref));
    }
    تهی visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گره_Var_Ref));
    }
    تهی visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گره_Var_Ref));
    }
    تهی visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), درست);
    }
    تهی visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), درست);
    }
    تهی visit(AstSFormatF* nodep) override {
        اگر (nodep->user2SetOnce()) بازگشت;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        اگر (nodep->عبارت_Format_2()) {
            اگر (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->رقم().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->رقم().toString());
                nodep->عبارت_Format_2(نادرست);
                VL_DO_DANGLING(افزودن_Deletep(fmtp), fmtp);
            }
        }
        اگر (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(نادرست);
            nodep->عبارت_Format_2(نادرست);
        }
        اگر ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(جدید AstScopeName{nodep->fileline(), درست});
        }
        اگر (!nodep->عبارت_Format_2()) {
            ثابت string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), نادرست);
            nodep->text(newFormat);
        }
    }

    تهی visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        اگر (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } وگرنه {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ثابت AstVar* varoutp = nullptr;
            برای (Ast_گره* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    اگر (varp->isReadOnly()) {
                    } وگرنه اگر (varp->isWritable()) {
                        اگر (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* ثابت ap
                            = جدید AstAssignW{varp->fileline(),
                                             جدید AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             جدید AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(جدید AstAlways{ap});
                    } وگرنه {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    تهی visit(AstSystemCSection* nodep) override {
        انتخاب (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        حالت VSystemCSectionType::CTOR:
        حالت VSystemCSectionType::DTOR:
        حالت VSystemCSectionType::INT: m_modp->modPublic(درست); بشکن;
        پیشفرض: بشکن;
        }
        // Has no children
    }

    تهی visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    تهی visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = درست;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.برداشت_back();
    }
    تهی visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = درست;
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = پیشفرض;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

رده LinkBotupVisitor final : عمومی VNVisitorConst {
    // STATE
    Ast_گره_Module* m_modp = nullptr;  // Current module

    // VISITORS
    تهی visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        اگر (nodep->modp()->modPublic()) m_modp->modPublic(درست);
        //** No iteration for speed
    }
    تهی visit(Ast_گره_عبارت*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = پیشفرض;
};

//######################################################################
// V3LinkResolve class functions

تهی V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ثابت LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkresolve", 0, dump_درخت_Either_Level() >= 6);
}
