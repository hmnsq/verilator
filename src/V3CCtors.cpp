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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3CCtors.h"

#अन्तर्भाव "V3EmitCBase.h"

#अन्तर्भाव <list>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग VCtorType final {
सार्वजनिक:
    गणना en : uint8_t { MODULE, CLASS, COVERAGE };

गुप्त:
    ध्रुव गणना en m_e;

सार्वजनिक:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    तार्किक isClass() ध्रुव { प्रत्यागम m_e == CLASS; }
    तार्किक isCoverage() ध्रुव { प्रत्यागम m_e == COVERAGE; }
};

वर्ग V3CCtorsBuilder final {
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Current module/class
    ध्रुव string m_basename;
    ध्रुव VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    पूर्णाङ्क m_अङ्कः_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ध्रुव पूर्णाङ्क func_अङ्कः = m_newFunctions.size();
        ध्रुव string funcName = m_basename + "_" + cvtToStr(func_अङ्कः);
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(मिच्छ);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(सच्च);  // TODO relax
        funcp->declPrivate(सच्च);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        जइ (!m_type.isCoverage()) m_modp->ctorVarReset(सच्च);
        string preventUnusedStmt;
        जइ (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } अण्णहा जइ (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        जइ (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(नूतन AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_अङ्कः_Stmts = 0;
        प्रत्यागम funcp;
    }

सार्वजनिक:
    शून्य add(Ast_ग्रन्थिः* nodep) {
        जइ (m_newFunctions.empty()) m_newFunctions.प्रक्षिप_back(makeNewFunc());
        जइ (v3Global.opt.outputSplitCFuncs() && m_अङ्कः_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.प्रक्षिप_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_अङ्कः_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_ग्रन्थिः_Module* nodep, ध्रुव string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        जइ (m_type.isCoverage()) m_newFunctions.प्रक्षिप_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        जइ (m_newFunctions.size() == 0) {
        } अण्णहा जइ (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } अण्णहा {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ध्रुव मूलम्_Funcp = makeNewFunc();
            मूलम्_Funcp->name(m_basename);
            प्रति (AstCFunc* ध्रुव funcp : m_newFunctions) {
                AstCCall* ध्रुव callp = नूतन AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                जइ (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } अण्णहा {
                    जइ (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                मूलम्_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

गुप्त:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

वर्ग CCtorsVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ध्रुव Ast_ग्रन्थिः_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    स्थिर शून्य निवेशय_Sc(AstCFunc* cfuncp, ध्रुव Ast_ग्रन्थिः_Module* modp, VSystemCSectionType type) {
        ध्रुव auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        जइ (txtAndFlp.first.empty()) प्रत्यागम;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ध्रुव cstmtp = नूतन AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        जइ (nodep->isConstPool()) प्रत्यागम;  // Special emit rules
        जइ (ध्रुव AstClass* ध्रुव classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            जइ (classp->isInterfaceClass()) प्रत्यागम;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        जइ (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            प्रति (Ast_ग्रन्थिः *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                जइ (Ast_ग्रन्थिः_Cover_Decl* ध्रुव coverp = VN_CAST(np, ग्रन्थिः_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        जइ (AstClass* ध्रुव classp = VN_CAST(nodep, Class)) {
            AstCFunc* ध्रुव funcp = नूतन AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(सच्च);
            funcp->isStatic(मिच्छ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(मिच्छ);
            निवेशय_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        जइ (nodep->name() == "new") निवेशय_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    शून्य visit(AstCStmt* nodep) override {
        जइ (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        जइ (nodep->needsCReset()) {
            Ast_ग्रन्थिः* ध्रुव crstp = नूतन AstAssign{
                nodep->fileline(), नूतन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                नूतन AstCReset{nodep->fileline(), nodep, सच्च}};
            जइ (m_varResetp) {
                m_varResetp->add(crstp);
            } अण्णहा जइ (m_cfuncp) {
                nodep->addNextHere(crstp);
            } अण्णहा {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_ग्रन्थिः* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        प्रति (auto& itr : m_ctorCalls) {
            AstCStmt* ध्रुव nodep = itr.first;
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = itr.second;
            जइ (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
        }
    }
};

//######################################################################

शून्य V3CCtors::evalAsserts() {
    Ast_ग्रन्थिः_Module* ध्रुव modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ध्रुव funcp
        = नूतन AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(सच्च);
    funcp->isStatic(मिच्छ);
    funcp->isLoose(सच्च);
    funcp->keepIfEmpty(सच्च);
    funcp->slow(मिच्छ);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    प्रति (Ast_ग्रन्थिः* np = modp->stmtsp(); np; np = np->nextp()) {
        जइ (AstVar* ध्रुव varp = VN_CAST(np, Var)) {
            जइ (varp->isPrimaryInish() && !varp->isSc()) {
                जइ (ध्रुव AstBasicDType* ध्रुव basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ध्रुव पूर्णाङ्क storedWidth = basicp->widthAlignBytes() * 8;
                    ध्रुव पूर्णाङ्क lastWordWidth = varp->width() % storedWidth;
                    जइ (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ध्रुव vrefp
                            = नूतन AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_ग्रन्थिः_व्यञ्जकम्* newp = vrefp;
                        जइ (varp->isWide()) {
                            newp = नूतन AstWordSel{
                                varp->fileline(), newp,
                                नूतन AstConst(varp->fileline(), varp->width_शब्दाः() - 1)};
                        }
                        ध्रुव uint64_t मानम् = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = नूतन AstAnd{varp->fileline(), newp,
                                          नूतन AstConst(varp->fileline(), AstConst::Widthed_मानम्{},
                                                       storedWidth, मानम्)};
                        Ast_ग्रन्थिः_If* ध्रुव ifp = नूतन AstIf{
                            varp->fileline(), newp,
                            नूतन AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

शून्य V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_वृक्षः("cctors", 0, dump_वृक्षः_Either_Level() >= 3);
}
