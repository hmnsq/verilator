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
    ثابت VNUser2_درون_Use m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    AstClass* m_classp = اشاره_تهی;  // Class we're inside
    Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Function or task we're inside
    صحيح m_senitem_Cvt_رقم = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    منطقي m_underGenerate = خطأ;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    فراغ بازدید(Ast_گره_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_رقم);
        m_modp = nodep;
        m_senitem_Cvt_رقم = 0;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        لكل (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    فراغ بازدید(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        إذا (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_پشت_With_بعدی());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    فراغ بازدید(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_ftaskp) nodep->funcLocal(درست);
        إذا (nodep->is_Sig_Mod_عمومی()) {
            nodep->sig_Mod_عمومی(خطأ);  // We're done with this attribute
            m_modp->mod_عمومی(درست);  // Avoid flattening if signals are exposed
        }
    }

    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        إذا (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->استفاده_شده_Param(درست);
            // Look for where genvar is valid
            منطقي ok = خطأ;
            // cppcheck-suppress constVariablePointer
            لكل (AstGenFor* ثابت forp : m_underGenFors) {
                إذا (ok) اكسر;
                إذا (forp->initsp())
                    forp->initsp()->foreach([&](ثابت AstVarRef* refp) {  //
                        إذا (refp->varp() == nodep->varp()) ok = درست;
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

    فراغ بازدید(Ast_گره_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        إذا (m_underGenerate) nodep->underGenerate(درست);
        // Remember the existing symbol table scope
        إذا (m_classp) nodep->class_روش(درست);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(خطأ);
        إذا (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
            أرجع;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        إذا (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ثابت fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_گره* ثابت crstp = جديد Ast_تخصیص_بده{
                fvarp->fileline(), جديد AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                جديد AstCReset{fvarp->fileline(), fvarp, خطأ}};
            fvarp->noCReset(درست);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        إذا (nodep->dpiExport()) nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), خطأ});
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        إذا (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            إذا (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            }
            letp->user2(درست);
            إذا (VN_IS(nodep->backp(), Stmt_عبارت)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_عبارت* ثابت letStmtp = VN_AS(letp->stmtsp(), Stmt_عبارت);
            Ast_گره_عبارت* ثابت newp = letStmtp->exprp()->clone_درخت(خطأ);
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ثابت AstVar*, Ast_گره_عبارت*> پورت_مقصد_Exprs;
            لكل (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.اولی;
                ثابت AstArg* ثابت argp = tconnect.دومی;
                Ast_گره_عبارت* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                پورت_مقصد_Exprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ثابت خودکار it = پورت_مقصد_Exprs.پیدا_کن(refp->varp());
                إذا (it != پورت_مقصد_Exprs.انتها()) {
                    Ast_گره_عبارت* ثابت pinp = it->دومی;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_درخت(خطأ));
                    VL_DO_DANGLING(افزودن_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            بازدید(newp);
            letp->user2(خطأ);
            أرجع;
        }
    }

    فراغ بازدید(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlink_Fr_پشت();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    فراغ بازدید(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::COVERAGE_بلوک_OFF) {
            // Strip pragma if not needed, may optimize better without
            إذا (!v3Global.opt.coverage_سطر() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
                أرجع;
            }
        }
        iterateChildren(nodep);
    }

    فراغ بازدید(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::HIER_بلوک) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hier_بلوک(v3Global.opt.سلسلهمراتبی());
            nodep->unlink_Fr_پشت();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::عمومی_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->mod_عمومی(درست);
            nodep->unlink_Fr_پشت();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::عمومی_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->task_عمومی(درست);
            m_modp->mod_عمومی(درست);  // Need to get to the task...
            nodep->unlink_Fr_پشت();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(درست);
            nodep->unlink_Fr_پشت();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_گره* nodep, ثابت string& format, Ast_گره* argp, منطقي is_پیمایش) {
        // Check display arguments, return new format string
        string newFormat;
        منطقي درون_Pct = خطأ;
        منطقي درون_Ignore = خطأ;
        string fmt;
        string parseFormat = format;
        طالما (!parseFormat.خالی() || argp) {
            لكل (ثابت حرف ch : parseFormat) {
                إذا (!درون_Pct && ch == '%') {
                    درون_Pct = درست;
                    درون_Ignore = خطأ;
                    fmt = ch;
                } وإلا إذا (درون_Pct && (std::آیا_رقم(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وإلا إذا (درون_Pct) {
                    درون_Pct = خطأ;
                    fmt += ch;
                    بدل (std::tolower(ch)) {
                    حالة '%':  // %% - just output a %
                        اكسر;
                    حالة '*':
                        درون_Pct = درست;
                        درون_Ignore = درست;
                        اكسر;
                    حالة 'm':  // %m - auto insert "name"
                        إذا (is_پیمایش) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        اكسر;
                    حالة 'l':  // %l - auto insert "library"
                        إذا (is_پیمایش) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        إذا (m_modp)
                            fmt = Ast_گره::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        اكسر;
                    افتراضي:  // Most operators, just move to next argument
                        إذا (!V3_عدد::displayedFmtHasArg(ch, is_پیمایش)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } وإلا إذا (!درون_Ignore) {
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

            إذا (is_پیمایش) اكسر;
            طالما (argp) {
                ثابت AstConst* ثابت constp = VN_CAST(argp, Const);
                ثابت منطقي isFromString = (constp) ? constp->رقم().isFromString() : خطأ;
                إذا (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } وإلا {  // New format string
                    parseFormat += constp->رقم().مقصد_String();
                    Ast_گره* ثابت nextp = argp->nextp();
                    argp->unlink_Fr_پشت();
                    VL_DO_DANGLING(افزودن_Deletep(argp), argp);
                    argp = nextp;
                    اكسر;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        أرجع newFormat;
    }

    ساكن فراغ expectDescriptor(Ast_گره* /*nodep*/, ثابت Ast_گره_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        إذا (filep && filep->varp()) filep->varp()->attrFileDescr(درست);
    }

    فراغ بازدید(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گره_Var_Ref));
    }
    فراغ بازدید(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گره_Var_Ref));
    }
    فراغ بازدید(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گره_Var_Ref));
    }
    فراغ بازدید(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گره_Var_Ref));
    }
    فراغ بازدید(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->متن(), nodep->exprsp(), درست);
    }
    فراغ بازدید(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->متن(), nodep->exprsp(), درست);
    }
    فراغ بازدید(AstSFormatF* nodep) override {
        إذا (nodep->user2_مجموعه_Once()) أرجع;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        إذا (nodep->عبارت_Format_2()) {
            إذا (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->رقم().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlink_Fr_پشت(), Const);
                nodep->متن(fmtp->رقم().مقصد_String());
                nodep->عبارت_Format_2(خطأ);
                VL_DO_DANGLING(افزودن_Deletep(fmtp), fmtp);
            }
        }
        إذا (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(خطأ);
            nodep->عبارت_Format_2(خطأ);
        }
        إذا ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), درست});
        }
        إذا (!nodep->عبارت_Format_2()) {
            ثابت string newFormat = expectFormat(nodep, nodep->متن(), nodep->exprsp(), خطأ);
            nodep->متن(newFormat);
        }
    }

    فراغ بازدید(Ast_Udp_جدول* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        إذا (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } وإلا {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ثابت AstVar* varoutp = اشاره_تهی;
            لكل (Ast_گره* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    إذا (varp->isReadOnly()) {
                    } وإلا إذا (varp->isWritable()) {
                        إذا (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        Ast_تخصیص_بده_W* ثابت ap
                            = جديد Ast_تخصیص_بده_W{varp->fileline(),
                                             جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             جديد AstConst{varp->fileline(), AstConst::بیت_False{}}};
                        m_modp->جمع_کن_Stmtsp(جديد AstAlways{ap});
                    } وإلا {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlink_Fr_پشت();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    فراغ بازدید(AstSystemCSection* nodep) override {
        بدل (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        حالة VSystemCSectionType::CTOR:
        حالة VSystemCSectionType::DTOR:
        حالة VSystemCSectionType::INT: m_modp->mod_عمومی(درست); اكسر;
        افتراضي: اكسر;
        }
        // Has no children
    }

    فراغ بازدید(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(اشاره_تهی);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    فراغ بازدید(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = درست;
        m_underGenFors.emplace_پشت(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.خالی(), nodep, "Underflow");
        m_underGenFors.برداشت_پشت();
    }
    فراغ بازدید(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = درست;
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = افتراضي;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

صنف LinkBotupVisitor final : عام VNVisitorConst {
    // STATE
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module

    // VISITORS
    فراغ بازدید(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstCell* nodep) override {
        // Parent module inherits child's publicity
        إذا (nodep->modp()->mod_عمومی()) m_modp->mod_عمومی(درست);
        //** No iteration for speed
    }
    فراغ بازدید(Ast_گره_عبارت*) override {}  // Accelerate
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    صریح LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
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
    V3Global::dump_Check_Global_درخت("linkresolve", 0, dump_درخت_Either_Level() >= 6);
}
