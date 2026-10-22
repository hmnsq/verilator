// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Netlist (top level) functions
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
// COVERAGE TRANSFORMATIONS:
//      At each IF/(IF else)/CASEITEM,
//         If there's no coverage off on the block below it,
//         or a $stop
//              Insert a COVERDECL node in the module.
//              (V3Emit reencodes into per-module numbers for emitting.)
//              Insert a COVERINC node at the end of the statement list
//              for that if/else/case.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Coverage.h"

#ضمن "V3EmitV.h"
#ضمن "V3UniqueNames.h"

#ضمن <list>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف بیان_Coverage_Eligible_Visitor final : عام VNVisitorConst {
    // STATE
    منطقي m_eligible = ریښتیا;

    ساكن منطقي elemDTypeEligible(ثابت Ast_غوټه_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت Ast_غوټه_DType* ثابت dtp = dtypep->virtRefDTypep()) {
            إذا (!elemDTypeEligible(dtp)) أرجع خطأ;
        }
        إذا (ثابت Ast_غوټه_DType* ثابت dtp = dtypep->virtRefDType2p()) {
            إذا (!elemDTypeEligible(dtp)) أرجع خطأ;
        }
        أرجع !VN_IS(dtypep, ClassRefDType);
    }

    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        ثابت Ast_غوټه_DType* ثابت dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        إذا (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } وإلا {
            m_eligible = خطأ;
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (!nodep->is_بیان_Coverage_Eligible()) {
            m_eligible = خطأ;
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }

عام:
    // CONSTRUCTORS
    څرګند بیان_Coverage_Eligible_Visitor(Ast_غوټه* nodep) { iterateConst(nodep); }
    ~بیان_Coverage_Eligible_Visitor() override = افتراضي;

    منطقي eligible() ثابت { أرجع m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

صنف CoverageVisitor final : عام VNVisitor {
    // TYPES
    کارول Lineno_سېټ = std::سېټ<صحيح>;

    بنية Cover_غړی final {
        Ast_غوټه_بیان* m_exprp;  // Expression branch term
        منطقي m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        Cover_غړی(Ast_غوټه_بیان* exprp, منطقي objective, ثابت string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    کارول Cover_بیان = std::deque<Cover_غړی>;
    کارول CoverExprs = std::list<Cover_بیان>;

    بنية ToggleEnt final {
        ثابت string m_comment;  // Comment for coverage dump
        Ast_غوټه_بیان* m_varRefp;  // How to get to this element
        Ast_غوټه_بیان* m_chgRefp;  // How to get to this element
        ToggleEnt(ثابت string& comment, Ast_غوټه_بیان* vp, Ast_غوټه_بیان* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = افتراضي;
        فراغ cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_ونه(), m_varRefp = تش_اشاره);
            VL_DO_CLEAR(m_chgRefp->delete_ونه(), m_chgRefp = تش_اشاره);
        }
    };

    بنية CheckState final {  // State save-restored on each new coverage scope/block
        منطقي m_on = خطأ;  // Should this block get covered?
        منطقي m_دننه_Mod_Off = خطأ;  // In module with no coverage
        صحيح m_handle = 0;  // Opaque handle for index into line tracking
        ثابت Ast_غوټه* m_nodep = تش_اشاره;  // Node establishing this state
        CheckState() = افتراضي;
        منطقي coverageOn(ثابت Ast_غوټه* nodep) ثابت {
            أرجع m_on && !m_دننه_Mod_Off && nodep->fileline()->coverageOn();
        }
        منطقي کرښه_Coverage_On(ثابت Ast_غوټه* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverage_کرښه();
        }
        منطقي branchCoverageOn(ثابت Ast_غوټه* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        منطقي بیان_Coverage_On(ثابت Ast_غوټه* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverage_بیان();
        }
    };

    تعداد Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;
    V3_Unique_نومونه m_بیان_لنډمهاله_نومونه;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_غوټه_بیان*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    صحيح m_بل_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module to add statement to
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    Ast_غوټه* m_بیان_Stmtsp = تش_اشاره;  // Node to add expr coverage to
    منطقي m_then = خطأ;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    منطقي m_objective = خطأ;  // Expression objective
    منطقي m_ifCond = خطأ;  // Visiting if condition
    منطقي m_دننه_Toggle_Off = خطأ;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<صحيح, Lineno_سېټ> m_handle_کرښې;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ثابت حرف* varIgnoreToggle(ثابت AstVar* nodep) {
        ثابت منطقي cover = nodep->isIO() || (nodep->isSignal() && nodep->is_بټ_Logic());
        إذا (!cover) أرجع "Not relevant signal";
        إذا (nodep->isConst()) أرجع "Signal is constant";
        إذا (nodep->isDouble()) أرجع "Signal is double";
        إذا (nodep->isString()) أرجع "Signal is string";
        إذا (!v3Global.opt.coverageUnderscore()) {
            ثابت string prettyName = nodep->prettyName();
            إذا (prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.پیدا_کړه("._") != string::npos) أرجع "Inlined leading underscore";
        }
        إذا ((nodep->پلنوالی() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverage_Max_پلنوالی())) {
            أرجع "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        أرجع تش_اشاره;
    }

    AstCoverInc* newCoverInc(File_کرښه* fl, Ast_غوټه_Cover_Decl* ثابت declp,
                             ثابت string& trace_var_name) {
        AstCoverInc* ثابت incp = جديد AstCoverInc{fl, declp};
        إذا (!trace_var_name.تشه()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_کرښه* ثابت fl_nowarn = جديد File_کرښه{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3_Error_کوډ::UNUSEDSIGNAL, ریښتیا);
            AstVar* ثابت varp = جديد AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->پیدا_کړه_UInt32_DType()};
            varp->سېټ_Ignore_Sched_Write();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(ریښتیا);
            m_modp->جمع_کړه_Stmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            Ast_ټاکل* ثابت assp = جديد Ast_ټاکل{
                incp->fileline(), جديد AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                جديد Ast_جمع_کړه{incp->fileline(), جديد AstVarRef{incp->fileline(), varp, VAccess::READ},
                           جديد AstConst{incp->fileline(), AstConst::Widthed_ارزښت{}, 32, 1}}};
            Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(incp, assp);
        }
        أرجع incp;
    }
    string trace_Name_For_کرښه(ثابت Ast_غوټه* nodep, ثابت string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvt_منزل_Str(nodep->fileline()->lineno()) + "_" + type;
        إذا (ثابت uint32_t suffix = m_varnames[name]++) name += "_" + cvt_منزل_Str(suffix);
        أرجع name;
    }

    // Line tracking
    فراغ createHandle(ثابت Ast_غوټه* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = ریښتیا;
        m_state.m_handle = ++m_بل_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    فراغ کرښه_Track(ثابت Ast_غوټه* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        إذا ((m_state.کرښه_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            لكل (صحيح lineno = nodep->fileline()->لومړی_Lineno();
                 lineno <= nodep->fileline()->وروستی_Lineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_کرښې[m_state.m_handle].ورننباسه(lineno);
            }
        }
    }
    ساكن string کرښې_لومړی_وروستی(ثابت صحيح لومړی, ثابت صحيح وروستی) {
        إذا (لومړی && لومړی == وروستی) {
            أرجع cvt_منزل_Str(لومړی);
        } وإلا إذا (لومړی && وروستی) {
            أرجع cvt_منزل_Str(لومړی) + "-" + cvt_منزل_Str(وروستی);
        } وإلا {
            أرجع "";
        }
    }
    string کرښې_Cov(ثابت CheckState& state, ثابت Ast_غوټه* nodep) {
        // Return comma separated list of ranged numbers
        string بهر;
        ثابت Lineno_سېټ& کرښې = m_handle_کرښې[state.m_handle];
        صحيح لومړی = 0;
        صحيح وروستی = 0;
        لكل (ثابت صحيح linen : کرښې) {
            إذا (!لومړی) {
                لومړی = وروستی = linen;
            } وإلا إذا (linen == وروستی + 1) {
                ++وروستی;
            } وإلا {
                إذا (!بهر.تشه()) بهر += ",";
                بهر += کرښې_لومړی_وروستی(لومړی, وروستی);
                لومړی = وروستی = linen;
            }
        }
        إذا (لومړی) {
            إذا (!بهر.تشه()) بهر += ",";
            بهر += کرښې_لومړی_وروستی(لومړی, وروستی);
        }
        UINFO(9, "lines out " << بهر << " for h" << state.m_handle << " " << nodep);
        أرجع بهر;
    }

    // VISITORS - BOTH
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        ثابت Ast_غوټه_Module* ثابت origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_بیان_لنډمهاله_نومونه);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_دننه_Mod_Off = nodep->is_سر();  // Already made top shell, no coverage for it
        إذا (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_کرښې.clear();
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_بیان_لنډمهاله_نومونه);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_دننه_Mod_Off = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_ټاکل) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_بیان_Stmtsp);
            VL_RESTORER(m_دننه_Toggle_Off);
            m_بیان_Stmtsp = nodep;
            m_دننه_Toggle_Off = ریښتیا;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            أرجع;
        }
        iterateProcedure(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override { iterateProcedure(nodep); }
    فراغ لیدنه(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_بیان_Stmtsp);
        VL_RESTORER(m_دننه_Toggle_Off);
        m_بیان_Stmtsp = nodep;
        m_دننه_Toggle_Off = ریښتیا;
        createHandle(nodep);
        iterate_And_بل_Null(nodep->stmtsp());
        إذا (m_state.کرښه_Coverage_On(nodep)) {
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", کرښې_Cov(m_state, nodep), 0};
            m_modp->جمع_کړه_Stmtsp(declp);
            Ast_غوټه* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_کرښه(nodep, "block"));
            ورننباسه_Proc_Statement(nodep, newp);
        }
    }
    فراغ لیدنه(AstLoopTest* nodep) override {
        إذا (nodep->user2_سېټ_Once()) أرجع;
        کرښه_Track(nodep);
        إذا (m_state.کرښه_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", کرښې_Cov(m_state, nodep), 0};
            m_modp->جمع_کړه_Stmtsp(declp);
            Ast_غوټه* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_کرښه(nodep, "block"));
            nodep->جمع_کړه_دلته_This_As_بل(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_بیان_لنډمهاله_نومونه);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        إذا (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    فراغ ورننباسه_Proc_Statement(Ast_غوټه* nodep, Ast_غوټه* stmtp) {
        إذا (Ast_غوټه_Procedure* ثابت itemp = VN_CAST(nodep, غوټه_Procedure)) {
            itemp->جمع_کړه_Stmtsp(stmtp);
        } وإلا إذا (Ast_غوټه_FTask* ثابت itemp = VN_CAST(nodep, غوټه_FTask)) {
            itemp->جمع_کړه_Stmtsp(stmtp);
        } وإلا إذا (AstLoop* ثابت itemp = VN_CAST(nodep, Loop)) {
            itemp->جمع_کړه_Stmtsp(stmtp);
        } وإلا إذا (AstIf* ثابت itemp = VN_CAST(nodep, If)) {
            إذا (m_then) {
                itemp->جمع_کړه_Thensp(stmtp);
            } وإلا {
                itemp->جمع_کړه_Elsesp(stmtp);
            }
        } وإلا إذا (AstBegin* ثابت itemp = VN_CAST(nodep, Begin)) {
            itemp->جمع_کړه_Stmtsp(stmtp);
        } وإلا {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    فراغ iterateProcedure(Ast_غوټه* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_بیان_Stmtsp);
        VL_RESTORER(m_دننه_Toggle_Off);
        m_بیان_Stmtsp = nodep;
        m_دننه_Toggle_Off = ریښتیا;
        createHandle(nodep);
        iterateChildren(nodep);
        إذا (m_state.کرښه_Coverage_On(nodep)) {
            کرښه_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", کرښې_Cov(m_state, nodep), 0};
            m_modp->جمع_کړه_Stmtsp(declp);
            Ast_غوټه* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_کرښه(nodep, "block"));
            ورننباسه_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    فراغ لیدنه(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_modp && !m_دننه_Toggle_Off && !m_state.m_دننه_Mod_Off && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ثابت حرف* ثابت disablep = varIgnoreToggle(nodep);
            إذا (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } وإلا {
                UINFO(4, "    Toggle: " << nodep);
                // There's several overall ways to approach this
                //    Treat like tracing, where a end-of-timestamp action sees all changes
                //      Works ok, but would be quite slow as need to reform
                //      vectors before the calls
                //    Convert to "always @ (posedge signal[#]) coverinc"
                //      Would mark many signals as clocks, precluding many later optimizations
                //    Convert to "if (x & !lastx) CoverInc"
                //      OK, but we couldn't later detect them to schedule where the IFs get called
                //    Convert to "AstCoverInc(CoverInc...)"
                //      We'll do this, and make the if(...) coverinc later.

                // Add signal to hold the old value
                ثابت string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_کرښه* ثابت fl_nowarn = جديد File_کرښه{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3_Error_کوډ::UNUSEDSIGNAL, ریښتیا);
                AstVar* ثابت chgVarp
                    = جديد AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->جمع_کړه_Stmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, جديد AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 جديد AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    فراغ toggleVarBottom(ثابت ToggleEnt& above, ثابت AstVar* varp, ثابت VNumRange& range) {
        ثابت std::string hier_مختاړی
            = (m_beginHier != "") ? Ast_غوټه::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ثابت declp
            = جديد AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hier_مختاړی + varp->name() + above.m_comment, range};
        m_modp->جمع_کړه_Stmtsp(declp);
        AstCoverToggle* ثابت newp = جديد AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_ونه(خطأ), above.m_chgRefp->clone_ونه(خطأ)};
        m_modp->جمع_کړه_Stmtsp(newp);
    }

    فراغ toggleVarRecurse(ثابت Ast_غوټه_DType* ثابت dtypep, ثابت صحيح ژوروالی,  // per-iteration
                          ثابت ToggleEnt& above, ثابت AstVar* ثابت varp) {  // Constant
        إذا (ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            لكل (ثابت صحيح شاخص_docs : adtypep->declRange().seq_Lo_منزل_Hi()) {
                ثابت صحيح شاخص_کوډ = شاخص_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_منزل_Str(شاخص_docs) + "]",
                                 جديد AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_ونه(خطأ), شاخص_کوډ},
                                 جديد AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_ونه(خطأ), شاخص_کوډ}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), ژوروالی + 1, newent, varp);
                newent.cleanup();
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(dtypep, PackArrayDType)) {
            لكل (ثابت صحيح شاخص_docs : adtypep->declRange().seq_Lo_منزل_Hi()) {
                ثابت Ast_غوټه_DType* ثابت subtypep = adtypep->subDTypep()->skipRefp();
                ثابت صحيح شاخص_کوډ = شاخص_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_منزل_Str(شاخص_docs) + "]",
                                 جديد AstSel{varp->fileline(), above.m_varRefp->clone_ونه(خطأ),
                                            شاخص_کوډ * subtypep->پلنوالی(), subtypep->پلنوالی()},
                                 جديد AstSel{varp->fileline(), above.m_chgRefp->clone_ونه(خطأ),
                                            شاخص_کوډ * subtypep->پلنوالی(), subtypep->پلنوالی()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), ژوروالی + 1, newent, varp);
                newent.cleanup();
            }
        } وإلا إذا (ثابت AstStructDType* ثابت adtypep = VN_CAST(dtypep, StructDType)) {
            إذا (adtypep->packed()) {
                لكل (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت Ast_غوټه_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    ثابت صحيح شاخص_کوډ = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        جديد AstSel{varp->fileline(), above.m_varRefp->clone_ونه(خطأ), شاخص_کوډ,
                                   subtypep->پلنوالی()},
                        جديد AstSel{varp->fileline(), above.m_chgRefp->clone_ونه(خطأ), شاخص_کوډ,
                                   subtypep->پلنوالی()}};
                    toggleVarRecurse(subtypep, ژوروالی + 1, newent, varp);
                    newent.cleanup();
                }
            } وإلا {
                لكل (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_غوټه_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    Ast_غوټه_بیان* ثابت varRefp = جديد AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ونه(خطأ), itemp->name()};
                    Ast_غوټه_بیان* ثابت chgRefp = جديد AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ونه(خطأ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, ژوروالی + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وإلا إذا (ثابت AstUnionDType* ثابت adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            إذا (ثابت AstMemberDType* ثابت itemp = adtypep->membersp()) {
                Ast_غوټه_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                إذا (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_ونه(خطأ),
                                     above.m_chgRefp->clone_ونه(خطأ)};
                    toggleVarRecurse(subtypep, ژوروالی + 1, newent, varp);
                    newent.cleanup();
                } وإلا {
                    Ast_غوټه_بیان* ثابت varRefp = جديد AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ونه(خطأ), itemp->name()};
                    Ast_غوټه_بیان* ثابت chgRefp = جديد AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ونه(خطأ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, ژوروالی + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وإلا إذا (VN_IS(dtypep, کتار_DType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } وإلا {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    منطقي include_Cond_منزل_Branch_Recursive(ثابت Ast_غوټه* ثابت nodep) {
        ثابت Ast_غوټه* ثابت backp = nodep->backp();
        إذا (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            أرجع include_Cond_منزل_Branch_Recursive(backp);
        } وإلا إذا (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            أرجع include_Cond_منزل_Branch_Recursive(backp);
        } وإلا إذا (VN_IS(backp, غوټه_ټاکل) && VN_AS(backp, غوټه_ټاکل)->rhsp() == nodep) {
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    // VISITORS - LINE COVERAGE
    فراغ لیدنه(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        إذا (m_seeking == NONE) coverExprs(nodep->condp());

        إذا (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && include_Cond_منزل_Branch_Recursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ثابت CheckState وروستی_State = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            کرښه_Track(nodep);
            Ast_غوټه_بیان* ثابت thenp = nodep->thenp()->unlink_Fr_شا();
            AstCoverOtherDecl* ثابت thenDeclp
                = جديد AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", کرښې_Cov(m_state, nodep), 0};
            m_modp->جمع_کړه_Stmtsp(thenDeclp);
            nodep->thenp(جديد Ast_بیان_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_کرښه(nodep, "cond_then")),
                thenp});
            m_state = وروستی_State;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_غوټه_بیان* ثابت elsep = nodep->elsep()->unlink_Fr_شا();
            AstCoverOtherDecl* ثابت elseDeclp
                = جديد AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", کرښې_Cov(m_state, nodep), 1};
            m_modp->جمع_کړه_Stmtsp(elseDeclp);
            nodep->elsep(جديد Ast_بیان_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_کرښه(nodep, "cond_else")),
                elsep});

            m_state = وروستی_State;
        } وإلا {
            کرښه_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    فراغ لیدنه(AstIf* nodep) override {
        إذا (nodep->user2()) أرجع;

        UINFO(4, " IF: " << nodep);
        إذا (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ثابت منطقي elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            إذا (elsif) VN_AS(nodep->elsesp(), If)->user1(ریښتیا);
            ثابت منطقي لومړی_elsif = !nodep->user1() && elsif;
            ثابت منطقي cont_elsif = nodep->user1() && elsif;
            ثابت منطقي final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ثابت CheckState وروستی_State = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_بیان_Stmtsp);
                VL_RESTORER(m_then);
                m_بیان_Stmtsp = nodep;
                m_then = ریښتیا;
                createHandle(nodep);
                iterate_And_بل_Null(nodep->thensp());
                کرښه_Track(nodep);
                ifState = m_state;
            }
            m_state = وروستی_State;
            {
                VL_RESTORER(m_بیان_Stmtsp);
                VL_RESTORER(m_then);
                m_بیان_Stmtsp = nodep;
                m_then = خطأ;
                createHandle(nodep);
                iterate_And_بل_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = وروستی_State;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            إذا (!(لومړی_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ثابت ifDeclp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", کرښې_Cov(ifState, nodep), 0};
                m_modp->جمع_کړه_Stmtsp(ifDeclp);
                nodep->جمع_کړه_Thensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_کرښه(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ثابت elseDeclp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", کرښې_Cov(elseState, nodep), 1};
                m_modp->جمع_کړه_Stmtsp(elseDeclp);
                nodep->جمع_کړه_Elsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_کرښه(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            وإلا إذا (لومړی_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                إذا (ifState.کرښه_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ثابت elsifDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        کرښې_Cov(ifState, nodep), 0};
                    m_modp->جمع_کړه_Stmtsp(elsifDeclp);

                    nodep->جمع_کړه_Thensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_کرښه(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } وإلا {
                // Cover as separate blocks (not a branch as is not two-legged)
                إذا (ifState.کرښه_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ثابت ifDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        کرښې_Cov(ifState, nodep), 0};
                    m_modp->جمع_کړه_Stmtsp(ifDeclp);
                    nodep->جمع_کړه_Thensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_کرښه(nodep, "if")));
                }
                إذا (elseState.کرښه_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ثابت elseDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        کرښې_Cov(elseState, nodep), 1};
                    m_modp->جمع_کړه_Stmtsp(elseDeclp);

                    nodep->جمع_کړه_Elsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_کرښه(nodep, "else")));
                }
            }
            m_state = وروستی_State;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = ریښتیا;
        iterate_And_بل_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    فراغ لیدنه(Ast_Case_توکی* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        إذا (m_state.کرښه_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_بل_Null(nodep->stmtsp());
            إذا (m_state.کرښه_Coverage_On(nodep)) {  // if the case body didn't disable it
                کرښه_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ثابت declp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", کرښې_Cov(m_state, nodep), 0};
                m_modp->جمع_کړه_Stmtsp(declp);
                nodep->جمع_کړه_Stmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_کرښه(nodep, "case")));
            }
        }
    }
    فراغ لیدنه(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = ریښتیا;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        إذا (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            کرښه_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", کرښې_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->جمع_کړه_Stmtsp(declp);
            nodep->جمع_کړه_Coverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    فراغ لیدنه(AstPropSpec* nodep) override {
        VL_RESTORER(m_بیان_Stmtsp);
        m_بیان_Stmtsp = تش_اشاره;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = خطأ;
    }
    فراغ لیدنه(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::COVERAGE_بلاک_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = خطأ;
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا {
            إذا (m_state.m_on) iterateChildren(nodep);
            کرښه_Track(nodep);
        }
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        إذا (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        کرښه_Track(nodep);
    }

    فراغ لیدنه(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_دننه_Toggle_Off);
        VL_RESTORER(m_بیان_Stmtsp);
        m_بیان_Stmtsp = nodep;
        m_دننه_Toggle_Off = ریښتیا;
        إذا (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        کرښه_Track(nodep);
    }

    فراغ abort_بیان_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        إذا (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    منطقي checkMaxExprs(اندازه_t additional = 0) {
        إذا (m_seeking != ABORTED
            && static_cast<صحيح>(m_exprs.اندازه() + additional) <= v3Global.opt.coverage_بیان_Max())
            أرجع خطأ;
        abort_بیان_Coverage();
        أرجع ریښتیا;
    }

    فراغ جمع_کړه_بیان_Cover_Inc(Ast_غوټه_بیان* nodep, صحيح پیل = 0) {
        File_کرښه* ثابت fl = nodep->fileline();
        صحيح شمېر = پیل;
        لكل (Cover_بیان& بیان : m_exprs) {
            ثابت string name = "expr_" + std::منزل_string(شمېر);
            string comment = "(";
            منطقي لومړی = ریښتیا;
            Ast_غوټه_بیان* condp = تش_اشاره;
            لكل (Cover_غړی& غړی : بیان) {
                comment += (لومړی ? "" : " && ") + غړی.m_emitV
                           + "==" + (غړی.m_objective ? "1" : "0");
                Ast_غوټه_بیان* covExprp = تش_اشاره;
                إذا (VN_IS(غړی.m_exprp, FuncRef) || غړی.m_exprp->isSystemFunc()) {
                    Ast_غوټه_بیان* ثابت frefp = غړی.m_exprp;
                    Ast_غوټه_DType* ثابت dtypep = frefp->dtypep();
                    ثابت خپلکاری pair = m_funcTemps.emplace(frefp, تش_اشاره);
                    AstVar* varp = pair.لومړی->دوهم;
                    إذا (pair.دوهم) {
                        varp = جديد AstVar{fl, VVarType::MODULETEMP, m_بیان_لنډمهاله_نومونه.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.لومړی->دوهم = varp;
                        إذا (m_ftaskp) {
                            varp->funcLocal(ریښتیا);
                            m_ftaskp->prependStmtsp(varp);
                        } وإلا {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlink_Fr_شا(&relinkHandle);
                        relinkHandle.relink(جديد Ast_بیان_Stmt{
                            fl, جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            جديد AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = جديد AstVarRef{fl, varp, VAccess::READ};
                } وإلا {
                    covExprp = غړی.m_exprp->clone_ونه(خطأ);
                }
                Ast_غوټه_بیان* ثابت termp
                    = غړی.m_objective ? covExprp : جديد AstLogNot{fl, covExprp};
                إذا (condp) {
                    condp = جديد AstLogAnd{fl, condp, termp};
                } وإلا {
                    condp = termp;
                }
                لومړی = خطأ;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ثابت declp = جديد AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->جمع_کړه_Stmtsp(declp);
            Ast_غوټه* ثابت newp = newCoverInc(fl, declp, trace_Name_For_کرښه(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ثابت ifp = جديد AstIf{fl, condp, newp, تش_اشاره};
            ifp->user2(ریښتیا);
            ورننباسه_Proc_Statement(m_بیان_Stmtsp, ifp);
            ++شمېر;
        }
    }

    فراغ coverExprs(Ast_غوټه_بیان* nodep) {
        إذا (!m_state.بیان_Coverage_On(nodep) || nodep->dtypep()->پلنوالی() != 1 || !m_بیان_Stmtsp) {
            أرجع;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.تشه(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = خطأ;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.بدلون(falseExprs);

        m_objective = ریښتیا;
        iterate(nodep);
        إذا (checkMaxExprs(falseExprs.اندازه())) أرجع;
        إذا (m_seeking == ABORTED) أرجع;

        جمع_کړه_بیان_Cover_Inc(nodep);
        ثابت صحيح پیل = m_exprs.اندازه();
        m_objective = خطأ;
        m_exprs.بدلون(falseExprs);
        جمع_کړه_بیان_Cover_Inc(nodep, پیل);
    }

    فراغ بیان_Either(Ast_غوټه_Biop* nodep, منطقي overrideObjective = خطأ, منطقي lObjective = خطأ,
                    منطقي rObjective = خطأ) {
        VL_RESTORER(m_objective);
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp();

        إذا (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs lhsExprs;
        m_exprs.بدلون(lhsExprs);
        إذا (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.پای_ټکی(), lhsExprs);
        checkMaxExprs();
    }

    فراغ بیان_Both(Ast_غوټه_Biop* nodep, منطقي overrideObjective = خطأ, منطقي lObjective = خطأ,
                  منطقي rObjective = خطأ) {
        VL_RESTORER(m_objective);
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp();

        إذا (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs lhsExprs;
        m_exprs.بدلون(lhsExprs);
        إذا (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs rhsExprs;
        m_exprs.بدلون(rhsExprs);

        لكل (ثابت Cover_بیان& l : lhsExprs) {
            لكل (ثابت Cover_بیان& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::سېټ<AstVar*>, 2> varps;
                std::array<std::سېټ<std::string>, 2> strs;

                UASSERT_OBJ(!l.تشه() && !r.تشه(), nodep, "Empty coverage expression branch");
                Cover_بیان بیان;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                لكل (ثابت Cover_غړی& غړی : l) {
                    إذا (ثابت AstVarRef* ثابت refp = VN_CAST(غړی.m_exprp, VarRef)) {
                        varps[غړی.m_objective].ورننباسه(refp->varp());
                    } وإلا {
                        strs[غړی.m_objective].ورننباسه(غړی.m_emitV);
                    }
                    بیان.ټېل_شا(غړی);
                }
                منطقي impossible = خطأ;
                لكل (ثابت Cover_غړی& غړی : r) {
                    منطقي redundant = خطأ;
                    إذا (ثابت Ast_غوټه_Var_Ref* ثابت refp = VN_CAST(غړی.m_exprp, غوټه_Var_Ref)) {
                        إذا (varps[غړی.m_objective].پیدا_کړه(refp->varp())
                            != varps[غړی.m_objective].پای_ټکی())
                            redundant = ریښتیا;
                        إذا (varps[!غړی.m_objective].پیدا_کړه(refp->varp())
                            != varps[!غړی.m_objective].پای_ټکی())
                            impossible = ریښتیا;
                    } وإلا {
                        إذا (strs[غړی.m_objective].پیدا_کړه(غړی.m_emitV)
                            != strs[غړی.m_objective].پای_ټکی())
                            redundant = ریښتیا;
                        إذا (strs[!غړی.m_objective].پیدا_کړه(غړی.m_emitV)
                            != strs[!غړی.m_objective].پای_ټکی())
                            impossible = ریښتیا;
                    }
                    إذا (!redundant) بیان.ټېل_شا(غړی);
                }
                إذا (!impossible) m_exprs.ټېل_شا(std::move(بیان));
                إذا (checkMaxExprs()) أرجع;
            }
        }
    }

    فراغ or_بیان(Ast_غوټه_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            بیان_Either(nodep);
        } وإلا {
            بیان_Both(nodep);
        }
        کرښه_Track(nodep);
    }
    فراغ لیدنه(AstLogOr* nodep) override { or_بیان(nodep); }
    فراغ لیدنه(AstOr* nodep) override { or_بیان(nodep); }

    فراغ and_بیان(Ast_غوټه_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            بیان_Both(nodep);
        } وإلا {
            بیان_Either(nodep);
        }
        کرښه_Track(nodep);
    }
    فراغ لیدنه(AstLogAnd* nodep) override { and_بیان(nodep); }
    فراغ لیدنه(AstAnd* nodep) override { and_بیان(nodep); }

    فراغ xor_بیان(Ast_غوټه_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            لكل (ثابت منطقي lObjective : {خطأ, ریښتیا}) {
                CoverExprs prevExprs;
                m_exprs.بدلون(prevExprs);
                ثابت منطقي rObjective = lObjective ^ m_objective;
                بیان_Both(nodep, ریښتیا, lObjective, rObjective);
                m_exprs.splice(m_exprs.پای_ټکی(), prevExprs);
                إذا (checkMaxExprs()) اكسر;
            }
        }
        کرښه_Track(nodep);
    }
    فراغ لیدنه(AstXor* nodep) override { xor_بیان(nodep); }

    فراغ بیان_Not(Ast_غوټه_بیان* nodep) {
        VL_RESTORER(m_objective);
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            m_objective = !m_objective;
            iterateChildren(nodep);
            کرښه_Track(nodep);
        }
    }
    فراغ لیدنه(AstNot* nodep) override { بیان_Not(nodep); }
    فراغ لیدنه(AstLogNot* nodep) override { بیان_Not(nodep); }

    کينډۍ <ډول_نوم T_Oper>
    فراغ بیان_Reduce(Ast_غوټه_Uniop* nodep) {
        إذا (m_seeking != ABORTED) {
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp();
            ثابت صحيح پلنوالی = lhsp->dtypep()->پلنوالی();
            ثابت اندازه_t expected = std::is_same<T_Oper, AstXor>::ارزښت ? 0x1 << پلنوالی : پلنوالی + 1;
            إذا (checkMaxExprs(expected)) أرجع;
            Ast_غوټه_بیان* unrolledp = جديد AstSel{
                fl, lhsp->clone_ونه(خطأ), جديد AstConst{fl, static_cast<uint32_t>(پلنوالی - 1)}, 1};
            لكل (صحيح بټ_2 = پلنوالی - 2; بټ_2 >= 0; بټ_2--) {
                AstSel* ثابت selp = جديد AstSel{fl, lhsp->clone_ونه(خطأ),
                                                جديد AstConst{fl, static_cast<uint32_t>(بټ_2)}, 1};
                unrolledp = جديد T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ټېل_Deletep(unrolledp), unrolledp);
        } وإلا {
            iterateChildren(nodep);
            کرښه_Track(nodep);
        }
    }
    فراغ لیدنه(AstRedOr* nodep) override { بیان_Reduce<AstOr>(nodep); }
    فراغ لیدنه(AstRedAnd* nodep) override { بیان_Reduce<AstAnd>(nodep); }
    فراغ لیدنه(AstRedXor* nodep) override { بیان_Reduce<AstXor>(nodep); }

    فراغ لیدنه(AstLogIf* nodep) override {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            بیان_Either(nodep, ریښتیا, خطأ, ریښتیا);
        } وإلا {
            بیان_Both(nodep, ریښتیا, ریښتیا, خطأ);
        }
        کرښه_Track(nodep);
    }

    فراغ لیدنه(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            m_objective = !m_objective;
            xor_بیان(nodep);
            کرښه_Track(nodep);
        }
    }

    فراغ لیدنه(AstFuncRef* nodep) override {
        إذا (nodep->taskp()->lifetime().isAutomatic()) {
            لیدنه(static_cast<Ast_غوټه_بیان*>(nodep));
        } وإلا {
            بیان_Unsupported(nodep, "non-automatic function");
        }
    }

    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {
        إذا (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } وإلا {
            بیان_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            إذا (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_ونه(nodep, emitV);
                // Add new expression with a single term
                Cover_بیان بیان;
                بیان.emplace_شا(nodep, m_objective, emitV.str());
                m_exprs.ټېل_شا(std::move(بیان));
                checkMaxExprs();
            } وإلا {
                بیان_Unsupported(nodep, "not coverage eligible");
            }
        }
        کرښه_Track(nodep);
    }

    // VISITORS - BOTH
    فراغ لیدنه(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_دننه_Toggle_Off);
        m_state.m_on = خطأ;
        m_دننه_Toggle_Off = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_دننه_Toggle_Off);
        m_state.m_on = خطأ;
        m_دننه_Toggle_Off = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        iterateChildren(nodep);
        کرښه_Track(nodep);
    }

    فراغ بیان_Unsupported(Ast_غوټه* nodep, ثابت string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ثابت منطقي wasSeeking = m_seeking == SEEKING;
        ثابت Objective oldSeeking = m_seeking;
        إذا (wasSeeking) abort_بیان_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        کرښه_Track(nodep);
        إذا (!wasSeeking) m_seeking = oldSeeking;
    }

عام:
    // CONSTRUCTORS
    څرګند CoverageVisitor(AstNetlist* rootp)
        : m_بیان_لنډمهاله_نومونه{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = افتراضي;
};

//######################################################################
// Coverage class functions

فراغ V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("coverage", 0, dump_ونه_Either_Level() >= 3);
}
