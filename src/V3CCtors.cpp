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

खाजगी:
    स्थिरांक गणना en m_e;

सार्वजनिक:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    तार्किक isClass() स्थिरांक { परत m_e == CLASS; }
    तार्किक isCoverage() स्थिरांक { परत m_e == COVERAGE; }
};

वर्ग V3CCtorsBuilder final {
    Ast_गाठ_Module* स्थिरांक m_modp;  // Current module/class
    स्थिरांक string m_basename;
    स्थिरांक VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    पूर्णांक m_अंक_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        स्थिरांक पूर्णांक func_अंक = m_newFunctions.size();
        स्थिरांक string funcName = m_basename + "_" + cvtToStr(func_अंक);
        AstCFunc* स्थिरांक funcp = नवीन AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(असत्य);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(सत्य);  // TODO relax
        funcp->declPrivate(सत्य);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        जर (!m_type.isCoverage()) m_modp->ctorVarReset(सत्य);
        string preventUnusedStmt;
        जर (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } अन्यथा जर (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        जर (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(नवीन AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_अंक_Stmts = 0;
        परत funcp;
    }

सार्वजनिक:
    शून्य add(Ast_गाठ* nodep) {
        जर (m_newFunctions.empty()) m_newFunctions.ढकला_back(makeNewFunc());
        जर (v3Global.opt.outputSplitCFuncs() && m_अंक_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.ढकला_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_अंक_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_गाठ_Module* nodep, स्थिरांक string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        जर (m_type.isCoverage()) m_newFunctions.ढकला_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        जर (m_newFunctions.size() == 0) {
        } अन्यथा जर (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } अन्यथा {
            // Split was necessary, create root function and call all others from that
            AstCFunc* स्थिरांक मूळ_Funcp = makeNewFunc();
            मूळ_Funcp->name(m_basename);
            साठी (AstCFunc* स्थिरांक funcp : m_newFunctions) {
                AstCCall* स्थिरांक callp = नवीन AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                जर (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } अन्यथा {
                    जर (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                मूळ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

खाजगी:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

वर्ग CCtorsVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, स्थिरांक Ast_गाठ_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    स्थिर शून्य घाला_Sc(AstCFunc* cfuncp, स्थिरांक Ast_गाठ_Module* modp, VSystemCSectionType type) {
        स्थिरांक स्वतः txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        जर (txtAndFlp.first.empty()) परत;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* स्थिरांक cstmtp = नवीन AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        जर (nodep->isConstPool()) परत;  // Special emit rules
        जर (स्थिरांक AstClass* स्थिरांक classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            जर (classp->isInterfaceClass()) परत;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        जर (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            साठी (Ast_गाठ *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                जर (Ast_गाठ_Cover_Decl* स्थिरांक coverp = VN_CAST(np, गाठ_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        जर (AstClass* स्थिरांक classp = VN_CAST(nodep, Class)) {
            AstCFunc* स्थिरांक funcp = नवीन AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(सत्य);
            funcp->isStatic(असत्य);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(असत्य);
            घाला_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        जर (nodep->name() == "new") घाला_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    शून्य visit(AstCStmt* nodep) override {
        जर (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        जर (nodep->needsCReset()) {
            Ast_गाठ* स्थिरांक crstp = नवीन AstAssign{
                nodep->fileline(), नवीन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                नवीन AstCReset{nodep->fileline(), nodep, सत्य}};
            जर (m_varResetp) {
                m_varResetp->add(crstp);
            } अन्यथा जर (m_cfuncp) {
                nodep->addNextHere(crstp);
            } अन्यथा {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_गाठ* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        साठी (स्वतः& itr : m_ctorCalls) {
            AstCStmt* स्थिरांक nodep = itr.first;
            स्थिरांक Ast_गाठ_Module* स्थिरांक modp = itr.second;
            जर (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        }
    }
};

//######################################################################

शून्य V3CCtors::evalAsserts() {
    Ast_गाठ_Module* स्थिरांक modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* स्थिरांक funcp
        = नवीन AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(सत्य);
    funcp->isStatic(असत्य);
    funcp->isLoose(सत्य);
    funcp->keepIfEmpty(सत्य);
    funcp->slow(असत्य);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    साठी (Ast_गाठ* np = modp->stmtsp(); np; np = np->nextp()) {
        जर (AstVar* स्थिरांक varp = VN_CAST(np, Var)) {
            जर (varp->isPrimaryInish() && !varp->isSc()) {
                जर (स्थिरांक AstBasicDType* स्थिरांक basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    स्थिरांक पूर्णांक storedWidth = basicp->widthAlignBytes() * 8;
                    स्थिरांक पूर्णांक lastWordWidth = varp->width() % storedWidth;
                    जर (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* स्थिरांक vrefp
                            = नवीन AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_गाठ_अभिव्यक्ती* newp = vrefp;
                        जर (varp->isWide()) {
                            newp = नवीन AstWordSel{
                                varp->fileline(), newp,
                                नवीन AstConst(varp->fileline(), varp->width_शब्द() - 1)};
                        }
                        स्थिरांक uint64_t मूल्य = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = नवीन AstAnd{varp->fileline(), newp,
                                          नवीन AstConst(varp->fileline(), AstConst::Widthed_मूल्य{},
                                                       storedWidth, मूल्य)};
                        Ast_गाठ_If* स्थिरांक ifp = नवीन AstIf{
                            varp->fileline(), newp,
                            नवीन AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
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
    V3Global::dump_Check_Global_वृक्ष("cctors", 0, dump_वृक्ष_Either_Level() >= 3);
}
