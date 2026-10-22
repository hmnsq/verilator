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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3CCtors.h"

#અન્તર્ભાવ "V3EmitCBase.h"

#અન્તર્ભાવ <list>

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ VCtorType final {
જાહેર:
    ગણના en : uint8_t { MODULE, CLASS, COVERAGE };

ખાનગી:
    અચલ ગણના en m_e;

જાહેર:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    તાર્કિક isClass() અચલ { પાછા m_e == CLASS; }
    તાર્કિક isCoverage() અચલ { પાછા m_e == COVERAGE; }
};

વર્ગ V3CCtorsBuilder final {
    Ast_ગાંઠ_Module* અચલ m_modp;  // Current module/class
    અચલ string m_basename;
    અચલ VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    પૂર્ણાંક m_અંક_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        અચલ પૂર્ણાંક func_અંક = m_newFunctions.size();
        અચલ string funcName = m_basename + "_" + cvtToStr(func_અંક);
        AstCFunc* અચલ funcp = નવું AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(ખોટું);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(સાચું);  // TODO relax
        funcp->declPrivate(સાચું);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        જો (!m_type.isCoverage()) m_modp->ctorVarReset(સાચું);
        string preventUnusedStmt;
        જો (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } નહીંતર જો (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        જો (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(નવું AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_અંક_Stmts = 0;
        પાછા funcp;
    }

જાહેર:
    શૂન્ય add(Ast_ગાંઠ* nodep) {
        જો (m_newFunctions.empty()) m_newFunctions.ધકેલો_back(makeNewFunc());
        જો (v3Global.opt.outputSplitCFuncs() && m_અંક_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.ધકેલો_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_અંક_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_ગાંઠ_Module* nodep, અચલ string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        જો (m_type.isCoverage()) m_newFunctions.ધકેલો_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        જો (m_newFunctions.size() == 0) {
        } નહીંતર જો (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } નહીંતર {
            // Split was necessary, create root function and call all others from that
            AstCFunc* અચલ મૂળ_Funcp = makeNewFunc();
            મૂળ_Funcp->name(m_basename);
            માટે (AstCFunc* અચલ funcp : m_newFunctions) {
                AstCCall* અચલ callp = નવું AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                જો (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } નહીંતર {
                    જો (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                મૂળ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

ખાનગી:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

વર્ગ CCtorsVisitor final : જાહેર VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, અચલ Ast_ગાંઠ_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    સ્થિર શૂન્ય ઉમેરો_Sc(AstCFunc* cfuncp, અચલ Ast_ગાંઠ_Module* modp, VSystemCSectionType type) {
        અચલ auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        જો (txtAndFlp.first.empty()) પાછા;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* અચલ cstmtp = નવું AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (nodep->isConstPool()) પાછા;  // Special emit rules
        જો (અચલ AstClass* અચલ classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            જો (classp->isInterfaceClass()) પાછા;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        જો (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            માટે (Ast_ગાંઠ *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                જો (Ast_ગાંઠ_Cover_Decl* અચલ coverp = VN_CAST(np, ગાંઠ_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        જો (AstClass* અચલ classp = VN_CAST(nodep, Class)) {
            AstCFunc* અચલ funcp = નવું AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(સાચું);
            funcp->isStatic(ખોટું);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(ખોટું);
            ઉમેરો_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        જો (nodep->name() == "new") ઉમેરો_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    શૂન્ય visit(AstCStmt* nodep) override {
        જો (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {
        જો (nodep->needsCReset()) {
            Ast_ગાંઠ* અચલ crstp = નવું AstAssign{
                nodep->fileline(), નવું AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                નવું AstCReset{nodep->fileline(), nodep, સાચું}};
            જો (m_varResetp) {
                m_varResetp->add(crstp);
            } નહીંતર જો (m_cfuncp) {
                nodep->addNextHere(crstp);
            } નહીંતર {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_ગાંઠ* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        માટે (auto& itr : m_ctorCalls) {
            AstCStmt* અચલ nodep = itr.first;
            અચલ Ast_ગાંઠ_Module* અચલ modp = itr.second;
            જો (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        }
    }
};

//######################################################################

શૂન્ય V3CCtors::evalAsserts() {
    Ast_ગાંઠ_Module* અચલ modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* અચલ funcp
        = નવું AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(સાચું);
    funcp->isStatic(ખોટું);
    funcp->isLoose(સાચું);
    funcp->keepIfEmpty(સાચું);
    funcp->slow(ખોટું);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    માટે (Ast_ગાંઠ* np = modp->stmtsp(); np; np = np->nextp()) {
        જો (AstVar* અચલ varp = VN_CAST(np, Var)) {
            જો (varp->isPrimaryInish() && !varp->isSc()) {
                જો (અચલ AstBasicDType* અચલ basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    અચલ પૂર્ણાંક storedWidth = basicp->widthAlignBytes() * 8;
                    અચલ પૂર્ણાંક lastWordWidth = varp->width() % storedWidth;
                    જો (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* અચલ vrefp
                            = નવું AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_ગાંઠ_પદાવલિ* newp = vrefp;
                        જો (varp->isWide()) {
                            newp = નવું AstWordSel{
                                varp->fileline(), newp,
                                નવું AstConst(varp->fileline(), varp->width_શબ્દો() - 1)};
                        }
                        અચલ uint64_t કિંમત = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = નવું AstAnd{varp->fileline(), newp,
                                          નવું AstConst(varp->fileline(), AstConst::Widthed_કિંમત{},
                                                       storedWidth, કિંમત)};
                        Ast_ગાંઠ_If* અચલ ifp = નવું AstIf{
                            varp->fileline(), newp,
                            નવું AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

શૂન્ય V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_વૃક્ષ("cctors", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
