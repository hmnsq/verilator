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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Coverage.h"

#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    ತಾರ್ಕಿಕ m_eligible = ಸತ್ಯ;

    ಸ್ಥಿರ ತಾರ್ಕಿಕ elemDTypeEligible(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = dtypep->virtRefDTypep()) {
            ಯದಿ (!elemDTypeEligible(dtp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = dtypep->virtRefDType2p()) {
            ಯದಿ (!elemDTypeEligible(dtp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ !VN_IS(dtypep, ClassRefDType);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        ಯದಿ (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } ಅನ್ಯಥಾ {
            m_eligible = ಅಸತ್ಯ;
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (!nodep->is_ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible()) {
            m_eligible = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            iterateChildrenConst(nodep);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible_Visitor(Ast_ಗಂಟು* nodep) { iterateConst(nodep); }
    ~ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible_Visitor() override = ಸ್ವಭಾವ;

    ತಾರ್ಕಿಕ eligible() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

ವರ್ಗ CoverageVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    using LinenoSet = std::set<ಪೂರ್ಣಾಙ್ಕ>;

    ಸಂರಚನಾ CoverTerm final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_exprp;  // Expression branch term
        ತಾರ್ಕಿಕ m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ objective, ಧ್ರುವ string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_ಅಭಿವ್ಯಕ್ತಿ = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_ಅಭಿವ್ಯಕ್ತಿ>;

    ಸಂರಚನಾ ToggleEnt final {
        ಧ್ರುವ string m_comment;  // Comment for coverage dump
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_varRefp;  // How to get to this element
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_chgRefp;  // How to get to this element
        ToggleEnt(ಧ್ರುವ string& comment, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* vp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = ಸ್ವಭಾವ;
        ಶೂನ್ಯ cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_ವೃಕ್ಷ(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_ವೃಕ್ಷ(), m_chgRefp = nullptr);
        }
    };

    ಸಂರಚನಾ CheckState final {  // State save-restored on each new coverage scope/block
        ತಾರ್ಕಿಕ m_on = ಅಸತ್ಯ;  // Should this block get covered?
        ತಾರ್ಕಿಕ m_inModOff = ಅಸತ್ಯ;  // In module with no coverage
        ಪೂರ್ಣಾಙ್ಕ m_handle = 0;  // Opaque handle for index into line tracking
        ಧ್ರುವ Ast_ಗಂಟು* m_nodep = nullptr;  // Node establishing this state
        CheckState() = ಸ್ವಭಾವ;
        ತಾರ್ಕಿಕ coverageOn(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        ತಾರ್ಕಿಕ ಸಾಲು_Coverage_On(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ coverageOn(nodep) && v3Global.opt.coverage_ಸಾಲು();
        }
        ತಾರ್ಕಿಕ branchCoverageOn(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        ತಾರ್ಕಿಕ ಅಭಿವ್ಯಕ್ತಿ_Coverage_On(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ coverageOn(nodep) && v3Global.opt.coverage_ಅಭಿವ್ಯಕ್ತಿ();
        }
    };

    ಗಣನಾ Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;
    V3UniqueNames m_ಅಭಿವ್ಯಕ್ತಿ_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    ಪೂರ್ಣಾಙ್ಕ m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ಗಂಟು* m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nullptr;  // Node to add expr coverage to
    ತಾರ್ಕಿಕ m_then = ಅಸತ್ಯ;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    ತಾರ್ಕಿಕ m_objective = ಅಸತ್ಯ;  // Expression objective
    ತಾರ್ಕಿಕ m_ifCond = ಅಸತ್ಯ;  // Visiting if condition
    ತಾರ್ಕಿಕ m_inToggleOff = ಅಸತ್ಯ;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<ಪೂರ್ಣಾಙ್ಕ, LinenoSet> m_handle_ಸಾಲುಗಳು;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ಧ್ರುವ ವರ್ಣ* varIgnoreToggle(ಧ್ರುವ AstVar* nodep) {
        ಧ್ರುವ ತಾರ್ಕಿಕ cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        ಯದಿ (!cover) ಪ್ರತ್ಯಾಗಮ "Not relevant signal";
        ಯದಿ (nodep->isConst()) ಪ್ರತ್ಯಾಗಮ "Signal is constant";
        ಯದಿ (nodep->isDouble()) ಪ್ರತ್ಯಾಗಮ "Signal is double";
        ಯದಿ (nodep->isString()) ಪ್ರತ್ಯಾಗಮ "Signal is string";
        ಯದಿ (!v3Global.opt.coverageUnderscore()) {
            ಧ್ರುವ string prettyName = nodep->prettyName();
            ಯದಿ (prettyName[0] == '_') ಪ್ರತ್ಯಾಗಮ "Leading underscore";
            ಯದಿ (prettyName.find("._") != string::npos) ಪ್ರತ್ಯಾಗಮ "Inlined leading underscore";
        }
        ಯದಿ ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            ಪ್ರತ್ಯಾಗಮ "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    AstCoverInc* newCoverInc(File_ಸಾಲು* fl, Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ declp,
                             ಧ್ರುವ string& trace_var_name) {
        AstCoverInc* ಧ್ರುವ incp = ನೂತನ AstCoverInc{fl, declp};
        ಯದಿ (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_ಸಾಲು* ಧ್ರುವ fl_nowarn = ನೂತನ File_ಸಾಲು{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ಸತ್ಯ);
            AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(ಸತ್ಯ);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* ಧ್ರುವ assp = ನೂತನ AstAssign{
                incp->fileline(), ನೂತನ AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                ನೂತನ AstAdd{incp->fileline(), ನೂತನ AstVarRef{incp->fileline(), varp, VAccess::READ},
                           ನೂತನ AstConst{incp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}}};
            Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(incp, assp);
        }
        ಪ್ರತ್ಯಾಗಮ incp;
    }
    string trace_Name_For_ಸಾಲು(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        ಯದಿ (ಧ್ರುವ uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        ಪ್ರತ್ಯಾಗಮ name;
    }

    // Line tracking
    ಶೂನ್ಯ createHandle(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = ಸತ್ಯ;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    ಶೂನ್ಯ ಸಾಲು_Track(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        ಯದಿ ((m_state.ಸಾಲು_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_ಸಾಲುಗಳು[m_state.m_handle].ಸೇರಿಸು(lineno);
            }
        }
    }
    ಸ್ಥಿರ string ಸಾಲುಗಳು_First_Last(ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ first, ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ last) {
        ಯದಿ (first && first == last) {
            ಪ್ರತ್ಯಾಗಮ cvtToStr(first);
        } ಅನ್ಯಥಾ ಯದಿ (first && last) {
            ಪ್ರತ್ಯಾಗಮ cvtToStr(first) + "-" + cvtToStr(last);
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ "";
        }
    }
    string ಸಾಲುಗಳು_Cov(ಧ್ರುವ CheckState& state, ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        ಧ್ರುವ LinenoSet& ಸಾಲುಗಳು = m_handle_ಸಾಲುಗಳು[state.m_handle];
        ಪೂರ್ಣಾಙ್ಕ first = 0;
        ಪೂರ್ಣಾಙ್ಕ last = 0;
        ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ linen : ಸಾಲುಗಳು) {
            ಯದಿ (!first) {
                first = last = linen;
            } ಅನ್ಯಥಾ ಯದಿ (linen == last + 1) {
                ++last;
            } ಅನ್ಯಥಾ {
                ಯದಿ (!out.empty()) out += ",";
                out += ಸಾಲುಗಳು_First_Last(first, last);
                first = last = linen;
            }
        }
        ಯದಿ (first) {
            ಯದಿ (!out.empty()) out += ",";
            out += ಸಾಲುಗಳು_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        ಪ್ರತ್ಯಾಗಮ out;
    }

    // VISITORS - BOTH
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ಅಭಿವ್ಯಕ್ತಿ_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_ತುದಿ();  // Already made top shell, no coverage for it
        ಯದಿ (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_ಸಾಲುಗಳು.clear();
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ಅಭಿವ್ಯಕ್ತಿ_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
            m_inToggleOff = ಸತ್ಯ;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateProcedure(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override { iterateProcedure(nodep); }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
        m_inToggleOff = ಸತ್ಯ;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        ಯದಿ (m_state.ಸಾಲು_Coverage_On(nodep)) {
            AstCoverOtherDecl* ಧ್ರುವ declp
                = ನೂತನ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ಸಾಲು(nodep, "block"));
            ಸೇರಿಸು_Proc_Statement(nodep, newp);
        }
    }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        ಯದಿ (nodep->user2SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        ಸಾಲು_Track(nodep);
        ಯದಿ (m_state.ಸಾಲು_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ಧ್ರುವ declp
                = ನೂತನ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ಸಾಲು(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_ಅಭಿವ್ಯಕ್ತಿ_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        ಯದಿ (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Proc_Statement(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* stmtp) {
        ಯದಿ (Ast_ಗಂಟು_Procedure* ಧ್ರುವ itemp = VN_CAST(nodep, ಗಂಟು_Procedure)) {
            itemp->addStmtsp(stmtp);
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ itemp = VN_CAST(nodep, ಗಂಟು_FTask)) {
            itemp->addStmtsp(stmtp);
        } ಅನ್ಯಥಾ ಯದಿ (AstLoop* ಧ್ರುವ itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } ಅನ್ಯಥಾ ಯದಿ (AstIf* ಧ್ರುವ itemp = VN_CAST(nodep, If)) {
            ಯದಿ (m_then) {
                itemp->addThensp(stmtp);
            } ಅನ್ಯಥಾ {
                itemp->addElsesp(stmtp);
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstBegin* ಧ್ರುವ itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    ಶೂನ್ಯ iterateProcedure(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
        m_inToggleOff = ಸತ್ಯ;
        createHandle(nodep);
        iterateChildren(nodep);
        ಯದಿ (m_state.ಸಾಲು_Coverage_On(nodep)) {
            ಸಾಲು_Track(nodep);
            AstCoverOtherDecl* ಧ್ರುವ declp
                = ನೂತನ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ಸಾಲು(nodep, "block"));
            ಸೇರಿಸು_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ disablep = varIgnoreToggle(nodep);
            ಯದಿ (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } ಅನ್ಯಥಾ {
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
                ಧ್ರುವ string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_ಸಾಲು* ಧ್ರುವ fl_nowarn = ನೂತನ File_ಸಾಲು{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ಸತ್ಯ);
                AstVar* ಧ್ರುವ chgVarp
                    = ನೂತನ AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, ನೂತನ AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 ನೂತನ AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    ಶೂನ್ಯ toggleVarBottom(ಧ್ರುವ ToggleEnt& above, ಧ್ರುವ AstVar* varp, ಧ್ರುವ VNumRange& range) {
        ಧ್ರುವ std::string hierPrefix
            = (m_beginHier != "") ? Ast_ಗಂಟು::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ಧ್ರುವ declp
            = ನೂತನ AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* ಧ್ರುವ newp = ನೂತನ AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), above.m_chgRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
        m_modp->addStmtsp(newp);
    }

    ಶೂನ್ಯ toggleVarRecurse(ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep, ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ depth,  // per-iteration
                          ಧ್ರುವ ToggleEnt& above, ಧ್ರುವ AstVar* ಧ್ರುವ varp) {  // Constant
        ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ index_docs : adtypep->declRange().seqLoToHi()) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 ನೂತನ AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), index_code},
                                 ನೂತನ AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ index_docs : adtypep->declRange().seqLoToHi()) {
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = adtypep->subDTypep()->skipRefp();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 ನೂತನ AstSel{varp->fileline(), above.m_varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                            index_code * subtypep->width(), subtypep->width()},
                                 ನೂತನ AstSel{varp->fileline(), above.m_chgRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstStructDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, StructDType)) {
            ಯದಿ (adtypep->packed()) {
                ಪ್ರತಿ (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = itemp->subDTypep()->skipRefp();
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        ನೂತನ AstSel{varp->fileline(), above.m_varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), index_code,
                                   subtypep->width()},
                        ನೂತನ AstSel{varp->fileline(), above.m_chgRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } ಅನ್ಯಥಾ {
                ಪ್ರತಿ (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = itemp->subDTypep()->skipRefp();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ varRefp = ನೂತನ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), itemp->name()};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ chgRefp = ನೂತನ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnionDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            ಯದಿ (ಧ್ರುವ AstMemberDType* ಧ್ರುವ itemp = adtypep->membersp()) {
                Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = itemp->subDTypep()->skipRefp();
                ಯದಿ (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                     above.m_chgRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } ಅನ್ಯಥಾ {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ varRefp = ನೂತನ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), itemp->name()};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ chgRefp = ನೂತನ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } ಅನ್ಯಥಾ {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    ತಾರ್ಕಿಕ includeCondToBranchRecursive(ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ backp = nodep->backp();
        ಯದಿ (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            ಪ್ರತ್ಯಾಗಮ includeCondToBranchRecursive(backp);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            ಪ್ರತ್ಯಾಗಮ includeCondToBranchRecursive(backp);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(backp, ಗಂಟು_Assign) && VN_AS(backp, ಗಂಟು_Assign)->rhsp() == nodep) {
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // VISITORS - LINE COVERAGE
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        ಯದಿ (m_seeking == NONE) coverExprs(nodep->condp());

        ಯದಿ (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ಧ್ರುವ CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            ಸಾಲು_Track(nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* ಧ್ರುವ thenDeclp
                = ನೂತನ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_ಸಾಲು(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* ಧ್ರುವ elseDeclp
                = ನೂತನ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", ಸಾಲುಗಳು_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_ಸಾಲು(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } ಅನ್ಯಥಾ {
            ಸಾಲು_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    ಶೂನ್ಯ visit(AstIf* nodep) override {
        ಯದಿ (nodep->user2()) ಪ್ರತ್ಯಾಗಮ;

        UINFO(4, " IF: " << nodep);
        ಯದಿ (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ಧ್ರುವ ತಾರ್ಕಿಕ elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            ಯದಿ (elsif) VN_AS(nodep->elsesp(), If)->user1(ಸತ್ಯ);
            ಧ್ರುವ ತಾರ್ಕಿಕ first_elsif = !nodep->user1() && elsif;
            ಧ್ರುವ ತಾರ್ಕಿಕ cont_elsif = nodep->user1() && elsif;
            ಧ್ರುವ ತಾರ್ಕಿಕ final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ಧ್ರುವ CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
                VL_RESTORER(m_then);
                m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
                m_then = ಸತ್ಯ;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                ಸಾಲು_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
                VL_RESTORER(m_then);
                m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
                m_then = ಅಸತ್ಯ;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            ಯದಿ (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ಧ್ರುವ ifDeclp
                    = ನೂತನ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", ಸಾಲುಗಳು_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_ಸಾಲು(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ಧ್ರುವ elseDeclp
                    = ನೂತನ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", ಸಾಲುಗಳು_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_ಸಾಲು(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            ಅನ್ಯಥಾ ಯದಿ (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                ಯದಿ (ifState.ಸಾಲು_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ಧ್ರುವ elsifDeclp = ನೂತನ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        ಸಾಲುಗಳು_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_ಸಾಲು(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } ಅನ್ಯಥಾ {
                // Cover as separate blocks (not a branch as is not two-legged)
                ಯದಿ (ifState.ಸಾಲು_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ಧ್ರುವ ifDeclp = ನೂತನ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        ಸಾಲುಗಳು_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_ಸಾಲು(nodep, "if")));
                }
                ಯದಿ (elseState.ಸಾಲು_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ಧ್ರುವ elseDeclp = ನೂತನ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        ಸಾಲುಗಳು_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_ಸಾಲು(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = ಸತ್ಯ;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    ಶೂನ್ಯ visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        ಯದಿ (m_state.ಸಾಲು_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            ಯದಿ (m_state.ಸಾಲು_Coverage_On(nodep)) {  // if the case body didn't disable it
                ಸಾಲು_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ಧ್ರುವ declp
                    = ನೂತನ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_ಸಾಲು(nodep, "case")));
            }
        }
    }
    ಶೂನ್ಯ visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = ಸತ್ಯ;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        ಯದಿ (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            ಸಾಲು_Track(nodep);
            AstCoverOtherDecl* ಧ್ರುವ declp
                = ನೂತನ AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    ಶೂನ್ಯ visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
        m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstStmtPragma* nodep) override {
        ಯದಿ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = ಅಸತ್ಯ;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_state.m_on) iterateChildren(nodep);
            ಸಾಲು_Track(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        ಯದಿ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        ಸಾಲು_Track(nodep);
    }

    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
        m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
        m_inToggleOff = ಸತ್ಯ;
        ಯದಿ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        ಸಾಲು_Track(nodep);
    }

    ಶೂನ್ಯ abort_ಅಭಿವ್ಯಕ್ತಿ_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        ಯದಿ (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    ತಾರ್ಕಿಕ checkMaxExprs(size_t additional = 0) {
        ಯದಿ (m_seeking != ABORTED
            && static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_exprs.size() + additional) <= v3Global.opt.coverage_ಅಭಿವ್ಯಕ್ತಿ_Max())
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        abort_ಅಭಿವ್ಯಕ್ತಿ_Coverage();
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ add_ಅಭಿವ್ಯಕ್ತಿ_Cover_Inc(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಙ್ಕ start = 0) {
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಪೂರ್ಣಾಙ್ಕ ಎಣಿಕೆ = start;
        ಪ್ರತಿ (Cover_ಅಭಿವ್ಯಕ್ತಿ& ಅಭಿವ್ಯಕ್ತಿ : m_exprs) {
            ಧ್ರುವ string name = "expr_" + std::to_string(ಎಣಿಕೆ);
            string comment = "(";
            ತಾರ್ಕಿಕ first = ಸತ್ಯ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;
            ಪ್ರತಿ (CoverTerm& term : ಅಭಿವ್ಯಕ್ತಿ) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* covExprp = nullptr;
                ಯದಿ (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ frefp = term.m_exprp;
                    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = frefp->dtypep();
                    ಧ್ರುವ auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    ಯದಿ (pair.second) {
                        varp = ನೂತನ AstVar{fl, VVarType::MODULETEMP, m_ಅಭಿವ್ಯಕ್ತಿ_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        ಯದಿ (m_ftaskp) {
                            varp->funcLocal(ಸತ್ಯ);
                            m_ftaskp->prependStmtsp(varp);
                        } ಅನ್ಯಥಾ {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                            fl, ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            ನೂತನ AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = ನೂತನ AstVarRef{fl, varp, VAccess::READ};
                } ಅನ್ಯಥಾ {
                    covExprp = term.m_exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp
                    = term.m_objective ? covExprp : ನೂತನ AstLogNot{fl, covExprp};
                ಯದಿ (condp) {
                    condp = ನೂತನ AstLogAnd{fl, condp, termp};
                } ಅನ್ಯಥಾ {
                    condp = termp;
                }
                first = ಅಸತ್ಯ;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ಧ್ರುವ declp = ನೂತನ AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_ಗಂಟು* ಧ್ರುವ newp = newCoverInc(fl, declp, trace_Name_For_ಸಾಲು(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{fl, condp, newp, nullptr};
            ifp->user2(ಸತ್ಯ);
            ಸೇರಿಸು_Proc_Statement(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp, ifp);
            ++ಎಣಿಕೆ;
        }
    }

    ಶೂನ್ಯ coverExprs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (!m_state.ಅಭಿವ್ಯಕ್ತಿ_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp) {
            ಪ್ರತ್ಯಾಗಮ;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = ಅಸತ್ಯ;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = ಸತ್ಯ;
        iterate(nodep);
        ಯದಿ (checkMaxExprs(falseExprs.size())) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_seeking == ABORTED) ಪ್ರತ್ಯಾಗಮ;

        add_ಅಭಿವ್ಯಕ್ತಿ_Cover_Inc(nodep);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ start = m_exprs.size();
        m_objective = ಅಸತ್ಯ;
        m_exprs.swap(falseExprs);
        add_ಅಭಿವ್ಯಕ್ತಿ_Cover_Inc(nodep, start);
    }

    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Either(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ overrideObjective = ಅಸತ್ಯ, ತಾರ್ಕಿಕ lObjective = ಅಸತ್ಯ,
                    ತಾರ್ಕಿಕ rObjective = ಅಸತ್ಯ) {
        VL_RESTORER(m_objective);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp();

        ಯದಿ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ಯದಿ (checkMaxExprs()) ಪ್ರತ್ಯಾಗಮ;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        ಯದಿ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Both(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ overrideObjective = ಅಸತ್ಯ, ತಾರ್ಕಿಕ lObjective = ಅಸತ್ಯ,
                  ತಾರ್ಕಿಕ rObjective = ಅಸತ್ಯ) {
        VL_RESTORER(m_objective);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp();

        ಯದಿ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ಯದಿ (checkMaxExprs()) ಪ್ರತ್ಯಾಗಮ;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        ಯದಿ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        ಯದಿ (checkMaxExprs()) ಪ್ರತ್ಯಾಗಮ;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        ಪ್ರತಿ (ಧ್ರುವ Cover_ಅಭಿವ್ಯಕ್ತಿ& l : lhsExprs) {
            ಪ್ರತಿ (ಧ್ರುವ Cover_ಅಭಿವ್ಯಕ್ತಿ& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_ಅಭಿವ್ಯಕ್ತಿ ಅಭಿವ್ಯಕ್ತಿ;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                ಪ್ರತಿ (ಧ್ರುವ CoverTerm& term : l) {
                    ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].ಸೇರಿಸು(refp->varp());
                    } ಅನ್ಯಥಾ {
                        strs[term.m_objective].ಸೇರಿಸು(term.m_emitV);
                    }
                    ಅಭಿವ್ಯಕ್ತಿ.ತಳ್ಳು_back(term);
                }
                ತಾರ್ಕಿಕ impossible = ಅಸತ್ಯ;
                ಪ್ರತಿ (ಧ್ರುವ CoverTerm& term : r) {
                    ತಾರ್ಕಿಕ redundant = ಅಸತ್ಯ;
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ refp = VN_CAST(term.m_exprp, ಗಂಟು_Var_Ref)) {
                        ಯದಿ (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = ಸತ್ಯ;
                        ಯದಿ (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = ಸತ್ಯ;
                    } ಅನ್ಯಥಾ {
                        ಯದಿ (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = ಸತ್ಯ;
                        ಯದಿ (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = ಸತ್ಯ;
                    }
                    ಯದಿ (!redundant) ಅಭಿವ್ಯಕ್ತಿ.ತಳ್ಳು_back(term);
                }
                ಯದಿ (!impossible) m_exprs.ತಳ್ಳು_back(std::move(ಅಭಿವ್ಯಕ್ತಿ));
                ಯದಿ (checkMaxExprs()) ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ or_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_Biop* nodep) {
        ಯದಿ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಅನ್ಯಥಾ ಯದಿ (m_objective) {
            ಅಭಿವ್ಯಕ್ತಿ_Either(nodep);
        } ಅನ್ಯಥಾ {
            ಅಭಿವ್ಯಕ್ತಿ_Both(nodep);
        }
        ಸಾಲು_Track(nodep);
    }
    ಶೂನ್ಯ visit(AstLogOr* nodep) override { or_ಅಭಿವ್ಯಕ್ತಿ(nodep); }
    ಶೂನ್ಯ visit(AstOr* nodep) override { or_ಅಭಿವ್ಯಕ್ತಿ(nodep); }

    ಶೂನ್ಯ and_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_Biop* nodep) {
        ಯದಿ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಅನ್ಯಥಾ ಯದಿ (m_objective) {
            ಅಭಿವ್ಯಕ್ತಿ_Both(nodep);
        } ಅನ್ಯಥಾ {
            ಅಭಿವ್ಯಕ್ತಿ_Either(nodep);
        }
        ಸಾಲು_Track(nodep);
    }
    ಶೂನ್ಯ visit(AstLogAnd* nodep) override { and_ಅಭಿವ್ಯಕ್ತಿ(nodep); }
    ಶೂನ್ಯ visit(AstAnd* nodep) override { and_ಅಭಿವ್ಯಕ್ತಿ(nodep); }

    ಶೂನ್ಯ xor_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_Biop* nodep) {
        ಯದಿ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (ಧ್ರುವ ತಾರ್ಕಿಕ lObjective : {ಅಸತ್ಯ, ಸತ್ಯ}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                ಧ್ರುವ ತಾರ್ಕಿಕ rObjective = lObjective ^ m_objective;
                ಅಭಿವ್ಯಕ್ತಿ_Both(nodep, ಸತ್ಯ, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                ಯದಿ (checkMaxExprs()) ಭಙ್ಗ;
            }
        }
        ಸಾಲು_Track(nodep);
    }
    ಶೂನ್ಯ visit(AstXor* nodep) override { xor_ಅಭಿವ್ಯಕ್ತಿ(nodep); }

    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Not(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        VL_RESTORER(m_objective);
        ಯದಿ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಅನ್ಯಥಾ {
            m_objective = !m_objective;
            iterateChildren(nodep);
            ಸಾಲು_Track(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstNot* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Not(nodep); }
    ಶೂನ್ಯ visit(AstLogNot* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Not(nodep); }

    template <typename T_Oper>
    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Reduce(Ast_ಗಂಟು_Uniop* nodep) {
        ಯದಿ (m_seeking != ABORTED) {
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = lhsp->dtypep()->width();
            ಧ್ರುವ size_t expected = std::is_same<T_Oper, AstXor>::ಮೌಲ್ಯ ? 0x1 << width : width + 1;
            ಯದಿ (checkMaxExprs(expected)) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* unrolledp = ನೂತನ AstSel{
                fl, lhsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ನೂತನ AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = width - 2; bit >= 0; bit--) {
                AstSel* ಧ್ರುವ selp = ನೂತನ AstSel{fl, lhsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                                ನೂತನ AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = ನೂತನ T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(unrolledp), unrolledp);
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
            ಸಾಲು_Track(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstRedOr* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Reduce<AstOr>(nodep); }
    ಶೂನ್ಯ visit(AstRedAnd* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Reduce<AstAnd>(nodep); }
    ಶೂನ್ಯ visit(AstRedXor* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Reduce<AstXor>(nodep); }

    ಶೂನ್ಯ visit(AstLogIf* nodep) override {
        ಯದಿ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಅನ್ಯಥಾ ಯದಿ (m_objective) {
            ಅಭಿವ್ಯಕ್ತಿ_Either(nodep, ಸತ್ಯ, ಅಸತ್ಯ, ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            ಅಭಿವ್ಯಕ್ತಿ_Both(nodep, ಸತ್ಯ, ಸತ್ಯ, ಅಸತ್ಯ);
        }
        ಸಾಲು_Track(nodep);
    }

    ಶೂನ್ಯ visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        ಯದಿ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಅನ್ಯಥಾ {
            m_objective = !m_objective;
            xor_ಅಭಿವ್ಯಕ್ತಿ(nodep);
            ಸಾಲು_Track(nodep);
        }
    }

    ಶೂನ್ಯ visit(AstFuncRef* nodep) override {
        ಯದಿ (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(nodep));
        } ಅನ್ಯಥಾ {
            ಅಭಿವ್ಯಕ್ತಿ_Unsupported(nodep, "non-automatic function");
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಯದಿ (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ {
            ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            ಯದಿ (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_ವೃಕ್ಷ(nodep, emitV);
                // Add new expression with a single term
                Cover_ಅಭಿವ್ಯಕ್ತಿ ಅಭಿವ್ಯಕ್ತಿ;
                ಅಭಿವ್ಯಕ್ತಿ.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.ತಳ್ಳು_back(std::move(ಅಭಿವ್ಯಕ್ತಿ));
                checkMaxExprs();
            } ಅನ್ಯಥಾ {
                ಅಭಿವ್ಯಕ್ತಿ_Unsupported(nodep, "not coverage eligible");
            }
        }
        ಸಾಲು_Track(nodep);
    }

    // VISITORS - BOTH
    ಶೂನ್ಯ visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = ಅಸತ್ಯ;
        m_inToggleOff = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = ಅಸತ್ಯ;
        m_inToggleOff = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        iterateChildren(nodep);
        ಸಾಲು_Track(nodep);
    }

    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Unsupported(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ಧ್ರುವ ತಾರ್ಕಿಕ wasSeeking = m_seeking == SEEKING;
        ಧ್ರುವ Objective oldSeeking = m_seeking;
        ಯದಿ (wasSeeking) abort_ಅಭಿವ್ಯಕ್ತಿ_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        ಸಾಲು_Track(nodep);
        ಯದಿ (!wasSeeking) m_seeking = oldSeeking;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_ಅಭಿವ್ಯಕ್ತಿ_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Coverage class functions

ಶೂನ್ಯ V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("coverage", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
