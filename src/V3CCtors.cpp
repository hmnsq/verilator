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
    Ast_گره_Module* ثابت m_modp;  // Current module/class
    ثابت string m_basename;
    ثابت VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    صحيح m_رقم_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ثابت صحيح func_رقم = m_newFunctions.اندازه();
        ثابت string funcName = m_basename + "_" + cvt_مقصد_Str(func_رقم);
        AstCFunc* ثابت funcp = جديد AstCFunc{m_modp->fileline(), funcName, اشاره_تهی, "void"};
        funcp->isStatic(خطأ);
        funcp->isLoose(!m_type.isClass());
        funcp->keep_If_خالی(درست);  // TODO relax
        funcp->decl_خصوصی(درست);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        إذا (!m_type.isCoverage()) m_modp->ctorVarReset(درست);
        string prevent_استفادهنشده_Stmt;
        إذا (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            prevent_استفادهنشده_Stmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } وإلا إذا (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            prevent_استفادهنشده_Stmt = "(void)first;  // Prevent unused variable warning";
        }
        إذا (!prevent_استفادهنشده_Stmt.خالی()) {
            funcp->جمع_کن_Stmtsp(جديد AstCStmt{m_modp->fileline(), prevent_استفادهنشده_Stmt});
        }
        m_modp->جمع_کن_Stmtsp(funcp);
        m_رقم_Stmts = 0;
        أرجع funcp;
    }

عام:
    فراغ جمع_کن(Ast_گره* nodep) {
        إذا (m_newFunctions.خالی()) m_newFunctions.افزودن_پشت(makeNewFunc());
        إذا (v3Global.opt.output_تقسیم_CFuncs() && m_رقم_Stmts > v3Global.opt.output_تقسیم_CFuncs()) {
            m_newFunctions.افزودن_پشت(makeNewFunc());
        }
        m_newFunctions.پشت()->جمع_کن_Stmtsp(nodep);
        m_رقم_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_گره_Module* nodep, ثابت string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        إذا (m_type.isCoverage()) m_newFunctions.افزودن_پشت(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        إذا (m_newFunctions.اندازه() == 0) {
        } وإلا إذا (m_newFunctions.اندازه() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } وإلا {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ثابت ریشه_Funcp = makeNewFunc();
            ریشه_Funcp->name(m_basename);
            لكل (AstCFunc* ثابت funcp : m_newFunctions) {
                AstCCall* ثابت callp = جديد AstCCall{m_modp->fileline(), funcp};
                callp->dtype_مجموعه_Void();
                إذا (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } وإلا {
                    إذا (m_type.isCoverage()) callp->argTypes("first");
                    callp->self_اشاره_گر(VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::This{}});
                }
                ریشه_Funcp->جمع_کن_Stmtsp(callp->makeStmt());
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
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    AstCFunc* m_cfuncp = اشاره_تهی;  // Current function
    V3CCtorsBuilder* m_varResetp = اشاره_تهی;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ثابت Ast_گره_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ساكن فراغ درج_Sc(AstCFunc* cfuncp, ثابت Ast_گره_Module* modp, VSystemCSectionType type) {
        ثابت خودکار txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        إذا (txtAndFlp.اولی.خالی()) أرجع;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ثابت cstmtp = جديد AstCStmtUser{txtAndFlp.دومی};
        cstmtp->جمع_کن(txtAndFlp.اولی);
        cfuncp->جمع_کن_Stmtsp(cstmtp);
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
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
            لكل (Ast_گره *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                إذا (Ast_گره_Cover_Decl* ثابت coverp = VN_CAST(np, گره_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.جمع_کن(coverp->unlink_Fr_پشت());
                }
            }
        }
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            AstCFunc* ثابت funcp = جديد AstCFunc{classp->fileline(), "~", اشاره_تهی, ""};
            funcp->isDestructor(درست);
            funcp->isStatic(خطأ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(خطأ);
            درج_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->جمع_کن_Stmtsp(funcp);
        }
    }

    فراغ بازدید(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = اشاره_تهی;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        إذا (nodep->name() == "new") درج_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    فراغ بازدید(AstCStmt* nodep) override {
        إذا (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstVar* nodep) override {
        إذا (nodep->needsCReset()) {
            Ast_گره* ثابت crstp = جديد Ast_تخصیص_بده{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جديد AstCReset{nodep->fileline(), nodep, درست}};
            إذا (m_varResetp) {
                m_varResetp->جمع_کن(crstp);
            } وإلا إذا (m_cfuncp) {
                nodep->جمع_کن_بعدی_اینجا(crstp);
            } وإلا {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح CCtorsVisitor(Ast_گره* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        لكل (خودکار& itr : m_ctorCalls) {
            AstCStmt* ثابت nodep = itr.اولی;
            ثابت Ast_گره_Module* ثابت modp = itr.دومی;
            إذا (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
        }
    }
};

//######################################################################

فراغ V3CCtors::evalAsserts() {
    Ast_گره_Module* ثابت modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ثابت funcp
        = جديد AstCFunc{modp->fileline(), "_eval_debug_assertions", اشاره_تهی, "void"};
    funcp->decl_خصوصی(درست);
    funcp->isStatic(خطأ);
    funcp->isLoose(درست);
    funcp->keep_If_خالی(درست);
    funcp->slow(خطأ);
    funcp->ifdef("VL_DEBUG");
    modp->جمع_کن_Stmtsp(funcp);
    لكل (Ast_گره* np = modp->stmtsp(); np; np = np->nextp()) {
        إذا (AstVar* ثابت varp = VN_CAST(np, Var)) {
            إذا (varp->isPrimaryInish() && !varp->isSc()) {
                إذا (ثابت AstBasicDType* ثابت basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ثابت صحيح stored_عرض = basicp->عرض_Align_بایتها() * 8;
                    ثابت صحيح آخرین_کلمه_عرض = varp->عرض() % stored_عرض;
                    إذا (آخرین_کلمه_عرض != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ثابت vrefp
                            = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->self_اشاره_گر(VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::This{}});
                        Ast_گره_عبارت* newp = vrefp;
                        إذا (varp->isWide()) {
                            newp = جديد Ast_کلمه_Sel{
                                varp->fileline(), newp,
                                جديد AstConst(varp->fileline(), varp->عرض_واژگان() - 1)};
                        }
                        ثابت uint64_t مقدار = VL_ماسک_Q(stored_عرض) & ~VL_ماسک_Q(آخرین_کلمه_عرض);
                        newp = جديد AstAnd{varp->fileline(), newp,
                                          جديد AstConst(varp->fileline(), AstConst::Widthed_مقدار{},
                                                       stored_عرض, مقدار)};
                        Ast_گره_If* ثابت ifp = جديد AstIf{
                            varp->fileline(), newp,
                            جديد AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->جمع_کن_Stmtsp(ifp);
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
    V3Global::dump_Check_Global_درخت("cctors", 0, dump_درخت_Either_Level() >= 3);
}
