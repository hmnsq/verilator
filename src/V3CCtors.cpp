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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3CCtors.h"

#அந்தர்பாவ "V3EmitCBase.h"

#அந்தர்பாவ <list>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு VCtorType final {
பொது:
    எண்ணிக்கை en : uint8_t { MODULE, CLASS, COVERAGE };

தனிப்பட்ட:
    மாறிலி எண்ணிக்கை en m_e;

பொது:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    தருக்க isClass() மாறிலி { திரும்பு m_e == CLASS; }
    தருக்க isCoverage() மாறிலி { திரும்பு m_e == COVERAGE; }
};

வகுப்பு V3CCtorsBuilder final {
    Ast_முனை_Module* மாறிலி m_modp;  // Current module/class
    மாறிலி string m_basename;
    மாறிலி VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    முழு m_இலக்கம்_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        மாறிலி முழு func_இலக்கம் = m_newFunctions.size();
        மாறிலி string funcName = m_basename + "_" + cvtToStr(func_இலக்கம்);
        AstCFunc* மாறிலி funcp = புதிய AstCFunc{m_modp->fileline(), funcName, nullptr, "void"};
        funcp->isStatic(பொய்);
        funcp->isLoose(!m_type.isClass());
        funcp->keepIfEmpty(உண்மை);  // TODO relax
        funcp->declPrivate(உண்மை);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        எனில் (!m_type.isCoverage()) m_modp->ctorVarReset(உண்மை);
        string preventUnusedStmt;
        எனில் (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            preventUnusedStmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } இல்லையெனில் எனில் (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            preventUnusedStmt = "(void)first;  // Prevent unused variable warning";
        }
        எனில் (!preventUnusedStmt.empty()) {
            funcp->addStmtsp(புதிய AstCStmt{m_modp->fileline(), preventUnusedStmt});
        }
        m_modp->addStmtsp(funcp);
        m_இலக்கம்_Stmts = 0;
        திரும்பு funcp;
    }

பொது:
    வெறுமை add(Ast_முனை* nodep) {
        எனில் (m_newFunctions.empty()) m_newFunctions.தள்ளு_back(makeNewFunc());
        எனில் (v3Global.opt.outputSplitCFuncs() && m_இலக்கம்_Stmts > v3Global.opt.outputSplitCFuncs()) {
            m_newFunctions.தள்ளு_back(makeNewFunc());
        }
        m_newFunctions.back()->addStmtsp(nodep);
        m_இலக்கம்_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_முனை_Module* nodep, மாறிலி string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        எனில் (m_type.isCoverage()) m_newFunctions.தள்ளு_back(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        எனில் (m_newFunctions.size() == 0) {
        } இல்லையெனில் எனில் (m_newFunctions.size() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } இல்லையெனில் {
            // Split was necessary, create root function and call all others from that
            AstCFunc* மாறிலி வேர்_Funcp = makeNewFunc();
            வேர்_Funcp->name(m_basename);
            க்கு (AstCFunc* மாறிலி funcp : m_newFunctions) {
                AstCCall* மாறிலி callp = புதிய AstCCall{m_modp->fileline(), funcp};
                callp->dtypeSetVoid();
                எனில் (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } இல்லையெனில் {
                    எனில் (m_type.isCoverage()) callp->argTypes("first");
                    callp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                }
                வேர்_Funcp->addStmtsp(callp->makeStmt());
            }
        }
    }

தனிப்பட்ட:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

வகுப்பு CCtorsVisitor final : பொது VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    AstCFunc* m_cfuncp = nullptr;  // Current function
    V3CCtorsBuilder* m_varResetp = nullptr;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, மாறிலி Ast_முனை_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    நிலையான வெறுமை செருகு_Sc(AstCFunc* cfuncp, மாறிலி Ast_முனை_Module* modp, VSystemCSectionType type) {
        மாறிலி auto txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        எனில் (txtAndFlp.first.empty()) திரும்பு;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* மாறிலி cstmtp = புதிய AstCStmtUser{txtAndFlp.second};
        cstmtp->add(txtAndFlp.first);
        cfuncp->addStmtsp(cstmtp);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (nodep->isConstPool()) திரும்பு;  // Special emit rules
        எனில் (மாறிலி AstClass* மாறிலி classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            எனில் (classp->isInterfaceClass()) திரும்பு;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        எனில் (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            க்கு (Ast_முனை *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                எனில் (Ast_முனை_Cover_Decl* மாறிலி coverp = VN_CAST(np, முனை_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.add(coverp->unlinkFrBack());
                }
            }
        }
        எனில் (AstClass* மாறிலி classp = VN_CAST(nodep, Class)) {
            AstCFunc* மாறிலி funcp = புதிய AstCFunc{classp->fileline(), "~", nullptr, ""};
            funcp->isDestructor(உண்மை);
            funcp->isStatic(பொய்);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(பொய்);
            செருகு_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->addStmtsp(funcp);
        }
    }

    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = nullptr;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        எனில் (nodep->name() == "new") செருகு_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    வெறுமை visit(AstCStmt* nodep) override {
        எனில் (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVar* nodep) override {
        எனில் (nodep->needsCReset()) {
            Ast_முனை* மாறிலி crstp = புதிய AstAssign{
                nodep->fileline(), புதிய AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                புதிய AstCReset{nodep->fileline(), nodep, உண்மை}};
            எனில் (m_varResetp) {
                m_varResetp->add(crstp);
            } இல்லையெனில் எனில் (m_cfuncp) {
                nodep->addNextHere(crstp);
            } இல்லையெனில் {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit CCtorsVisitor(Ast_முனை* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        க்கு (auto& itr : m_ctorCalls) {
            AstCStmt* மாறிலி nodep = itr.first;
            மாறிலி Ast_முனை_Module* மாறிலி modp = itr.second;
            எனில் (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        }
    }
};

//######################################################################

வெறுமை V3CCtors::evalAsserts() {
    Ast_முனை_Module* மாறிலி modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* மாறிலி funcp
        = புதிய AstCFunc{modp->fileline(), "_eval_debug_assertions", nullptr, "void"};
    funcp->declPrivate(உண்மை);
    funcp->isStatic(பொய்);
    funcp->isLoose(உண்மை);
    funcp->keepIfEmpty(உண்மை);
    funcp->slow(பொய்);
    funcp->ifdef("VL_DEBUG");
    modp->addStmtsp(funcp);
    க்கு (Ast_முனை* np = modp->stmtsp(); np; np = np->nextp()) {
        எனில் (AstVar* மாறிலி varp = VN_CAST(np, Var)) {
            எனில் (varp->isPrimaryInish() && !varp->isSc()) {
                எனில் (மாறிலி AstBasicDType* மாறிலி basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    மாறிலி முழு storedWidth = basicp->widthAlignBytes() * 8;
                    மாறிலி முழு lastWordWidth = varp->width() % storedWidth;
                    எனில் (lastWordWidth != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* மாறிலி vrefp
                            = புதிய AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->selfPointer(VSelfPointerText{VSelfPointerText::This{}});
                        Ast_முனை_கோவை* newp = vrefp;
                        எனில் (varp->isWide()) {
                            newp = புதிய AstWordSel{
                                varp->fileline(), newp,
                                புதிய AstConst(varp->fileline(), varp->width_சொற்கள்() - 1)};
                        }
                        மாறிலி uint64_t மதிப்பு = VL_MASK_Q(storedWidth) & ~VL_MASK_Q(lastWordWidth);
                        newp = புதிய AstAnd{varp->fileline(), newp,
                                          புதிய AstConst(varp->fileline(), AstConst::Widthed_மதிப்பு{},
                                                       storedWidth, மதிப்பு)};
                        Ast_முனை_If* மாறிலி ifp = புதிய AstIf{
                            varp->fileline(), newp,
                            புதிய AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->addStmtsp(ifp);
                    }
                }
            }
        }
    }
}

வெறுமை V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_மரம்("cctors", 0, dump_மரம்_Either_Level() >= 3);
}
