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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3CCtors.h"

#অন্তর্ভাৱ "V3EmitCBase.h"

#অন্তর্ভাৱ <list>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্ৰেণী VCtorType final {
সার্ৱজনিক:
    গণনা en : uint8_t { MODULE, CLASS, COVERAGE };

গুপ্ত:
    ধ্রুৱ গণনা en m_e;

সার্ৱজনিক:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    তার্কিক isClass() ধ্রুৱ { ঘূৰাই m_e == CLASS; }
    তার্কিক isCoverage() ধ্রুৱ { ঘূৰাই m_e == COVERAGE; }
};

শ্ৰেণী V3CCtorsBuilder final {
    Ast_গ্ৰন্থি_Module* ধ্রুৱ m_modp;  // Current module/class
    ধ্রুৱ string m_basename;
    ধ্রুৱ VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    পূর্ণাঙ্ক m_অঙ্ক_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ধ্রুৱ পূর্ণাঙ্ক func_অঙ্ক = m_newFunctions.size();
        ধ্রুৱ string funcName = m_basename + "_" + cvtToStr(func_অঙ্ক);
        AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(মিছা);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(সঁচা);  // TODO relax
        funcp->declPrivate(সঁচা);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        যদি (!m_type.isCoverage()) m_modp->ctorVarReset(সঁচা);
        string preventUnusedStmt;
        যদি (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } অন্যথা যদি (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        যদি (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(নূতন AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_অঙ্ক_Stmts = 0;
        ঘূৰাই funcp;
    }

সার্ৱজনিক:
    শূন্য add(Ast_গ্ৰন্থি* nodep) {
        যদি (m_newFunctions.empty()) m_newFunctions.ঠেলো_back(makeNewFunc());
        যদি (v3Global.opt.outputSplitCFuncs() && m_অঙ্ক_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.ঠেলো_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_অঙ্ক_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_গ্ৰন্থি_Module* nodep, ধ্রুৱ string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        যদি (m_type.isCoverage()) m_newFunctions.ঠেলো_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        যদি (m_newFunctions.size() == 0) {
        } অন্যথা যদি (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } অন্যথা {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ধ্রুৱ মূল_Funcp = makeNewFunc();
            মূল_Funcp->name(m_basename);
            বাবে (AstCFunc* ধ্রুৱ funcp : m_newFunctions) {
                AstCCall* ধ্রুৱ callp = নূতন AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                যদি (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } অন্যথা {
                    যদি (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                মূল_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

গুপ্ত:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

শ্ৰেণী CCtorsVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ধ্রুৱ Ast_গ্ৰন্থি_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    স্থির শূন্য ঢোকাও_Sc(AstCFunc* cfuncp, ধ্রুৱ Ast_গ্ৰন্থি_Module* modp, VSystemCSectionType type) {
        ধ্রুৱ auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        যদি (txtAndFlp.first.empty()) ঘূৰাই;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ধ্রুৱ cstmtp = নূতন AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        যদি (nodep->isConstPool()) ঘূৰাই;  // Special emit rules
        যদি (ধ্রুৱ AstClass* ধ্রুৱ classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            যদি (classp->isInterfaceClass()) ঘূৰাই;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        যদি (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            বাবে (Ast_গ্ৰন্থি *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                যদি (Ast_গ্ৰন্থি_Cover_Decl* ধ্রুৱ coverp = VN_CAST(np, গ্ৰন্থি_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        যদি (AstClass* ধ্রুৱ classp = VN_CAST(nodep, Class)) {
            AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(সঁচা);
            funcp->isStatic(মিছা);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(মিছা);
            ঢোকাও_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        যদি (nodep->name() == "new") ঢোকাও_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    শূন্য visit(AstCStmt* nodep) override {
        যদি (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstVar* nodep) override {
        যদি (nodep->needsCReset()) {
            Ast_গ্ৰন্থি* ধ্রুৱ crstp = নূতন AstAssign{
                nodep->fileline(), নূতন AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                নূতন AstCReset{nodep->fileline(), nodep, সঁচা}};
            যদি (m_varResetp) {
                m_varResetp->add(crstp);
            } অন্যথা যদি (m_cfuncp) {
                nodep->addNextHere(crstp);
            } অন্যথা {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_গ্ৰন্থি* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        বাবে (auto& itr : m_ctorCalls) {
            AstCStmt* ধ্রুৱ nodep = itr.first;
            ধ্রুৱ Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = itr.second;
            যদি (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        }
    }
};

//######################################################################

শূন্য V3CCtors::evalAsserts() {
    Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ধ্রুৱ funcp
        = নূতন AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(সঁচা);
    funcp->isStatic(মিছা);
    funcp->isLoose(সঁচা);
    funcp->keepIfEmpty(সঁচা);
    funcp->slow(মিছা);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    বাবে (Ast_গ্ৰন্থি* np = modp->stmtsp(); np; np = np->nextp()) {
        যদি (AstVar* ধ্রুৱ varp = VN_CAST(np, Var)) {
            যদি (varp->isPrimaryInish() && !varp->isSc()) {
                যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ধ্রুৱ পূর্ণাঙ্ক storedWidth = basicp->widthAlignBytes() * 8;
                    ধ্রুৱ পূর্ণাঙ্ক lastWordWidth = varp->width() % storedWidth;
                    যদি (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ধ্রুৱ vrefp
                            = নূতন AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_গ্ৰন্থি_ৰাশিমালা* newp = vrefp;
                        যদি (varp->isWide()) {
                            newp = নূতন AstWordSel{
                                varp->fileline(), newp,
                                নূতন AstConst(varp->fileline(), varp->width_শব্দগুলি() - 1)};
                        }
                        ধ্রুৱ uint64_t মান = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = নূতন AstAnd{varp->fileline(), newp,
                                          নূতন AstConst(varp->fileline(), AstConst::Widthed_মান{},
                                                       storedWidth, মান)};
                        Ast_গ্ৰন্থি_If* ধ্রুৱ ifp = নূতন AstIf{
                            varp->fileline(), newp,
                            নূতন AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

শূন্য V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_বৃক্ষ("cctors", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
