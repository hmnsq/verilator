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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3CCtors.h"

#അന്തര്ഭാവ "V3EmitCBase.h"

#അന്തര്ഭാവ <list>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം VCtorType final {
പൊതു:
    എണ്ണൽ en : uint8_t { MODULE, CLASS, COVERAGE };

സ്വകാര്യ:
    സ്ഥിരം എണ്ണൽ en m_e;

പൊതു:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    തർക്കം isClass() സ്ഥിരം { തിരികെ m_e == CLASS; }
    തർക്കം isCoverage() സ്ഥിരം { തിരികെ m_e == COVERAGE; }
};

വർഗ്ഗം V3CCtorsBuilder final {
    Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Current module/class
    സ്ഥിരം string m_basename;
    സ്ഥിരം VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    പൂർണ്ണസംഖ്യ m_അക്കം_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        സ്ഥിരം പൂർണ്ണസംഖ്യ func_അക്കം = m_newFunctions.size();
        സ്ഥിരം string funcName = m_basename + "_" + cvtToStr(func_അക്കം);
        AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(അസത്യം);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(സത്യം);  // TODO relax
        funcp->declPrivate(സത്യം);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        എങ്കിൽ (!m_type.isCoverage()) m_modp->ctorVarReset(സത്യം);
        string preventUnusedStmt;
        എങ്കിൽ (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } അല്ലെങ്കിൽ എങ്കിൽ (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        എങ്കിൽ (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(പുതിയ AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_അക്കം_Stmts = 0;
        തിരികെ funcp;
    }

പൊതു:
    ശൂന്യം add(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (m_newFunctions.empty()) m_newFunctions.തള്ളുക_back(makeNewFunc());
        എങ്കിൽ (v3Global.opt.outputSplitCFuncs() && m_അക്കം_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.തള്ളുക_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_അക്കം_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_കണ്ണി_Module* nodep, സ്ഥിരം string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        എങ്കിൽ (m_type.isCoverage()) m_newFunctions.തള്ളുക_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        എങ്കിൽ (m_newFunctions.size() == 0) {
        } അല്ലെങ്കിൽ എങ്കിൽ (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } അല്ലെങ്കിൽ {
            // Split was necessary, create root function and call all others from that
            AstCFunc* സ്ഥിരം വേര്_Funcp = makeNewFunc();
            വേര്_Funcp->name(m_basename);
            വേണ്ടി (AstCFunc* സ്ഥിരം funcp : m_newFunctions) {
                AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                എങ്കിൽ (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                വേര്_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

സ്വകാര്യ:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

വർഗ്ഗം CCtorsVisitor final : പൊതു VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, സ്ഥിരം Ast_കണ്ണി_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    സ്ഥിതം ശൂന്യം തിരുകുക_Sc(AstCFunc* cfuncp, സ്ഥിരം Ast_കണ്ണി_Module* modp, VSystemCSectionType type) {
        സ്ഥിരം auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        എങ്കിൽ (txtAndFlp.first.empty()) തിരികെ;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* സ്ഥിരം cstmtp = പുതിയ AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        എങ്കിൽ (nodep->isConstPool()) തിരികെ;  // Special emit rules
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            എങ്കിൽ (classp->isInterfaceClass()) തിരികെ;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        എങ്കിൽ (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            വേണ്ടി (Ast_കണ്ണി *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                എങ്കിൽ (Ast_കണ്ണി_Cover_Decl* സ്ഥിരം coverp = VN_CAST(np, കണ്ണി_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(nodep, Class)) {
            AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(സത്യം);
            funcp->isStatic(അസത്യം);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(അസത്യം);
            തിരുകുക_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        എങ്കിൽ (nodep->name() == "new") തിരുകുക_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    ശൂന്യം visit(AstCStmt* nodep) override {
        എങ്കിൽ (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (nodep->needsCReset()) {
            Ast_കണ്ണി* സ്ഥിരം crstp = പുതിയ AstAssign{
                nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                പുതിയ AstCReset{nodep->fileline(), nodep, സത്യം}};
            എങ്കിൽ (m_varResetp) {
                m_varResetp->add(crstp);
            } അല്ലെങ്കിൽ എങ്കിൽ (m_cfuncp) {
                nodep->addNextHere(crstp);
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_കണ്ണി* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        വേണ്ടി (auto& itr : m_ctorCalls) {
            AstCStmt* സ്ഥിരം nodep = itr.first;
            സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = itr.second;
            എങ്കിൽ (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        }
    }
};

//######################################################################

ശൂന്യം V3CCtors::evalAsserts() {
    Ast_കണ്ണി_Module* സ്ഥിരം modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* സ്ഥിരം funcp
        = പുതിയ AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(സത്യം);
    funcp->isStatic(അസത്യം);
    funcp->isLoose(സത്യം);
    funcp->keepIfEmpty(സത്യം);
    funcp->slow(അസത്യം);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    വേണ്ടി (Ast_കണ്ണി* np = modp->stmtsp(); np; np = np->nextp()) {
        എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(np, Var)) {
            എങ്കിൽ (varp->isPrimaryInish() && !varp->isSc()) {
                എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ storedWidth = basicp->widthAlignBytes() * 8;
                    സ്ഥിരം പൂർണ്ണസംഖ്യ lastWordWidth = varp->width() % storedWidth;
                    എങ്കിൽ (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* സ്ഥിരം vrefp
                            = പുതിയ AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_കണ്ണി_വ്യഞ്ജകം* newp = vrefp;
                        എങ്കിൽ (varp->isWide()) {
                            newp = പുതിയ AstWordSel{
                                varp->fileline(), newp,
                                പുതിയ AstConst(varp->fileline(), varp->width_വാക്കുകൾ() - 1)};
                        }
                        സ്ഥിരം uint64_t മൂല്യം = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = പുതിയ AstAnd{varp->fileline(), newp,
                                          പുതിയ AstConst(varp->fileline(), AstConst::Widthed_മൂല്യം{},
                                                       storedWidth, മൂല്യം)};
                        Ast_കണ്ണി_If* സ്ഥിരം ifp = പുതിയ AstIf{
                            varp->fileline(), newp,
                            പുതിയ AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

ശൂന്യം V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_വൃക്ഷം("cctors", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
