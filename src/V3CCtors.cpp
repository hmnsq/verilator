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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3CCtors.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"

#ଅନ୍ତର୍ଭାଵ <list>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ VCtorType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { MODULE, CLASS, COVERAGE };

ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ ଗଣନା en m_e;

ସର୍ବସାଧାରଣ:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    ତାର୍କିକ isClass() ଅଚଳ { ଫେରନ୍ତୁ m_e == CLASS; }
    ତାର୍କିକ isCoverage() ଅଚଳ { ଫେରନ୍ତୁ m_e == COVERAGE; }
};

ଶ୍ରେଣୀ V3CCtorsBuilder final {
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Current module/class
    ଅଚଳ string m_basename;
    ଅଚଳ VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ଅଙ୍କ_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା func_ଅଙ୍କ = m_newFunctions.size();
        ଅଚଳ string funcName = m_basename + "_" + cvtToStr(func_ଅଙ୍କ);
        AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(ମିଥ୍ୟା);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(ସତ୍ୟ);  // TODO relax
        funcp->declPrivate(ସତ୍ୟ);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        ଯଦି (!m_type.isCoverage()) m_modp->ctorVarReset(ସତ୍ୟ);
        string preventUnusedStmt;
        ଯଦି (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } ଅନ୍ୟଥା ଯଦି (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        ଯଦି (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(ନୂତନ AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_ଅଙ୍କ_Stmts = 0;
        ଫେରନ୍ତୁ funcp;
    }

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ add(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (m_newFunctions.empty()) m_newFunctions.ଠେଲ_back(makeNewFunc());
        ଯଦି (v3Global.opt.outputSplitCFuncs() && m_ଅଙ୍କ_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.ଠେଲ_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_ଅଙ୍କ_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_ଗଣ୍ଠି_Module* nodep, ଅଚଳ string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        ଯଦି (m_type.isCoverage()) m_newFunctions.ଠେଲ_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        ଯଦି (m_newFunctions.size() == 0) {
        } ଅନ୍ୟଥା ଯଦି (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } ଅନ୍ୟଥା {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ଅଚଳ ମୂଳ_Funcp = makeNewFunc();
            ମୂଳ_Funcp->name(m_basename);
            ପାଇଁ (AstCFunc* ଅଚଳ funcp : m_newFunctions) {
                AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                ଯଦି (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } ଅନ୍ୟଥା {
                    ଯଦି (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                ମୂଳ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

ବ୍ୟକ୍ତିଗତ:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

ଶ୍ରେଣୀ CCtorsVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ଅଚଳ Ast_ଗଣ୍ଠି_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ସ୍ଥିର ଶୂନ୍ୟ ଭର୍ତ୍ତି_Sc(AstCFunc* cfuncp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, VSystemCSectionType type) {
        ଅଚଳ auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        ଯଦି (txtAndFlp.first.empty()) ଫେରନ୍ତୁ;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ଅଚଳ cstmtp = ନୂତନ AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (nodep->isConstPool()) ଫେରନ୍ତୁ;  // Special emit rules
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            ଯଦି (classp->isInterfaceClass()) ଫେରନ୍ତୁ;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        ଯଦି (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            ପାଇଁ (Ast_ଗଣ୍ଠି *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                ଯଦି (Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ coverp = VN_CAST(np, ଗଣ୍ଠି_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(nodep, Class)) {
            AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(ସତ୍ୟ);
            funcp->isStatic(ମିଥ୍ୟା);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(ମିଥ୍ୟା);
            ଭର୍ତ୍ତି_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ଯଦି (nodep->name() == "new") ଭର୍ତ୍ତି_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    ଶୂନ୍ୟ visit(AstCStmt* nodep) override {
        ଯଦି (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (nodep->needsCReset()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ crstp = ନୂତନ AstAssign{
                nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                ନୂତନ AstCReset{nodep->fileline(), nodep, ସତ୍ୟ}};
            ଯଦି (m_varResetp) {
                m_varResetp->add(crstp);
            } ଅନ୍ୟଥା ଯଦି (m_cfuncp) {
                nodep->addNextHere(crstp);
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_ଗଣ୍ଠି* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        ପାଇଁ (auto& itr : m_ctorCalls) {
            AstCStmt* ଅଚଳ nodep = itr.first;
            ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = itr.second;
            ଯଦି (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        }
    }
};

//######################################################################

ଶୂନ୍ୟ V3CCtors::evalAsserts() {
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ଅଚଳ funcp
        = ନୂତନ AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(ସତ୍ୟ);
    funcp->isStatic(ମିଥ୍ୟା);
    funcp->isLoose(ସତ୍ୟ);
    funcp->keepIfEmpty(ସତ୍ୟ);
    funcp->slow(ମିଥ୍ୟା);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    ପାଇଁ (Ast_ଗଣ୍ଠି* np = modp->stmtsp(); np; np = np->nextp()) {
        ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(np, Var)) {
            ଯଦି (varp->isPrimaryInish() && !varp->isSc()) {
                ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା storedWidth = basicp->widthAlignBytes() * 8;
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lastWordWidth = varp->width() % storedWidth;
                    ଯଦି (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ଅଚଳ vrefp
                            = ନୂତନ AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = vrefp;
                        ଯଦି (varp->isWide()) {
                            newp = ନୂତନ AstWordSel{
                                varp->fileline(), newp,
                                ନୂତନ AstConst(varp->fileline(), varp->width_ଶବ୍ଦଗୁଡ଼ିକ() - 1)};
                        }
                        ଅଚଳ uint64_t ମୂଲ୍ୟ = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = ନୂତନ AstAnd{varp->fileline(), newp,
                                          ନୂତନ AstConst(varp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{},
                                                       storedWidth, ମୂଲ୍ୟ)};
                        Ast_ଗଣ୍ଠି_If* ଅଚଳ ifp = ନୂତନ AstIf{
                            varp->fileline(), newp,
                            ନୂତନ AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

ଶୂନ୍ୟ V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("cctors", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
