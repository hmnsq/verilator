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
    अचर गणना en m_e;

सार्वजनिक:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    तार्किक isClass() अचर { फर्काऊ m_e == CLASS; }
    तार्किक isCoverage() अचर { फर्काऊ m_e == COVERAGE; }
};

वर्ग V3CCtorsBuilder final {
    Ast_गाँठो_Module* अचर m_modp;  // Current module/class
    अचर string m_basename;
    अचर VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    पूर्णाङ्क m_अङ्क_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        अचर पूर्णाङ्क func_अङ्क = m_newFunctions.size();
        अचर string funcName = m_basename + "_" + cvtToStr(func_अङ्क);
        AstCFunc* अचर funcp = नूतन AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(असत्य);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(सत्य);  // TODO relax
        funcp->declPrivate(सत्य);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        यदि (!m_type.isCoverage()) m_modp->ctorVarReset(सत्य);
        string preventUnusedStmt;
        यदि (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } अन्यथा यदि (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        यदि (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(नूतन AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_अङ्क_Stmts = 0;
        फर्काऊ funcp;
    }

सार्वजनिक:
    शून्य add(Ast_गाँठो* nodep) {
        यदि (m_newFunctions.empty()) m_newFunctions.थप_back(makeNewFunc());
        यदि (v3Global.opt.outputSplitCFuncs() && m_अङ्क_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.थप_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_अङ्क_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_गाँठो_Module* nodep, अचर string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        यदि (m_type.isCoverage()) m_newFunctions.थप_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        यदि (m_newFunctions.size() == 0) {
        } अन्यथा यदि (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } अन्यथा {
            // Split was necessary, create root function and call all others from that
            AstCFunc* अचर जरा_Funcp = makeNewFunc();
            जरा_Funcp->name(m_basename);
            लागि (AstCFunc* अचर funcp : m_newFunctions) {
                AstCCall* अचर callp = नूतन AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                यदि (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } अन्यथा {
                    यदि (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                जरा_Funcp->addStmtsp(callp->makeStmt());
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
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, अचर Ast_गाँठो_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    स्थिर शून्य घुसाउ_Sc(AstCFunc* cfuncp, अचर Ast_गाँठो_Module* modp, VSystemCSectionType type) {
        अचर स्वतः txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        यदि (txtAndFlp.first.empty()) फर्काऊ;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* अचर cstmtp = नूतन AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        यदि (nodep->isConstPool()) फर्काऊ;  // Special emit rules
        यदि (अचर AstClass* अचर classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            यदि (classp->isInterfaceClass()) फर्काऊ;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        यदि (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            लागि (Ast_गाँठो *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                यदि (Ast_गाँठो_Cover_Decl* अचर coverp = VN_CAST(np, गाँठो_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        यदि (AstClass* अचर classp = VN_CAST(nodep, Class)) {
            AstCFunc* अचर funcp = नूतन AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(सत्य);
            funcp->isStatic(असत्य);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(असत्य);
            घुसाउ_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        यदि (nodep->name() == "new") घुसाउ_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    शून्य visit(AstCStmt* nodep) override {
        यदि (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        यदि (nodep->needsCReset()) {
            Ast_गाँठो* अचर crstp = नूतन AstAssign{
                nodep->fileline(), नूतन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                नूतन AstCReset{nodep->fileline(), nodep, सत्य}};
            यदि (m_varResetp) {
                m_varResetp->add(crstp);
            } अन्यथा यदि (m_cfuncp) {
                nodep->addNextHere(crstp);
            } अन्यथा {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_गाँठो* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        लागि (स्वतः& itr : m_ctorCalls) {
            AstCStmt* अचर nodep = itr.first;
            अचर Ast_गाँठो_Module* अचर modp = itr.second;
            यदि (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
        }
    }
};

//######################################################################

शून्य V3CCtors::evalAsserts() {
    Ast_गाँठो_Module* अचर modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* अचर funcp
        = नूतन AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(सत्य);
    funcp->isStatic(असत्य);
    funcp->isLoose(सत्य);
    funcp->keepIfEmpty(सत्य);
    funcp->slow(असत्य);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    लागि (Ast_गाँठो* np = modp->stmtsp(); np; np = np->nextp()) {
        यदि (AstVar* अचर varp = VN_CAST(np, Var)) {
            यदि (varp->isPrimaryInish() && !varp->isSc()) {
                यदि (अचर AstBasicDType* अचर basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    अचर पूर्णाङ्क storedWidth = basicp->widthAlignBytes() * 8;
                    अचर पूर्णाङ्क lastWordWidth = varp->width() % storedWidth;
                    यदि (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* अचर vrefp
                            = नूतन AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_गाँठो_अभिव्यक्ति* newp = vrefp;
                        यदि (varp->isWide()) {
                            newp = नूतन AstWordSel{
                                varp->fileline(), newp,
                                नूतन AstConst(varp->fileline(), varp->width_शब्दहरू() - 1)};
                        }
                        अचर uint64_t मान = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = नूतन AstAnd{varp->fileline(), newp,
                                          नूतन AstConst(varp->fileline(), AstConst::Widthed_मान{},
                                                       storedWidth, मान)};
                        Ast_गाँठो_If* अचर ifp = नूतन AstIf{
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
    V3Global::dump_Check_Global_रूख("cctors", 0, dump_रूख_Either_Level() >= 3);
}
