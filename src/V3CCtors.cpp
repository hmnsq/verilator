// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Generate C language constructors and AstCReset nodes.
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
// V3CCtors's Transformations:
//      Iterates over all modules and
//      for all AstVar, create a creates a AstCReset node in an _ctor_var_reset AstCFunc.
//      for all AstCoverDecl, move the declaration into a _configure_coverage AstCFunc.
//      For each variable that needs reset, add a AstCReset node.
//
//      For primary inputs, add _eval_debug_assertions.
//
//      This transformation honors outputSplitCFuncs.
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3CCtors.h"

#ضمن "V3EmitCBase.h"

#ضمن <list>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف VCtorType final {
عام:
    تعداد en : uint8_t { MODULE, CLASS, COVERAGE };

خاص:
    ثابت تعداد en m_e;

عام:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    منطقي isClass() ثابت { أرجع m_e == CLASS; }
    منطقي isCoverage() ثابت { أرجع m_e == COVERAGE; }
};

صنف V3CCtorsBuilder final {
    Ast_غوټه_Module* ثابت m_modp;  // Current module/class
    ثابت string m_basename;
    ثابت VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    صحيح m_عدد_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ثابت صحيح func_عدد = m_newFunctions.اندازه();
        ثابت string funcName = m_basename + "_" + cvt_منزل_Str(func_عدد);
        AstCFunc* ثابت funcp = جديد AstCFunc{m_modp->fileline(), funcName, تش_اشاره, "void"};
        funcp->isStatic(خطأ);
        funcp->isLoose(!m_type.isClass());
        funcp->keep_If_تشه(ریښتیا);  // TODO relax
        funcp->decl_شخصي(ریښتیا);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        إذا (!m_type.isCoverage()) m_modp->ctorVarReset(ریښتیا);
        string prevent_ناکارول_شوی_Stmt;
        إذا (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            prevent_ناکارول_شوی_Stmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } وإلا إذا (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            prevent_ناکارول_شوی_Stmt = "(void)first;  // Prevent unused variable warning";
        }
        إذا (!prevent_ناکارول_شوی_Stmt.تشه()) {
            funcp->جمع_کړه_Stmtsp(جديد AstCStmt{m_modp->fileline(), prevent_ناکارول_شوی_Stmt});
        }
        m_modp->جمع_کړه_Stmtsp(funcp);
        m_عدد_Stmts = 0;
        أرجع funcp;
    }

عام:
    فراغ جمع_کړه(Ast_غوټه* nodep) {
        إذا (m_newFunctions.تشه()) m_newFunctions.ټېل_شا(makeNewFunc());
        إذا (v3Global.opt.output_وېشل_CFuncs() && m_عدد_Stmts > v3Global.opt.output_وېشل_CFuncs()) {
            m_newFunctions.ټېل_شا(makeNewFunc());
        }
        m_newFunctions.شا()->جمع_کړه_Stmtsp(nodep);
        m_عدد_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_غوټه_Module* nodep, ثابت string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        إذا (m_type.isCoverage()) m_newFunctions.ټېل_شا(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        إذا (m_newFunctions.اندازه() == 0) {
        } وإلا إذا (m_newFunctions.اندازه() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } وإلا {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ثابت ریښه_Funcp = makeNewFunc();
            ریښه_Funcp->name(m_basename);
            لكل (AstCFunc* ثابت funcp : m_newFunctions) {
                AstCCall* ثابت callp = جديد AstCCall{m_modp->fileline(), funcp};
                callp->dtype_سېټ_Void();
                إذا (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } وإلا {
                    إذا (m_type.isCoverage()) callp->argTypes("first");
                    callp->self_اشاره_ګر(VSelf_اشاره_ګر_متن{VSelf_اشاره_ګر_متن::This{}});
                }
                ریښه_Funcp->جمع_کړه_Stmtsp(callp->makeStmt());
            }
        }
    }

خاص:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

صنف CCtorsVisitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    AstCFunc* m_cfuncp = تش_اشاره;  // Current function
    V3CCtorsBuilder* m_varResetp = تش_اشاره;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ثابت Ast_غوټه_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ساكن فراغ ورننباسه_Sc(AstCFunc* cfuncp, ثابت Ast_غوټه_Module* modp, VSystemCSectionType type) {
        ثابت خپلکاری txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        إذا (txtAndFlp.لومړی.تشه()) أرجع;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ثابت cstmtp = جديد AstCStmtUser{txtAndFlp.دوهم};
        cstmtp->جمع_کړه(txtAndFlp.لومړی);
        cfuncp->جمع_کړه_Stmtsp(cstmtp);
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        إذا (nodep->isConstPool()) أرجع;  // Special emit rules
        إذا (ثابت AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            إذا (classp->isInterfaceClass()) أرجع;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        إذا (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            لكل (Ast_غوټه *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                إذا (Ast_غوټه_Cover_Decl* ثابت coverp = VN_CAST(np, غوټه_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.جمع_کړه(coverp->unlink_Fr_شا());
                }
            }
        }
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            AstCFunc* ثابت funcp = جديد AstCFunc{classp->fileline(), "~", تش_اشاره, ""};
            funcp->isDestructor(ریښتیا);
            funcp->isStatic(خطأ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(خطأ);
            ورننباسه_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->جمع_کړه_Stmtsp(funcp);
        }
    }

    فراغ لیدنه(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = تش_اشاره;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        إذا (nodep->name() == "new") ورننباسه_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    فراغ لیدنه(AstCStmt* nodep) override {
        إذا (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVar* nodep) override {
        إذا (nodep->needsCReset()) {
            Ast_غوټه* ثابت crstp = جديد Ast_ټاکل{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جديد AstCReset{nodep->fileline(), nodep, ریښتیا}};
            إذا (m_varResetp) {
                m_varResetp->جمع_کړه(crstp);
            } وإلا إذا (m_cfuncp) {
                nodep->جمع_کړه_بل_دلته(crstp);
            } وإلا {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند CCtorsVisitor(Ast_غوټه* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        لكل (خپلکاری& itr : m_ctorCalls) {
            AstCStmt* ثابت nodep = itr.لومړی;
            ثابت Ast_غوټه_Module* ثابت modp = itr.دوهم;
            إذا (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        }
    }
};

//######################################################################

فراغ V3CCtors::evalAsserts() {
    Ast_غوټه_Module* ثابت modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ثابت funcp
        = جديد AstCFunc{modp->fileline(), "_eval_debug_assertions", تش_اشاره, "void"};
    funcp->decl_شخصي(ریښتیا);
    funcp->isStatic(خطأ);
    funcp->isLoose(ریښتیا);
    funcp->keep_If_تشه(ریښتیا);
    funcp->slow(خطأ);
    funcp->ifdef("VL_DEBUG");
    modp->جمع_کړه_Stmtsp(funcp);
    لكل (Ast_غوټه* np = modp->stmtsp(); np; np = np->nextp()) {
        إذا (AstVar* ثابت varp = VN_CAST(np, Var)) {
            إذا (varp->isPrimaryInish() && !varp->isSc()) {
                إذا (ثابت AstBasicDType* ثابت basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ثابت صحيح stored_پلنوالی = basicp->پلنوالی_Align_بایټونه() * 8;
                    ثابت صحيح وروستی_کلمه_پلنوالی = varp->پلنوالی() % stored_پلنوالی;
                    إذا (وروستی_کلمه_پلنوالی != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ثابت vrefp
                            = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->self_اشاره_ګر(VSelf_اشاره_ګر_متن{VSelf_اشاره_ګر_متن::This{}});
                        Ast_غوټه_بیان* newp = vrefp;
                        إذا (varp->isWide()) {
                            newp = جديد Ast_کلمه_Sel{
                                varp->fileline(), newp,
                                جديد AstConst(varp->fileline(), varp->پلنوالی_ټکي() - 1)};
                        }
                        ثابت uint64_t ارزښت = VL_ماسک_Q(stored_پلنوالی) & ~VL_ماسک_Q(وروستی_کلمه_پلنوالی);
                        newp = جديد AstAnd{varp->fileline(), newp,
                                          جديد AstConst(varp->fileline(), AstConst::Widthed_ارزښت{},
                                                       stored_پلنوالی, ارزښت)};
                        Ast_غوټه_If* ثابت ifp = جديد AstIf{
                            varp->fileline(), newp,
                            جديد AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->جمع_کړه_Stmtsp(ifp);
                    }
                }
            }
        }
    }
}

فراغ V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_ونه("cctors", 0, dump_ونه_Either_Level() >= 3);
}
