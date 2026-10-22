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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3CCtors.h"

#شامل "V3EmitCBase.h"

#شامل <list>

VL_DEFINE_DEBUG_FUNCTIONS;

رده VCtorType final {
عمومی:
    شمارش en : uint8_t { MODULE, CLASS, COVERAGE };

خصوصی:
    ثابت شمارش en m_e;

عمومی:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    منطقی isClass() ثابت { بازگشت m_e == CLASS; }
    منطقی isCoverage() ثابت { بازگشت m_e == COVERAGE; }
};

رده V3CCtorsBuilder final {
    Ast_گره_Module* ثابت m_modp;  // Current module/class
    ثابت string m_basename;
    ثابت VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    صحیح m_رقم_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        ثابت صحیح func_رقم = m_newFunctions.size();
        ثابت string funcName = m_basename + "_" + cvtToStr(func_رقم);
        AstCFunc* ثابت funcp = جدید AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(نادرست);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(درست);  // TODO relax
        funcp->declPrivate(درست);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        اگر (!m_type.isCoverage()) m_modp->ctorVarReset(درست);
        string preventUnusedStmt;
        اگر (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } وگرنه اگر (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        اگر (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(جدید AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_رقم_Stmts = 0;
        بازگشت funcp;
    }

عمومی:
    تهی add(Ast_گره* nodep) {
        اگر (m_newFunctions.empty()) m_newFunctions.افزودن_back(makeNewFunc());
        اگر (v3Global.opt.outputSplitCFuncs() && m_رقم_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.افزودن_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_رقم_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_گره_Module* nodep, ثابت string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        اگر (m_type.isCoverage()) m_newFunctions.افزودن_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        اگر (m_newFunctions.size() == 0) {
        } وگرنه اگر (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } وگرنه {
            // Split was necessary, create root function and call all others from that
            AstCFunc* ثابت ریشه_Funcp = makeNewFunc();
            ریشه_Funcp->name(m_basename);
            برای (AstCFunc* ثابت funcp : m_newFunctions) {
                AstCCall* ثابت callp = جدید AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                اگر (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } وگرنه {
                    اگر (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                ریشه_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

خصوصی:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

رده CCtorsVisitor final : عمومی VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گره_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, ثابت Ast_گره_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    ایستا تهی درج_Sc(AstCFunc* cfuncp, ثابت Ast_گره_Module* modp, VSystemCSectionType type) {
        ثابت auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        اگر (txtAndFlp.first.empty()) بازگشت;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* ثابت cstmtp = جدید AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        اگر (nodep->isConstPool()) بازگشت;  // Special emit rules
        اگر (ثابت AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            اگر (classp->isInterfaceClass()) بازگشت;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        اگر (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            برای (Ast_گره *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                اگر (Ast_گره_Cover_Decl* ثابت coverp = VN_CAST(np, گره_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            AstCFunc* ثابت funcp = جدید AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(درست);
            funcp->isStatic(نادرست);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(نادرست);
            درج_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        اگر (nodep->name() == "new") درج_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    تهی visit(AstCStmt* nodep) override {
        اگر (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    تهی visit(AstVar* nodep) override {
        اگر (nodep->needsCReset()) {
            Ast_گره* ثابت crstp = جدید AstAssign{
                nodep->fileline(), جدید AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جدید AstCReset{nodep->fileline(), nodep, درست}};
            اگر (m_varResetp) {
                m_varResetp->add(crstp);
            } وگرنه اگر (m_cfuncp) {
                nodep->addNextHere(crstp);
            } وگرنه {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_گره* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        برای (auto& itr : m_ctorCalls) {
            AstCStmt* ثابت nodep = itr.first;
            ثابت Ast_گره_Module* ثابت modp = itr.second;
            اگر (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        }
    }
};

//######################################################################

تهی V3CCtors::evalAsserts() {
    Ast_گره_Module* ثابت modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* ثابت funcp
        = جدید AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(درست);
    funcp->isStatic(نادرست);
    funcp->isLoose(درست);
    funcp->keepIfEmpty(درست);
    funcp->slow(نادرست);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    برای (Ast_گره* np = modp->stmtsp(); np; np = np->nextp()) {
        اگر (AstVar* ثابت varp = VN_CAST(np, Var)) {
            اگر (varp->isPrimaryInish() && !varp->isSc()) {
                اگر (ثابت AstBasicDType* ثابت basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    ثابت صحیح storedWidth = basicp->widthAlignBytes() * 8;
                    ثابت صحیح lastWordWidth = varp->width() % storedWidth;
                    اگر (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* ثابت vrefp
                            = جدید AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_گره_عبارت* newp = vrefp;
                        اگر (varp->isWide()) {
                            newp = جدید AstWordSel{
                                varp->fileline(), newp,
                                جدید AstConst(varp->fileline(), varp->width_واژگان() - 1)};
                        }
                        ثابت uint64_t مقدار = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = جدید AstAnd{varp->fileline(), newp,
                                          جدید AstConst(varp->fileline(), AstConst::Widthed_مقدار{},
                                                       storedWidth, مقدار)};
                        Ast_گره_If* ثابت ifp = جدید AstIf{
                            varp->fileline(), newp,
                            جدید AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

تهی V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_درخت("cctors", 0, dump_درخت_Either_Level() >= 3);
}
