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
    ثابت VNUser2_وچ_Use m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    AstClass* m_classp = خالی_سوچک;  // Class we're inside
    Ast_گرہ_FTask* m_ftaskp = خالی_سوچک;  // Function or task we're inside
    صحيح m_senitem_Cvt_عدد = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    منطقي m_underGenerate = خطأ;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_عدد);
        m_modp = nodep;
        m_senitem_Cvt_عدد = 0;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        لكل (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        إذا (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_پٹھ_With_اگلا());
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    فراغ ویکھو(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_ftaskp) nodep->funcLocal(سچ);
        إذا (nodep->is_Sig_Mod_عوامی()) {
            nodep->sig_Mod_عوامی(خطأ);  // We're done with this attribute
            m_modp->mod_عوامی(سچ);  // Avoid flattening if signals are exposed
        }
    }

    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        إذا (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->ورتیا_Param(سچ);
            // Look for where genvar is valid
            منطقي ok = خطأ;
            // cppcheck-suppress constVariablePointer
            لكل (AstGenFor* ثابت forp : m_underGenFors) {
                إذا (ok) اكسر;
                إذا (forp->initsp())
                    forp->initsp()->foreach([&](ثابت AstVarRef* refp) {  //
                        إذا (refp->varp() == nodep->varp()) ok = سچ;
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

    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        إذا (m_underGenerate) nodep->underGenerate(سچ);
        // Remember the existing symbol table scope
        إذا (m_classp) nodep->class_طریقہ(سچ);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(خطأ);
        إذا (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
            أرجع;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        إذا (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ثابت fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_گرہ* ثابت crstp = جديد Ast_سونپو{
                fvarp->fileline(), جديد AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                جديد AstCReset{fvarp->fileline(), fvarp, خطأ}};
            fvarp->noCReset(سچ);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        إذا (nodep->dpiExport()) nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), خطأ});
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        إذا (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            إذا (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                أرجع;
            }
            letp->user2(سچ);
            إذا (VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_اظہاریہ* ثابت letStmtp = VN_AS(letp->stmtsp(), Stmt_اظہاریہ);
            Ast_گرہ_اظہاریہ* ثابت newp = letStmtp->exprp()->clone_درخت(خطأ);
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_منزل_Exprs;
            لكل (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.پہلا;
                ثابت AstArg* ثابت argp = tconnect.دوجا;
                Ast_گرہ_اظہاریہ* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                پورٹ_منزل_Exprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ثابت خودکار it = پورٹ_منزل_Exprs.لبھو(refp->varp());
                إذا (it != پورٹ_منزل_Exprs.سرا()) {
                    Ast_گرہ_اظہاریہ* ثابت pinp = it->دوجا;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_درخت(خطأ));
                    VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            ویکھو(newp);
            letp->user2(خطأ);
            أرجع;
        }
    }

    فراغ ویکھو(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlink_Fr_پٹھ();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    فراغ ویکھو(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::COVERAGE_بلاک_OFF) {
            // Strip pragma if not needed, may optimize better without
            إذا (!v3Global.opt.coverage_سطر() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
                أرجع;
            }
        }
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::HIER_بلاک) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hier_بلاک(v3Global.opt.درجہ_وار());
            nodep->unlink_Fr_پٹھ();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::عوامی_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->mod_عوامی(سچ);
            nodep->unlink_Fr_پٹھ();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::عوامی_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->task_عوامی(سچ);
            m_modp->mod_عوامی(سچ);  // Need to get to the task...
            nodep->unlink_Fr_پٹھ();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(سچ);
            nodep->unlink_Fr_پٹھ();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_گرہ* nodep, ثابت string& format, Ast_گرہ* argp, منطقي is_جانچ) {
        // Check display arguments, return new format string
        string newFormat;
        منطقي وچ_Pct = خطأ;
        منطقي وچ_Ignore = خطأ;
        string fmt;
        string parseFormat = format;
        طالما (!parseFormat.خالی_تھاواں() || argp) {
            لكل (ثابت حرف ch : parseFormat) {
                إذا (!وچ_Pct && ch == '%') {
                    وچ_Pct = سچ;
                    وچ_Ignore = خطأ;
                    fmt = ch;
                } وإلا إذا (وچ_Pct && (std::ہندسہ_اے(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وإلا إذا (وچ_Pct) {
                    وچ_Pct = خطأ;
                    fmt += ch;
                    بدل (std::tolower(ch)) {
                    حالة '%':  // %% - just output a %
                        اكسر;
                    حالة '*':
                        وچ_Pct = سچ;
                        وچ_Ignore = سچ;
                        اكسر;
                    حالة 'm':  // %m - auto insert "name"
                        إذا (is_جانچ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        اكسر;
                    حالة 'l':  // %l - auto insert "library"
                        إذا (is_جانچ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        إذا (m_modp)
                            fmt = Ast_گرہ::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        اكسر;
                    افتراضي:  // Most operators, just move to next argument
                        إذا (!V3_نمبر::displayedFmtHasArg(ch, is_جانچ)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } وإلا إذا (!وچ_Ignore) {
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

            إذا (is_جانچ) اكسر;
            طالما (argp) {
                ثابت AstConst* ثابت constp = VN_CAST(argp, Const);
                ثابت منطقي isFromString = (constp) ? constp->عدد().isFromString() : خطأ;
                إذا (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } وإلا {  // New format string
                    parseFormat += constp->عدد().منزل_String();
                    Ast_گرہ* ثابت nextp = argp->nextp();
                    argp->unlink_Fr_پٹھ();
                    VL_DO_DANGLING(ڈالو_Deletep(argp), argp);
                    argp = nextp;
                    اكسر;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        أرجع newFormat;
    }

    ساكن فراغ expectDescriptor(Ast_گرہ* /*nodep*/, ثابت Ast_گرہ_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        إذا (filep && filep->varp()) filep->varp()->attrFileDescr(سچ);
    }

    فراغ ویکھو(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گرہ_Var_Ref));
    }
    فراغ ویکھو(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گرہ_Var_Ref));
    }
    فراغ ویکھو(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گرہ_Var_Ref));
    }
    فراغ ویکھو(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گرہ_Var_Ref));
    }
    فراغ ویکھو(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->متن(), nodep->exprsp(), سچ);
    }
    فراغ ویکھو(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->متن(), nodep->exprsp(), سچ);
    }
    فراغ ویکھو(AstSFormatF* nodep) override {
        إذا (nodep->user2_سیٹ_Once()) أرجع;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        إذا (nodep->اظہاریہ_Format_2()) {
            إذا (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->عدد().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlink_Fr_پٹھ(), Const);
                nodep->متن(fmtp->عدد().منزل_String());
                nodep->اظہاریہ_Format_2(خطأ);
                VL_DO_DANGLING(ڈالو_Deletep(fmtp), fmtp);
            }
        }
        إذا (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(خطأ);
            nodep->اظہاریہ_Format_2(خطأ);
        }
        إذا ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), سچ});
        }
        إذا (!nodep->اظہاریہ_Format_2()) {
            ثابت string newFormat = expectFormat(nodep, nodep->متن(), nodep->exprsp(), خطأ);
            nodep->متن(newFormat);
        }
    }

    فراغ ویکھو(Ast_Udp_جدول* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        إذا (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } وإلا {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ثابت AstVar* varoutp = خالی_سوچک;
            لكل (Ast_گرہ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    إذا (varp->isReadOnly()) {
                    } وإلا إذا (varp->isWritable()) {
                        إذا (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        Ast_سونپو_W* ثابت ap
                            = جديد Ast_سونپو_W{varp->fileline(),
                                             جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             جديد AstConst{varp->fileline(), AstConst::ثنائی_ہندسہ_False{}}};
                        m_modp->جوڑو_Stmtsp(جديد AstAlways{ap});
                    } وإلا {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlink_Fr_پٹھ();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }

    فراغ ویکھو(AstSystemCSection* nodep) override {
        بدل (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        حالة VSystemCSectionType::CTOR:
        حالة VSystemCSectionType::DTOR:
        حالة VSystemCSectionType::INT: m_modp->mod_عوامی(سچ); اكسر;
        افتراضي: اكسر;
        }
        // Has no children
    }

    فراغ ویکھو(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(خالی_سوچک);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    فراغ ویکھو(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = سچ;
        m_underGenFors.emplace_پٹھ(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.خالی_تھاواں(), nodep, "Underflow");
        m_underGenFors.نکالو_پٹھ();
    }
    فراغ ویکھو(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = سچ;
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = افتراضي;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

صنف LinkBotupVisitor final : عام VNVisitorConst {
    // STATE
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module

    // VISITORS
    فراغ ویکھو(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstCell* nodep) override {
        // Parent module inherits child's publicity
        إذا (nodep->modp()->mod_عوامی()) m_modp->mod_عوامی(سچ);
        //** No iteration for speed
    }
    فراغ ویکھو(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    واضح LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
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
