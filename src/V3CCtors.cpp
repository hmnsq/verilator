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
    Ast_گرہ_Module* ثابت m_modp;  // Current module/class
    ثابت string m_basename;
    ثابت VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    صحيح m_عدد_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ثابت صحيح func_عدد = m_newFunctions.ناپ();
        ثابت string funcName = m_basename + "_" + cvt_منزل_Str(func_عدد);
        AstCFunc* ثابت funcp = جديد AstCFunc{m_modp->fileline(), funcName, خالی_سوچک, "void"};
        funcp->isStatic(خطأ);
        funcp->isLoose(!m_type.isClass());
        funcp->keep_If_خالی_تھاواں(سچ);  // TODO relax
        funcp->decl_نجی(سچ);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        إذا (!m_type.isCoverage()) m_modp->ctorVarReset(سچ);
        string prevent_بن_ورتیا_Stmt;
        إذا (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            prevent_بن_ورتیا_Stmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } وإلا إذا (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            prevent_بن_ورتیا_Stmt = "(void)first;  // Prevent unused variable warning";
        }
        إذا (!prevent_بن_ورتیا_Stmt.خالی_تھاواں()) {
            funcp->جوڑو_Stmtsp(جديد AstCStmt{m_modp->fileline(), prevent_بن_ورتیا_Stmt});
        }
        m_modp->جوڑو_Stmtsp(funcp);
        m_عدد_Stmts = 0;
        أرجع funcp;
    }

عام:
    فراغ جوڑو(Ast_گرہ* nodep) {
        إذا (m_newFunctions.خالی_تھاواں()) m_newFunctions.ڈالو_پٹھ(makeNewFunc());
        إذا (v3Global.opt.output_ونڈو_CFuncs() && m_عدد_Stmts > v3Global.opt.output_ونڈو_CFuncs()) {
            m_newFunctions.ڈالو_پٹھ(makeNewFunc());
        }
        m_newFunctions.پٹھ()->جوڑو_Stmtsp(nodep);
        m_عدد_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_گرہ_Module* nodep, ثابت string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        إذا (m_type.isCoverage()) m_newFunctions.ڈالو_پٹھ(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        إذا (m_newFunctions.ناپ() == 0) {
        } وإلا إذا (m_newFunctions.ناپ() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } وإلا {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ثابت جڑ_Funcp = makeNewFunc();
            جڑ_Funcp->name(m_basename);
            لكل (AstCFunc* ثابت funcp : m_newFunctions) {
                AstCCall* ثابت callp = جديد AstCCall{m_modp->fileline(), funcp};
                callp->dtype_سیٹ_Void();
                إذا (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } وإلا {
                    إذا (m_type.isCoverage()) callp->argTypes("first");
                    callp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}});
                }
                جڑ_Funcp->جوڑو_Stmtsp(callp->makeStmt());
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
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    AstCFunc* m_cfuncp = خالی_سوچک;  // Current function
    V3CCtorsBuilder* m_varResetp = خالی_سوچک;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ثابت Ast_گرہ_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ساكن فراغ داخل_کرو_Sc(AstCFunc* cfuncp, ثابت Ast_گرہ_Module* modp, VSystemCSectionType type) {
        ثابت خودکار txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        إذا (txtAndFlp.پہلا.خالی_تھاواں()) أرجع;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ثابت cstmtp = جديد AstCStmtUser{txtAndFlp.دوجا};
        cstmtp->جوڑو(txtAndFlp.پہلا);
        cfuncp->جوڑو_Stmtsp(cstmtp);
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
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
            لكل (Ast_گرہ *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                إذا (Ast_گرہ_Cover_Decl* ثابت coverp = VN_CAST(np, گرہ_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.جوڑو(coverp->unlink_Fr_پٹھ());
                }
            }
        }
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            AstCFunc* ثابت funcp = جديد AstCFunc{classp->fileline(), "~", خالی_سوچک, ""};
            funcp->isDestructor(سچ);
            funcp->isStatic(خطأ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(خطأ);
            داخل_کرو_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->جوڑو_Stmtsp(funcp);
        }
    }

    فراغ ویکھو(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = خالی_سوچک;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        إذا (nodep->name() == "new") داخل_کرو_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    فراغ ویکھو(AstCStmt* nodep) override {
        إذا (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstVar* nodep) override {
        إذا (nodep->needsCReset()) {
            Ast_گرہ* ثابت crstp = جديد Ast_سونپو{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جديد AstCReset{nodep->fileline(), nodep, سچ}};
            إذا (m_varResetp) {
                m_varResetp->جوڑو(crstp);
            } وإلا إذا (m_cfuncp) {
                nodep->جوڑو_اگلا_ایتھے(crstp);
            } وإلا {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح CCtorsVisitor(Ast_گرہ* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        لكل (خودکار& itr : m_ctorCalls) {
            AstCStmt* ثابت nodep = itr.پہلا;
            ثابت Ast_گرہ_Module* ثابت modp = itr.دوجا;
            إذا (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
        }
    }
};

//######################################################################

فراغ V3CCtors::evalAsserts() {
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ثابت funcp
        = جديد AstCFunc{modp->fileline(), "_eval_debug_assertions", خالی_سوچک, "void"};
    funcp->decl_نجی(سچ);
    funcp->isStatic(خطأ);
    funcp->isLoose(سچ);
    funcp->keep_If_خالی_تھاواں(سچ);
    funcp->slow(خطأ);
    funcp->ifdef("VL_DEBUG");
    modp->جوڑو_Stmtsp(funcp);
    لكل (Ast_گرہ* np = modp->stmtsp(); np; np = np->nextp()) {
        إذا (AstVar* ثابت varp = VN_CAST(np, Var)) {
            إذا (varp->isPrimaryInish() && !varp->isSc()) {
                إذا (ثابت AstBasicDType* ثابت basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ثابت صحيح stored_چوڑائی = basicp->چوڑائی_Align_بائٹاں() * 8;
                    ثابت صحيح آخری_لفظ_چوڑائی = varp->چوڑائی() % stored_چوڑائی;
                    إذا (آخری_لفظ_چوڑائی != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ثابت vrefp
                            = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}});
                        Ast_گرہ_اظہاریہ* newp = vrefp;
                        إذا (varp->isWide()) {
                            newp = جديد Ast_لفظ_Sel{
                                varp->fileline(), newp,
                                جديد AstConst(varp->fileline(), varp->چوڑائی_الفاظ() - 1)};
                        }
                        ثابت uint64_t قدر = VL_نقاب_Q(stored_چوڑائی) & ~VL_نقاب_Q(آخری_لفظ_چوڑائی);
                        newp = جديد AstAnd{varp->fileline(), newp,
                                          جديد AstConst(varp->fileline(), AstConst::Widthed_قدر{},
                                                       stored_چوڑائی, قدر)};
                        Ast_گرہ_If* ثابت ifp = جديد AstIf{
                            varp->fileline(), newp,
                            جديد AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->جوڑو_Stmtsp(ifp);
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
