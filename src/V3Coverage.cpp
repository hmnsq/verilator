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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Coverage.h"

#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <list>
#అన్తర్భావ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి సమాసం_Coverage_Eligible_Visitor final : బహిరంగ VNVisitorConst {
    // STATE
    తార్కిక m_eligible = నిజం;

    స్థిర తార్కిక elemDTypeEligible(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtp = dtypep->virtRefDTypep()) {
            ఒకవేళ (!elemDTypeEligible(dtp)) తిరిగి అబద్ధం;
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtp = dtypep->virtRefDType2p()) {
            ఒకవేళ (!elemDTypeEligible(dtp)) తిరిగి అబద్ధం;
        }
        తిరిగి !VN_IS(dtypep, ClassRefDType);
    }

    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        ఒకవేళ (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } లేకపోతే {
            m_eligible = అబద్ధం;
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (!nodep->is_సమాసం_Coverage_Eligible()) {
            m_eligible = అబద్ధం;
        } లేకపోతే {
            iterateChildrenConst(nodep);
        }
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన సమాసం_Coverage_Eligible_Visitor(Ast_కణుపు* nodep) { iterateConst(nodep); }
    ~సమాసం_Coverage_Eligible_Visitor() override = అప్రమేయ;

    తార్కిక eligible() స్థిరరాశి { తిరిగి m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

తరగతి CoverageVisitor final : బహిరంగ VNVisitor {
    // TYPES
    ఉపయోగించి Lineno_సమితి = std::సమితి<పూర్ణాంకం>;

    నిర్మాణం Cover_పదం final {
        Ast_కణుపు_సమాసం* m_exprp;  // Expression branch term
        తార్కిక m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        Cover_పదం(Ast_కణుపు_సమాసం* exprp, తార్కిక objective, స్థిరరాశి string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    ఉపయోగించి Cover_సమాసం = std::deque<Cover_పదం>;
    ఉపయోగించి CoverExprs = std::list<Cover_సమాసం>;

    నిర్మాణం ToggleEnt final {
        స్థిరరాశి string m_comment;  // Comment for coverage dump
        Ast_కణుపు_సమాసం* m_varRefp;  // How to get to this element
        Ast_కణుపు_సమాసం* m_chgRefp;  // How to get to this element
        ToggleEnt(స్థిరరాశి string& comment, Ast_కణుపు_సమాసం* vp, Ast_కణుపు_సమాసం* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = అప్రమేయ;
        శూన్య cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_వృక్షం(), m_varRefp = శూన్యసూచిక);
            VL_DO_CLEAR(m_chgRefp->delete_వృక్షం(), m_chgRefp = శూన్యసూచిక);
        }
    };

    నిర్మాణం CheckState final {  // State save-restored on each new coverage scope/block
        తార్కిక m_on = అబద్ధం;  // Should this block get covered?
        తార్కిక m_లోపల_Mod_Off = అబద్ధం;  // In module with no coverage
        పూర్ణాంకం m_handle = 0;  // Opaque handle for index into line tracking
        స్థిరరాశి Ast_కణుపు* m_nodep = శూన్యసూచిక;  // Node establishing this state
        CheckState() = అప్రమేయ;
        తార్కిక coverageOn(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి {
            తిరిగి m_on && !m_లోపల_Mod_Off && nodep->fileline()->coverageOn();
        }
        తార్కిక పంక్తి_Coverage_On(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి {
            తిరిగి coverageOn(nodep) && v3Global.opt.coverage_పంక్తి();
        }
        తార్కిక branchCoverageOn(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి {
            తిరిగి coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        తార్కిక సమాసం_Coverage_On(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి {
            తిరిగి coverageOn(nodep) && v3Global.opt.coverage_సమాసం();
        }
    };

    గణన Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    V3_Unique_పేర్లు m_సమాసం_తాత్కాలిక_పేర్లు;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_కణుపు_సమాసం*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    పూర్ణాంకం m_తదుపరి_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module to add statement to
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    Ast_కణుపు* m_సమాసం_Stmtsp = శూన్యసూచిక;  // Node to add expr coverage to
    తార్కిక m_then = అబద్ధం;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    తార్కిక m_objective = అబద్ధం;  // Expression objective
    తార్కిక m_ifCond = అబద్ధం;  // Visiting if condition
    తార్కిక m_లోపల_Toggle_Off = అబద్ధం;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<పూర్ణాంకం, Lineno_సమితి> m_handle_పంక్తులు;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    స్థిరరాశి అక్షరం* varIgnoreToggle(స్థిరరాశి AstVar* nodep) {
        స్థిరరాశి తార్కిక cover = nodep->isIO() || (nodep->isSignal() && nodep->is_బిట్టు_Logic());
        ఒకవేళ (!cover) తిరిగి "Not relevant signal";
        ఒకవేళ (nodep->isConst()) తిరిగి "Signal is constant";
        ఒకవేళ (nodep->isDouble()) తిరిగి "Signal is double";
        ఒకవేళ (nodep->isString()) తిరిగి "Signal is string";
        ఒకవేళ (!v3Global.opt.coverageUnderscore()) {
            స్థిరరాశి string prettyName = nodep->prettyName();
            ఒకవేళ (prettyName[0] == '_') తిరిగి "Leading underscore";
            ఒకవేళ (prettyName.కనుగొను("._") != string::npos) తిరిగి "Inlined leading underscore";
        }
        ఒకవేళ ((nodep->వెడల్పు() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverage_Max_వెడల్పు())) {
            తిరిగి "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        తిరిగి శూన్యసూచిక;
    }

    AstCoverInc* newCoverInc(File_పంక్తి* fl, Ast_కణుపు_Cover_Decl* స్థిరరాశి declp,
                             స్థిరరాశి string& trace_var_name) {
        AstCoverInc* స్థిరరాశి incp = కొత్త AstCoverInc{fl, declp};
        ఒకవేళ (!trace_var_name.ఖాళీలు()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_పంక్తి* స్థిరరాశి fl_nowarn = కొత్త File_పంక్తి{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3_Error_సంకేతం::UNUSEDSIGNAL, నిజం);
            AstVar* స్థిరరాశి varp = కొత్త AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->కనుగొను_UInt32_DType()};
            varp->సమితి_Ignore_Sched_Write();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(నిజం);
            m_modp->కూడు_Stmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            Ast_కేటాయించు* స్థిరరాశి assp = కొత్త Ast_కేటాయించు{
                incp->fileline(), కొత్త AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                కొత్త Ast_కూడు{incp->fileline(), కొత్త AstVarRef{incp->fileline(), varp, VAccess::READ},
                           కొత్త AstConst{incp->fileline(), AstConst::Widthed_విలువ{}, 32, 1}}};
            Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(incp, assp);
        }
        తిరిగి incp;
    }
    string trace_Name_For_పంక్తి(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvt_గమ్యం_Str(nodep->fileline()->lineno()) + "_" + type;
        ఒకవేళ (స్థిరరాశి uint32_t suffix = m_varnames[name]++) name += "_" + cvt_గమ్యం_Str(suffix);
        తిరిగి name;
    }

    // Line tracking
    శూన్య createHandle(స్థిరరాశి Ast_కణుపు* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = నిజం;
        m_state.m_handle = ++m_తదుపరి_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    శూన్య పంక్తి_Track(స్థిరరాశి Ast_కణుపు* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        ఒకవేళ ((m_state.పంక్తి_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            కొరకు (పూర్ణాంకం lineno = nodep->fileline()->మొదటి_Lineno();
                 lineno <= nodep->fileline()->చివరి_Lineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_పంక్తులు[m_state.m_handle].చొప్పించు(lineno);
            }
        }
    }
    స్థిర string పంక్తులు_మొదటి_చివరి(స్థిరరాశి పూర్ణాంకం మొదటి, స్థిరరాశి పూర్ణాంకం చివరి) {
        ఒకవేళ (మొదటి && మొదటి == చివరి) {
            తిరిగి cvt_గమ్యం_Str(మొదటి);
        } లేకపోతే ఒకవేళ (మొదటి && చివరి) {
            తిరిగి cvt_గమ్యం_Str(మొదటి) + "-" + cvt_గమ్యం_Str(చివరి);
        } లేకపోతే {
            తిరిగి "";
        }
    }
    string పంక్తులు_Cov(స్థిరరాశి CheckState& state, స్థిరరాశి Ast_కణుపు* nodep) {
        // Return comma separated list of ranged numbers
        string బహిర్గతం;
        స్థిరరాశి Lineno_సమితి& పంక్తులు = m_handle_పంక్తులు[state.m_handle];
        పూర్ణాంకం మొదటి = 0;
        పూర్ణాంకం చివరి = 0;
        కొరకు (స్థిరరాశి పూర్ణాంకం linen : పంక్తులు) {
            ఒకవేళ (!మొదటి) {
                మొదటి = చివరి = linen;
            } లేకపోతే ఒకవేళ (linen == చివరి + 1) {
                ++చివరి;
            } లేకపోతే {
                ఒకవేళ (!బహిర్గతం.ఖాళీలు()) బహిర్గతం += ",";
                బహిర్గతం += పంక్తులు_మొదటి_చివరి(మొదటి, చివరి);
                మొదటి = చివరి = linen;
            }
        }
        ఒకవేళ (మొదటి) {
            ఒకవేళ (!బహిర్గతం.ఖాళీలు()) బహిర్గతం += ",";
            బహిర్గతం += పంక్తులు_మొదటి_చివరి(మొదటి, చివరి);
        }
        UINFO(9, "lines out " << బహిర్గతం << " for h" << state.m_handle << " " << nodep);
        తిరిగి బహిర్గతం;
    }

    // VISITORS - BOTH
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_సమాసం_తాత్కాలిక_పేర్లు);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_లోపల_Mod_Off = nodep->is_శిఖరం();  // Already made top shell, no coverage for it
        ఒకవేళ (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_పంక్తులు.clear();
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_సమాసం_తాత్కాలిక_పేర్లు);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_లోపల_Mod_Off = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (nodep->keyword() == VAlwaysKwd::CONT_కేటాయించు) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_సమాసం_Stmtsp);
            VL_RESTORER(m_లోపల_Toggle_Off);
            m_సమాసం_Stmtsp = nodep;
            m_లోపల_Toggle_Off = నిజం;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            తిరిగి;
        }
        iterateProcedure(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override { iterateProcedure(nodep); }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_సమాసం_Stmtsp);
        VL_RESTORER(m_లోపల_Toggle_Off);
        m_సమాసం_Stmtsp = nodep;
        m_లోపల_Toggle_Off = నిజం;
        createHandle(nodep);
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        ఒకవేళ (m_state.పంక్తి_Coverage_On(nodep)) {
            AstCoverOtherDecl* స్థిరరాశి declp
                = కొత్త AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", పంక్తులు_Cov(m_state, nodep), 0};
            m_modp->కూడు_Stmtsp(declp);
            Ast_కణుపు* స్థిరరాశి newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_పంక్తి(nodep, "block"));
            చొప్పించు_Proc_Statement(nodep, newp);
        }
    }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        ఒకవేళ (nodep->user2_సమితి_Once()) తిరిగి;
        పంక్తి_Track(nodep);
        ఒకవేళ (m_state.పంక్తి_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* స్థిరరాశి declp
                = కొత్త AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", పంక్తులు_Cov(m_state, nodep), 0};
            m_modp->కూడు_Stmtsp(declp);
            Ast_కణుపు* స్థిరరాశి newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_పంక్తి(nodep, "block"));
            nodep->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_సమాసం_తాత్కాలిక_పేర్లు);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        ఒకవేళ (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    శూన్య చొప్పించు_Proc_Statement(Ast_కణుపు* nodep, Ast_కణుపు* stmtp) {
        ఒకవేళ (Ast_కణుపు_Procedure* స్థిరరాశి itemp = VN_CAST(nodep, కణుపు_Procedure)) {
            itemp->కూడు_Stmtsp(stmtp);
        } లేకపోతే ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి itemp = VN_CAST(nodep, కణుపు_FTask)) {
            itemp->కూడు_Stmtsp(stmtp);
        } లేకపోతే ఒకవేళ (AstLoop* స్థిరరాశి itemp = VN_CAST(nodep, Loop)) {
            itemp->కూడు_Stmtsp(stmtp);
        } లేకపోతే ఒకవేళ (AstIf* స్థిరరాశి itemp = VN_CAST(nodep, If)) {
            ఒకవేళ (m_then) {
                itemp->కూడు_Thensp(stmtp);
            } లేకపోతే {
                itemp->కూడు_Elsesp(stmtp);
            }
        } లేకపోతే ఒకవేళ (AstBegin* స్థిరరాశి itemp = VN_CAST(nodep, Begin)) {
            itemp->కూడు_Stmtsp(stmtp);
        } లేకపోతే {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    శూన్య iterateProcedure(Ast_కణుపు* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_సమాసం_Stmtsp);
        VL_RESTORER(m_లోపల_Toggle_Off);
        m_సమాసం_Stmtsp = nodep;
        m_లోపల_Toggle_Off = నిజం;
        createHandle(nodep);
        iterateChildren(nodep);
        ఒకవేళ (m_state.పంక్తి_Coverage_On(nodep)) {
            పంక్తి_Track(nodep);
            AstCoverOtherDecl* స్థిరరాశి declp
                = కొత్త AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", పంక్తులు_Cov(m_state, nodep), 0};
            m_modp->కూడు_Stmtsp(declp);
            Ast_కణుపు* స్థిరరాశి newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_పంక్తి(nodep, "block"));
            చొప్పించు_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    శూన్య సందర్శించు(AstVar* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_modp && !m_లోపల_Toggle_Off && !m_state.m_లోపల_Mod_Off && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            స్థిరరాశి అక్షరం* స్థిరరాశి disablep = varIgnoreToggle(nodep);
            ఒకవేళ (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } లేకపోతే {
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
                స్థిరరాశి string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_పంక్తి* స్థిరరాశి fl_nowarn = కొత్త File_పంక్తి{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3_Error_సంకేతం::UNUSEDSIGNAL, నిజం);
                AstVar* స్థిరరాశి chgVarp
                    = కొత్త AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->కూడు_Stmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, కొత్త AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 కొత్త AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    శూన్య toggleVarBottom(స్థిరరాశి ToggleEnt& above, స్థిరరాశి AstVar* varp, స్థిరరాశి VNumRange& range) {
        స్థిరరాశి std::string hier_ఉపసర్గ
            = (m_beginHier != "") ? Ast_కణుపు::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* స్థిరరాశి declp
            = కొత్త AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hier_ఉపసర్గ + varp->name() + above.m_comment, range};
        m_modp->కూడు_Stmtsp(declp);
        AstCoverToggle* స్థిరరాశి newp = కొత్త AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_వృక్షం(అబద్ధం), above.m_chgRefp->clone_వృక్షం(అబద్ధం)};
        m_modp->కూడు_Stmtsp(newp);
    }

    శూన్య toggleVarRecurse(స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep, స్థిరరాశి పూర్ణాంకం లోతు,  // per-iteration
                          స్థిరరాశి ToggleEnt& above, స్థిరరాశి AstVar* స్థిరరాశి varp) {  // Constant
        ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            కొరకు (స్థిరరాశి పూర్ణాంకం సూచీ_docs : adtypep->declRange().seq_Lo_గమ్యం_Hi()) {
                స్థిరరాశి పూర్ణాంకం సూచీ_సంకేతం = సూచీ_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_గమ్యం_Str(సూచీ_docs) + "]",
                                 కొత్త AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_వృక్షం(అబద్ధం), సూచీ_సంకేతం},
                                 కొత్త AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_వృక్షం(అబద్ధం), సూచీ_సంకేతం}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), లోతు + 1, newent, varp);
                newent.cleanup();
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstPackArrayDType* స్థిరరాశి adtypep = VN_CAST(dtypep, PackArrayDType)) {
            కొరకు (స్థిరరాశి పూర్ణాంకం సూచీ_docs : adtypep->declRange().seq_Lo_గమ్యం_Hi()) {
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి subtypep = adtypep->subDTypep()->skipRefp();
                స్థిరరాశి పూర్ణాంకం సూచీ_సంకేతం = సూచీ_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_గమ్యం_Str(సూచీ_docs) + "]",
                                 కొత్త AstSel{varp->fileline(), above.m_varRefp->clone_వృక్షం(అబద్ధం),
                                            సూచీ_సంకేతం * subtypep->వెడల్పు(), subtypep->వెడల్పు()},
                                 కొత్త AstSel{varp->fileline(), above.m_chgRefp->clone_వృక్షం(అబద్ధం),
                                            సూచీ_సంకేతం * subtypep->వెడల్పు(), subtypep->వెడల్పు()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), లోతు + 1, newent, varp);
                newent.cleanup();
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstStructDType* స్థిరరాశి adtypep = VN_CAST(dtypep, StructDType)) {
            ఒకవేళ (adtypep->packed()) {
                కొరకు (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి subtypep = itemp->subDTypep()->skipRefp();
                    స్థిరరాశి పూర్ణాంకం సూచీ_సంకేతం = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        కొత్త AstSel{varp->fileline(), above.m_varRefp->clone_వృక్షం(అబద్ధం), సూచీ_సంకేతం,
                                   subtypep->వెడల్పు()},
                        కొత్త AstSel{varp->fileline(), above.m_chgRefp->clone_వృక్షం(అబద్ధం), సూచీ_సంకేతం,
                                   subtypep->వెడల్పు()}};
                    toggleVarRecurse(subtypep, లోతు + 1, newent, varp);
                    newent.cleanup();
                }
            } లేకపోతే {
                కొరకు (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_కణుపు_DType* స్థిరరాశి subtypep = itemp->subDTypep()->skipRefp();
                    Ast_కణుపు_సమాసం* స్థిరరాశి varRefp = కొత్త AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_వృక్షం(అబద్ధం), itemp->name()};
                    Ast_కణుపు_సమాసం* స్థిరరాశి chgRefp = కొత్త AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_వృక్షం(అబద్ధం), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, లోతు + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnionDType* స్థిరరాశి adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            ఒకవేళ (స్థిరరాశి AstMemberDType* స్థిరరాశి itemp = adtypep->membersp()) {
                Ast_కణుపు_DType* స్థిరరాశి subtypep = itemp->subDTypep()->skipRefp();
                ఒకవేళ (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_వృక్షం(అబద్ధం),
                                     above.m_chgRefp->clone_వృక్షం(అబద్ధం)};
                    toggleVarRecurse(subtypep, లోతు + 1, newent, varp);
                    newent.cleanup();
                } లేకపోతే {
                    Ast_కణుపు_సమాసం* స్థిరరాశి varRefp = కొత్త AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_వృక్షం(అబద్ధం), itemp->name()};
                    Ast_కణుపు_సమాసం* స్థిరరాశి chgRefp = కొత్త AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_వృక్షం(అబద్ధం), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, లోతు + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } లేకపోతే ఒకవేళ (VN_IS(dtypep, వరుస_DType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } లేకపోతే {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    తార్కిక include_Cond_గమ్యం_Branch_Recursive(స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep) {
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి backp = nodep->backp();
        ఒకవేళ (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            తిరిగి include_Cond_గమ్యం_Branch_Recursive(backp);
        } లేకపోతే ఒకవేళ (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            తిరిగి include_Cond_గమ్యం_Branch_Recursive(backp);
        } లేకపోతే ఒకవేళ (VN_IS(backp, కణుపు_కేటాయించు) && VN_AS(backp, కణుపు_కేటాయించు)->rhsp() == nodep) {
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // VISITORS - LINE COVERAGE
    శూన్య సందర్శించు(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        ఒకవేళ (m_seeking == NONE) coverExprs(nodep->condp());

        ఒకవేళ (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && include_Cond_గమ్యం_Branch_Recursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            స్థిరరాశి CheckState చివరి_State = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            పంక్తి_Track(nodep);
            Ast_కణుపు_సమాసం* స్థిరరాశి thenp = nodep->thenp()->unlink_Fr_వెనుక();
            AstCoverOtherDecl* స్థిరరాశి thenDeclp
                = కొత్త AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", పంక్తులు_Cov(m_state, nodep), 0};
            m_modp->కూడు_Stmtsp(thenDeclp);
            nodep->thenp(కొత్త Ast_సమాసం_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_పంక్తి(nodep, "cond_then")),
                thenp});
            m_state = చివరి_State;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_కణుపు_సమాసం* స్థిరరాశి elsep = nodep->elsep()->unlink_Fr_వెనుక();
            AstCoverOtherDecl* స్థిరరాశి elseDeclp
                = కొత్త AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", పంక్తులు_Cov(m_state, nodep), 1};
            m_modp->కూడు_Stmtsp(elseDeclp);
            nodep->elsep(కొత్త Ast_సమాసం_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_పంక్తి(nodep, "cond_else")),
                elsep});

            m_state = చివరి_State;
        } లేకపోతే {
            పంక్తి_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    శూన్య సందర్శించు(AstIf* nodep) override {
        ఒకవేళ (nodep->user2()) తిరిగి;

        UINFO(4, " IF: " << nodep);
        ఒకవేళ (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            స్థిరరాశి తార్కిక elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            ఒకవేళ (elsif) VN_AS(nodep->elsesp(), If)->user1(నిజం);
            స్థిరరాశి తార్కిక మొదటి_elsif = !nodep->user1() && elsif;
            స్థిరరాశి తార్కిక cont_elsif = nodep->user1() && elsif;
            స్థిరరాశి తార్కిక final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            స్థిరరాశి CheckState చివరి_State = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_సమాసం_Stmtsp);
                VL_RESTORER(m_then);
                m_సమాసం_Stmtsp = nodep;
                m_then = నిజం;
                createHandle(nodep);
                iterate_And_తదుపరి_Null(nodep->thensp());
                పంక్తి_Track(nodep);
                ifState = m_state;
            }
            m_state = చివరి_State;
            {
                VL_RESTORER(m_సమాసం_Stmtsp);
                VL_RESTORER(m_then);
                m_సమాసం_Stmtsp = nodep;
                m_then = అబద్ధం;
                createHandle(nodep);
                iterate_And_తదుపరి_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = చివరి_State;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            ఒకవేళ (!(మొదటి_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* స్థిరరాశి ifDeclp
                    = కొత్త AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", పంక్తులు_Cov(ifState, nodep), 0};
                m_modp->కూడు_Stmtsp(ifDeclp);
                nodep->కూడు_Thensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_పంక్తి(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* స్థిరరాశి elseDeclp
                    = కొత్త AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", పంక్తులు_Cov(elseState, nodep), 1};
                m_modp->కూడు_Stmtsp(elseDeclp);
                nodep->కూడు_Elsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_పంక్తి(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            లేకపోతే ఒకవేళ (మొదటి_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                ఒకవేళ (ifState.పంక్తి_Coverage_On(nodep)) {
                    AstCoverOtherDecl* స్థిరరాశి elsifDeclp = కొత్త AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        పంక్తులు_Cov(ifState, nodep), 0};
                    m_modp->కూడు_Stmtsp(elsifDeclp);

                    nodep->కూడు_Thensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_పంక్తి(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } లేకపోతే {
                // Cover as separate blocks (not a branch as is not two-legged)
                ఒకవేళ (ifState.పంక్తి_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* స్థిరరాశి ifDeclp = కొత్త AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        పంక్తులు_Cov(ifState, nodep), 0};
                    m_modp->కూడు_Stmtsp(ifDeclp);
                    nodep->కూడు_Thensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_పంక్తి(nodep, "if")));
                }
                ఒకవేళ (elseState.పంక్తి_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* స్థిరరాశి elseDeclp = కొత్త AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        పంక్తులు_Cov(elseState, nodep), 1};
                    m_modp->కూడు_Stmtsp(elseDeclp);

                    nodep->కూడు_Elsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_పంక్తి(nodep, "else")));
                }
            }
            m_state = చివరి_State;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = నిజం;
        iterate_And_తదుపరి_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        ఒకవేళ (m_state.పంక్తి_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_తదుపరి_Null(nodep->stmtsp());
            ఒకవేళ (m_state.పంక్తి_Coverage_On(nodep)) {  // if the case body didn't disable it
                పంక్తి_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* స్థిరరాశి declp
                    = కొత్త AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", పంక్తులు_Cov(m_state, nodep), 0};
                m_modp->కూడు_Stmtsp(declp);
                nodep->కూడు_Stmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_పంక్తి(nodep, "case")));
            }
        }
    }
    శూన్య సందర్శించు(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = నిజం;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        ఒకవేళ (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            పంక్తి_Track(nodep);
            AstCoverOtherDecl* స్థిరరాశి declp
                = కొత్త AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", పంక్తులు_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->కూడు_Stmtsp(declp);
            nodep->కూడు_Coverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    శూన్య సందర్శించు(AstPropSpec* nodep) override {
        VL_RESTORER(m_సమాసం_Stmtsp);
        m_సమాసం_Stmtsp = శూన్యసూచిక;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = అబద్ధం;
    }
    శూన్య సందర్శించు(AstStmtPragma* nodep) override {
        ఒకవేళ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_ఖండం_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = అబద్ధం;
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే {
            ఒకవేళ (m_state.m_on) iterateChildren(nodep);
            పంక్తి_Track(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        ఒకవేళ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        పంక్తి_Track(nodep);
    }

    శూన్య సందర్శించు(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_లోపల_Toggle_Off);
        VL_RESTORER(m_సమాసం_Stmtsp);
        m_సమాసం_Stmtsp = nodep;
        m_లోపల_Toggle_Off = నిజం;
        ఒకవేళ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        పంక్తి_Track(nodep);
    }

    శూన్య abort_సమాసం_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        ఒకవేళ (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    తార్కిక checkMaxExprs(కొలత_t additional = 0) {
        ఒకవేళ (m_seeking != ABORTED
            && static_cast<పూర్ణాంకం>(m_exprs.కొలత() + additional) <= v3Global.opt.coverage_సమాసం_Max())
            తిరిగి అబద్ధం;
        abort_సమాసం_Coverage();
        తిరిగి నిజం;
    }

    శూన్య కూడు_సమాసం_Cover_Inc(Ast_కణుపు_సమాసం* nodep, పూర్ణాంకం ప్రారంభం = 0) {
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        పూర్ణాంకం లెక్క = ప్రారంభం;
        కొరకు (Cover_సమాసం& సమాసం : m_exprs) {
            స్థిరరాశి string name = "expr_" + std::గమ్యం_string(లెక్క);
            string comment = "(";
            తార్కిక మొదటి = నిజం;
            Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;
            కొరకు (Cover_పదం& పదం : సమాసం) {
                comment += (మొదటి ? "" : " && ") + పదం.m_emitV
                           + "==" + (పదం.m_objective ? "1" : "0");
                Ast_కణుపు_సమాసం* covExprp = శూన్యసూచిక;
                ఒకవేళ (VN_IS(పదం.m_exprp, FuncRef) || పదం.m_exprp->isSystemFunc()) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి frefp = పదం.m_exprp;
                    Ast_కణుపు_DType* స్థిరరాశి dtypep = frefp->dtypep();
                    స్థిరరాశి స్వయంచాలక pair = m_funcTemps.emplace(frefp, శూన్యసూచిక);
                    AstVar* varp = pair.మొదటి->రెండవ;
                    ఒకవేళ (pair.రెండవ) {
                        varp = కొత్త AstVar{fl, VVarType::MODULETEMP, m_సమాసం_తాత్కాలిక_పేర్లు.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.మొదటి->రెండవ = varp;
                        ఒకవేళ (m_ftaskp) {
                            varp->funcLocal(నిజం);
                            m_ftaskp->prependStmtsp(varp);
                        } లేకపోతే {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlink_Fr_వెనుక(&relinkHandle);
                        relinkHandle.relink(కొత్త Ast_సమాసం_Stmt{
                            fl, కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            కొత్త AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = కొత్త AstVarRef{fl, varp, VAccess::READ};
                } లేకపోతే {
                    covExprp = పదం.m_exprp->clone_వృక్షం(అబద్ధం);
                }
                Ast_కణుపు_సమాసం* స్థిరరాశి termp
                    = పదం.m_objective ? covExprp : కొత్త AstLogNot{fl, covExprp};
                ఒకవేళ (condp) {
                    condp = కొత్త AstLogAnd{fl, condp, termp};
                } లేకపోతే {
                    condp = termp;
                }
                మొదటి = అబద్ధం;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* స్థిరరాశి declp = కొత్త AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->కూడు_Stmtsp(declp);
            Ast_కణుపు* స్థిరరాశి newp = newCoverInc(fl, declp, trace_Name_For_పంక్తి(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{fl, condp, newp, శూన్యసూచిక};
            ifp->user2(నిజం);
            చొప్పించు_Proc_Statement(m_సమాసం_Stmtsp, ifp);
            ++లెక్క;
        }
    }

    శూన్య coverExprs(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (!m_state.సమాసం_Coverage_On(nodep) || nodep->dtypep()->వెడల్పు() != 1 || !m_సమాసం_Stmtsp) {
            తిరిగి;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.ఖాళీలు(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = అబద్ధం;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.మార్పిడి(falseExprs);

        m_objective = నిజం;
        iterate(nodep);
        ఒకవేళ (checkMaxExprs(falseExprs.కొలత())) తిరిగి;
        ఒకవేళ (m_seeking == ABORTED) తిరిగి;

        కూడు_సమాసం_Cover_Inc(nodep);
        స్థిరరాశి పూర్ణాంకం ప్రారంభం = m_exprs.కొలత();
        m_objective = అబద్ధం;
        m_exprs.మార్పిడి(falseExprs);
        కూడు_సమాసం_Cover_Inc(nodep, ప్రారంభం);
    }

    శూన్య సమాసం_Either(Ast_కణుపు_Biop* nodep, తార్కిక overrideObjective = అబద్ధం, తార్కిక lObjective = అబద్ధం,
                    తార్కిక rObjective = అబద్ధం) {
        VL_RESTORER(m_objective);
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp();

        ఒకవేళ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ఒకవేళ (checkMaxExprs()) తిరిగి;
        CoverExprs lhsExprs;
        m_exprs.మార్పిడి(lhsExprs);
        ఒకవేళ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.చివర(), lhsExprs);
        checkMaxExprs();
    }

    శూన్య సమాసం_Both(Ast_కణుపు_Biop* nodep, తార్కిక overrideObjective = అబద్ధం, తార్కిక lObjective = అబద్ధం,
                  తార్కిక rObjective = అబద్ధం) {
        VL_RESTORER(m_objective);
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp();

        ఒకవేళ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ఒకవేళ (checkMaxExprs()) తిరిగి;
        CoverExprs lhsExprs;
        m_exprs.మార్పిడి(lhsExprs);
        ఒకవేళ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        ఒకవేళ (checkMaxExprs()) తిరిగి;
        CoverExprs rhsExprs;
        m_exprs.మార్పిడి(rhsExprs);

        కొరకు (స్థిరరాశి Cover_సమాసం& l : lhsExprs) {
            కొరకు (స్థిరరాశి Cover_సమాసం& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::సమితి<AstVar*>, 2> varps;
                std::array<std::సమితి<std::string>, 2> strs;

                UASSERT_OBJ(!l.ఖాళీలు() && !r.ఖాళీలు(), nodep, "Empty coverage expression branch");
                Cover_సమాసం సమాసం;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                కొరకు (స్థిరరాశి Cover_పదం& పదం : l) {
                    ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(పదం.m_exprp, VarRef)) {
                        varps[పదం.m_objective].చొప్పించు(refp->varp());
                    } లేకపోతే {
                        strs[పదం.m_objective].చొప్పించు(పదం.m_emitV);
                    }
                    సమాసం.నెట్టు_వెనుక(పదం);
                }
                తార్కిక impossible = అబద్ధం;
                కొరకు (స్థిరరాశి Cover_పదం& పదం : r) {
                    తార్కిక redundant = అబద్ధం;
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి refp = VN_CAST(పదం.m_exprp, కణుపు_Var_Ref)) {
                        ఒకవేళ (varps[పదం.m_objective].కనుగొను(refp->varp())
                            != varps[పదం.m_objective].చివర())
                            redundant = నిజం;
                        ఒకవేళ (varps[!పదం.m_objective].కనుగొను(refp->varp())
                            != varps[!పదం.m_objective].చివర())
                            impossible = నిజం;
                    } లేకపోతే {
                        ఒకవేళ (strs[పదం.m_objective].కనుగొను(పదం.m_emitV)
                            != strs[పదం.m_objective].చివర())
                            redundant = నిజం;
                        ఒకవేళ (strs[!పదం.m_objective].కనుగొను(పదం.m_emitV)
                            != strs[!పదం.m_objective].చివర())
                            impossible = నిజం;
                    }
                    ఒకవేళ (!redundant) సమాసం.నెట్టు_వెనుక(పదం);
                }
                ఒకవేళ (!impossible) m_exprs.నెట్టు_వెనుక(std::move(సమాసం));
                ఒకవేళ (checkMaxExprs()) తిరిగి;
            }
        }
    }

    శూన్య or_సమాసం(Ast_కణుపు_Biop* nodep) {
        ఒకవేళ (m_seeking == NONE) {
            coverExprs(nodep);
        } లేకపోతే ఒకవేళ (m_objective) {
            సమాసం_Either(nodep);
        } లేకపోతే {
            సమాసం_Both(nodep);
        }
        పంక్తి_Track(nodep);
    }
    శూన్య సందర్శించు(AstLogOr* nodep) override { or_సమాసం(nodep); }
    శూన్య సందర్శించు(AstOr* nodep) override { or_సమాసం(nodep); }

    శూన్య and_సమాసం(Ast_కణుపు_Biop* nodep) {
        ఒకవేళ (m_seeking == NONE) {
            coverExprs(nodep);
        } లేకపోతే ఒకవేళ (m_objective) {
            సమాసం_Both(nodep);
        } లేకపోతే {
            సమాసం_Either(nodep);
        }
        పంక్తి_Track(nodep);
    }
    శూన్య సందర్శించు(AstLogAnd* nodep) override { and_సమాసం(nodep); }
    శూన్య సందర్శించు(AstAnd* nodep) override { and_సమాసం(nodep); }

    శూన్య xor_సమాసం(Ast_కణుపు_Biop* nodep) {
        ఒకవేళ (m_seeking == NONE) {
            coverExprs(nodep);
        } లేకపోతే {
            కొరకు (స్థిరరాశి తార్కిక lObjective : {అబద్ధం, నిజం}) {
                CoverExprs prevExprs;
                m_exprs.మార్పిడి(prevExprs);
                స్థిరరాశి తార్కిక rObjective = lObjective ^ m_objective;
                సమాసం_Both(nodep, నిజం, lObjective, rObjective);
                m_exprs.splice(m_exprs.చివర(), prevExprs);
                ఒకవేళ (checkMaxExprs()) ఆపు;
            }
        }
        పంక్తి_Track(nodep);
    }
    శూన్య సందర్శించు(AstXor* nodep) override { xor_సమాసం(nodep); }

    శూన్య సమాసం_Not(Ast_కణుపు_సమాసం* nodep) {
        VL_RESTORER(m_objective);
        ఒకవేళ (m_seeking == NONE) {
            coverExprs(nodep);
        } లేకపోతే {
            m_objective = !m_objective;
            iterateChildren(nodep);
            పంక్తి_Track(nodep);
        }
    }
    శూన్య సందర్శించు(AstNot* nodep) override { సమాసం_Not(nodep); }
    శూన్య సందర్శించు(AstLogNot* nodep) override { సమాసం_Not(nodep); }

    మూస <రకం_పేరు T_Oper>
    శూన్య సమాసం_Reduce(Ast_కణుపు_Uniop* nodep) {
        ఒకవేళ (m_seeking != ABORTED) {
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
            స్థిరరాశి పూర్ణాంకం వెడల్పు = lhsp->dtypep()->వెడల్పు();
            స్థిరరాశి కొలత_t expected = std::is_same<T_Oper, AstXor>::విలువ ? 0x1 << వెడల్పు : వెడల్పు + 1;
            ఒకవేళ (checkMaxExprs(expected)) తిరిగి;
            Ast_కణుపు_సమాసం* unrolledp = కొత్త AstSel{
                fl, lhsp->clone_వృక్షం(అబద్ధం), కొత్త AstConst{fl, static_cast<uint32_t>(వెడల్పు - 1)}, 1};
            కొరకు (పూర్ణాంకం బిట్టు_2 = వెడల్పు - 2; బిట్టు_2 >= 0; బిట్టు_2--) {
                AstSel* స్థిరరాశి selp = కొత్త AstSel{fl, lhsp->clone_వృక్షం(అబద్ధం),
                                                కొత్త AstConst{fl, static_cast<uint32_t>(బిట్టు_2)}, 1};
                unrolledp = కొత్త T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(నెట్టు_Deletep(unrolledp), unrolledp);
        } లేకపోతే {
            iterateChildren(nodep);
            పంక్తి_Track(nodep);
        }
    }
    శూన్య సందర్శించు(AstRedOr* nodep) override { సమాసం_Reduce<AstOr>(nodep); }
    శూన్య సందర్శించు(AstRedAnd* nodep) override { సమాసం_Reduce<AstAnd>(nodep); }
    శూన్య సందర్శించు(AstRedXor* nodep) override { సమాసం_Reduce<AstXor>(nodep); }

    శూన్య సందర్శించు(AstLogIf* nodep) override {
        ఒకవేళ (m_seeking == NONE) {
            coverExprs(nodep);
        } లేకపోతే ఒకవేళ (m_objective) {
            సమాసం_Either(nodep, నిజం, అబద్ధం, నిజం);
        } లేకపోతే {
            సమాసం_Both(nodep, నిజం, నిజం, అబద్ధం);
        }
        పంక్తి_Track(nodep);
    }

    శూన్య సందర్శించు(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        ఒకవేళ (m_seeking == NONE) {
            coverExprs(nodep);
        } లేకపోతే {
            m_objective = !m_objective;
            xor_సమాసం(nodep);
            పంక్తి_Track(nodep);
        }
    }

    శూన్య సందర్శించు(AstFuncRef* nodep) override {
        ఒకవేళ (nodep->taskp()->lifetime().isAutomatic()) {
            సందర్శించు(static_cast<Ast_కణుపు_సమాసం*>(nodep));
        } లేకపోతే {
            సమాసం_Unsupported(nodep, "non-automatic function");
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        ఒకవేళ (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } లేకపోతే {
            సమాసం_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            ఒకవేళ (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_వృక్షం(nodep, emitV);
                // Add new expression with a single term
                Cover_సమాసం సమాసం;
                సమాసం.emplace_వెనుక(nodep, m_objective, emitV.str());
                m_exprs.నెట్టు_వెనుక(std::move(సమాసం));
                checkMaxExprs();
            } లేకపోతే {
                సమాసం_Unsupported(nodep, "not coverage eligible");
            }
        }
        పంక్తి_Track(nodep);
    }

    // VISITORS - BOTH
    శూన్య సందర్శించు(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_లోపల_Toggle_Off);
        m_state.m_on = అబద్ధం;
        m_లోపల_Toggle_Off = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_లోపల_Toggle_Off);
        m_state.m_on = అబద్ధం;
        m_లోపల_Toggle_Off = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        iterateChildren(nodep);
        పంక్తి_Track(nodep);
    }

    శూన్య సమాసం_Unsupported(Ast_కణుపు* nodep, స్థిరరాశి string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        స్థిరరాశి తార్కిక wasSeeking = m_seeking == SEEKING;
        స్థిరరాశి Objective oldSeeking = m_seeking;
        ఒకవేళ (wasSeeking) abort_సమాసం_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        పంక్తి_Track(nodep);
        ఒకవేళ (!wasSeeking) m_seeking = oldSeeking;
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CoverageVisitor(AstNetlist* rootp)
        : m_సమాసం_తాత్కాలిక_పేర్లు{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = అప్రమేయ;
};

//######################################################################
// Coverage class functions

శూన్య V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("coverage", 0, dump_వృక్షం_Either_Level() >= 3);
}
