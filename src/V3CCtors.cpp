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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3CCtors.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

#ಅನ್ತರ್ಭಾವ <list>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ VCtorType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { MODULE, CLASS, COVERAGE };

ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ ಎಣಿಕೆ en m_e;

ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    ತಾರ್ಕಿಕ isClass() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == CLASS; }
    ತಾರ್ಕಿಕ isCoverage() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == COVERAGE; }
};

ವರ್ಗ V3CCtorsBuilder final {
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Current module/class
    ಸ್ಥಿರಾಂಕ string m_basename;
    ಸ್ಥಿರಾಂಕ VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    ಪೂರ್ಣಾಂಕ m_ಅಂಕೆ_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ func_ಅಂಕೆ = m_newFunctions.ಗಾತ್ರ_2();
        ಸ್ಥಿರಾಂಕ string funcName = m_basename + "_" + cvt_ಗಮ್ಯ_Str(func_ಅಂಕೆ);
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{m_modp->fileline(), funcName, ಶೂನ್ಯಸೂಚಕ, "void"};
        funcp->isStatic(ಸುಳ್ಳು);
        funcp->isLoose(!m_type.isClass());
        funcp->keep_If_ಖಾಲಿಗಳು(ನಿಜ);  // TODO relax
        funcp->decl_ಖಾಸಗಿ(ನಿಜ);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        ಆದರೆ (!m_type.isCoverage()) m_modp->ctorVarReset(ನಿಜ);
        string prevent_ಬಳಸದ_Stmt;
        ಆದರೆ (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            prevent_ಬಳಸದ_Stmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            prevent_ಬಳಸದ_Stmt = "(void)first;  // Prevent unused variable warning";
        }
        ಆದರೆ (!prevent_ಬಳಸದ_Stmt.ಖಾಲಿಗಳು()) {
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{m_modp->fileline(), prevent_ಬಳಸದ_Stmt});
        }
        m_modp->ಕೂಡಿಸು_Stmtsp(funcp);
        m_ಅಂಕೆ_Stmts = 0;
        ಹಿಂತಿರುಗಿ funcp;
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ ಕೂಡಿಸು(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (m_newFunctions.ಖಾಲಿಗಳು()) m_newFunctions.ತಳ್ಳು_ಹಿಂಭಾಗ(makeNewFunc());
        ಆದರೆ (v3Global.opt.output_ವಿಭಜಿಸು_CFuncs() && m_ಅಂಕೆ_Stmts > v3Global.opt.output_ವಿಭಜಿಸು_CFuncs()) {
            m_newFunctions.ತಳ್ಳು_ಹಿಂಭಾಗ(makeNewFunc());
        }
        m_newFunctions.ಹಿಂಭಾಗ()->ಕೂಡಿಸು_Stmtsp(nodep);
        m_ಅಂಕೆ_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_ಗಂಟು_Module* nodep, ಸ್ಥಿರಾಂಕ string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        ಆದರೆ (m_type.isCoverage()) m_newFunctions.ತಳ್ಳು_ಹಿಂಭಾಗ(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        ಆದರೆ (m_newFunctions.ಗಾತ್ರ_2() == 0) {
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_newFunctions.ಗಾತ್ರ_2() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ಸ್ಥಿರಾಂಕ ಬೇರು_Funcp = makeNewFunc();
            ಬೇರು_Funcp->name(m_basename);
            ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp : m_newFunctions) {
                AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{m_modp->fileline(), funcp};
                callp->dtype_ಗಣ_Void();
                ಆದರೆ (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (m_type.isCoverage()) callp->argTypes("first");
                    callp->self_ಸೂಚಕ(VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::This{}});
                }
                ಬೇರು_Funcp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
            }
        }
    }

ಖಾಸಗಿ:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

ವರ್ಗ CCtorsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current function
    V3CCtorsBuilder* m_varResetp = ಶೂನ್ಯಸೂಚಕ;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ ಸೇರಿಸು_Sc(AstCFunc* cfuncp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, VSystemCSectionType type) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        ಆದರೆ (txtAndFlp.ಮೊದಲ.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmtUser{txtAndFlp.ಎರಡನೇ};
        cstmtp->ಕೂಡಿಸು(txtAndFlp.ಮೊದಲ);
        cfuncp->ಕೂಡಿಸು_Stmtsp(cstmtp);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (nodep->isConstPool()) ಹಿಂತಿರುಗಿ;  // Special emit rules
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            ಆದರೆ (classp->isInterfaceClass()) ಹಿಂತಿರುಗಿ;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        ಆದರೆ (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            ಗಾಗಿ (Ast_ಗಂಟು *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                ಆದರೆ (Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ coverp = VN_CAST(np, ಗಂಟು_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.ಕೂಡಿಸು(coverp->unlink_Fr_ಹಿಂಭಾಗ());
                }
            }
        }
        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep, Class)) {
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{classp->fileline(), "~", ಶೂನ್ಯಸೂಚಕ, ""};
            funcp->isDestructor(ನಿಜ);
            funcp->isStatic(ಸುಳ್ಳು);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(ಸುಳ್ಳು);
            ಸೇರಿಸು_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->ಕೂಡಿಸು_Stmtsp(funcp);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = ಶೂನ್ಯಸೂಚಕ;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ಆದರೆ (nodep->name() == "new") ಸೇರಿಸು_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmt* nodep) override {
        ಆದರೆ (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (nodep->needsCReset()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ crstp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                ಹೊಸ AstCReset{nodep->fileline(), nodep, ನಿಜ}};
            ಆದರೆ (m_varResetp) {
                m_varResetp->ಕೂಡಿಸು(crstp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_cfuncp) {
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(crstp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CCtorsVisitor(Ast_ಗಂಟು* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& itr : m_ctorCalls) {
            AstCStmt* ಸ್ಥಿರಾಂಕ nodep = itr.ಮೊದಲ;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = itr.ಎರಡನೇ;
            ಆದರೆ (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        }
    }
};

//######################################################################

ಶೂನ್ಯ V3CCtors::evalAsserts() {
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp
        = ಹೊಸ AstCFunc{modp->fileline(), "_eval_debug_assertions", ಶೂನ್ಯಸೂಚಕ, "void"};
    funcp->decl_ಖಾಸಗಿ(ನಿಜ);
    funcp->isStatic(ಸುಳ್ಳು);
    funcp->isLoose(ನಿಜ);
    funcp->keep_If_ಖಾಲಿಗಳು(ನಿಜ);
    funcp->slow(ಸುಳ್ಳು);
    funcp->ifdef("VL_DEBUG");
    modp->ಕೂಡಿಸು_Stmtsp(funcp);
    ಗಾಗಿ (Ast_ಗಂಟು* np = modp->stmtsp(); np; np = np->nextp()) {
        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(np, Var)) {
            ಆದರೆ (varp->isPrimaryInish() && !varp->isSc()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ stored_ಅಗಲ = basicp->ಅಗಲ_Align_ಬೈಟುಗಳು() * 8;
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಕೊನೆಯ_ಶಬ್ದ_ಅಗಲ = varp->ಅಗಲ() % stored_ಅಗಲ;
                    ಆದರೆ (ಕೊನೆಯ_ಶಬ್ದ_ಅಗಲ != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ಸ್ಥಿರಾಂಕ vrefp
                            = ಹೊಸ AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->self_ಸೂಚಕ(VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::This{}});
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = vrefp;
                        ಆದರೆ (varp->isWide()) {
                            newp = ಹೊಸ Ast_ಶಬ್ದ_Sel{
                                varp->fileline(), newp,
                                ಹೊಸ AstConst(varp->fileline(), varp->ಅಗಲ_ಪದಗಳು() - 1)};
                        }
                        ಸ್ಥಿರಾಂಕ uint64_t ಮೌಲ್ಯ = VL_ಮುಖವಾಡ_Q(stored_ಅಗಲ) & ~VL_ಮುಖವಾಡ_Q(ಕೊನೆಯ_ಶಬ್ದ_ಅಗಲ);
                        newp = ಹೊಸ AstAnd{varp->fileline(), newp,
                                          ಹೊಸ AstConst(varp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                                       stored_ಅಗಲ, ಮೌಲ್ಯ)};
                        Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{
                            varp->fileline(), newp,
                            ಹೊಸ AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->ಕೂಡಿಸು_Stmtsp(ifp);
                    }
                }
            }
        }
    }
}

ಶೂನ್ಯ V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("cctors", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
