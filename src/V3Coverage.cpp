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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Coverage.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ ਵਿਅੰਜਕ_Coverage_Eligible_Visitor final : ਜਨਤਕ VNVisitorConst {
    // STATE
    ਤਾਰਕਿਕ m_eligible = ਸੱਚ;

    ਸਥਿਰ ਤਾਰਕਿਕ elemDTypeEligible(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtp = dtypep->virtRefDTypep()) {
            ਜੇ (!elemDTypeEligible(dtp)) ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtp = dtypep->virtRefDType2p()) {
            ਜੇ (!elemDTypeEligible(dtp)) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ !VN_IS(dtypep, ClassRefDType);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        ਜੇ (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } ਨਹੀਂਤਾਂ {
            m_eligible = ਝੂਠ;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (!nodep->is_ਵਿਅੰਜਕ_Coverage_Eligible()) {
            m_eligible = ਝੂਠ;
        } ਨਹੀਂਤਾਂ {
            iterateChildrenConst(nodep);
        }
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਵਿਅੰਜਕ_Coverage_Eligible_Visitor(Ast_ਗੰਢ* nodep) { iterateConst(nodep); }
    ~ਵਿਅੰਜਕ_Coverage_Eligible_Visitor() override = ਮੂਲ;

    ਤਾਰਕਿਕ eligible() ਅਚਲ { ਵਾਪਸ m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

ਵਰਗ CoverageVisitor final : ਜਨਤਕ VNVisitor {
    // TYPES
    ਵਰਤੋਂ Lineno_ਸੈੱਟ = std::ਸੈੱਟ<ਪੂਰਨ>;

    ਬਣਤਰ Cover_ਪਦ final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_exprp;  // Expression branch term
        ਤਾਰਕਿਕ m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        Cover_ਪਦ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਤਾਰਕਿਕ objective, ਅਚਲ string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    ਵਰਤੋਂ Cover_ਵਿਅੰਜਕ = std::deque<Cover_ਪਦ>;
    ਵਰਤੋਂ CoverExprs = std::list<Cover_ਵਿਅੰਜਕ>;

    ਬਣਤਰ ToggleEnt final {
        ਅਚਲ string m_comment;  // Comment for coverage dump
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_varRefp;  // How to get to this element
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_chgRefp;  // How to get to this element
        ToggleEnt(ਅਚਲ string& comment, Ast_ਗੰਢ_ਵਿਅੰਜਕ* vp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = ਮੂਲ;
        ਖਾਲੀ cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_ਰੁੱਖ(), m_varRefp = ਖਾਲੀ_ਸੂਚਕ);
            VL_DO_CLEAR(m_chgRefp->delete_ਰੁੱਖ(), m_chgRefp = ਖਾਲੀ_ਸੂਚਕ);
        }
    };

    ਬਣਤਰ CheckState final {  // State save-restored on each new coverage scope/block
        ਤਾਰਕਿਕ m_on = ਝੂਠ;  // Should this block get covered?
        ਤਾਰਕਿਕ m_ਵਿੱਚ_Mod_Off = ਝੂਠ;  // In module with no coverage
        ਪੂਰਨ m_handle = 0;  // Opaque handle for index into line tracking
        ਅਚਲ Ast_ਗੰਢ* m_nodep = ਖਾਲੀ_ਸੂਚਕ;  // Node establishing this state
        CheckState() = ਮੂਲ;
        ਤਾਰਕਿਕ coverageOn(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ {
            ਵਾਪਸ m_on && !m_ਵਿੱਚ_Mod_Off && nodep->fileline()->coverageOn();
        }
        ਤਾਰਕਿਕ ਸਤਰ_Coverage_On(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ {
            ਵਾਪਸ coverageOn(nodep) && v3Global.opt.coverage_ਸਤਰ();
        }
        ਤਾਰਕਿਕ branchCoverageOn(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ {
            ਵਾਪਸ coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        ਤਾਰਕਿਕ ਵਿਅੰਜਕ_Coverage_On(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ {
            ਵਾਪਸ coverageOn(nodep) && v3Global.opt.coverage_ਵਿਅੰਜਕ();
        }
    };

    ਗਿਣਤੀ Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    V3_Unique_ਨਾਂ m_ਵਿਅੰਜਕ_ਆਰਜ਼ੀ_ਨਾਂ;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    ਪੂਰਨ m_ਅਗਲਾ_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module to add statement to
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    Ast_ਗੰਢ* m_ਵਿਅੰਜਕ_Stmtsp = ਖਾਲੀ_ਸੂਚਕ;  // Node to add expr coverage to
    ਤਾਰਕਿਕ m_then = ਝੂਠ;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    ਤਾਰਕਿਕ m_objective = ਝੂਠ;  // Expression objective
    ਤਾਰਕਿਕ m_ifCond = ਝੂਠ;  // Visiting if condition
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Toggle_Off = ਝੂਠ;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<ਪੂਰਨ, Lineno_ਸੈੱਟ> m_handle_ਸਤਰਾਂ;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ਅਚਲ ਅੱਖਰ* varIgnoreToggle(ਅਚਲ AstVar* nodep) {
        ਅਚਲ ਤਾਰਕਿਕ cover = nodep->isIO() || (nodep->isSignal() && nodep->is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic());
        ਜੇ (!cover) ਵਾਪਸ "Not relevant signal";
        ਜੇ (nodep->isConst()) ਵਾਪਸ "Signal is constant";
        ਜੇ (nodep->isDouble()) ਵਾਪਸ "Signal is double";
        ਜੇ (nodep->isString()) ਵਾਪਸ "Signal is string";
        ਜੇ (!v3Global.opt.coverageUnderscore()) {
            ਅਚਲ string prettyName = nodep->prettyName();
            ਜੇ (prettyName[0] == '_') ਵਾਪਸ "Leading underscore";
            ਜੇ (prettyName.ਲੱਭੋ("._") != string::npos) ਵਾਪਸ "Inlined leading underscore";
        }
        ਜੇ ((nodep->ਚੌੜਾਈ() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverage_Max_ਚੌੜਾਈ())) {
            ਵਾਪਸ "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    AstCoverInc* newCoverInc(File_ਸਤਰ* fl, Ast_ਗੰਢ_Cover_Decl* ਅਚਲ declp,
                             ਅਚਲ string& trace_var_name) {
        AstCoverInc* ਅਚਲ incp = ਨਵਾਂ AstCoverInc{fl, declp};
        ਜੇ (!trace_var_name.ਖਾਲੀ_ਥਾਵਾਂ()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_ਸਤਰ* ਅਚਲ fl_nowarn = ਨਵਾਂ File_ਸਤਰ{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDSIGNAL, ਸੱਚ);
            AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->ਲੱਭੋ_UInt32_DType()};
            varp->ਸੈੱਟ_Ignore_Sched_Write();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(ਸੱਚ);
            m_modp->ਜੋੜੋ_Stmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            Ast_ਸੌਂਪੋ* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                incp->fileline(), ਨਵਾਂ AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                ਨਵਾਂ Ast_ਜੋੜੋ{incp->fileline(), ਨਵਾਂ AstVarRef{incp->fileline(), varp, VAccess::READ},
                           ਨਵਾਂ AstConst{incp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 32, 1}}};
            Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(incp, assp);
        }
        ਵਾਪਸ incp;
    }
    string trace_Name_For_ਸਤਰ(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()) + "_" + type;
        ਜੇ (ਅਚਲ uint32_t suffix = m_varnames[name]++) name += "_" + cvt_ਮੰਜ਼ਿਲ_Str(suffix);
        ਵਾਪਸ name;
    }

    // Line tracking
    ਖਾਲੀ createHandle(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = ਸੱਚ;
        m_state.m_handle = ++m_ਅਗਲਾ_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    ਖਾਲੀ ਸਤਰ_Track(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        ਜੇ ((m_state.ਸਤਰ_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            ਲਈ (ਪੂਰਨ lineno = nodep->fileline()->ਪਹਿਲਾ_Lineno();
                 lineno <= nodep->fileline()->ਆਖਰੀ_Lineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_ਸਤਰਾਂ[m_state.m_handle].ਪਾਓ(lineno);
            }
        }
    }
    ਸਥਿਰ string ਸਤਰਾਂ_ਪਹਿਲਾ_ਆਖਰੀ(ਅਚਲ ਪੂਰਨ ਪਹਿਲਾ, ਅਚਲ ਪੂਰਨ ਆਖਰੀ) {
        ਜੇ (ਪਹਿਲਾ && ਪਹਿਲਾ == ਆਖਰੀ) {
            ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Str(ਪਹਿਲਾ);
        } ਨਹੀਂਤਾਂ ਜੇ (ਪਹਿਲਾ && ਆਖਰੀ) {
            ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Str(ਪਹਿਲਾ) + "-" + cvt_ਮੰਜ਼ਿਲ_Str(ਆਖਰੀ);
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ "";
        }
    }
    string ਸਤਰਾਂ_Cov(ਅਚਲ CheckState& state, ਅਚਲ Ast_ਗੰਢ* nodep) {
        // Return comma separated list of ranged numbers
        string ਬਾਹਰਲਾ;
        ਅਚਲ Lineno_ਸੈੱਟ& ਸਤਰਾਂ = m_handle_ਸਤਰਾਂ[state.m_handle];
        ਪੂਰਨ ਪਹਿਲਾ = 0;
        ਪੂਰਨ ਆਖਰੀ = 0;
        ਲਈ (ਅਚਲ ਪੂਰਨ linen : ਸਤਰਾਂ) {
            ਜੇ (!ਪਹਿਲਾ) {
                ਪਹਿਲਾ = ਆਖਰੀ = linen;
            } ਨਹੀਂਤਾਂ ਜੇ (linen == ਆਖਰੀ + 1) {
                ++ਆਖਰੀ;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (!ਬਾਹਰਲਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਬਾਹਰਲਾ += ",";
                ਬਾਹਰਲਾ += ਸਤਰਾਂ_ਪਹਿਲਾ_ਆਖਰੀ(ਪਹਿਲਾ, ਆਖਰੀ);
                ਪਹਿਲਾ = ਆਖਰੀ = linen;
            }
        }
        ਜੇ (ਪਹਿਲਾ) {
            ਜੇ (!ਬਾਹਰਲਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਬਾਹਰਲਾ += ",";
            ਬਾਹਰਲਾ += ਸਤਰਾਂ_ਪਹਿਲਾ_ਆਖਰੀ(ਪਹਿਲਾ, ਆਖਰੀ);
        }
        UINFO(9, "lines out " << ਬਾਹਰਲਾ << " for h" << state.m_handle << " " << nodep);
        ਵਾਪਸ ਬਾਹਰਲਾ;
    }

    // VISITORS - BOTH
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ਵਿਅੰਜਕ_ਆਰਜ਼ੀ_ਨਾਂ);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_ਵਿੱਚ_Mod_Off = nodep->is_ਸਿਖਰ();  // Already made top shell, no coverage for it
        ਜੇ (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_ਸਤਰਾਂ.clear();
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ਵਿਅੰਜਕ_ਆਰਜ਼ੀ_ਨਾਂ);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_ਵਿੱਚ_Mod_Off = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (nodep->keyword() == VAlwaysKwd::CONT_ਸੌਂਪੋ) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_ਵਿਅੰਜਕ_Stmtsp);
            VL_RESTORER(m_ਵਿੱਚ_Toggle_Off);
            m_ਵਿਅੰਜਕ_Stmtsp = nodep;
            m_ਵਿੱਚ_Toggle_Off = ਸੱਚ;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            ਵਾਪਸ;
        }
        iterateProcedure(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override { iterateProcedure(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_ਵਿਅੰਜਕ_Stmtsp);
        VL_RESTORER(m_ਵਿੱਚ_Toggle_Off);
        m_ਵਿਅੰਜਕ_Stmtsp = nodep;
        m_ਵਿੱਚ_Toggle_Off = ਸੱਚ;
        createHandle(nodep);
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        ਜੇ (m_state.ਸਤਰ_Coverage_On(nodep)) {
            AstCoverOtherDecl* ਅਚਲ declp
                = ਨਵਾਂ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ਸਤਰਾਂ_Cov(m_state, nodep), 0};
            m_modp->ਜੋੜੋ_Stmtsp(declp);
            Ast_ਗੰਢ* ਅਚਲ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ਸਤਰ(nodep, "block"));
            ਪਾਓ_Proc_Statement(nodep, newp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        ਜੇ (nodep->user2_ਸੈੱਟ_Once()) ਵਾਪਸ;
        ਸਤਰ_Track(nodep);
        ਜੇ (m_state.ਸਤਰ_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ਅਚਲ declp
                = ਨਵਾਂ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ਸਤਰਾਂ_Cov(m_state, nodep), 0};
            m_modp->ਜੋੜੋ_Stmtsp(declp);
            Ast_ਗੰਢ* ਅਚਲ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ਸਤਰ(nodep, "block"));
            nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_ਵਿਅੰਜਕ_ਆਰਜ਼ੀ_ਨਾਂ);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        ਜੇ (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    ਖਾਲੀ ਪਾਓ_Proc_Statement(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* stmtp) {
        ਜੇ (Ast_ਗੰਢ_Procedure* ਅਚਲ itemp = VN_CAST(nodep, ਗੰਢ_Procedure)) {
            itemp->ਜੋੜੋ_Stmtsp(stmtp);
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ itemp = VN_CAST(nodep, ਗੰਢ_FTask)) {
            itemp->ਜੋੜੋ_Stmtsp(stmtp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstLoop* ਅਚਲ itemp = VN_CAST(nodep, Loop)) {
            itemp->ਜੋੜੋ_Stmtsp(stmtp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstIf* ਅਚਲ itemp = VN_CAST(nodep, If)) {
            ਜੇ (m_then) {
                itemp->ਜੋੜੋ_Thensp(stmtp);
            } ਨਹੀਂਤਾਂ {
                itemp->ਜੋੜੋ_Elsesp(stmtp);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstBegin* ਅਚਲ itemp = VN_CAST(nodep, Begin)) {
            itemp->ਜੋੜੋ_Stmtsp(stmtp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    ਖਾਲੀ iterateProcedure(Ast_ਗੰਢ* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ਵਿਅੰਜਕ_Stmtsp);
        VL_RESTORER(m_ਵਿੱਚ_Toggle_Off);
        m_ਵਿਅੰਜਕ_Stmtsp = nodep;
        m_ਵਿੱਚ_Toggle_Off = ਸੱਚ;
        createHandle(nodep);
        iterateChildren(nodep);
        ਜੇ (m_state.ਸਤਰ_Coverage_On(nodep)) {
            ਸਤਰ_Track(nodep);
            AstCoverOtherDecl* ਅਚਲ declp
                = ਨਵਾਂ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ਸਤਰਾਂ_Cov(m_state, nodep), 0};
            m_modp->ਜੋੜੋ_Stmtsp(declp);
            Ast_ਗੰਢ* ਅਚਲ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ਸਤਰ(nodep, "block"));
            ਪਾਓ_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_modp && !m_ਵਿੱਚ_Toggle_Off && !m_state.m_ਵਿੱਚ_Mod_Off && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ਅਚਲ ਅੱਖਰ* ਅਚਲ disablep = varIgnoreToggle(nodep);
            ਜੇ (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } ਨਹੀਂਤਾਂ {
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
                ਅਚਲ string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_ਸਤਰ* ਅਚਲ fl_nowarn = ਨਵਾਂ File_ਸਤਰ{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDSIGNAL, ਸੱਚ);
                AstVar* ਅਚਲ chgVarp
                    = ਨਵਾਂ AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->ਜੋੜੋ_Stmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, ਨਵਾਂ AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 ਨਵਾਂ AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    ਖਾਲੀ toggleVarBottom(ਅਚਲ ToggleEnt& above, ਅਚਲ AstVar* varp, ਅਚਲ VNumRange& range) {
        ਅਚਲ std::string hier_ਅਗੇਤਰ
            = (m_beginHier != "") ? Ast_ਗੰਢ::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ਅਚਲ declp
            = ਨਵਾਂ AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hier_ਅਗੇਤਰ + varp->name() + above.m_comment, range};
        m_modp->ਜੋੜੋ_Stmtsp(declp);
        AstCoverToggle* ਅਚਲ newp = ਨਵਾਂ AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_ਰੁੱਖ(ਝੂਠ), above.m_chgRefp->clone_ਰੁੱਖ(ਝੂਠ)};
        m_modp->ਜੋੜੋ_Stmtsp(newp);
    }

    ਖਾਲੀ toggleVarRecurse(ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep, ਅਚਲ ਪੂਰਨ ਡੂੰਘਾਈ,  // per-iteration
                          ਅਚਲ ToggleEnt& above, ਅਚਲ AstVar* ਅਚਲ varp) {  // Constant
        ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            ਲਈ (ਅਚਲ ਪੂਰਨ ਸੂਚਕਾਂਕ_docs : adtypep->declRange().seq_Lo_ਮੰਜ਼ਿਲ_Hi()) {
                ਅਚਲ ਪੂਰਨ ਸੂਚਕਾਂਕ_ਕੋਡ = ਸੂਚਕਾਂਕ_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_ਮੰਜ਼ਿਲ_Str(ਸੂਚਕਾਂਕ_docs) + "]",
                                 ਨਵਾਂ AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_ਰੁੱਖ(ਝੂਠ), ਸੂਚਕਾਂਕ_ਕੋਡ},
                                 ਨਵਾਂ AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_ਰੁੱਖ(ਝੂਠ), ਸੂਚਕਾਂਕ_ਕੋਡ}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), ਡੂੰਘਾਈ + 1, newent, varp);
                newent.cleanup();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstPackArrayDType* ਅਚਲ adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ਲਈ (ਅਚਲ ਪੂਰਨ ਸੂਚਕਾਂਕ_docs : adtypep->declRange().seq_Lo_ਮੰਜ਼ਿਲ_Hi()) {
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ subtypep = adtypep->subDTypep()->skipRefp();
                ਅਚਲ ਪੂਰਨ ਸੂਚਕਾਂਕ_ਕੋਡ = ਸੂਚਕਾਂਕ_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_ਮੰਜ਼ਿਲ_Str(ਸੂਚਕਾਂਕ_docs) + "]",
                                 ਨਵਾਂ AstSel{varp->fileline(), above.m_varRefp->clone_ਰੁੱਖ(ਝੂਠ),
                                            ਸੂਚਕਾਂਕ_ਕੋਡ * subtypep->ਚੌੜਾਈ(), subtypep->ਚੌੜਾਈ()},
                                 ਨਵਾਂ AstSel{varp->fileline(), above.m_chgRefp->clone_ਰੁੱਖ(ਝੂਠ),
                                            ਸੂਚਕਾਂਕ_ਕੋਡ * subtypep->ਚੌੜਾਈ(), subtypep->ਚੌੜਾਈ()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), ਡੂੰਘਾਈ + 1, newent, varp);
                newent.cleanup();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstStructDType* ਅਚਲ adtypep = VN_CAST(dtypep, StructDType)) {
            ਜੇ (adtypep->packed()) {
                ਲਈ (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ subtypep = itemp->subDTypep()->skipRefp();
                    ਅਚਲ ਪੂਰਨ ਸੂਚਕਾਂਕ_ਕੋਡ = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        ਨਵਾਂ AstSel{varp->fileline(), above.m_varRefp->clone_ਰੁੱਖ(ਝੂਠ), ਸੂਚਕਾਂਕ_ਕੋਡ,
                                   subtypep->ਚੌੜਾਈ()},
                        ਨਵਾਂ AstSel{varp->fileline(), above.m_chgRefp->clone_ਰੁੱਖ(ਝੂਠ), ਸੂਚਕਾਂਕ_ਕੋਡ,
                                   subtypep->ਚੌੜਾਈ()}};
                    toggleVarRecurse(subtypep, ਡੂੰਘਾਈ + 1, newent, varp);
                    newent.cleanup();
                }
            } ਨਹੀਂਤਾਂ {
                ਲਈ (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_ਗੰਢ_DType* ਅਚਲ subtypep = itemp->subDTypep()->skipRefp();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ varRefp = ਨਵਾਂ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ਰੁੱਖ(ਝੂਠ), itemp->name()};
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ chgRefp = ਨਵਾਂ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ਰੁੱਖ(ਝੂਠ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, ਡੂੰਘਾਈ + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnionDType* ਅਚਲ adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            ਜੇ (ਅਚਲ AstMemberDType* ਅਚਲ itemp = adtypep->membersp()) {
                Ast_ਗੰਢ_DType* ਅਚਲ subtypep = itemp->subDTypep()->skipRefp();
                ਜੇ (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_ਰੁੱਖ(ਝੂਠ),
                                     above.m_chgRefp->clone_ਰੁੱਖ(ਝੂਠ)};
                    toggleVarRecurse(subtypep, ਡੂੰਘਾਈ + 1, newent, varp);
                    newent.cleanup();
                } ਨਹੀਂਤਾਂ {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ varRefp = ਨਵਾਂ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ਰੁੱਖ(ਝੂਠ), itemp->name()};
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ chgRefp = ਨਵਾਂ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ਰੁੱਖ(ਝੂਠ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, ਡੂੰਘਾਈ + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, ਕਤਾਰ_DType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } ਨਹੀਂਤਾਂ {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    ਤਾਰਕਿਕ include_Cond_ਮੰਜ਼ਿਲ_Branch_Recursive(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep) {
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ backp = nodep->backp();
        ਜੇ (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            ਵਾਪਸ include_Cond_ਮੰਜ਼ਿਲ_Branch_Recursive(backp);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            ਵਾਪਸ include_Cond_ਮੰਜ਼ਿਲ_Branch_Recursive(backp);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(backp, ਗੰਢ_ਸੌਂਪੋ) && VN_AS(backp, ਗੰਢ_ਸੌਂਪੋ)->rhsp() == nodep) {
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // VISITORS - LINE COVERAGE
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        ਜੇ (m_seeking == NONE) coverExprs(nodep->condp());

        ਜੇ (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && include_Cond_ਮੰਜ਼ਿਲ_Branch_Recursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ਅਚਲ CheckState ਆਖਰੀ_State = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            ਸਤਰ_Track(nodep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thenp = nodep->thenp()->unlink_Fr_ਪਿੱਠ();
            AstCoverOtherDecl* ਅਚਲ thenDeclp
                = ਨਵਾਂ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", ਸਤਰਾਂ_Cov(m_state, nodep), 0};
            m_modp->ਜੋੜੋ_Stmtsp(thenDeclp);
            nodep->thenp(ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_ਸਤਰ(nodep, "cond_then")),
                thenp});
            m_state = ਆਖਰੀ_State;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elsep = nodep->elsep()->unlink_Fr_ਪਿੱਠ();
            AstCoverOtherDecl* ਅਚਲ elseDeclp
                = ਨਵਾਂ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", ਸਤਰਾਂ_Cov(m_state, nodep), 1};
            m_modp->ਜੋੜੋ_Stmtsp(elseDeclp);
            nodep->elsep(ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_ਸਤਰ(nodep, "cond_else")),
                elsep});

            m_state = ਆਖਰੀ_State;
        } ਨਹੀਂਤਾਂ {
            ਸਤਰ_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override {
        ਜੇ (nodep->user2()) ਵਾਪਸ;

        UINFO(4, " IF: " << nodep);
        ਜੇ (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ਅਚਲ ਤਾਰਕਿਕ elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            ਜੇ (elsif) VN_AS(nodep->elsesp(), If)->user1(ਸੱਚ);
            ਅਚਲ ਤਾਰਕਿਕ ਪਹਿਲਾ_elsif = !nodep->user1() && elsif;
            ਅਚਲ ਤਾਰਕਿਕ cont_elsif = nodep->user1() && elsif;
            ਅਚਲ ਤਾਰਕਿਕ final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ਅਚਲ CheckState ਆਖਰੀ_State = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_ਵਿਅੰਜਕ_Stmtsp);
                VL_RESTORER(m_then);
                m_ਵਿਅੰਜਕ_Stmtsp = nodep;
                m_then = ਸੱਚ;
                createHandle(nodep);
                iterate_And_ਅਗਲਾ_Null(nodep->thensp());
                ਸਤਰ_Track(nodep);
                ifState = m_state;
            }
            m_state = ਆਖਰੀ_State;
            {
                VL_RESTORER(m_ਵਿਅੰਜਕ_Stmtsp);
                VL_RESTORER(m_then);
                m_ਵਿਅੰਜਕ_Stmtsp = nodep;
                m_then = ਝੂਠ;
                createHandle(nodep);
                iterate_And_ਅਗਲਾ_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = ਆਖਰੀ_State;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            ਜੇ (!(ਪਹਿਲਾ_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ਅਚਲ ifDeclp
                    = ਨਵਾਂ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", ਸਤਰਾਂ_Cov(ifState, nodep), 0};
                m_modp->ਜੋੜੋ_Stmtsp(ifDeclp);
                nodep->ਜੋੜੋ_Thensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_ਸਤਰ(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ਅਚਲ elseDeclp
                    = ਨਵਾਂ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", ਸਤਰਾਂ_Cov(elseState, nodep), 1};
                m_modp->ਜੋੜੋ_Stmtsp(elseDeclp);
                nodep->ਜੋੜੋ_Elsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_ਸਤਰ(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            ਨਹੀਂਤਾਂ ਜੇ (ਪਹਿਲਾ_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                ਜੇ (ifState.ਸਤਰ_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ਅਚਲ elsifDeclp = ਨਵਾਂ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        ਸਤਰਾਂ_Cov(ifState, nodep), 0};
                    m_modp->ਜੋੜੋ_Stmtsp(elsifDeclp);

                    nodep->ਜੋੜੋ_Thensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_ਸਤਰ(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } ਨਹੀਂਤਾਂ {
                // Cover as separate blocks (not a branch as is not two-legged)
                ਜੇ (ifState.ਸਤਰ_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ਅਚਲ ifDeclp = ਨਵਾਂ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        ਸਤਰਾਂ_Cov(ifState, nodep), 0};
                    m_modp->ਜੋੜੋ_Stmtsp(ifDeclp);
                    nodep->ਜੋੜੋ_Thensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_ਸਤਰ(nodep, "if")));
                }
                ਜੇ (elseState.ਸਤਰ_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ਅਚਲ elseDeclp = ਨਵਾਂ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        ਸਤਰਾਂ_Cov(elseState, nodep), 1};
                    m_modp->ਜੋੜੋ_Stmtsp(elseDeclp);

                    nodep->ਜੋੜੋ_Elsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_ਸਤਰ(nodep, "else")));
                }
            }
            m_state = ਆਖਰੀ_State;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = ਸੱਚ;
        iterate_And_ਅਗਲਾ_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        ਜੇ (m_state.ਸਤਰ_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
            ਜੇ (m_state.ਸਤਰ_Coverage_On(nodep)) {  // if the case body didn't disable it
                ਸਤਰ_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ਅਚਲ declp
                    = ਨਵਾਂ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", ਸਤਰਾਂ_Cov(m_state, nodep), 0};
                m_modp->ਜੋੜੋ_Stmtsp(declp);
                nodep->ਜੋੜੋ_Stmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_ਸਤਰ(nodep, "case")));
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = ਸੱਚ;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        ਜੇ (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            ਸਤਰ_Track(nodep);
            AstCoverOtherDecl* ਅਚਲ declp
                = ਨਵਾਂ AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", ਸਤਰਾਂ_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->ਜੋੜੋ_Stmtsp(declp);
            nodep->ਜੋੜੋ_Coverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPropSpec* nodep) override {
        VL_RESTORER(m_ਵਿਅੰਜਕ_Stmtsp);
        m_ਵਿਅੰਜਕ_Stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = ਝੂਠ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstStmtPragma* nodep) override {
        ਜੇ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_ਖੰਡ_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = ਝੂਠ;
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_state.m_on) iterateChildren(nodep);
            ਸਤਰ_Track(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        ਜੇ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        ਸਤਰ_Track(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_ਵਿੱਚ_Toggle_Off);
        VL_RESTORER(m_ਵਿਅੰਜਕ_Stmtsp);
        m_ਵਿਅੰਜਕ_Stmtsp = nodep;
        m_ਵਿੱਚ_Toggle_Off = ਸੱਚ;
        ਜੇ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        ਸਤਰ_Track(nodep);
    }

    ਖਾਲੀ abort_ਵਿਅੰਜਕ_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        ਜੇ (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    ਤਾਰਕਿਕ checkMaxExprs(ਮਾਪ_t additional = 0) {
        ਜੇ (m_seeking != ABORTED
            && static_cast<ਪੂਰਨ>(m_exprs.ਮਾਪ() + additional) <= v3Global.opt.coverage_ਵਿਅੰਜਕ_Max())
            ਵਾਪਸ ਝੂਠ;
        abort_ਵਿਅੰਜਕ_Coverage();
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਜੋੜੋ_ਵਿਅੰਜਕ_Cover_Inc(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਪੂਰਨ ਸ਼ੁਰੂ = 0) {
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਪੂਰਨ ਗਿਣਤੀ_2 = ਸ਼ੁਰੂ;
        ਲਈ (Cover_ਵਿਅੰਜਕ& ਵਿਅੰਜਕ : m_exprs) {
            ਅਚਲ string name = "expr_" + std::ਮੰਜ਼ਿਲ_string(ਗਿਣਤੀ_2);
            string comment = "(";
            ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Cover_ਪਦ& ਪਦ : ਵਿਅੰਜਕ) {
                comment += (ਪਹਿਲਾ ? "" : " && ") + ਪਦ.m_emitV
                           + "==" + (ਪਦ.m_objective ? "1" : "0");
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* covExprp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (VN_IS(ਪਦ.m_exprp, FuncRef) || ਪਦ.m_exprp->isSystemFunc()) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ frefp = ਪਦ.m_exprp;
                    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = frefp->dtypep();
                    ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_funcTemps.emplace(frefp, ਖਾਲੀ_ਸੂਚਕ);
                    AstVar* varp = pair.ਪਹਿਲਾ->ਦੂਜਾ;
                    ਜੇ (pair.ਦੂਜਾ) {
                        varp = ਨਵਾਂ AstVar{fl, VVarType::MODULETEMP, m_ਵਿਅੰਜਕ_ਆਰਜ਼ੀ_ਨਾਂ.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.ਪਹਿਲਾ->ਦੂਜਾ = varp;
                        ਜੇ (m_ftaskp) {
                            varp->funcLocal(ਸੱਚ);
                            m_ftaskp->prependStmtsp(varp);
                        } ਨਹੀਂਤਾਂ {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlink_Fr_ਪਿੱਠ(&relinkHandle);
                        relinkHandle.relink(ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{
                            fl, ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
                } ਨਹੀਂਤਾਂ {
                    covExprp = ਪਦ.m_exprp->clone_ਰੁੱਖ(ਝੂਠ);
                }
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp
                    = ਪਦ.m_objective ? covExprp : ਨਵਾਂ AstLogNot{fl, covExprp};
                ਜੇ (condp) {
                    condp = ਨਵਾਂ AstLogAnd{fl, condp, termp};
                } ਨਹੀਂਤਾਂ {
                    condp = termp;
                }
                ਪਹਿਲਾ = ਝੂਠ;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ਅਚਲ declp = ਨਵਾਂ AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->ਜੋੜੋ_Stmtsp(declp);
            Ast_ਗੰਢ* ਅਚਲ newp = newCoverInc(fl, declp, trace_Name_For_ਸਤਰ(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{fl, condp, newp, ਖਾਲੀ_ਸੂਚਕ};
            ifp->user2(ਸੱਚ);
            ਪਾਓ_Proc_Statement(m_ਵਿਅੰਜਕ_Stmtsp, ifp);
            ++ਗਿਣਤੀ_2;
        }
    }

    ਖਾਲੀ coverExprs(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (!m_state.ਵਿਅੰਜਕ_Coverage_On(nodep) || nodep->dtypep()->ਚੌੜਾਈ() != 1 || !m_ਵਿਅੰਜਕ_Stmtsp) {
            ਵਾਪਸ;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = ਝੂਠ;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.ਅਦਲਾ_ਬਦਲੀ(falseExprs);

        m_objective = ਸੱਚ;
        iterate(nodep);
        ਜੇ (checkMaxExprs(falseExprs.ਮਾਪ())) ਵਾਪਸ;
        ਜੇ (m_seeking == ABORTED) ਵਾਪਸ;

        ਜੋੜੋ_ਵਿਅੰਜਕ_Cover_Inc(nodep);
        ਅਚਲ ਪੂਰਨ ਸ਼ੁਰੂ = m_exprs.ਮਾਪ();
        m_objective = ਝੂਠ;
        m_exprs.ਅਦਲਾ_ਬਦਲੀ(falseExprs);
        ਜੋੜੋ_ਵਿਅੰਜਕ_Cover_Inc(nodep, ਸ਼ੁਰੂ);
    }

    ਖਾਲੀ ਵਿਅੰਜਕ_Either(Ast_ਗੰਢ_Biop* nodep, ਤਾਰਕਿਕ overrideObjective = ਝੂਠ, ਤਾਰਕਿਕ lObjective = ਝੂਠ,
                    ਤਾਰਕਿਕ rObjective = ਝੂਠ) {
        VL_RESTORER(m_objective);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp();

        ਜੇ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ਜੇ (checkMaxExprs()) ਵਾਪਸ;
        CoverExprs lhsExprs;
        m_exprs.ਅਦਲਾ_ਬਦਲੀ(lhsExprs);
        ਜੇ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.ਸਿਰਾ(), lhsExprs);
        checkMaxExprs();
    }

    ਖਾਲੀ ਵਿਅੰਜਕ_Both(Ast_ਗੰਢ_Biop* nodep, ਤਾਰਕਿਕ overrideObjective = ਝੂਠ, ਤਾਰਕਿਕ lObjective = ਝੂਠ,
                  ਤਾਰਕਿਕ rObjective = ਝੂਠ) {
        VL_RESTORER(m_objective);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp();

        ਜੇ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ਜੇ (checkMaxExprs()) ਵਾਪਸ;
        CoverExprs lhsExprs;
        m_exprs.ਅਦਲਾ_ਬਦਲੀ(lhsExprs);
        ਜੇ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        ਜੇ (checkMaxExprs()) ਵਾਪਸ;
        CoverExprs rhsExprs;
        m_exprs.ਅਦਲਾ_ਬਦਲੀ(rhsExprs);

        ਲਈ (ਅਚਲ Cover_ਵਿਅੰਜਕ& l : lhsExprs) {
            ਲਈ (ਅਚਲ Cover_ਵਿਅੰਜਕ& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::ਸੈੱਟ<AstVar*>, 2> varps;
                std::array<std::ਸੈੱਟ<std::string>, 2> strs;

                UASSERT_OBJ(!l.ਖਾਲੀ_ਥਾਵਾਂ() && !r.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Empty coverage expression branch");
                Cover_ਵਿਅੰਜਕ ਵਿਅੰਜਕ;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                ਲਈ (ਅਚਲ Cover_ਪਦ& ਪਦ : l) {
                    ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(ਪਦ.m_exprp, VarRef)) {
                        varps[ਪਦ.m_objective].ਪਾਓ(refp->varp());
                    } ਨਹੀਂਤਾਂ {
                        strs[ਪਦ.m_objective].ਪਾਓ(ਪਦ.m_emitV);
                    }
                    ਵਿਅੰਜਕ.ਧੱਕੋ_ਪਿੱਠ(ਪਦ);
                }
                ਤਾਰਕਿਕ impossible = ਝੂਠ;
                ਲਈ (ਅਚਲ Cover_ਪਦ& ਪਦ : r) {
                    ਤਾਰਕਿਕ redundant = ਝੂਠ;
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp = VN_CAST(ਪਦ.m_exprp, ਗੰਢ_Var_Ref)) {
                        ਜੇ (varps[ਪਦ.m_objective].ਲੱਭੋ(refp->varp())
                            != varps[ਪਦ.m_objective].ਸਿਰਾ())
                            redundant = ਸੱਚ;
                        ਜੇ (varps[!ਪਦ.m_objective].ਲੱਭੋ(refp->varp())
                            != varps[!ਪਦ.m_objective].ਸਿਰਾ())
                            impossible = ਸੱਚ;
                    } ਨਹੀਂਤਾਂ {
                        ਜੇ (strs[ਪਦ.m_objective].ਲੱਭੋ(ਪਦ.m_emitV)
                            != strs[ਪਦ.m_objective].ਸਿਰਾ())
                            redundant = ਸੱਚ;
                        ਜੇ (strs[!ਪਦ.m_objective].ਲੱਭੋ(ਪਦ.m_emitV)
                            != strs[!ਪਦ.m_objective].ਸਿਰਾ())
                            impossible = ਸੱਚ;
                    }
                    ਜੇ (!redundant) ਵਿਅੰਜਕ.ਧੱਕੋ_ਪਿੱਠ(ਪਦ);
                }
                ਜੇ (!impossible) m_exprs.ਧੱਕੋ_ਪਿੱਠ(std::move(ਵਿਅੰਜਕ));
                ਜੇ (checkMaxExprs()) ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ or_ਵਿਅੰਜਕ(Ast_ਗੰਢ_Biop* nodep) {
        ਜੇ (m_seeking == NONE) {
            coverExprs(nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (m_objective) {
            ਵਿਅੰਜਕ_Either(nodep);
        } ਨਹੀਂਤਾਂ {
            ਵਿਅੰਜਕ_Both(nodep);
        }
        ਸਤਰ_Track(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogOr* nodep) override { or_ਵਿਅੰਜਕ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstOr* nodep) override { or_ਵਿਅੰਜਕ(nodep); }

    ਖਾਲੀ and_ਵਿਅੰਜਕ(Ast_ਗੰਢ_Biop* nodep) {
        ਜੇ (m_seeking == NONE) {
            coverExprs(nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (m_objective) {
            ਵਿਅੰਜਕ_Both(nodep);
        } ਨਹੀਂਤਾਂ {
            ਵਿਅੰਜਕ_Either(nodep);
        }
        ਸਤਰ_Track(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogAnd* nodep) override { and_ਵਿਅੰਜਕ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstAnd* nodep) override { and_ਵਿਅੰਜਕ(nodep); }

    ਖਾਲੀ xor_ਵਿਅੰਜਕ(Ast_ਗੰਢ_Biop* nodep) {
        ਜੇ (m_seeking == NONE) {
            coverExprs(nodep);
        } ਨਹੀਂਤਾਂ {
            ਲਈ (ਅਚਲ ਤਾਰਕਿਕ lObjective : {ਝੂਠ, ਸੱਚ}) {
                CoverExprs prevExprs;
                m_exprs.ਅਦਲਾ_ਬਦਲੀ(prevExprs);
                ਅਚਲ ਤਾਰਕਿਕ rObjective = lObjective ^ m_objective;
                ਵਿਅੰਜਕ_Both(nodep, ਸੱਚ, lObjective, rObjective);
                m_exprs.splice(m_exprs.ਸਿਰਾ(), prevExprs);
                ਜੇ (checkMaxExprs()) ਰੋਕੋ;
            }
        }
        ਸਤਰ_Track(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstXor* nodep) override { xor_ਵਿਅੰਜਕ(nodep); }

    ਖਾਲੀ ਵਿਅੰਜਕ_Not(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        VL_RESTORER(m_objective);
        ਜੇ (m_seeking == NONE) {
            coverExprs(nodep);
        } ਨਹੀਂਤਾਂ {
            m_objective = !m_objective;
            iterateChildren(nodep);
            ਸਤਰ_Track(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstNot* nodep) override { ਵਿਅੰਜਕ_Not(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLogNot* nodep) override { ਵਿਅੰਜਕ_Not(nodep); }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Oper>
    ਖਾਲੀ ਵਿਅੰਜਕ_Reduce(Ast_ਗੰਢ_Uniop* nodep) {
        ਜੇ (m_seeking != ABORTED) {
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = lhsp->dtypep()->ਚੌੜਾਈ();
            ਅਚਲ ਮਾਪ_t expected = std::is_same<T_Oper, AstXor>::ਮੁੱਲ ? 0x1 << ਚੌੜਾਈ : ਚੌੜਾਈ + 1;
            ਜੇ (checkMaxExprs(expected)) ਵਾਪਸ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* unrolledp = ਨਵਾਂ AstSel{
                fl, lhsp->clone_ਰੁੱਖ(ਝੂਠ), ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(ਚੌੜਾਈ - 1)}, 1};
            ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਚੌੜਾਈ - 2; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2--) {
                AstSel* ਅਚਲ selp = ਨਵਾਂ AstSel{fl, lhsp->clone_ਰੁੱਖ(ਝੂਠ),
                                                ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(ਦੋਆਧਾਰੀ_ਅੰਕ_2)}, 1};
                unrolledp = ਨਵਾਂ T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(unrolledp), unrolledp);
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
            ਸਤਰ_Track(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRedOr* nodep) override { ਵਿਅੰਜਕ_Reduce<AstOr>(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstRedAnd* nodep) override { ਵਿਅੰਜਕ_Reduce<AstAnd>(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstRedXor* nodep) override { ਵਿਅੰਜਕ_Reduce<AstXor>(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstLogIf* nodep) override {
        ਜੇ (m_seeking == NONE) {
            coverExprs(nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (m_objective) {
            ਵਿਅੰਜਕ_Either(nodep, ਸੱਚ, ਝੂਠ, ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            ਵਿਅੰਜਕ_Both(nodep, ਸੱਚ, ਸੱਚ, ਝੂਠ);
        }
        ਸਤਰ_Track(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        ਜੇ (m_seeking == NONE) {
            coverExprs(nodep);
        } ਨਹੀਂਤਾਂ {
            m_objective = !m_objective;
            xor_ਵਿਅੰਜਕ(nodep);
            ਸਤਰ_Track(nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstFuncRef* nodep) override {
        ਜੇ (nodep->taskp()->lifetime().isAutomatic()) {
            ਦੇਖੋ(static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(nodep));
        } ਨਹੀਂਤਾਂ {
            ਵਿਅੰਜਕ_Unsupported(nodep, "non-automatic function");
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            ਵਿਅੰਜਕ_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            ਜੇ (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_ਰੁੱਖ(nodep, emitV);
                // Add new expression with a single term
                Cover_ਵਿਅੰਜਕ ਵਿਅੰਜਕ;
                ਵਿਅੰਜਕ.emplace_ਪਿੱਠ(nodep, m_objective, emitV.str());
                m_exprs.ਧੱਕੋ_ਪਿੱਠ(std::move(ਵਿਅੰਜਕ));
                checkMaxExprs();
            } ਨਹੀਂਤਾਂ {
                ਵਿਅੰਜਕ_Unsupported(nodep, "not coverage eligible");
            }
        }
        ਸਤਰ_Track(nodep);
    }

    // VISITORS - BOTH
    ਖਾਲੀ ਦੇਖੋ(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ਵਿੱਚ_Toggle_Off);
        m_state.m_on = ਝੂਠ;
        m_ਵਿੱਚ_Toggle_Off = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ਵਿੱਚ_Toggle_Off);
        m_state.m_on = ਝੂਠ;
        m_ਵਿੱਚ_Toggle_Off = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        iterateChildren(nodep);
        ਸਤਰ_Track(nodep);
    }

    ਖਾਲੀ ਵਿਅੰਜਕ_Unsupported(Ast_ਗੰਢ* nodep, ਅਚਲ string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ਅਚਲ ਤਾਰਕਿਕ wasSeeking = m_seeking == SEEKING;
        ਅਚਲ Objective oldSeeking = m_seeking;
        ਜੇ (wasSeeking) abort_ਵਿਅੰਜਕ_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        ਸਤਰ_Track(nodep);
        ਜੇ (!wasSeeking) m_seeking = oldSeeking;
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CoverageVisitor(AstNetlist* rootp)
        : m_ਵਿਅੰਜਕ_ਆਰਜ਼ੀ_ਨਾਂ{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = ਮੂਲ;
};

//######################################################################
// Coverage class functions

ਖਾਲੀ V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("coverage", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
