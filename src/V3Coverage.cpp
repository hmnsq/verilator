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
    ತಾರ್ಕಿಕ m_eligible = ನಿಜ;

    ಸ್ಥಿರ ತಾರ್ಕಿಕ elemDTypeEligible(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = dtypep->virtRefDTypep()) {
            ಆದರೆ (!elemDTypeEligible(dtp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = dtypep->virtRefDType2p()) {
            ಆದರೆ (!elemDTypeEligible(dtp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ !VN_IS(dtypep, ClassRefDType);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        ಆದರೆ (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_eligible = ಸುಳ್ಳು;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (!nodep->is_ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible()) {
            m_eligible = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildrenConst(nodep);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible_Visitor(Ast_ಗಂಟು* nodep) { iterateConst(nodep); }
    ~ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;

    ತಾರ್ಕಿಕ eligible() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

ವರ್ಗ CoverageVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    ಬಳಸಿ Lineno_ಗಣ = std::ಗಣ<ಪೂರ್ಣಾಂಕ>;

    ರಚನೆ Cover_ಪದ final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_exprp;  // Expression branch term
        ತಾರ್ಕಿಕ m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        Cover_ಪದ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ತಾರ್ಕಿಕ objective, ಸ್ಥಿರಾಂಕ string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    ಬಳಸಿ Cover_ಅಭಿವ್ಯಕ್ತಿ = std::deque<Cover_ಪದ>;
    ಬಳಸಿ CoverExprs = std::list<Cover_ಅಭಿವ್ಯಕ್ತಿ>;

    ರಚನೆ ToggleEnt final {
        ಸ್ಥಿರಾಂಕ string m_comment;  // Comment for coverage dump
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_varRefp;  // How to get to this element
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_chgRefp;  // How to get to this element
        ToggleEnt(ಸ್ಥಿರಾಂಕ string& comment, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* vp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = ಪೂರ್ವನಿಯೋಜಿತ;
        ಶೂನ್ಯ cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_ವೃಕ್ಷ(), m_varRefp = ಶೂನ್ಯಸೂಚಕ);
            VL_DO_CLEAR(m_chgRefp->delete_ವೃಕ್ಷ(), m_chgRefp = ಶೂನ್ಯಸೂಚಕ);
        }
    };

    ರಚನೆ CheckState final {  // State save-restored on each new coverage scope/block
        ತಾರ್ಕಿಕ m_on = ಸುಳ್ಳು;  // Should this block get covered?
        ತಾರ್ಕಿಕ m_ಒಳ_Mod_Off = ಸುಳ್ಳು;  // In module with no coverage
        ಪೂರ್ಣಾಂಕ m_handle = 0;  // Opaque handle for index into line tracking
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_nodep = ಶೂನ್ಯಸೂಚಕ;  // Node establishing this state
        CheckState() = ಪೂರ್ವನಿಯೋಜಿತ;
        ತಾರ್ಕಿಕ coverageOn(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ m_on && !m_ಒಳ_Mod_Off && nodep->fileline()->coverageOn();
        }
        ತಾರ್ಕಿಕ ಸಾಲು_Coverage_On(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ coverageOn(nodep) && v3Global.opt.coverage_ಸಾಲು();
        }
        ತಾರ್ಕಿಕ branchCoverageOn(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        ತಾರ್ಕಿಕ ಅಭಿವ್ಯಕ್ತಿ_Coverage_On(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ coverageOn(nodep) && v3Global.opt.coverage_ಅಭಿವ್ಯಕ್ತಿ();
        }
    };

    ಎಣಿಕೆ Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    V3_Unique_ಹೆಸರುಗಳು m_ಅಭಿವ್ಯಕ್ತಿ_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    ಪೂರ್ಣಾಂಕ m_ಮುಂದಿನ_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module to add statement to
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    Ast_ಗಂಟು* m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = ಶೂನ್ಯಸೂಚಕ;  // Node to add expr coverage to
    ತಾರ್ಕಿಕ m_then = ಸುಳ್ಳು;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    ತಾರ್ಕಿಕ m_objective = ಸುಳ್ಳು;  // Expression objective
    ತಾರ್ಕಿಕ m_ifCond = ಸುಳ್ಳು;  // Visiting if condition
    ತಾರ್ಕಿಕ m_ಒಳ_Toggle_Off = ಸುಳ್ಳು;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<ಪೂರ್ಣಾಂಕ, Lineno_ಗಣ> m_handle_ಸಾಲುಗಳು;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* varIgnoreToggle(ಸ್ಥಿರಾಂಕ AstVar* nodep) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ cover = nodep->isIO() || (nodep->isSignal() && nodep->is_ಬಿಟ್_Logic());
        ಆದರೆ (!cover) ಹಿಂತಿರುಗಿ "Not relevant signal";
        ಆದರೆ (nodep->isConst()) ಹಿಂತಿರುಗಿ "Signal is constant";
        ಆದರೆ (nodep->isDouble()) ಹಿಂತಿರುಗಿ "Signal is double";
        ಆದರೆ (nodep->isString()) ಹಿಂತಿರುಗಿ "Signal is string";
        ಆದರೆ (!v3Global.opt.coverageUnderscore()) {
            ಸ್ಥಿರಾಂಕ string prettyName = nodep->prettyName();
            ಆದರೆ (prettyName[0] == '_') ಹಿಂತಿರುಗಿ "Leading underscore";
            ಆದರೆ (prettyName.ಕಂಡುಹಿಡಿ("._") != string::npos) ಹಿಂತಿರುಗಿ "Inlined leading underscore";
        }
        ಆದರೆ ((nodep->ಅಗಲ() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverage_Max_ಅಗಲ())) {
            ಹಿಂತಿರುಗಿ "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    AstCoverInc* newCoverInc(File_ಸಾಲು* fl, Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ declp,
                             ಸ್ಥಿರಾಂಕ string& trace_var_name) {
        AstCoverInc* ಸ್ಥಿರಾಂಕ incp = ಹೊಸ AstCoverInc{fl, declp};
        ಆದರೆ (!trace_var_name.ಖಾಲಿಗಳು()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl_nowarn = ಹೊಸ File_ಸಾಲು{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDSIGNAL, ನಿಜ);
            AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->ಕಂಡುಹಿಡಿ_UInt32_DType()};
            varp->ಗಣ_Ignore_Sched_Write();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(ನಿಜ);
            m_modp->ಕೂಡಿಸು_Stmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                incp->fileline(), ಹೊಸ AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                ಹೊಸ Ast_ಕೂಡಿಸು{incp->fileline(), ಹೊಸ AstVarRef{incp->fileline(), varp, VAccess::READ},
                           ಹೊಸ AstConst{incp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}}};
            Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(incp, assp);
        }
        ಹಿಂತಿರುಗಿ incp;
    }
    string trace_Name_For_ಸಾಲು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()) + "_" + type;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ uint32_t suffix = m_varnames[name]++) name += "_" + cvt_ಗಮ್ಯ_Str(suffix);
        ಹಿಂತಿರುಗಿ name;
    }

    // Line tracking
    ಶೂನ್ಯ createHandle(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = ನಿಜ;
        m_state.m_handle = ++m_ಮುಂದಿನ_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    ಶೂನ್ಯ ಸಾಲು_Track(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        ಆದರೆ ((m_state.ಸಾಲು_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ lineno = nodep->fileline()->ಮೊದಲ_Lineno();
                 lineno <= nodep->fileline()->ಕೊನೆಯ_Lineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_ಸಾಲುಗಳು[m_state.m_handle].ಸೇರಿಸು(lineno);
            }
        }
    }
    ಸ್ಥಿರ string ಸಾಲುಗಳು_ಮೊದಲ_ಕೊನೆಯ(ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೊದಲ, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಕೊನೆಯ) {
        ಆದರೆ (ಮೊದಲ && ಮೊದಲ == ಕೊನೆಯ) {
            ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Str(ಮೊದಲ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಮೊದಲ && ಕೊನೆಯ) {
            ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Str(ಮೊದಲ) + "-" + cvt_ಗಮ್ಯ_Str(ಕೊನೆಯ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ "";
        }
    }
    string ಸಾಲುಗಳು_Cov(ಸ್ಥಿರಾಂಕ CheckState& state, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // Return comma separated list of ranged numbers
        string ಹೊರಹೋಗು;
        ಸ್ಥಿರಾಂಕ Lineno_ಗಣ& ಸಾಲುಗಳು = m_handle_ಸಾಲುಗಳು[state.m_handle];
        ಪೂರ್ಣಾಂಕ ಮೊದಲ = 0;
        ಪೂರ್ಣಾಂಕ ಕೊನೆಯ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ linen : ಸಾಲುಗಳು) {
            ಆದರೆ (!ಮೊದಲ) {
                ಮೊದಲ = ಕೊನೆಯ = linen;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (linen == ಕೊನೆಯ + 1) {
                ++ಕೊನೆಯ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (!ಹೊರಹೋಗು.ಖಾಲಿಗಳು()) ಹೊರಹೋಗು += ",";
                ಹೊರಹೋಗು += ಸಾಲುಗಳು_ಮೊದಲ_ಕೊನೆಯ(ಮೊದಲ, ಕೊನೆಯ);
                ಮೊದಲ = ಕೊನೆಯ = linen;
            }
        }
        ಆದರೆ (ಮೊದಲ) {
            ಆದರೆ (!ಹೊರಹೋಗು.ಖಾಲಿಗಳು()) ಹೊರಹೋಗು += ",";
            ಹೊರಹೋಗು += ಸಾಲುಗಳು_ಮೊದಲ_ಕೊನೆಯ(ಮೊದಲ, ಕೊನೆಯ);
        }
        UINFO(9, "lines out " << ಹೊರಹೋಗು << " for h" << state.m_handle << " " << nodep);
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
    }

    // VISITORS - BOTH
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ಅಭಿವ್ಯಕ್ತಿ_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_ಒಳ_Mod_Off = nodep->is_ತುದಿ();  // Already made top shell, no coverage for it
        ಆದರೆ (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_ಸಾಲುಗಳು.clear();
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ಅಭಿವ್ಯಕ್ತಿ_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_ಒಳ_Mod_Off = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (nodep->keyword() == VAlwaysKwd::CONT_ನಿಯೋಜಿಸು) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
            VL_RESTORER(m_ಒಳ_Toggle_Off);
            m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
            m_ಒಳ_Toggle_Off = ನಿಜ;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            ಹಿಂತಿರುಗಿ;
        }
        iterateProcedure(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override { iterateProcedure(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
        VL_RESTORER(m_ಒಳ_Toggle_Off);
        m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
        m_ಒಳ_Toggle_Off = ನಿಜ;
        createHandle(nodep);
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        ಆದರೆ (m_state.ಸಾಲು_Coverage_On(nodep)) {
            AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp
                = ಹೊಸ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            m_modp->ಕೂಡಿಸು_Stmtsp(declp);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ಸಾಲು(nodep, "block"));
            ಸೇರಿಸು_Proc_Statement(nodep, newp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        ಆದರೆ (nodep->user2_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        ಸಾಲು_Track(nodep);
        ಆದರೆ (m_state.ಸಾಲು_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp
                = ಹೊಸ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            m_modp->ಕೂಡಿಸು_Stmtsp(declp);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ಸಾಲು(nodep, "block"));
            nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_ಅಭಿವ್ಯಕ್ತಿ_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        ಆದರೆ (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Proc_Statement(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* stmtp) {
        ಆದರೆ (Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(nodep, ಗಂಟು_Procedure)) {
            itemp->ಕೂಡಿಸು_Stmtsp(stmtp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(nodep, ಗಂಟು_FTask)) {
            itemp->ಕೂಡಿಸು_Stmtsp(stmtp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstLoop* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(nodep, Loop)) {
            itemp->ಕೂಡಿಸು_Stmtsp(stmtp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(nodep, If)) {
            ಆದರೆ (m_then) {
                itemp->ಕೂಡಿಸು_Thensp(stmtp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                itemp->ಕೂಡಿಸು_Elsesp(stmtp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(nodep, Begin)) {
            itemp->ಕೂಡಿಸು_Stmtsp(stmtp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    ಶೂನ್ಯ iterateProcedure(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
        VL_RESTORER(m_ಒಳ_Toggle_Off);
        m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
        m_ಒಳ_Toggle_Off = ನಿಜ;
        createHandle(nodep);
        iterateChildren(nodep);
        ಆದರೆ (m_state.ಸಾಲು_Coverage_On(nodep)) {
            ಸಾಲು_Track(nodep);
            AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp
                = ಹೊಸ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            m_modp->ಕೂಡಿಸು_Stmtsp(declp);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ಸಾಲು(nodep, "block"));
            ಸೇರಿಸು_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_modp && !m_ಒಳ_Toggle_Off && !m_state.m_ಒಳ_Mod_Off && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ disablep = varIgnoreToggle(nodep);
            ಆದರೆ (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
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
                ಸ್ಥಿರಾಂಕ string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl_nowarn = ಹೊಸ File_ಸಾಲು{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDSIGNAL, ನಿಜ);
                AstVar* ಸ್ಥಿರಾಂಕ chgVarp
                    = ಹೊಸ AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->ಕೂಡಿಸು_Stmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, ಹೊಸ AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 ಹೊಸ AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    ಶೂನ್ಯ toggleVarBottom(ಸ್ಥಿರಾಂಕ ToggleEnt& above, ಸ್ಥಿರಾಂಕ AstVar* varp, ಸ್ಥಿರಾಂಕ VNumRange& range) {
        ಸ್ಥಿರಾಂಕ std::string hier_ಪೂರ್ವಪ್ರತ್ಯಯ
            = (m_beginHier != "") ? Ast_ಗಂಟು::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ಸ್ಥಿರಾಂಕ declp
            = ಹೊಸ AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hier_ಪೂರ್ವಪ್ರತ್ಯಯ + varp->name() + above.m_comment, range};
        m_modp->ಕೂಡಿಸು_Stmtsp(declp);
        AstCoverToggle* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), above.m_chgRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
        m_modp->ಕೂಡಿಸು_Stmtsp(newp);
    }

    ಶೂನ್ಯ toggleVarRecurse(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಆಳ,  // per-iteration
                          ಸ್ಥಿರಾಂಕ ToggleEnt& above, ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp) {  // Constant
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ_docs : adtypep->declRange().seq_Lo_ಗಮ್ಯ_Hi()) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ_ಸಂಕೇತ = ಸೂಚ್ಯಂಕ_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_ಗಮ್ಯ_Str(ಸೂಚ್ಯಂಕ_docs) + "]",
                                 ಹೊಸ AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಸೂಚ್ಯಂಕ_ಸಂಕೇತ},
                                 ಹೊಸ AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಸೂಚ್ಯಂಕ_ಸಂಕೇತ}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), ಆಳ + 1, newent, varp);
                newent.cleanup();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ_docs : adtypep->declRange().seq_Lo_ಗಮ್ಯ_Hi()) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = adtypep->subDTypep()->skipRefp();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ_ಸಂಕೇತ = ಸೂಚ್ಯಂಕ_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_ಗಮ್ಯ_Str(ಸೂಚ್ಯಂಕ_docs) + "]",
                                 ಹೊಸ AstSel{varp->fileline(), above.m_varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                            ಸೂಚ್ಯಂಕ_ಸಂಕೇತ * subtypep->ಅಗಲ(), subtypep->ಅಗಲ()},
                                 ಹೊಸ AstSel{varp->fileline(), above.m_chgRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                            ಸೂಚ್ಯಂಕ_ಸಂಕೇತ * subtypep->ಅಗಲ(), subtypep->ಅಗಲ()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), ಆಳ + 1, newent, varp);
                newent.cleanup();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, StructDType)) {
            ಆದರೆ (adtypep->packed()) {
                ಗಾಗಿ (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = itemp->subDTypep()->skipRefp();
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ_ಸಂಕೇತ = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        ಹೊಸ AstSel{varp->fileline(), above.m_varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಸೂಚ್ಯಂಕ_ಸಂಕೇತ,
                                   subtypep->ಅಗಲ()},
                        ಹೊಸ AstSel{varp->fileline(), above.m_chgRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಸೂಚ್ಯಂಕ_ಸಂಕೇತ,
                                   subtypep->ಅಗಲ()}};
                    toggleVarRecurse(subtypep, ಆಳ + 1, newent, varp);
                    newent.cleanup();
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಗಾಗಿ (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = itemp->subDTypep()->skipRefp();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ varRefp = ಹೊಸ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), itemp->name()};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ chgRefp = ಹೊಸ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, ಆಳ + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnionDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberDType* ಸ್ಥಿರಾಂಕ itemp = adtypep->membersp()) {
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = itemp->subDTypep()->skipRefp();
                ಆದರೆ (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                     above.m_chgRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
                    toggleVarRecurse(subtypep, ಆಳ + 1, newent, varp);
                    newent.cleanup();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ varRefp = ಹೊಸ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), itemp->name()};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ chgRefp = ಹೊಸ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, ಆಳ + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(dtypep, ಸರತಿ_DType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } ಇಲ್ಲದಿದ್ದರೆ {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    ತಾರ್ಕಿಕ include_Cond_ಗಮ್ಯ_Branch_Recursive(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = nodep->backp();
        ಆದರೆ (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            ಹಿಂತಿರುಗಿ include_Cond_ಗಮ್ಯ_Branch_Recursive(backp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            ಹಿಂತಿರುಗಿ include_Cond_ಗಮ್ಯ_Branch_Recursive(backp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(backp, ಗಂಟು_ನಿಯೋಜಿಸು) && VN_AS(backp, ಗಂಟು_ನಿಯೋಜಿಸು)->rhsp() == nodep) {
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // VISITORS - LINE COVERAGE
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        ಆದರೆ (m_seeking == NONE) coverExprs(nodep->condp());

        ಆದರೆ (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && include_Cond_ಗಮ್ಯ_Branch_Recursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ಸ್ಥಿರಾಂಕ CheckState ಕೊನೆಯ_State = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            ಸಾಲು_Track(nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thenp = nodep->thenp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ thenDeclp
                = ಹೊಸ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            m_modp->ಕೂಡಿಸು_Stmtsp(thenDeclp);
            nodep->thenp(ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_ಸಾಲು(nodep, "cond_then")),
                thenp});
            m_state = ಕೊನೆಯ_State;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elsep = nodep->elsep()->unlink_Fr_ಹಿಂಭಾಗ();
            AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ elseDeclp
                = ಹೊಸ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", ಸಾಲುಗಳು_Cov(m_state, nodep), 1};
            m_modp->ಕೂಡಿಸು_Stmtsp(elseDeclp);
            nodep->elsep(ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_ಸಾಲು(nodep, "cond_else")),
                elsep});

            m_state = ಕೊನೆಯ_State;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸಾಲು_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override {
        ಆದರೆ (nodep->user2()) ಹಿಂತಿರುಗಿ;

        UINFO(4, " IF: " << nodep);
        ಆದರೆ (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            ಆದರೆ (elsif) VN_AS(nodep->elsesp(), If)->user1(ನಿಜ);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮೊದಲ_elsif = !nodep->user1() && elsif;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ cont_elsif = nodep->user1() && elsif;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ಸ್ಥಿರಾಂಕ CheckState ಕೊನೆಯ_State = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
                VL_RESTORER(m_then);
                m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
                m_then = ನಿಜ;
                createHandle(nodep);
                iterate_And_ಮುಂದಿನ_Null(nodep->thensp());
                ಸಾಲು_Track(nodep);
                ifState = m_state;
            }
            m_state = ಕೊನೆಯ_State;
            {
                VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
                VL_RESTORER(m_then);
                m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
                m_then = ಸುಳ್ಳು;
                createHandle(nodep);
                iterate_And_ಮುಂದಿನ_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = ಕೊನೆಯ_State;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            ಆದರೆ (!(ಮೊದಲ_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ ifDeclp
                    = ಹೊಸ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", ಸಾಲುಗಳು_Cov(ifState, nodep), 0};
                m_modp->ಕೂಡಿಸು_Stmtsp(ifDeclp);
                nodep->ಕೂಡಿಸು_Thensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_ಸಾಲು(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ elseDeclp
                    = ಹೊಸ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", ಸಾಲುಗಳು_Cov(elseState, nodep), 1};
                m_modp->ಕೂಡಿಸು_Stmtsp(elseDeclp);
                nodep->ಕೂಡಿಸು_Elsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_ಸಾಲು(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಮೊದಲ_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                ಆದರೆ (ifState.ಸಾಲು_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ elsifDeclp = ಹೊಸ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        ಸಾಲುಗಳು_Cov(ifState, nodep), 0};
                    m_modp->ಕೂಡಿಸು_Stmtsp(elsifDeclp);

                    nodep->ಕೂಡಿಸು_Thensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_ಸಾಲು(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Cover as separate blocks (not a branch as is not two-legged)
                ಆದರೆ (ifState.ಸಾಲು_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ ifDeclp = ಹೊಸ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        ಸಾಲುಗಳು_Cov(ifState, nodep), 0};
                    m_modp->ಕೂಡಿಸು_Stmtsp(ifDeclp);
                    nodep->ಕೂಡಿಸು_Thensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_ಸಾಲು(nodep, "if")));
                }
                ಆದರೆ (elseState.ಸಾಲು_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ elseDeclp = ಹೊಸ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        ಸಾಲುಗಳು_Cov(elseState, nodep), 1};
                    m_modp->ಕೂಡಿಸು_Stmtsp(elseDeclp);

                    nodep->ಕೂಡಿಸು_Elsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_ಸಾಲು(nodep, "else")));
                }
            }
            m_state = ಕೊನೆಯ_State;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = ನಿಜ;
        iterate_And_ಮುಂದಿನ_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        ಆದರೆ (m_state.ಸಾಲು_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
            ಆದರೆ (m_state.ಸಾಲು_Coverage_On(nodep)) {  // if the case body didn't disable it
                ಸಾಲು_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp
                    = ಹೊಸ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
                m_modp->ಕೂಡಿಸು_Stmtsp(declp);
                nodep->ಕೂಡಿಸು_Stmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_ಸಾಲು(nodep, "case")));
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = ನಿಜ;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        ಆದರೆ (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            ಸಾಲು_Track(nodep);
            AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp
                = ಹೊಸ AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", ಸಾಲುಗಳು_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->ಕೂಡಿಸು_Stmtsp(declp);
            nodep->ಕೂಡಿಸು_Coverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPropSpec* nodep) override {
        VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
        m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStmtPragma* nodep) override {
        ಆದರೆ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_ಖಂಡ_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = ಸುಳ್ಳು;
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_state.m_on) iterateChildren(nodep);
            ಸಾಲು_Track(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        ಆದರೆ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        ಸಾಲು_Track(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_ಒಳ_Toggle_Off);
        VL_RESTORER(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp);
        m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp = nodep;
        m_ಒಳ_Toggle_Off = ನಿಜ;
        ಆದರೆ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        ಸಾಲು_Track(nodep);
    }

    ಶೂನ್ಯ abort_ಅಭಿವ್ಯಕ್ತಿ_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        ಆದರೆ (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    ತಾರ್ಕಿಕ checkMaxExprs(ಗಾತ್ರ_t additional = 0) {
        ಆದರೆ (m_seeking != ABORTED
            && static_cast<ಪೂರ್ಣಾಂಕ>(m_exprs.ಗಾತ್ರ_2() + additional) <= v3Global.opt.coverage_ಅಭಿವ್ಯಕ್ತಿ_Max())
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        abort_ಅಭಿವ್ಯಕ್ತಿ_Coverage();
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಭಿವ್ಯಕ್ತಿ_Cover_Inc(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಂಕ ಆರಂಭ = 0) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಪೂರ್ಣಾಂಕ ಎಣಿಕೆ_2 = ಆರಂಭ;
        ಗಾಗಿ (Cover_ಅಭಿವ್ಯಕ್ತಿ& ಅಭಿವ್ಯಕ್ತಿ : m_exprs) {
            ಸ್ಥಿರಾಂಕ string name = "expr_" + std::ಗಮ್ಯ_string(ಎಣಿಕೆ_2);
            string comment = "(";
            ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Cover_ಪದ& ಪದ : ಅಭಿವ್ಯಕ್ತಿ) {
                comment += (ಮೊದಲ ? "" : " && ") + ಪದ.m_emitV
                           + "==" + (ಪದ.m_objective ? "1" : "0");
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* covExprp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (VN_IS(ಪದ.m_exprp, FuncRef) || ಪದ.m_exprp->isSystemFunc()) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ frefp = ಪದ.m_exprp;
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = frefp->dtypep();
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_funcTemps.emplace(frefp, ಶೂನ್ಯಸೂಚಕ);
                    AstVar* varp = pair.ಮೊದಲ->ಎರಡನೇ;
                    ಆದರೆ (pair.ಎರಡನೇ) {
                        varp = ಹೊಸ AstVar{fl, VVarType::MODULETEMP, m_ಅಭಿವ್ಯಕ್ತಿ_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.ಮೊದಲ->ಎರಡನೇ = varp;
                        ಆದರೆ (m_ftaskp) {
                            varp->funcLocal(ನಿಜ);
                            m_ftaskp->prependStmtsp(varp);
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
                        relinkHandle.relink(ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                            fl, ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            ಹೊಸ AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    covExprp = ಪದ.m_exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp
                    = ಪದ.m_objective ? covExprp : ಹೊಸ AstLogNot{fl, covExprp};
                ಆದರೆ (condp) {
                    condp = ಹೊಸ AstLogAnd{fl, condp, termp};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    condp = termp;
                }
                ಮೊದಲ = ಸುಳ್ಳು;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp = ಹೊಸ AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->ಕೂಡಿಸು_Stmtsp(declp);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = newCoverInc(fl, declp, trace_Name_For_ಸಾಲು(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{fl, condp, newp, ಶೂನ್ಯಸೂಚಕ};
            ifp->user2(ನಿಜ);
            ಸೇರಿಸು_Proc_Statement(m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp, ifp);
            ++ಎಣಿಕೆ_2;
        }
    }

    ಶೂನ್ಯ coverExprs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (!m_state.ಅಭಿವ್ಯಕ್ತಿ_Coverage_On(nodep) || nodep->dtypep()->ಅಗಲ() != 1 || !m_ಅಭಿವ್ಯಕ್ತಿ_Stmtsp) {
            ಹಿಂತಿರುಗಿ;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.ಖಾಲಿಗಳು(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = ಸುಳ್ಳು;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.ಅದಲುಬದಲು(falseExprs);

        m_objective = ನಿಜ;
        iterate(nodep);
        ಆದರೆ (checkMaxExprs(falseExprs.ಗಾತ್ರ_2())) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_seeking == ABORTED) ಹಿಂತಿರುಗಿ;

        ಕೂಡಿಸು_ಅಭಿವ್ಯಕ್ತಿ_Cover_Inc(nodep);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಆರಂಭ = m_exprs.ಗಾತ್ರ_2();
        m_objective = ಸುಳ್ಳು;
        m_exprs.ಅದಲುಬದಲು(falseExprs);
        ಕೂಡಿಸು_ಅಭಿವ್ಯಕ್ತಿ_Cover_Inc(nodep, ಆರಂಭ);
    }

    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Either(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ overrideObjective = ಸುಳ್ಳು, ತಾರ್ಕಿಕ lObjective = ಸುಳ್ಳು,
                    ತಾರ್ಕಿಕ rObjective = ಸುಳ್ಳು) {
        VL_RESTORER(m_objective);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();

        ಆದರೆ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ಆದರೆ (checkMaxExprs()) ಹಿಂತಿರುಗಿ;
        CoverExprs lhsExprs;
        m_exprs.ಅದಲುಬದಲು(lhsExprs);
        ಆದರೆ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.ಕೊನೆ(), lhsExprs);
        checkMaxExprs();
    }

    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Both(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ overrideObjective = ಸುಳ್ಳು, ತಾರ್ಕಿಕ lObjective = ಸುಳ್ಳು,
                  ತಾರ್ಕಿಕ rObjective = ಸುಳ್ಳು) {
        VL_RESTORER(m_objective);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();

        ಆದರೆ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ಆದರೆ (checkMaxExprs()) ಹಿಂತಿರುಗಿ;
        CoverExprs lhsExprs;
        m_exprs.ಅದಲುಬದಲು(lhsExprs);
        ಆದರೆ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        ಆದರೆ (checkMaxExprs()) ಹಿಂತಿರುಗಿ;
        CoverExprs rhsExprs;
        m_exprs.ಅದಲುಬದಲು(rhsExprs);

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Cover_ಅಭಿವ್ಯಕ್ತಿ& l : lhsExprs) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Cover_ಅಭಿವ್ಯಕ್ತಿ& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::ಗಣ<AstVar*>, 2> varps;
                std::array<std::ಗಣ<std::string>, 2> strs;

                UASSERT_OBJ(!l.ಖಾಲಿಗಳು() && !r.ಖಾಲಿಗಳು(), nodep, "Empty coverage expression branch");
                Cover_ಅಭಿವ್ಯಕ್ತಿ ಅಭಿವ್ಯಕ್ತಿ;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Cover_ಪದ& ಪದ : l) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(ಪದ.m_exprp, VarRef)) {
                        varps[ಪದ.m_objective].ಸೇರಿಸು(refp->varp());
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        strs[ಪದ.m_objective].ಸೇರಿಸು(ಪದ.m_emitV);
                    }
                    ಅಭಿವ್ಯಕ್ತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(ಪದ);
                }
                ತಾರ್ಕಿಕ impossible = ಸುಳ್ಳು;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Cover_ಪದ& ಪದ : r) {
                    ತಾರ್ಕಿಕ redundant = ಸುಳ್ಳು;
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp = VN_CAST(ಪದ.m_exprp, ಗಂಟು_Var_Ref)) {
                        ಆದರೆ (varps[ಪದ.m_objective].ಕಂಡುಹಿಡಿ(refp->varp())
                            != varps[ಪದ.m_objective].ಕೊನೆ())
                            redundant = ನಿಜ;
                        ಆದರೆ (varps[!ಪದ.m_objective].ಕಂಡುಹಿಡಿ(refp->varp())
                            != varps[!ಪದ.m_objective].ಕೊನೆ())
                            impossible = ನಿಜ;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಆದರೆ (strs[ಪದ.m_objective].ಕಂಡುಹಿಡಿ(ಪದ.m_emitV)
                            != strs[ಪದ.m_objective].ಕೊನೆ())
                            redundant = ನಿಜ;
                        ಆದರೆ (strs[!ಪದ.m_objective].ಕಂಡುಹಿಡಿ(ಪದ.m_emitV)
                            != strs[!ಪದ.m_objective].ಕೊನೆ())
                            impossible = ನಿಜ;
                    }
                    ಆದರೆ (!redundant) ಅಭಿವ್ಯಕ್ತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(ಪದ);
                }
                ಆದರೆ (!impossible) m_exprs.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(ಅಭಿವ್ಯಕ್ತಿ));
                ಆದರೆ (checkMaxExprs()) ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ or_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_Biop* nodep) {
        ಆದರೆ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_objective) {
            ಅಭಿವ್ಯಕ್ತಿ_Either(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಅಭಿವ್ಯಕ್ತಿ_Both(nodep);
        }
        ಸಾಲು_Track(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogOr* nodep) override { or_ಅಭಿವ್ಯಕ್ತಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstOr* nodep) override { or_ಅಭಿವ್ಯಕ್ತಿ(nodep); }

    ಶೂನ್ಯ and_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_Biop* nodep) {
        ಆದರೆ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_objective) {
            ಅಭಿವ್ಯಕ್ತಿ_Both(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಅಭಿವ್ಯಕ್ತಿ_Either(nodep);
        }
        ಸಾಲು_Track(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogAnd* nodep) override { and_ಅಭಿವ್ಯಕ್ತಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstAnd* nodep) override { and_ಅಭಿವ್ಯಕ್ತಿ(nodep); }

    ಶೂನ್ಯ xor_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_Biop* nodep) {
        ಆದರೆ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lObjective : {ಸುಳ್ಳು, ನಿಜ}) {
                CoverExprs prevExprs;
                m_exprs.ಅದಲುಬದಲು(prevExprs);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rObjective = lObjective ^ m_objective;
                ಅಭಿವ್ಯಕ್ತಿ_Both(nodep, ನಿಜ, lObjective, rObjective);
                m_exprs.splice(m_exprs.ಕೊನೆ(), prevExprs);
                ಆದರೆ (checkMaxExprs()) ನಿಲ್ಲಿಸು;
            }
        }
        ಸಾಲು_Track(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstXor* nodep) override { xor_ಅಭಿವ್ಯಕ್ತಿ(nodep); }

    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Not(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        VL_RESTORER(m_objective);
        ಆದರೆ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_objective = !m_objective;
            iterateChildren(nodep);
            ಸಾಲು_Track(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNot* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Not(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogNot* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Not(nodep); }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Oper>
    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Reduce(Ast_ಗಂಟು_Uniop* nodep) {
        ಆದರೆ (m_seeking != ABORTED) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = lhsp->dtypep()->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t expected = std::is_same<T_Oper, AstXor>::ಮೌಲ್ಯ ? 0x1 << ಅಗಲ : ಅಗಲ + 1;
            ಆದರೆ (checkMaxExprs(expected)) ಹಿಂತಿರುಗಿ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* unrolledp = ಹೊಸ AstSel{
                fl, lhsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, static_cast<uint32_t>(ಅಗಲ - 1)}, 1};
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = ಅಗಲ - 2; ಬಿಟ್_2 >= 0; ಬಿಟ್_2--) {
                AstSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstSel{fl, lhsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                                ಹೊಸ AstConst{fl, static_cast<uint32_t>(ಬಿಟ್_2)}, 1};
                unrolledp = ಹೊಸ T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(unrolledp), unrolledp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
            ಸಾಲು_Track(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedOr* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Reduce<AstOr>(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedAnd* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Reduce<AstAnd>(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedXor* nodep) override { ಅಭಿವ್ಯಕ್ತಿ_Reduce<AstXor>(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(AstLogIf* nodep) override {
        ಆದರೆ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_objective) {
            ಅಭಿವ್ಯಕ್ತಿ_Either(nodep, ನಿಜ, ಸುಳ್ಳು, ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಅಭಿವ್ಯಕ್ತಿ_Both(nodep, ನಿಜ, ನಿಜ, ಸುಳ್ಳು);
        }
        ಸಾಲು_Track(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        ಆದರೆ (m_seeking == NONE) {
            coverExprs(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_objective = !m_objective;
            xor_ಅಭಿವ್ಯಕ್ತಿ(nodep);
            ಸಾಲು_Track(nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstFuncRef* nodep) override {
        ಆದರೆ (nodep->taskp()->lifetime().isAutomatic()) {
            ಭೇಟಿ(static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(nodep));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಅಭಿವ್ಯಕ್ತಿ_Unsupported(nodep, "non-automatic function");
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            ಆದರೆ (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_ವೃಕ್ಷ(nodep, emitV);
                // Add new expression with a single term
                Cover_ಅಭಿವ್ಯಕ್ತಿ ಅಭಿವ್ಯಕ್ತಿ;
                ಅಭಿವ್ಯಕ್ತಿ.emplace_ಹಿಂಭಾಗ(nodep, m_objective, emitV.str());
                m_exprs.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(ಅಭಿವ್ಯಕ್ತಿ));
                checkMaxExprs();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಅಭಿವ್ಯಕ್ತಿ_Unsupported(nodep, "not coverage eligible");
            }
        }
        ಸಾಲು_Track(nodep);
    }

    // VISITORS - BOTH
    ಶೂನ್ಯ ಭೇಟಿ(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ಒಳ_Toggle_Off);
        m_state.m_on = ಸುಳ್ಳು;
        m_ಒಳ_Toggle_Off = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ಒಳ_Toggle_Off);
        m_state.m_on = ಸುಳ್ಳು;
        m_ಒಳ_Toggle_Off = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        iterateChildren(nodep);
        ಸಾಲು_Track(nodep);
    }

    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Unsupported(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ wasSeeking = m_seeking == SEEKING;
        ಸ್ಥಿರಾಂಕ Objective oldSeeking = m_seeking;
        ಆದರೆ (wasSeeking) abort_ಅಭಿವ್ಯಕ್ತಿ_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        ಸಾಲು_Track(nodep);
        ಆದರೆ (!wasSeeking) m_seeking = oldSeeking;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CoverageVisitor(AstNetlist* rootp)
        : m_ಅಭಿವ್ಯಕ್ತಿ_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Coverage class functions

ಶೂನ್ಯ V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("coverage", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
