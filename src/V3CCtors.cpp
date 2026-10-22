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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3CCtors.h"

#ضمن "V3EmitCBase.h"

#ضمن <list>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف VCtorType final {
عام:
    تعداد en : uint8_t { MODULE, CLASS, COVERAGE };

خاص:
    ثابت تعداد en m_e;

عام:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    منطقي isClass() ثابت { ܦܢܐ m_e == CLASS; }
    منطقي isCoverage() ثابت { ܦܢܐ m_e == COVERAGE; }
};

صنف V3CCtorsBuilder final {
    AstNodeModule* ثابت m_modp;  // Current module/class
    ثابت string m_basename;
    ثابت VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    صحيح m_numStmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ثابت صحيح funcNum = m_newFunctions.size();
        ثابت string funcName = m_basename + "_" + cvtToStr(funcNum);
        AstCFunc* ثابت funcp = جديد AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(ܕܓܠܐ);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(ܫܪܪܐ);  // TODO relax
        funcp->declPrivate(ܫܪܪܐ);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        ܐܢ (!m_type.isCoverage()) m_modp->ctorVarReset(ܫܪܪܐ);
        string preventUnusedStmt;
        ܐܢ (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } ܐܚܪܢܐ ܐܢ (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        ܐܢ (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(جديد AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_numStmts = 0;
        ܦܢܐ funcp;
    }

عام:
    فراغ add(AstNode* nodep) {
        ܐܢ (m_newFunctions.empty()) m_newFunctions.push_back(makeNewFunc());
        ܐܢ (v3Global.opt.outputSplitCFuncs() && m_numStmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.push_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_numStmts += 1;
    }

    V3CCtorsBuilder(AstNodeModule* nodep, ثابت string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        ܐܢ (m_type.isCoverage()) m_newFunctions.push_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        ܐܢ (m_newFunctions.size() == 0) {
        } ܐܚܪܢܐ ܐܢ (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } ܐܚܪܢܐ {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ثابت rootFuncp = makeNewFunc();
            rootFuncp->name(m_basename);
            لكل (AstCFunc* ثابت funcp : m_newFunctions) {
                AstCCall* ثابت callp = جديد AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                ܐܢ (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } ܐܚܪܢܐ {
                    ܐܢ (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                rootFuncp->addStmtsp(callp->makeStmt());
            }
        }
    }

خاص:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

صنف CCtorsVisitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstNodeModule* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ثابت AstNodeModule*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ساكن فراغ insertSc(AstCFunc* cfuncp, ثابت AstNodeModule* modp, VSystemCSectionType type) {
        ثابت auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        ܐܢ (txtAndFlp.first.empty()) ܦܢܐ;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ثابت cstmtp = جديد AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        ܐܢ (nodep->isConstPool()) ܦܢܐ;  // Special emit rules
        ܐܢ (ثابت AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            ܐܢ (classp->isInterfaceClass()) ܦܢܐ;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        ܐܢ (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            لكل (AstNode *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                ܐܢ (AstNodeCoverDecl* ثابت coverp = VN_CAST(np, NodeCoverDecl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        ܐܢ (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            AstCFunc* ثابت funcp = جديد AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(ܫܪܪܐ);
            funcp->isStatic(ܕܓܠܐ);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(ܕܓܠܐ);
            insertSc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ܐܢ (nodep->name() == "new") insertSc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    فراغ visit(AstCStmt* nodep) override {
        ܐܢ (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        ܐܢ (nodep->needsCReset()) {
            AstNode* ثابت crstp = جديد AstAssign{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جديد AstCReset{nodep->fileline(), nodep, ܫܪܪܐ}};
            ܐܢ (m_varResetp) {
                m_varResetp->add(crstp);
            } ܐܚܪܢܐ ܐܢ (m_cfuncp) {
                nodep->addNextHere(crstp);
            } ܐܚܪܢܐ {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit CCtorsVisitor(AstNode* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        لكل (auto& itr : m_ctorCalls) {
            AstCStmt* ثابت nodep = itr.first;
            ثابت AstNodeModule* ثابت modp = itr.second;
            ܐܢ (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        }
    }
};

//######################################################################

فراغ V3CCtors::evalAsserts() {
    AstNodeModule* ثابت modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ثابت funcp
        = جديد AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(ܫܪܪܐ);
    funcp->isStatic(ܕܓܠܐ);
    funcp->isLoose(ܫܪܪܐ);
    funcp->keepIfEmpty(ܫܪܪܐ);
    funcp->slow(ܕܓܠܐ);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    لكل (AstNode* np = modp->stmtsp(); np; np = np->nextp()) {
        ܐܢ (AstVar* ثابت varp = VN_CAST(np, Var)) {
            ܐܢ (varp->isPrimaryInish() && !varp->isSc()) {
                ܐܢ (ثابت AstBasicDType* ثابت basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ثابت صحيح storedWidth = basicp->widthAlignBytes() * 8;
                    ثابت صحيح lastWordWidth = varp->width() % storedWidth;
                    ܐܢ (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ثابت vrefp
                            = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        AstNodeExpr* newp = vrefp;
                        ܐܢ (varp->isWide()) {
                            newp = جديد AstWordSel{
                                varp->fileline(), newp,
                                جديد AstConst(varp->fileline(), varp->widthWords() - 1)};
                        }
                        ثابت uint64_t value = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = جديد AstAnd{varp->fileline(), newp,
                                          جديد AstConst(varp->fileline(), AstConst::WidthedValue{},
                                                       storedWidth, value)};
                        AstNodeIf* ثابت ifp = جديد AstIf{
                            varp->fileline(), newp,
                            جديد AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

فراغ V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dumpCheckGlobalTree("cctors", 0, dumpTreeEitherLevel() >= 3);
}
