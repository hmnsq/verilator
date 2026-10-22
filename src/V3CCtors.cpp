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
    Ast_ڳنڍ_Module* ثابت m_modp;  // Current module/class
    ثابت string m_basename;
    ثابت VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    صحيح m_انگ_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ثابت صحيح func_انگ = m_newFunctions.سائيز();
        ثابت string funcName = m_basename + "_" + cvt_منزل_Str(func_انگ);
        AstCFunc* ثابت funcp = جديد AstCFunc{m_modp->fileline(), funcName, خالي_اشارو, "void"};
        funcp->isStatic(خطأ);
        funcp->isLoose(!m_type.isClass());
        funcp->keep_If_خالي_جايون(سچ);  // TODO relax
        funcp->decl_ذاتي(سچ);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        إذا (!m_type.isCoverage()) m_modp->ctorVarReset(سچ);
        string prevent_اڻ_استعمال_ٿيل_Stmt;
        إذا (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            prevent_اڻ_استعمال_ٿيل_Stmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } وإلا إذا (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            prevent_اڻ_استعمال_ٿيل_Stmt = "(void)first;  // Prevent unused variable warning";
        }
        إذا (!prevent_اڻ_استعمال_ٿيل_Stmt.خالي_جايون()) {
            funcp->جوڙيو_Stmtsp(جديد AstCStmt{m_modp->fileline(), prevent_اڻ_استعمال_ٿيل_Stmt});
        }
        m_modp->جوڙيو_Stmtsp(funcp);
        m_انگ_Stmts = 0;
        أرجع funcp;
    }

عام:
    فراغ جوڙيو(Ast_ڳنڍ* nodep) {
        إذا (m_newFunctions.خالي_جايون()) m_newFunctions.ڌڪو_پٺ(makeNewFunc());
        إذا (v3Global.opt.output_ورهايو_CFuncs() && m_انگ_Stmts > v3Global.opt.output_ورهايو_CFuncs()) {
            m_newFunctions.ڌڪو_پٺ(makeNewFunc());
        }
        m_newFunctions.پٺ()->جوڙيو_Stmtsp(nodep);
        m_انگ_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_ڳنڍ_Module* nodep, ثابت string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        إذا (m_type.isCoverage()) m_newFunctions.ڌڪو_پٺ(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        إذا (m_newFunctions.سائيز() == 0) {
        } وإلا إذا (m_newFunctions.سائيز() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } وإلا {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ثابت پاڙ_Funcp = makeNewFunc();
            پاڙ_Funcp->name(m_basename);
            لكل (AstCFunc* ثابت funcp : m_newFunctions) {
                AstCCall* ثابت callp = جديد AstCCall{m_modp->fileline(), funcp};
                callp->dtype_سيٽ_Void();
                إذا (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } وإلا {
                    إذا (m_type.isCoverage()) callp->argTypes("first");
                    callp->self_اشارو(VSelf_اشارو_متن{VSelf_اشارو_متن::This{}});
                }
                پاڙ_Funcp->جوڙيو_Stmtsp(callp->makeStmt());
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
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    AstCFunc* m_cfuncp = خالي_اشارو;  // Current function
    V3CCtorsBuilder* m_varResetp = خالي_اشارو;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ثابت Ast_ڳنڍ_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ساكن فراغ وجهو_Sc(AstCFunc* cfuncp, ثابت Ast_ڳنڍ_Module* modp, VSystemCSectionType type) {
        ثابت خودڪار txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        إذا (txtAndFlp.پهريون.خالي_جايون()) أرجع;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ثابت cstmtp = جديد AstCStmtUser{txtAndFlp.ٻيو};
        cstmtp->جوڙيو(txtAndFlp.پهريون);
        cfuncp->جوڙيو_Stmtsp(cstmtp);
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
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
            لكل (Ast_ڳنڍ *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                إذا (Ast_ڳنڍ_Cover_Decl* ثابت coverp = VN_CAST(np, ڳنڍ_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.جوڙيو(coverp->unlink_Fr_پٺ());
                }
            }
        }
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            AstCFunc* ثابت funcp = جديد AstCFunc{classp->fileline(), "~", خالي_اشارو, ""};
            funcp->isDestructor(سچ);
            funcp->isStatic(خطأ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(خطأ);
            وجهو_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->جوڙيو_Stmtsp(funcp);
        }
    }

    فراغ گهمو(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = خالي_اشارو;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        إذا (nodep->name() == "new") وجهو_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    فراغ گهمو(AstCStmt* nodep) override {
        إذا (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstVar* nodep) override {
        إذا (nodep->needsCReset()) {
            Ast_ڳنڍ* ثابت crstp = جديد Ast_سونپيو{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جديد AstCReset{nodep->fileline(), nodep, سچ}};
            إذا (m_varResetp) {
                m_varResetp->جوڙيو(crstp);
            } وإلا إذا (m_cfuncp) {
                nodep->جوڙيو_ايندڙ_هتي(crstp);
            } وإلا {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح CCtorsVisitor(Ast_ڳنڍ* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        لكل (خودڪار& itr : m_ctorCalls) {
            AstCStmt* ثابت nodep = itr.پهريون;
            ثابت Ast_ڳنڍ_Module* ثابت modp = itr.ٻيو;
            إذا (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
        }
    }
};

//######################################################################

فراغ V3CCtors::evalAsserts() {
    Ast_ڳنڍ_Module* ثابت modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ثابت funcp
        = جديد AstCFunc{modp->fileline(), "_eval_debug_assertions", خالي_اشارو, "void"};
    funcp->decl_ذاتي(سچ);
    funcp->isStatic(خطأ);
    funcp->isLoose(سچ);
    funcp->keep_If_خالي_جايون(سچ);
    funcp->slow(خطأ);
    funcp->ifdef("VL_DEBUG");
    modp->جوڙيو_Stmtsp(funcp);
    لكل (Ast_ڳنڍ* np = modp->stmtsp(); np; np = np->nextp()) {
        إذا (AstVar* ثابت varp = VN_CAST(np, Var)) {
            إذا (varp->isPrimaryInish() && !varp->isSc()) {
                إذا (ثابت AstBasicDType* ثابت basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ثابت صحيح stored_ويڪر = basicp->ويڪر_Align_بائيٽون() * 8;
                    ثابت صحيح آخري_لفظ_ويڪر = varp->ويڪر() % stored_ويڪر;
                    إذا (آخري_لفظ_ويڪر != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ثابت vrefp
                            = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->self_اشارو(VSelf_اشارو_متن{VSelf_اشارو_متن::This{}});
                        Ast_ڳنڍ_اظهار* newp = vrefp;
                        إذا (varp->isWide()) {
                            newp = جديد Ast_لفظ_Sel{
                                varp->fileline(), newp,
                                جديد AstConst(varp->fileline(), varp->ويڪر_لفظ() - 1)};
                        }
                        ثابت uint64_t قدر = VL_نقاب_Q(stored_ويڪر) & ~VL_نقاب_Q(آخري_لفظ_ويڪر);
                        newp = جديد AstAnd{varp->fileline(), newp,
                                          جديد AstConst(varp->fileline(), AstConst::Widthed_قدر{},
                                                       stored_ويڪر, قدر)};
                        Ast_ڳنڍ_If* ثابت ifp = جديد AstIf{
                            varp->fileline(), newp,
                            جديد AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->جوڙيو_Stmtsp(ifp);
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
    V3Global::dump_Check_Global_وڻ("cctors", 0, dump_وڻ_Either_Level() >= 3);
}
