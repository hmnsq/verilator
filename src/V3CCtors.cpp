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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3CCtors.h"

#అన్తర్భావ "V3EmitCBase.h"

#అన్తర్భావ <list>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి VCtorType final {
బహిరంగ:
    గణన en : uint8_t { MODULE, CLASS, COVERAGE };

అంతరంగ:
    స్థిరరాశి గణన en m_e;

బహిరంగ:
    // cppcheck-suppress noExplicitConstructor
    constexpr VCtorType(en _e)
        : m_e{_e} {}
    తార్కిక isClass() స్థిరరాశి { తిరిగి m_e == CLASS; }
    తార్కిక isCoverage() స్థిరరాశి { తిరిగి m_e == COVERAGE; }
};

తరగతి V3CCtorsBuilder final {
    Ast_కణుపు_Module* స్థిరరాశి m_modp;  // Current module/class
    స్థిరరాశి string m_basename;
    స్థిరరాశి VCtorType m_type;  // What kind of constructor are we creating
    std::list<AstCFunc*> m_newFunctions;  // Created functions, latest is at back
    పూర్ణాంకం m_అంకె_Stmts = 0;  // Number of statements output

    AstCFunc* makeNewFunc() {
        స్థిరరాశి పూర్ణాంకం func_అంకె = m_newFunctions.కొలత();
        స్థిరరాశి string funcName = m_basename + "_" + cvt_గమ్యం_Str(func_అంకె);
        AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{m_modp->fileline(), funcName, శూన్యసూచిక, "void"};
        funcp->isStatic(అబద్ధం);
        funcp->isLoose(!m_type.isClass());
        funcp->keep_If_ఖాళీలు(నిజం);  // TODO relax
        funcp->decl_వ్యక్తిగత(నిజం);
        funcp->slow(!m_type.isClass());  // Only classes construct on fast path
        ఒకవేళ (!m_type.isCoverage()) m_modp->ctorVarReset(నిజం);
        string prevent_వాడని_Stmt;
        ఒకవేళ (m_type.isClass()) {
            funcp->argTypes(EmitCUtil::symClassVar());
            prevent_వాడని_Stmt = "(void)vlSymsp;  // Prevent unused variable warning";
        } లేకపోతే ఒకవేళ (m_type.isCoverage()) {
            funcp->argTypes("bool first");
            prevent_వాడని_Stmt = "(void)first;  // Prevent unused variable warning";
        }
        ఒకవేళ (!prevent_వాడని_Stmt.ఖాళీలు()) {
            funcp->కూడు_Stmtsp(కొత్త AstCStmt{m_modp->fileline(), prevent_వాడని_Stmt});
        }
        m_modp->కూడు_Stmtsp(funcp);
        m_అంకె_Stmts = 0;
        తిరిగి funcp;
    }

బహిరంగ:
    శూన్య కూడు(Ast_కణుపు* nodep) {
        ఒకవేళ (m_newFunctions.ఖాళీలు()) m_newFunctions.నెట్టు_వెనుక(makeNewFunc());
        ఒకవేళ (v3Global.opt.output_విభజించు_CFuncs() && m_అంకె_Stmts > v3Global.opt.output_విభజించు_CFuncs()) {
            m_newFunctions.నెట్టు_వెనుక(makeNewFunc());
        }
        m_newFunctions.వెనుక()->కూడు_Stmtsp(nodep);
        m_అంకె_Stmts += 1;
    }

    V3CCtorsBuilder(Ast_కణుపు_Module* nodep, స్థిరరాశి string& basename, VCtorType type)
        : m_modp{nodep}
        , m_basename{basename}
        , m_type{type} {
        // Expect coverage function to always exist, so must always create at least one.
        ఒకవేళ (m_type.isCoverage()) m_newFunctions.నెట్టు_వెనుక(makeNewFunc());
    }

    ~V3CCtorsBuilder() {
        ఒకవేళ (m_newFunctions.కొలత() == 0) {
        } లేకపోతే ఒకవేళ (m_newFunctions.కొలత() == 1) {
            // No split was necessary, rename the one function to the basename
            m_newFunctions.front()->name(m_basename);
        } లేకపోతే {
            // Split was necessary, create root function and call all others from that
            AstCFunc* స్థిరరాశి మూలం_Funcp = makeNewFunc();
            మూలం_Funcp->name(m_basename);
            కొరకు (AstCFunc* స్థిరరాశి funcp : m_newFunctions) {
                AstCCall* స్థిరరాశి callp = కొత్త AstCCall{m_modp->fileline(), funcp};
                callp->dtype_సమితి_Void();
                ఒకవేళ (m_type.isClass()) {
                    callp->argTypes("vlSymsp");
                } లేకపోతే {
                    ఒకవేళ (m_type.isCoverage()) callp->argTypes("first");
                    callp->self_సూచిక(VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::This{}});
                }
                మూలం_Funcp->కూడు_Stmtsp(callp->makeStmt());
            }
        }
    }

అంతరంగ:
    VL_UNCOPYABLE(V3CCtorsBuilder);
};

//######################################################################

// Link state, as a visitor of each AstNode

తరగతి CCtorsVisitor final : బహిరంగ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    AstCFunc* m_cfuncp = శూన్యసూచిక;  // Current function
    V3CCtorsBuilder* m_varResetp = శూన్యసూచిక;  // Builder of _ctor_var_reset
    std::map<AstCStmt*, స్థిరరాశి Ast_కణుపు_Module*> m_ctorCalls;  // Calls to _ctor_var_reset

    // METHODS
    స్థిర శూన్య చొప్పించు_Sc(AstCFunc* cfuncp, స్థిరరాశి Ast_కణుపు_Module* modp, VSystemCSectionType type) {
        స్థిరరాశి స్వయంచాలక txtAndFlp = EmitCBaseVisitorConst::scSection(modp, type);
        ఒకవేళ (txtAndFlp.మొదటి.ఖాళీలు()) తిరిగి;
        // Use an AstCStmtUser as this is from user input
        AstCStmtUser* స్థిరరాశి cstmtp = కొత్త AstCStmtUser{txtAndFlp.రెండవ};
        cstmtp->కూడు(txtAndFlp.మొదటి);
        cfuncp->కూడు_Stmtsp(cstmtp);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (nodep->isConstPool()) తిరిగి;  // Special emit rules
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(nodep, Class)) {
            // Interface class may only have pure virtuals and params which do not need cctor reset
            ఒకవేళ (classp->isInterfaceClass()) తిరిగి;
        }
        VL_RESTORER(m_modp);
        VL_RESTORER(m_varResetp);
        m_modp = nodep;
        V3CCtorsBuilder var_reset{nodep, "_ctor_var_reset",
                                  VN_IS(nodep, Class) ? VCtorType::CLASS : VCtorType::MODULE};
        // cppcheck-suppress danglingLifetime
        m_varResetp = &var_reset;
        iterateChildren(nodep);

        ఒకవేళ (v3Global.opt.coverage()) {
            V3CCtorsBuilder configure_coverage{nodep, "_configure_coverage", VCtorType::COVERAGE};
            కొరకు (Ast_కణుపు *np = nodep->stmtsp(), *nextp; np; np = nextp) {
                nextp = np->nextp();
                ఒకవేళ (Ast_కణుపు_Cover_Decl* స్థిరరాశి coverp = VN_CAST(np, కణుపు_Cover_Decl)) {
                    // ... else we don't have a static VlSym to be able to coverage insert
                    UASSERT_OBJ(!VN_IS(nodep, Class), coverp,
                                "NodeCoverDecl should be in class's package, not class itself");
                    configure_coverage.కూడు(coverp->unlink_Fr_వెనుక());
                }
            }
        }
        ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(nodep, Class)) {
            AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{classp->fileline(), "~", శూన్యసూచిక, ""};
            funcp->isDestructor(నిజం);
            funcp->isStatic(అబద్ధం);
            // If can be referred to by base pointer, need virtual delete
            funcp->isVirtual(classp->isExtended());
            funcp->slow(అబద్ధం);
            చొప్పించు_Sc(funcp, classp, VSystemCSectionType::DTOR);
            classp->కూడు_Stmtsp(funcp);
        }
    }

    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_varResetp);
        VL_RESTORER(m_cfuncp);
        m_varResetp = శూన్యసూచిక;
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ఒకవేళ (nodep->name() == "new") చొప్పించు_Sc(nodep, m_modp, VSystemCSectionType::CTOR);
    }
    శూన్య సందర్శించు(AstCStmt* nodep) override {
        ఒకవేళ (nodep->stmtType() == VCStmtType::CTOR_VAR_RESET_CALL) {
            UASSERT_OBJ(m_modp, nodep, "ctor_var_reset call not under module");
            m_ctorCalls.emplace(nodep, m_modp);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (nodep->needsCReset()) {
            Ast_కణుపు* స్థిరరాశి crstp = కొత్త Ast_కేటాయించు{
                nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                కొత్త AstCReset{nodep->fileline(), nodep, నిజం}};
            ఒకవేళ (m_varResetp) {
                m_varResetp->కూడు(crstp);
            } లేకపోతే ఒకవేళ (m_cfuncp) {
                nodep->కూడు_తదుపరి_ఇక్కడ(crstp);
            } లేకపోతే {
                nodep->v3fatalSrc("Var needs CReset but nowhere to place it");
            }
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CCtorsVisitor(Ast_కణుపు* nodep) { iterate(nodep); }
    ~CCtorsVisitor() override {
        // Remove CStmts to ctor_var_resets that are no longer needed
        కొరకు (స్వయంచాలక& itr : m_ctorCalls) {
            AstCStmt* స్థిరరాశి nodep = itr.మొదటి;
            స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = itr.రెండవ;
            ఒకవేళ (!modp->ctorVarReset()) VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        }
    }
};

//######################################################################

శూన్య V3CCtors::evalAsserts() {
    Ast_కణుపు_Module* స్థిరరాశి modp = v3Global.rootp()->modulesp();  // Top module wrapper
    AstCFunc* స్థిరరాశి funcp
        = కొత్త AstCFunc{modp->fileline(), "_eval_debug_assertions", శూన్యసూచిక, "void"};
    funcp->decl_వ్యక్తిగత(నిజం);
    funcp->isStatic(అబద్ధం);
    funcp->isLoose(నిజం);
    funcp->keep_If_ఖాళీలు(నిజం);
    funcp->slow(అబద్ధం);
    funcp->ifdef("VL_DEBUG");
    modp->కూడు_Stmtsp(funcp);
    కొరకు (Ast_కణుపు* np = modp->stmtsp(); np; np = np->nextp()) {
        ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(np, Var)) {
            ఒకవేళ (varp->isPrimaryInish() && !varp->isSc()) {
                ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి basicp
                    = VN_CAST(varp->dtypeSkipRefp(), BasicDType)) {
                    స్థిరరాశి పూర్ణాంకం stored_వెడల్పు = basicp->వెడల్పు_Align_బైట్లు() * 8;
                    స్థిరరాశి పూర్ణాంకం చివరి_మాట_వెడల్పు = varp->వెడల్పు() % stored_వెడల్పు;
                    ఒకవేళ (చివరి_మాట_వెడల్పు != 0) {
                        // if (signal & CONST(upper_non_clean_mask)) { fail; }
                        AstVarRef* స్థిరరాశి vrefp
                            = కొత్త AstVarRef{varp->fileline(), varp, VAccess::READ};
                        vrefp->self_సూచిక(VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::This{}});
                        Ast_కణుపు_సమాసం* newp = vrefp;
                        ఒకవేళ (varp->isWide()) {
                            newp = కొత్త Ast_మాట_Sel{
                                varp->fileline(), newp,
                                కొత్త AstConst(varp->fileline(), varp->వెడల్పు_పదాలు() - 1)};
                        }
                        స్థిరరాశి uint64_t విలువ = VL_ముసుగు_Q(stored_వెడల్పు) & ~VL_ముసుగు_Q(చివరి_మాట_వెడల్పు);
                        newp = కొత్త AstAnd{varp->fileline(), newp,
                                          కొత్త AstConst(varp->fileline(), AstConst::Widthed_విలువ{},
                                                       stored_వెడల్పు, విలువ)};
                        Ast_కణుపు_If* స్థిరరాశి ifp = కొత్త AstIf{
                            varp->fileline(), newp,
                            కొత్త AstCStmt{varp->fileline(), "Verilated::overWidthError(\""
                                                               + varp->prettyName() + "\");"}};
                        ifp->branchPred(VBranchPred::BP_UNLIKELY);
                        funcp->కూడు_Stmtsp(ifp);
                    }
                }
            }
        }
    }
}

శూన్య V3CCtors::cctorsAll() {
    UINFO(2, __FUNCTION__ << ":");
    evalAsserts();
    { CCtorsVisitor{v3Global.rootp()}; }
    V3Global::dump_Check_Global_వృక్షం("cctors", 0, dump_వృక్షం_Either_Level() >= 3);
}
