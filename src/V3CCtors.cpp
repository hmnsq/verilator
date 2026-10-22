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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3CCtors.h"

#অন্তর্ভাব "V3EmitCBase.h"

#অন্তর্ভাব <list>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্রেণী VCtorType final {
সর্বজনীন:
    গণনা en : uint8_t { MODULE, CLASS, COVERAGE };

ব্যক্তিগত:
    ধ্রুবক গণনা en m_e;

সর্বজনীন:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    যৌক্তিক isClass() ধ্রুবক { ফেরত m_e == CLASS; }
    যৌক্তিক isCoverage() ধ্রুবক { ফেরত m_e == COVERAGE; }
};

শ্রেণী V3CCtorsBuilder final {
    Ast_গ্রন্থি_Module* ধ্রুবক m_modp;  // Current module/class
    ধ্রুবক string m_basename;
    ধ্রুবক VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    পূর্ণসংখ্যা m_অঙ্ক_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ধ্রুবক পূর্ণসংখ্যা func_অঙ্ক = m_newFunctions.আকার_2();
        ধ্রুবক string funcName = m_basename + "_" + cvtToStr(func_অঙ্ক);
        AstCFunc* ধ্রুবক funcp = নতুন AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(মিথ্যা);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(সত্য);  // TODO relax
        funcp->declPrivate(সত্য);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        যদি (!m_type.isCoverage()) m_modp->ctorVarReset(সত্য);
        string preventUnusedStmt;
        যদি (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } অন্যথা যদি (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        যদি (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(নতুন AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_অঙ্ক_Stmts = 0;
        ফেরত funcp;
    }

সর্বজনীন:
    শূন্য add(Ast_গ্রন্থি* nodep) {
        যদি (m_newFunctions.empty()) m_newFunctions.ঠেলো_back(makeNewFunc());
        যদি (v3Global.opt.outputSplitCFuncs() && m_অঙ্ক_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.ঠেলো_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_অঙ্ক_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_গ্রন্থি_Module* nodep, ধ্রুবক string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        যদি (m_type.isCoverage()) m_newFunctions.ঠেলো_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        যদি (m_newFunctions.আকার_2() == 0) {
        } অন্যথা যদি (m_newFunctions.আকার_2() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } অন্যথা {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ধ্রুবক মূল_Funcp = makeNewFunc();
            মূল_Funcp->name(m_basename);
            জন্য (AstCFunc* ধ্রুবক funcp : m_newFunctions) {
                AstCCall* ধ্রুবক callp = নতুন AstCCall{m_modp->fileline(), funcp};
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

ব্যক্তিগত:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

শ্রেণী CCtorsVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ধ্রুবক Ast_গ্রন্থি_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    স্থির শূন্য ঢোকাও_Sc(AstCFunc* cfuncp, ধ্রুবক Ast_গ্রন্থি_Module* modp, VSystemCSectionType type) {
        ধ্রুবক auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        যদি (txtAndFlp.first.empty()) ফেরত;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ধ্রুবক cstmtp = নতুন AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        যদি (nodep->isConstPool()) ফেরত;  // Special emit rules
        যদি (ধ্রুবক AstClass* ধ্রুবক classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            যদি (classp->isInterfaceClass()) ফেরত;
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
            জন্য (Ast_গ্রন্থি *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                যদি (Ast_গ্রন্থি_Cover_Decl* ধ্রুবক coverp = VN_CAST(np, গ্রন্থি_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        যদি (AstClass* ধ্রুবক classp = VN_CAST(nodep, Class)) {
            AstCFunc* ধ্রুবক funcp = নতুন AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(সত্য);
            funcp->isStatic(মিথ্যা);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(মিথ্যা);
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
            Ast_গ্রন্থি* ধ্রুবক crstp = নতুন AstAssign{
                nodep->fileline(), নতুন AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                নতুন AstCReset{nodep->fileline(), nodep, সত্য}};
            যদি (m_varResetp) {
                m_varResetp->add(crstp);
            } অন্যথা যদি (m_cfuncp) {
                nodep->add_পরবর্তী_Here(crstp);
            } অন্যথা {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_গ্রন্থি* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        জন্য (auto& itr : m_ctorCalls) {
            AstCStmt* ধ্রুবক nodep = itr.first;
            ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = itr.second;
            যদি (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        }
    }
};

//######################################################################

শূন্য V3CCtors::evalAsserts() {
    Ast_গ্রন্থি_Module* ধ্রুবক modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ধ্রুবক funcp
        = নতুন AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(সত্য);
    funcp->isStatic(মিথ্যা);
    funcp->isLoose(সত্য);
    funcp->keepIfEmpty(সত্য);
    funcp->slow(মিথ্যা);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    জন্য (Ast_গ্রন্থি* np = modp->stmtsp(); np; np = np->nextp()) {
        যদি (AstVar* ধ্রুবক varp = VN_CAST(np, Var)) {
            যদি (varp->isPrimaryInish() && !varp->isSc()) {
                যদি (ধ্রুবক AstBasicDType* ধ্রুবক basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ধ্রুবক পূর্ণসংখ্যা storedWidth = basicp->widthAlignBytes() * 8;
                    ধ্রুবক পূর্ণসংখ্যা lastWordWidth = varp->width() % storedWidth;
                    যদি (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ধ্রুবক vrefp
                            = নতুন AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_গ্রন্থি_রাশিমালা* newp = vrefp;
                        যদি (varp->isWide()) {
                            newp = নতুন AstWordSel{
                                varp->fileline(), newp,
                                নতুন AstConst(varp->fileline(), varp->width_শব্দগুলি() - 1)};
                        }
                        ধ্রুবক uint64_t মান = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = নতুন AstAnd{varp->fileline(), newp,
                                          নতুন AstConst(varp->fileline(), AstConst::Widthed_মান{},
                                                       storedWidth, মান)};
                        Ast_গ্রন্থি_If* ধ্রুবক ifp = নতুন AstIf{
                            varp->fileline(), newp,
                            নতুন AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
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
