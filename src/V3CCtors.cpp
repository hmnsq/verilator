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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3CCtors.h"

#شامل "V3EmitCBase.h"

#شامل <list>

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت VCtorType final {
عوامی:
    شمار en : uint8_t { MODULE, CLASS, COVERAGE };

نجی:
    ثابت شمار en m_e;

عوامی:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    منطقی isClass() ثابت { واپس m_e == CLASS; }
    منطقی isCoverage() ثابت { واپس m_e == COVERAGE; }
};

جماعت V3CCtorsBuilder final {
    Ast_گرہ_Module* ثابت m_modp;  // Current module/class
    ثابت string m_basename;
    ثابت VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    صحیح m_عدد_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ثابت صحیح func_عدد = m_newFunctions.سائز();
        ثابت string funcName = m_basename + "_" + cvt_مقصد_Str(func_عدد);
        AstCFunc* ثابت funcp = نیا AstCFunc{m_modp->fileline(), funcName, خالی_اشاریہ, "void"};
        funcp->isStatic(جھوٹ);
        funcp->isLoose(!m_type.isClass());
        funcp->keep_If_خالی_خانے(سچ);  // TODO relax
        funcp->decl_نجی(سچ);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        اگر (!m_type.isCoverage()) m_modp->ctorVarReset(سچ);
        string prevent_غیر_مستعمل_Stmt;
        اگر (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            prevent_غیر_مستعمل_Stmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } ورنہ اگر (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            prevent_غیر_مستعمل_Stmt = "(void)first;  // Prevent unused variable warning";
        }
        اگر (!prevent_غیر_مستعمل_Stmt.خالی_خانے()) {
            funcp->جوڑو_Stmtsp(نیا AstCStmt{m_modp->fileline(), prevent_غیر_مستعمل_Stmt});
        }
        m_modp->جوڑو_Stmtsp(funcp);
        m_عدد_Stmts = 0;
        واپس funcp;
    }

عوامی:
    خالی جوڑو(Ast_گرہ* nodep) {
        اگر (m_newFunctions.خالی_خانے()) m_newFunctions.ڈالو_عقب(makeNewFunc());
        اگر (v3Global.opt.output_بانٹو_CFuncs() && m_عدد_Stmts > v3Global.opt.output_بانٹو_CFuncs()) {
            m_newFunctions.ڈالو_عقب(makeNewFunc());
        }
        m_newFunctions.عقب()->جوڑو_Stmtsp(nodep);
        m_عدد_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_گرہ_Module* nodep, ثابت string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        اگر (m_type.isCoverage()) m_newFunctions.ڈالو_عقب(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        اگر (m_newFunctions.سائز() == 0) {
        } ورنہ اگر (m_newFunctions.سائز() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } ورنہ {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ثابت جڑ_Funcp = makeNewFunc();
            جڑ_Funcp->name(m_basename);
            برائے (AstCFunc* ثابت funcp : m_newFunctions) {
                AstCCall* ثابت callp = نیا AstCCall{m_modp->fileline(), funcp};
                callp->dtype_سیٹ_Void();
                اگر (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } ورنہ {
                    اگر (m_type.isCoverage()) callp->argTypes("first");
                    callp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}});
                }
                جڑ_Funcp->جوڑو_Stmtsp(callp->makeStmt());
            }
        }
    }

نجی:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

جماعت CCtorsVisitor final : عوامی VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    AstCFunc* m_cfuncp = خالی_اشاریہ;  // Current function
    V3CCtorsBuilder* m_varResetp = خالی_اشاریہ;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ثابت Ast_گرہ_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ساکن خالی داخل_کرو_Sc(AstCFunc* cfuncp, ثابت Ast_گرہ_Module* modp, VSystemCSectionType type) {
        ثابت خودکار txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        اگر (txtAndFlp.پہلا.خالی_خانے()) واپس;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ثابت cstmtp = نیا AstCStmtUser{txtAndFlp.دوسرا};
        cstmtp->جوڑو(txtAndFlp.پہلا);
        cfuncp->جوڑو_Stmtsp(cstmtp);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        اگر (nodep->isConstPool()) واپس;  // Special emit rules
        اگر (ثابت AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            اگر (classp->isInterfaceClass()) واپس;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        اگر (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            برائے (Ast_گرہ *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                اگر (Ast_گرہ_Cover_Decl* ثابت coverp = VN_CAST(np, گرہ_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.جوڑو(coverp->unlink_Fr_عقب());
                }
            }
        }
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            AstCFunc* ثابت funcp = نیا AstCFunc{classp->fileline(), "~", خالی_اشاریہ, ""};
            funcp->isDestructor(سچ);
            funcp->isStatic(جھوٹ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(جھوٹ);
            داخل_کرو_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->جوڑو_Stmtsp(funcp);
        }
    }

    خالی دورہ(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = خالی_اشاریہ;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        اگر (nodep->name() == "new") داخل_کرو_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    خالی دورہ(AstCStmt* nodep) override {
        اگر (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstVar* nodep) override {
        اگر (nodep->needsCReset()) {
            Ast_گرہ* ثابت crstp = نیا Ast_تفویض{
                nodep->fileline(), نیا AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                نیا AstCReset{nodep->fileline(), nodep, سچ}};
            اگر (m_varResetp) {
                m_varResetp->جوڑو(crstp);
            } ورنہ اگر (m_cfuncp) {
                nodep->جوڑو_اگلا_یہاں(crstp);
            } ورنہ {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح CCtorsVisitor(Ast_گرہ* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        برائے (خودکار& itr : m_ctorCalls) {
            AstCStmt* ثابت nodep = itr.پہلا;
            ثابت Ast_گرہ_Module* ثابت modp = itr.دوسرا;
            اگر (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        }
    }
};

//######################################################################

خالی V3CCtors::evalAsserts() {
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ثابت funcp
        = نیا AstCFunc{modp->fileline(), "_eval_debug_assertions", خالی_اشاریہ, "void"};
    funcp->decl_نجی(سچ);
    funcp->isStatic(جھوٹ);
    funcp->isLoose(سچ);
    funcp->keep_If_خالی_خانے(سچ);
    funcp->slow(جھوٹ);
    funcp->ifdef("VL_DEBUG");
    modp->جوڑو_Stmtsp(funcp);
    برائے (Ast_گرہ* np = modp->stmtsp(); np; np = np->nextp()) {
        اگر (AstVar* ثابت varp = VN_CAST(np, Var)) {
            اگر (varp->isPrimaryInish() && !varp->isSc()) {
                اگر (ثابت AstBasicDType* ثابت basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ثابت صحیح stored_چوڑائی = basicp->چوڑائی_Align_بائٹیں() * 8;
                    ثابت صحیح آخری_لفظ_چوڑائی = varp->چوڑائی() % stored_چوڑائی;
                    اگر (آخری_لفظ_چوڑائی != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ثابت vrefp
                            = نیا AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::This{}});
                        Ast_گرہ_اظہاریہ* newp = vrefp;
                        اگر (varp->isWide()) {
                            newp = نیا Ast_لفظ_Sel{
                                varp->fileline(), newp,
                                نیا AstConst(varp->fileline(), varp->چوڑائی_الفاظ() - 1)};
                        }
                        ثابت uint64_t قدر = VL_نقاب_Q(stored_چوڑائی) & ~VL_نقاب_Q(آخری_لفظ_چوڑائی);
                        newp = نیا AstAnd{varp->fileline(), newp,
                                          نیا AstConst(varp->fileline(), AstConst::Widthed_قدر{},
                                                       stored_چوڑائی, قدر)};
                        Ast_گرہ_If* ثابت ifp = نیا AstIf{
                            varp->fileline(), newp,
                            نیا AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->جوڑو_Stmtsp(ifp);
                    }
                }
            }
        }
    }
}

خالی V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_درخت("cctors", 0, dump_درخت_Either_Level() >= 3);
}
