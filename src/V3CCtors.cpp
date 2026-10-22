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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3CCtors.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <list>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ VCtorType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { MODULE, CLASS, COVERAGE };

ਨਿਜੀ:
    ਅਚਲ ਗਿਣਤੀ en m_e;

ਜਨਤਕ:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    ਤਾਰਕਿਕ isClass() ਅਚਲ { ਵਾਪਸ m_e == CLASS; }
    ਤਾਰਕਿਕ isCoverage() ਅਚਲ { ਵਾਪਸ m_e == COVERAGE; }
};

ਵਰਗ V3CCtorsBuilder final {
    Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Current module/class
    ਅਚਲ string m_basename;
    ਅਚਲ VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    ਪੂਰਨ m_ਅੰਕ_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ਅਚਲ ਪੂਰਨ func_ਅੰਕ = m_newFunctions.ਮਾਪ();
        ਅਚਲ string funcName = m_basename + "_" + cvt_ਮੰਜ਼ਿਲ_Str(func_ਅੰਕ);
        AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{m_modp->fileline(), funcName, ਖਾਲੀ_ਸੂਚਕ, "void"};
        funcp->isStatic(ਝੂਠ);
        funcp->isLoose(!m_type.isClass());
        funcp->keep_If_ਖਾਲੀ_ਥਾਵਾਂ(ਸੱਚ);  // TODO relax
        funcp->decl_ਨਿੱਜੀ(ਸੱਚ);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        ਜੇ (!m_type.isCoverage()) m_modp->ctorVarReset(ਸੱਚ);
        string prevent_ਅਣਵਰਤਿਆ_Stmt;
        ਜੇ (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            prevent_ਅਣਵਰਤਿਆ_Stmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } ਨਹੀਂਤਾਂ ਜੇ (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            prevent_ਅਣਵਰਤਿਆ_Stmt = "(void)first;  // Prevent unused variable warning";
        }
        ਜੇ (!prevent_ਅਣਵਰਤਿਆ_Stmt.ਖਾਲੀ_ਥਾਵਾਂ()) {
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{m_modp->fileline(), prevent_ਅਣਵਰਤਿਆ_Stmt});
        }
        m_modp->ਜੋੜੋ_Stmtsp(funcp);
        m_ਅੰਕ_Stmts = 0;
        ਵਾਪਸ funcp;
    }

ਜਨਤਕ:
    ਖਾਲੀ ਜੋੜੋ(Ast_ਗੰਢ* nodep) {
        ਜੇ (m_newFunctions.ਖਾਲੀ_ਥਾਵਾਂ()) m_newFunctions.ਧੱਕੋ_ਪਿੱਠ(makeNewFunc());
        ਜੇ (v3Global.opt.output_ਵੰਡੋ_CFuncs() && m_ਅੰਕ_Stmts > v3Global.opt.output_ਵੰਡੋ_CFuncs()) {
            m_newFunctions.ਧੱਕੋ_ਪਿੱਠ(makeNewFunc());
        }
        m_newFunctions.ਪਿੱਠ()->ਜੋੜੋ_Stmtsp(nodep);
        m_ਅੰਕ_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_ਗੰਢ_Module* nodep, ਅਚਲ string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        ਜੇ (m_type.isCoverage()) m_newFunctions.ਧੱਕੋ_ਪਿੱਠ(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        ਜੇ (m_newFunctions.ਮਾਪ() == 0) {
        } ਨਹੀਂਤਾਂ ਜੇ (m_newFunctions.ਮਾਪ() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } ਨਹੀਂਤਾਂ {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ਅਚਲ ਜੜ੍ਹ_Funcp = makeNewFunc();
            ਜੜ੍ਹ_Funcp->name(m_basename);
            ਲਈ (AstCFunc* ਅਚਲ funcp : m_newFunctions) {
                AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{m_modp->fileline(), funcp};
                callp->dtype_ਸੈੱਟ_Void();
                ਜੇ (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (m_type.isCoverage()) callp->argTypes("first");
                    callp->self_ਸੂਚਕ(VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::This{}});
                }
                ਜੜ੍ਹ_Funcp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
            }
        }
    }

ਨਿਜੀ:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

ਵਰਗ CCtorsVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current function
    V3CCtorsBuilder* m_varResetp = ਖਾਲੀ_ਸੂਚਕ;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ਅਚਲ Ast_ਗੰਢ_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ਸਥਿਰ ਖਾਲੀ ਪਾਓ_Sc(AstCFunc* cfuncp, ਅਚਲ Ast_ਗੰਢ_Module* modp, VSystemCSectionType type) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        ਜੇ (txtAndFlp.ਪਹਿਲਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmtUser{txtAndFlp.ਦੂਜਾ};
        cstmtp->ਜੋੜੋ(txtAndFlp.ਪਹਿਲਾ);
        cfuncp->ਜੋੜੋ_Stmtsp(cstmtp);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (nodep->isConstPool()) ਵਾਪਸ;  // Special emit rules
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            ਜੇ (classp->isInterfaceClass()) ਵਾਪਸ;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        ਜੇ (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            ਲਈ (Ast_ਗੰਢ *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                ਜੇ (Ast_ਗੰਢ_Cover_Decl* ਅਚਲ coverp = VN_CAST(np, ਗੰਢ_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.ਜੋੜੋ(coverp->unlink_Fr_ਪਿੱਠ());
                }
            }
        }
        ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(nodep, Class)) {
            AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{classp->fileline(), "~", ਖਾਲੀ_ਸੂਚਕ, ""};
            funcp->isDestructor(ਸੱਚ);
            funcp->isStatic(ਝੂਠ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(ਝੂਠ);
            ਪਾਓ_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->ਜੋੜੋ_Stmtsp(funcp);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = ਖਾਲੀ_ਸੂਚਕ;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ਜੇ (nodep->name() == "new") ਪਾਓ_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmt* nodep) override {
        ਜੇ (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (nodep->needsCReset()) {
            Ast_ਗੰਢ* ਅਚਲ crstp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                ਨਵਾਂ AstCReset{nodep->fileline(), nodep, ਸੱਚ}};
            ਜੇ (m_varResetp) {
                m_varResetp->ਜੋੜੋ(crstp);
            } ਨਹੀਂਤਾਂ ਜੇ (m_cfuncp) {
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(crstp);
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CCtorsVisitor(Ast_ਗੰਢ* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        ਲਈ (ਸਵੈਚਾਲਿਤ& itr : m_ctorCalls) {
            AstCStmt* ਅਚਲ nodep = itr.ਪਹਿਲਾ;
            ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = itr.ਦੂਜਾ;
            ਜੇ (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        }
    }
};

//######################################################################

ਖਾਲੀ V3CCtors::evalAsserts() {
    Ast_ਗੰਢ_Module* ਅਚਲ modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ਅਚਲ funcp
        = ਨਵਾਂ AstCFunc{modp->fileline(), "_eval_debug_assertions", ਖਾਲੀ_ਸੂਚਕ, "void"};
    funcp->decl_ਨਿੱਜੀ(ਸੱਚ);
    funcp->isStatic(ਝੂਠ);
    funcp->isLoose(ਸੱਚ);
    funcp->keep_If_ਖਾਲੀ_ਥਾਵਾਂ(ਸੱਚ);
    funcp->slow(ਝੂਠ);
    funcp->ifdef("VL_DEBUG");
    modp->ਜੋੜੋ_Stmtsp(funcp);
    ਲਈ (Ast_ਗੰਢ* np = modp->stmtsp(); np; np = np->nextp()) {
        ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(np, Var)) {
            ਜੇ (varp->isPrimaryInish() && !varp->isSc()) {
                ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ਅਚਲ ਪੂਰਨ stored_ਚੌੜਾਈ = basicp->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() * 8;
                    ਅਚਲ ਪੂਰਨ ਆਖਰੀ_ਸ਼ਬਦ_ਚੌੜਾਈ = varp->ਚੌੜਾਈ() % stored_ਚੌੜਾਈ;
                    ਜੇ (ਆਖਰੀ_ਸ਼ਬਦ_ਚੌੜਾਈ != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ਅਚਲ vrefp
                            = ਨਵਾਂ AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->self_ਸੂਚਕ(VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::This{}});
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = vrefp;
                        ਜੇ (varp->isWide()) {
                            newp = ਨਵਾਂ Ast_ਸ਼ਬਦ_Sel{
                                varp->fileline(), newp,
                                ਨਵਾਂ AstConst(varp->fileline(), varp->ਚੌੜਾਈ_ਸ਼ਬਦ() - 1)};
                        }
                        ਅਚਲ uint64_t ਮੁੱਲ = VL_ਨਕਾਬ_Q(stored_ਚੌੜਾਈ) & ~VL_ਨਕਾਬ_Q(ਆਖਰੀ_ਸ਼ਬਦ_ਚੌੜਾਈ);
                        newp = ਨਵਾਂ AstAnd{varp->fileline(), newp,
                                          ਨਵਾਂ AstConst(varp->fileline(), AstConst::Widthed_ਮੁੱਲ{},
                                                       stored_ਚੌੜਾਈ, ਮੁੱਲ)};
                        Ast_ਗੰਢ_If* ਅਚਲ ifp = ਨਵਾਂ AstIf{
                            varp->fileline(), newp,
                            ਨਵਾਂ AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->ਜੋੜੋ_Stmtsp(ifp);
                    }
                }
            }
        }
    }
}

ਖਾਲੀ V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("cctors", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
