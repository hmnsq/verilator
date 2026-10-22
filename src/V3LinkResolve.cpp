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
    ثابت VNUser2_اندريون_Use m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    AstClass* m_classp = خالي_اشارو;  // Class we're inside
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Function or task we're inside
    صحيح m_senitem_Cvt_انگ = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    منطقي m_underGenerate = خطأ;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_انگ);
        m_modp = nodep;
        m_senitem_Cvt_انگ = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        لكل (Ast_ڳنڍ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    فراغ گهمو(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        إذا (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }
    فراغ گهمو(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_ftaskp) nodep->funcLocal(سچ);
        إذا (nodep->is_Sig_Mod_عوامي()) {
            nodep->sig_Mod_عوامي(خطأ);  // We're done with this attribute
            m_modp->mod_عوامي(سچ);  // Avoid flattening if signals are exposed
        }
    }

    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        إذا (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->استعمال_ٿيل_Param(سچ);
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

    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        إذا (m_underGenerate) nodep->underGenerate(سچ);
        // Remember the existing symbol table scope
        إذا (m_classp) nodep->class_طريقو(سچ);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(خطأ);
        إذا (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
            أرجع;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        إذا (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ثابت fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_ڳنڍ* ثابت crstp = جديد Ast_سونپيو{
                fvarp->fileline(), جديد AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                جديد AstCReset{fvarp->fileline(), fvarp, خطأ}};
            fvarp->noCReset(سچ);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        إذا (nodep->dpiExport()) nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), خطأ});
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        إذا (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            إذا (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False_Erroring{}});
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع;
            }
            letp->user2(سچ);
            إذا (VN_IS(nodep->backp(), Stmt_اظهار)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_اظهار* ثابت letStmtp = VN_AS(letp->stmtsp(), Stmt_اظهار);
            Ast_ڳنڍ_اظهار* ثابت newp = letStmtp->exprp()->clone_وڻ(خطأ);
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ثابت AstVar*, Ast_ڳنڍ_اظهار*> پورٽ_منزل_Exprs;
            لكل (ثابت خودڪار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.پهريون;
                ثابت AstArg* ثابت argp = tconnect.ٻيو;
                Ast_ڳنڍ_اظهار* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                پورٽ_منزل_Exprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ثابت خودڪار it = پورٽ_منزل_Exprs.ڳوليو(refp->varp());
                إذا (it != پورٽ_منزل_Exprs.پڇاڙي()) {
                    Ast_ڳنڍ_اظهار* ثابت pinp = it->ٻيو;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_وڻ(خطأ));
                    VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            گهمو(newp);
            letp->user2(خطأ);
            أرجع;
        }
    }

    فراغ گهمو(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::COVERAGE_بلاڪ_OFF) {
            // Strip pragma if not needed, may optimize better without
            إذا (!v3Global.opt.coverage_سٽ() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                أرجع;
            }
        }
        iterateChildren(nodep);
    }

    فراغ گهمو(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::HIER_بلاڪ) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hier_بلاڪ(v3Global.opt.درجيوار());
            nodep->unlink_Fr_پٺ();
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::عوامي_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->mod_عوامي(سچ);
            nodep->unlink_Fr_پٺ();
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::عوامي_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->task_عوامي(سچ);
            m_modp->mod_عوامي(سچ);  // Need to get to the task...
            nodep->unlink_Fr_پٺ();
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        } وإلا إذا (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(سچ);
            nodep->unlink_Fr_پٺ();
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_ڳنڍ* nodep, ثابت string& format, Ast_ڳنڍ* argp, منطقي is_جاچ) {
        // Check display arguments, return new format string
        string newFormat;
        منطقي اندريون_Pct = خطأ;
        منطقي اندريون_Ignore = خطأ;
        string fmt;
        string parseFormat = format;
        طالما (!parseFormat.خالي_جايون() || argp) {
            لكل (ثابت حرف ch : parseFormat) {
                إذا (!اندريون_Pct && ch == '%') {
                    اندريون_Pct = سچ;
                    اندريون_Ignore = خطأ;
                    fmt = ch;
                } وإلا إذا (اندريون_Pct && (std::انگ_آهي(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } وإلا إذا (اندريون_Pct) {
                    اندريون_Pct = خطأ;
                    fmt += ch;
                    بدل (std::tolower(ch)) {
                    حالة '%':  // %% - just output a %
                        اكسر;
                    حالة '*':
                        اندريون_Pct = سچ;
                        اندريون_Ignore = سچ;
                        اكسر;
                    حالة 'm':  // %m - auto insert "name"
                        إذا (is_جاچ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        اكسر;
                    حالة 'l':  // %l - auto insert "library"
                        إذا (is_جاچ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        إذا (m_modp)
                            fmt = Ast_ڳنڍ::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        اكسر;
                    افتراضي:  // Most operators, just move to next argument
                        إذا (!V3_عدد::displayedFmtHasArg(ch, is_جاچ)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } وإلا إذا (!اندريون_Ignore) {
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

            إذا (is_جاچ) اكسر;
            طالما (argp) {
                ثابت AstConst* ثابت constp = VN_CAST(argp, Const);
                ثابت منطقي isFromString = (constp) ? constp->انگ().isFromString() : خطأ;
                إذا (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } وإلا {  // New format string
                    parseFormat += constp->انگ().منزل_String();
                    Ast_ڳنڍ* ثابت nextp = argp->nextp();
                    argp->unlink_Fr_پٺ();
                    VL_DO_DANGLING(ڌڪو_Deletep(argp), argp);
                    argp = nextp;
                    اكسر;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        أرجع newFormat;
    }

    ساكن فراغ expectDescriptor(Ast_ڳنڍ* /*nodep*/, ثابت Ast_ڳنڍ_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        إذا (filep && filep->varp()) filep->varp()->attrFileDescr(سچ);
    }

    فراغ گهمو(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ڳنڍ_Var_Ref));
    }
    فراغ گهمو(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ڳنڍ_Var_Ref));
    }
    فراغ گهمو(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ڳنڍ_Var_Ref));
    }
    فراغ گهمو(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ڳنڍ_Var_Ref));
    }
    فراغ گهمو(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->متن(), nodep->exprsp(), سچ);
    }
    فراغ گهمو(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->متن(), nodep->exprsp(), سچ);
    }
    فراغ گهمو(AstSFormatF* nodep) override {
        إذا (nodep->user2_سيٽ_Once()) أرجع;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        إذا (nodep->اظهار_Format_2()) {
            إذا (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->انگ().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlink_Fr_پٺ(), Const);
                nodep->متن(fmtp->انگ().منزل_String());
                nodep->اظهار_Format_2(خطأ);
                VL_DO_DANGLING(ڌڪو_Deletep(fmtp), fmtp);
            }
        }
        إذا (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(خطأ);
            nodep->اظهار_Format_2(خطأ);
        }
        إذا ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), سچ});
        }
        إذا (!nodep->اظهار_Format_2()) {
            ثابت string newFormat = expectFormat(nodep, nodep->متن(), nodep->exprsp(), خطأ);
            nodep->متن(newFormat);
        }
    }

    فراغ گهمو(Ast_Udp_جدول* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        إذا (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } وإلا {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ثابت AstVar* varoutp = خالي_اشارو;
            لكل (Ast_ڳنڍ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    إذا (varp->isReadOnly()) {
                    } وإلا إذا (varp->isWritable()) {
                        إذا (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        Ast_سونپيو_W* ثابت ap
                            = جديد Ast_سونپيو_W{varp->fileline(),
                                             جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             جديد AstConst{varp->fileline(), AstConst::ٻٽو_انگ_False{}}};
                        m_modp->جوڙيو_Stmtsp(جديد AstAlways{ap});
                    } وإلا {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlink_Fr_پٺ();
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }

    فراغ گهمو(AstSystemCSection* nodep) override {
        بدل (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        حالة VSystemCSectionType::CTOR:
        حالة VSystemCSectionType::DTOR:
        حالة VSystemCSectionType::INT: m_modp->mod_عوامي(سچ); اكسر;
        افتراضي: اكسر;
        }
        // Has no children
    }

    فراغ گهمو(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(خالي_اشارو);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    فراغ گهمو(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = سچ;
        m_underGenFors.emplace_پٺ(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.خالي_جايون(), nodep, "Underflow");
        m_underGenFors.ڪڍو_پٺ();
    }
    فراغ گهمو(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = سچ;
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

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
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module

    // VISITORS
    فراغ گهمو(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstCell* nodep) override {
        // Parent module inherits child's publicity
        إذا (nodep->modp()->mod_عوامي()) m_modp->mod_عوامي(سچ);
        //** No iteration for speed
    }
    فراغ گهمو(Ast_ڳنڍ_اظهار*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

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
    V3Global::dump_Check_Global_وڻ("linkresolve", 0, dump_وڻ_Either_Level() >= 6);
}
