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
    ಗಣನಾ en : uint8_t { MODULE, CLASS, COVERAGE };

ಗುಪ್ತ:
    ಧ್ರುವ ಗಣನಾ en m_e;

ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    ತಾರ್ಕಿಕ isClass() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == CLASS; }
    ತಾರ್ಕಿಕ isCoverage() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == COVERAGE; }
};

ವರ್ಗ V3CCtorsBuilder final {
    Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Current module/class
    ಧ್ರುವ string m_basename;
    ಧ್ರುವ VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    ಪೂರ್ಣಾಙ್ಕ m_ಅಂಕೆ_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ func_ಅಂಕೆ = m_newFunctions.size();
        ಧ್ರುವ string funcName = m_basename + "_" + cvtToStr(func_ಅಂಕೆ);
        AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(ಅಸತ್ಯ);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(ಸತ್ಯ);  // TODO relax
        funcp->declPrivate(ಸತ್ಯ);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        ಯದಿ (!m_type.isCoverage()) m_modp->ctorVarReset(ಸತ್ಯ);
        string preventUnusedStmt;
        ಯದಿ (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } ಅನ್ಯಥಾ ಯದಿ (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        ಯದಿ (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(ನೂತನ AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_ಅಂಕೆ_Stmts = 0;
        ಪ್ರತ್ಯಾಗಮ funcp;
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ add(Ast_ಗಂಟು* nodep) {
        ಯದಿ (m_newFunctions.empty()) m_newFunctions.ತಳ್ಳು_back(makeNewFunc());
        ಯದಿ (v3Global.opt.outputSplitCFuncs() && m_ಅಂಕೆ_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.ತಳ್ಳು_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_ಅಂಕೆ_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_ಗಂಟು_Module* nodep, ಧ್ರುವ string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        ಯದಿ (m_type.isCoverage()) m_newFunctions.ತಳ್ಳು_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        ಯದಿ (m_newFunctions.size() == 0) {
        } ಅನ್ಯಥಾ ಯದಿ (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } ಅನ್ಯಥಾ {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ಧ್ರುವ ಬೇರು_Funcp = makeNewFunc();
            ಬೇರು_Funcp->name(m_basename);
            ಪ್ರತಿ (AstCFunc* ಧ್ರುವ funcp : m_newFunctions) {
                AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                ಯದಿ (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } ಅನ್ಯಥಾ {
                    ಯದಿ (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                ಬೇರು_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

ಗುಪ್ತ:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

ವರ್ಗ CCtorsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ಧ್ರುವ Ast_ಗಂಟು_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ ಸೇರಿಸು_Sc(AstCFunc* cfuncp, ಧ್ರುವ Ast_ಗಂಟು_Module* modp, VSystemCSectionType type) {
        ಧ್ರುವ auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        ಯದಿ (txtAndFlp.first.empty()) ಪ್ರತ್ಯಾಗಮ;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ಧ್ರುವ cstmtp = ನೂತನ AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (nodep->isConstPool()) ಪ್ರತ್ಯಾಗಮ;  // Special emit rules
        ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            ಯದಿ (classp->isInterfaceClass()) ಪ್ರತ್ಯಾಗಮ;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        ಯದಿ (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            ಪ್ರತಿ (Ast_ಗಂಟು *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                ಯದಿ (Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ coverp = VN_CAST(np, ಗಂಟು_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(nodep, Class)) {
            AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(ಸತ್ಯ);
            funcp->isStatic(ಅಸತ್ಯ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(ಅಸತ್ಯ);
            ಸೇರಿಸು_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ಯದಿ (nodep->name() == "new") ಸೇರಿಸು_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    ಶೂನ್ಯ visit(AstCStmt* nodep) override {
        ಯದಿ (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (nodep->needsCReset()) {
            Ast_ಗಂಟು* ಧ್ರುವ crstp = ನೂತನ AstAssign{
                nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                ನೂತನ AstCReset{nodep->fileline(), nodep, ಸತ್ಯ}};
            ಯದಿ (m_varResetp) {
                m_varResetp->add(crstp);
            } ಅನ್ಯಥಾ ಯದಿ (m_cfuncp) {
                nodep->addNextHere(crstp);
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_ಗಂಟು* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        ಪ್ರತಿ (auto& itr : m_ctorCalls) {
            AstCStmt* ಧ್ರುವ nodep = itr.first;
            ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = itr.second;
            ಯದಿ (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        }
    }
};

//######################################################################

ಶೂನ್ಯ V3CCtors::evalAsserts() {
    Ast_ಗಂಟು_Module* ಧ್ರುವ modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ಧ್ರುವ funcp
        = ನೂತನ AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(ಸತ್ಯ);
    funcp->isStatic(ಅಸತ್ಯ);
    funcp->isLoose(ಸತ್ಯ);
    funcp->keepIfEmpty(ಸತ್ಯ);
    funcp->slow(ಅಸತ್ಯ);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    ಪ್ರತಿ (Ast_ಗಂಟು* np = modp->stmtsp(); np; np = np->nextp()) {
        ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(np, Var)) {
            ಯದಿ (varp->isPrimaryInish() && !varp->isSc()) {
                ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ storedWidth = basicp->widthAlignBytes() * 8;
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lastWordWidth = varp->width() % storedWidth;
                    ಯದಿ (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ಧ್ರುವ vrefp
                            = ನೂತನ AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = vrefp;
                        ಯದಿ (varp->isWide()) {
                            newp = ನೂತನ AstWordSel{
                                varp->fileline(), newp,
                                ನೂತನ AstConst(varp->fileline(), varp->width_ಪದಗಳು() - 1)};
                        }
                        ಧ್ರುವ uint64_t ಮೌಲ್ಯ = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = ನೂತನ AstAnd{varp->fileline(), newp,
                                          ನೂತನ AstConst(varp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                                       storedWidth, ಮೌಲ್ಯ)};
                        Ast_ಗಂಟು_If* ಧ್ರುವ ifp = ನೂತನ AstIf{
                            varp->fileline(), newp,
                            ನೂತನ AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
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
