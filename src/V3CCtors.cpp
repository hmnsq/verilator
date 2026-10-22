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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3CCtors.h"

#समावेश "V3EmitCBase.h"

#समावेश <list>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी VCtorType final {
खुला:
    क्रमागत en : uint8_t { MODULE, CLASS, COVERAGE };

निजी:
    स्थिर क्रमागत en m_e;

खुला:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    bool isClass() स्थिर { वापस m_e == CLASS; }
    bool isCoverage() स्थिर { वापस m_e == COVERAGE; }
};

श्रेणी V3CCtorsBuilder final {
    Ast_गाँठ_Module* स्थिर m_modp;  // Current module/class
    स्थिर string m_basename;
    स्थिर VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    पूर्णांक m_अंक_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        स्थिर पूर्णांक func_अंक = m_newFunctions.आकार();
        स्थिर string funcName = m_basename + "_" + cvtToStr(func_अंक);
        AstCFunc* स्थिर funcp = नया AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(false);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(true);  // TODO relax
        funcp->declPrivate(true);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        अगर (!m_type.isCoverage()) m_modp->ctorVarReset(true);
        string preventUnusedStmt;
        अगर (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } अन्यथा अगर (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        अगर (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(नया AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_अंक_Stmts = 0;
        वापस funcp;
    }

खुला:
    व्योम add(Ast_गाँठ* nodep) {
        अगर (m_newFunctions.empty()) m_newFunctions.धकेलो_back(makeNewFunc());
        अगर (v3Global.opt.outputSplitCFuncs() && m_अंक_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.धकेलो_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_अंक_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_गाँठ_Module* nodep, स्थिर string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        अगर (m_type.isCoverage()) m_newFunctions.धकेलो_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        अगर (m_newFunctions.आकार() == 0) {
        } अन्यथा अगर (m_newFunctions.आकार() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } अन्यथा {
            // Split was necessary, create root function and call all others from that
            AstCFunc* स्थिर मूल_Funcp = makeNewFunc();
            मूल_Funcp->name(m_basename);
            क्रम (AstCFunc* स्थिर funcp : m_newFunctions) {
                AstCCall* स्थिर callp = नया AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                अगर (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } अन्यथा {
                    अगर (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                मूल_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

निजी:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

श्रेणी CCtorsVisitor final : खुला VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, स्थिर Ast_गाँठ_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    अटल व्योम जोड़ो_Sc(AstCFunc* cfuncp, स्थिर Ast_गाँठ_Module* modp, VSystemCSectionType type) {
        स्थिर स्वतः txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        अगर (txtAndFlp.first.empty()) वापस;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* स्थिर cstmtp = नया AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        अगर (nodep->isConstPool()) वापस;  // Special emit rules
        अगर (स्थिर AstClass* स्थिर classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            अगर (classp->isInterfaceClass()) वापस;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        अगर (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            क्रम (Ast_गाँठ *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                अगर (Ast_गाँठ_Cover_Decl* स्थिर coverp = VN_CAST(np, गाँठ_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        अगर (AstClass* स्थिर classp = VN_CAST(nodep, Class)) {
            AstCFunc* स्थिर funcp = नया AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(true);
            funcp->isStatic(false);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(false);
            जोड़ो_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        अगर (nodep->name() == "new") जोड़ो_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    व्योम visit(AstCStmt* nodep) override {
        अगर (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstVar* nodep) override {
        अगर (nodep->needsCReset()) {
            Ast_गाँठ* स्थिर crstp = नया AstAssign{
                nodep->fileline(), नया AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                नया AstCReset{nodep->fileline(), nodep, true}};
            अगर (m_varResetp) {
                m_varResetp->add(crstp);
            } अन्यथा अगर (m_cfuncp) {
                nodep->add_अग्र_Here(crstp);
            } अन्यथा {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_गाँठ* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        क्रम (स्वतः& itr : m_ctorCalls) {
            AstCStmt* स्थिर nodep = itr.first;
            स्थिर Ast_गाँठ_Module* स्थिर modp = itr.second;
            अगर (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        }
    }
};

//######################################################################

व्योम V3CCtors::evalAsserts() {
    Ast_गाँठ_Module* स्थिर modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* स्थिर funcp
        = नया AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(true);
    funcp->isStatic(false);
    funcp->isLoose(true);
    funcp->keepIfEmpty(true);
    funcp->slow(false);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    क्रम (Ast_गाँठ* np = modp->stmtsp(); np; np = np->nextp()) {
        अगर (AstVar* स्थिर varp = VN_CAST(np, Var)) {
            अगर (varp->isPrimaryInish() && !varp->isSc()) {
                अगर (स्थिर AstBasicDType* स्थिर basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    स्थिर पूर्णांक storedWidth = basicp->widthAlignBytes() * 8;
                    स्थिर पूर्णांक lastWordWidth = varp->width() % storedWidth;
                    अगर (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* स्थिर vrefp
                            = नया AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_गाँठ_व्यंजक* newp = vrefp;
                        अगर (varp->isWide()) {
                            newp = नया AstWordSel{
                                varp->fileline(), newp,
                                नया AstConst(varp->fileline(), varp->width_शब्दगण() - 1)};
                        }
                        स्थिर uint64_t मूल्य = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = नया AstAnd{varp->fileline(), newp,
                                          नया AstConst(varp->fileline(), AstConst::Widthed_मूल्य{},
                                                       storedWidth, मूल्य)};
                        Ast_गाँठ_If* स्थिर ifp = नया AstIf{
                            varp->fileline(), newp,
                            नया AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

व्योम V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_वृक्ष("cctors", 0, dump_वृक्ष_Either_Level() >= 3);
}
