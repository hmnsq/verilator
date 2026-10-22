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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Coverage.h"

#অন্তর্ভাৱ "V3EmitV.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

#অন্তর্ভাৱ <list>
#অন্তর্ভাৱ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্ৰেণী ৰাশিমালা_Coverage_Eligible_Visitor final : সার্ৱজনিক VNVisitorConst {
    // STATE
    তার্কিক m_eligible = সঁচা;

    স্থির তার্কিক elemDTypeEligible(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtp = dtypep->virtRefDTypep()) {
            যদি (!elemDTypeEligible(dtp)) ঘূৰাই মিছা;
        }
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtp = dtypep->virtRefDType2p()) {
            যদি (!elemDTypeEligible(dtp)) ঘূৰাই মিছা;
        }
        ঘূৰাই !VN_IS(dtypep, ClassRefDType);
    }

    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        যদি (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            m_eligible = মিছা;
        }
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        যদি (!nodep->is_ৰাশিমালা_Coverage_Eligible()) {
            m_eligible = মিছা;
        } অন্যথা {
            iterateChildrenConst(nodep);
        }
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit ৰাশিমালা_Coverage_Eligible_Visitor(Ast_গ্ৰন্থি* nodep) { iterateConst(nodep); }
    ~ৰাশিমালা_Coverage_Eligible_Visitor() override = স্ৱভাৱ;

    তার্কিক eligible() ধ্রুৱ { ঘূৰাই m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

শ্ৰেণী CoverageVisitor final : সার্ৱজনিক VNVisitor {
    // TYPES
    using LinenoSet = std::set<পূর্ণাঙ্ক>;

    সংরচনা CoverTerm final {
        Ast_গ্ৰন্থি_ৰাশিমালা* m_exprp;  // Expression branch term
        তার্কিক m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_গ্ৰন্থি_ৰাশিমালা* exprp, তার্কিক objective, ধ্রুৱ string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_ৰাশিমালা = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_ৰাশিমালা>;

    সংরচনা ToggleEnt final {
        ধ্রুৱ string m_comment;  // Comment for coverage dump
        Ast_গ্ৰন্থি_ৰাশিমালা* m_varRefp;  // How to get to this element
        Ast_গ্ৰন্থি_ৰাশিমালা* m_chgRefp;  // How to get to this element
        ToggleEnt(ধ্রুৱ string& comment, Ast_গ্ৰন্থি_ৰাশিমালা* vp, Ast_গ্ৰন্থি_ৰাশিমালা* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = স্ৱভাৱ;
        শূন্য cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_বৃক্ষ(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_বৃক্ষ(), m_chgRefp = nullptr);
        }
    };

    সংরচনা CheckState final {  // State save-restored on each new coverage scope/block
        তার্কিক m_on = মিছা;  // Should this block get covered?
        তার্কিক m_inModOff = মিছা;  // In module with no coverage
        পূর্ণাঙ্ক m_handle = 0;  // Opaque handle for index into line tracking
        ধ্রুৱ Ast_গ্ৰন্থি* m_nodep = nullptr;  // Node establishing this state
        CheckState() = স্ৱভাৱ;
        তার্কিক coverageOn(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ {
            ঘূৰাই m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        তার্কিক সাৰি_Coverage_On(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ {
            ঘূৰাই coverageOn(nodep) && v3Global.opt.coverage_সাৰি();
        }
        তার্কিক branchCoverageOn(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ {
            ঘূৰাই coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        তার্কিক ৰাশিমালা_Coverage_On(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ {
            ঘূৰাই coverageOn(nodep) && v3Global.opt.coverage_ৰাশিমালা();
        }
    };

    গণনা Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ধ্রুৱ VNUser1InUse m_inuser1;
    ধ্রুৱ VNUser2InUse m_inuser2;
    V3UniqueNames m_ৰাশিমালা_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_গ্ৰন্থি_ৰাশিমালা*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    পূর্ণাঙ্ক m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_গ্ৰন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্ৰন্থি* m_ৰাশিমালা_Stmtsp = nullptr;  // Node to add expr coverage to
    তার্কিক m_then = মিছা;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    তার্কিক m_objective = মিছা;  // Expression objective
    তার্কিক m_ifCond = মিছা;  // Visiting if condition
    তার্কিক m_inToggleOff = মিছা;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<পূর্ণাঙ্ক, LinenoSet> m_handle_সাৰিগুলি;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ধ্রুৱ ৱর্ণ* varIgnoreToggle(ধ্রুৱ AstVar* nodep) {
        ধ্রুৱ তার্কিক cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        যদি (!cover) ঘূৰাই "Not relevant signal";
        যদি (nodep->isConst()) ঘূৰাই "Signal is constant";
        যদি (nodep->isDouble()) ঘূৰাই "Signal is double";
        যদি (nodep->isString()) ঘূৰাই "Signal is string";
        যদি (!v3Global.opt.coverageUnderscore()) {
            ধ্রুৱ string prettyName = nodep->prettyName();
            যদি (prettyName[0] == '_') ঘূৰাই "Leading underscore";
            যদি (prettyName.find("._") != string::npos) ঘূৰাই "Inlined leading underscore";
        }
        যদি ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            ঘূৰাই "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        ঘূৰাই nullptr;
    }

    AstCoverInc* newCoverInc(File_সাৰি* fl, Ast_গ্ৰন্থি_Cover_Decl* ধ্রুৱ declp,
                             ধ্রুৱ string& trace_var_name) {
        AstCoverInc* ধ্রুৱ incp = নূতন AstCoverInc{fl, declp};
        যদি (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_সাৰি* ধ্রুৱ fl_nowarn = নূতন File_সাৰি{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, সঁচা);
            AstVar* ধ্রুৱ varp = নূতন AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(সঁচা);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* ধ্রুৱ assp = নূতন AstAssign{
                incp->fileline(), নূতন AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                নূতন AstAdd{incp->fileline(), নূতন AstVarRef{incp->fileline(), varp, VAccess::READ},
                           নূতন AstConst{incp->fileline(), AstConst::Widthed_মান{}, 32, 1}}};
            Ast_গ্ৰন্থি::addNext<Ast_গ্ৰন্থি, Ast_গ্ৰন্থি>(incp, assp);
        }
        ঘূৰাই incp;
    }
    string trace_Name_For_সাৰি(ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        যদি (ধ্রুৱ uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        ঘূৰাই name;
    }

    // Line tracking
    শূন্য createHandle(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = সঁচা;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    শূন্য সাৰি_Track(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        যদি ((m_state.সাৰি_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            বাবে (পূর্ণাঙ্ক lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_সাৰিগুলি[m_state.m_handle].ঢোকাও(lineno);
            }
        }
    }
    স্থির string সাৰিগুলি_First_Last(ধ্রুৱ পূর্ণাঙ্ক first, ধ্রুৱ পূর্ণাঙ্ক last) {
        যদি (first && first == last) {
            ঘূৰাই cvtToStr(first);
        } অন্যথা যদি (first && last) {
            ঘূৰাই cvtToStr(first) + "-" + cvtToStr(last);
        } অন্যথা {
            ঘূৰাই "";
        }
    }
    string সাৰিগুলি_Cov(ধ্রুৱ CheckState& state, ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        ধ্রুৱ LinenoSet& সাৰিগুলি = m_handle_সাৰিগুলি[state.m_handle];
        পূর্ণাঙ্ক first = 0;
        পূর্ণাঙ্ক last = 0;
        বাবে (ধ্রুৱ পূর্ণাঙ্ক linen : সাৰিগুলি) {
            যদি (!first) {
                first = last = linen;
            } অন্যথা যদি (linen == last + 1) {
                ++last;
            } অন্যথা {
                যদি (!out.empty()) out += ",";
                out += সাৰিগুলি_First_Last(first, last);
                first = last = linen;
            }
        }
        যদি (first) {
            যদি (!out.empty()) out += ",";
            out += সাৰিগুলি_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        ঘূৰাই out;
    }

    // VISITORS - BOTH
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        ধ্রুৱ Ast_গ্ৰন্থি_Module* ধ্রুৱ origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ৰাশিমালা_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_শীৰ্ষ();  // Already made top shell, no coverage for it
        যদি (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_সাৰিগুলি.clear();
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ৰাশিমালা_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    শূন্য visit(AstAlways* nodep) override {
        যদি (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_ৰাশিমালা_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_ৰাশিমালা_Stmtsp = nodep;
            m_inToggleOff = সঁচা;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            ঘূৰাই;
        }
        iterateProcedure(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Procedure* nodep) override { iterateProcedure(nodep); }
    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_ৰাশিমালা_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_ৰাশিমালা_Stmtsp = nodep;
        m_inToggleOff = সঁচা;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        যদি (m_state.সাৰি_Coverage_On(nodep)) {
            AstCoverOtherDecl* ধ্রুৱ declp
                = নূতন AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", সাৰিগুলি_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_সাৰি(nodep, "block"));
            ঢোকাও_Proc_Statement(nodep, newp);
        }
    }
    শূন্য visit(AstLoopTest* nodep) override {
        যদি (nodep->user2SetOnce()) ঘূৰাই;
        সাৰি_Track(nodep);
        যদি (m_state.সাৰি_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ধ্রুৱ declp
                = নূতন AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", সাৰিগুলি_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_সাৰি(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_ৰাশিমালা_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        যদি (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    শূন্য ঢোকাও_Proc_Statement(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* stmtp) {
        যদি (Ast_গ্ৰন্থি_Procedure* ধ্রুৱ itemp = VN_CAST(nodep, গ্ৰন্থি_Procedure)) {
            itemp->addStmtsp(stmtp);
        } অন্যথা যদি (Ast_গ্ৰন্থি_FTask* ধ্রুৱ itemp = VN_CAST(nodep, গ্ৰন্থি_FTask)) {
            itemp->addStmtsp(stmtp);
        } অন্যথা যদি (AstLoop* ধ্রুৱ itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } অন্যথা যদি (AstIf* ধ্রুৱ itemp = VN_CAST(nodep, If)) {
            যদি (m_then) {
                itemp->addThensp(stmtp);
            } অন্যথা {
                itemp->addElsesp(stmtp);
            }
        } অন্যথা যদি (AstBegin* ধ্রুৱ itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } অন্যথা {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    শূন্য iterateProcedure(Ast_গ্ৰন্থি* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ৰাশিমালা_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_ৰাশিমালা_Stmtsp = nodep;
        m_inToggleOff = সঁচা;
        createHandle(nodep);
        iterateChildren(nodep);
        যদি (m_state.সাৰি_Coverage_On(nodep)) {
            সাৰি_Track(nodep);
            AstCoverOtherDecl* ধ্রুৱ declp
                = নূতন AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", সাৰিগুলি_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_সাৰি(nodep, "block"));
            ঢোকাও_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    শূন্য visit(AstVar* nodep) override {
        iterateChildren(nodep);
        যদি (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ধ্রুৱ ৱর্ণ* ধ্রুৱ disablep = varIgnoreToggle(nodep);
            যদি (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } অন্যথা {
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
                ধ্রুৱ string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_সাৰি* ধ্রুৱ fl_nowarn = নূতন File_সাৰি{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, সঁচা);
                AstVar* ধ্রুৱ chgVarp
                    = নূতন AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, নূতন AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 নূতন AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    শূন্য toggleVarBottom(ধ্রুৱ ToggleEnt& above, ধ্রুৱ AstVar* varp, ধ্রুৱ VNumRange& range) {
        ধ্রুৱ std::string hierPrefix
            = (m_beginHier != "") ? Ast_গ্ৰন্থি::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ধ্রুৱ declp
            = নূতন AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* ধ্রুৱ newp = নূতন AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_বৃক্ষ(মিছা), above.m_chgRefp->clone_বৃক্ষ(মিছা)};
        m_modp->addStmtsp(newp);
    }

    শূন্য toggleVarRecurse(ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep, ধ্রুৱ পূর্ণাঙ্ক depth,  // per-iteration
                          ধ্রুৱ ToggleEnt& above, ধ্রুৱ AstVar* ধ্রুৱ varp) {  // Constant
        যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } অন্যথা যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            বাবে (ধ্রুৱ পূর্ণাঙ্ক index_docs : adtypep->declRange().seqLoToHi()) {
                ধ্রুৱ পূর্ণাঙ্ক index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 নূতন AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_বৃক্ষ(মিছা), index_code},
                                 নূতন AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_বৃক্ষ(মিছা), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } অন্যথা যদি (ধ্রুৱ AstPackArrayDType* ধ্রুৱ adtypep = VN_CAST(dtypep, PackArrayDType)) {
            বাবে (ধ্রুৱ পূর্ণাঙ্ক index_docs : adtypep->declRange().seqLoToHi()) {
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = adtypep->subDTypep()->skipRefp();
                ধ্রুৱ পূর্ণাঙ্ক index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 নূতন AstSel{varp->fileline(), above.m_varRefp->clone_বৃক্ষ(মিছা),
                                            index_code * subtypep->width(), subtypep->width()},
                                 নূতন AstSel{varp->fileline(), above.m_chgRefp->clone_বৃক্ষ(মিছা),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } অন্যথা যদি (ধ্রুৱ AstStructDType* ধ্রুৱ adtypep = VN_CAST(dtypep, StructDType)) {
            যদি (adtypep->packed()) {
                বাবে (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = itemp->subDTypep()->skipRefp();
                    ধ্রুৱ পূর্ণাঙ্ক index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        নূতন AstSel{varp->fileline(), above.m_varRefp->clone_বৃক্ষ(মিছা), index_code,
                                   subtypep->width()},
                        নূতন AstSel{varp->fileline(), above.m_chgRefp->clone_বৃক্ষ(মিছা), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } অন্যথা {
                বাবে (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = itemp->subDTypep()->skipRefp();
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ varRefp = নূতন AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_বৃক্ষ(মিছা), itemp->name()};
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ chgRefp = নূতন AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_বৃক্ষ(মিছা), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } অন্যথা যদি (ধ্রুৱ AstUnionDType* ধ্রুৱ adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            যদি (ধ্রুৱ AstMemberDType* ধ্রুৱ itemp = adtypep->membersp()) {
                Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = itemp->subDTypep()->skipRefp();
                যদি (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_বৃক্ষ(মিছা),
                                     above.m_chgRefp->clone_বৃক্ষ(মিছা)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } অন্যথা {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ varRefp = নূতন AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_বৃক্ষ(মিছা), itemp->name()};
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ chgRefp = নূতন AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_বৃক্ষ(মিছা), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } অন্যথা যদি (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } অন্যথা {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    তার্কিক includeCondToBranchRecursive(ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ nodep) {
        ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ backp = nodep->backp();
        যদি (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            ঘূৰাই includeCondToBranchRecursive(backp);
        } অন্যথা যদি (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            ঘূৰাই includeCondToBranchRecursive(backp);
        } অন্যথা যদি (VN_IS(backp, গ্ৰন্থি_Assign) && VN_AS(backp, গ্ৰন্থি_Assign)->rhsp() == nodep) {
            ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }

    // VISITORS - LINE COVERAGE
    শূন্য visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        যদি (m_seeking == NONE) coverExprs(nodep->condp());

        যদি (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ধ্রুৱ CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            সাৰি_Track(nodep);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* ধ্রুৱ thenDeclp
                = নূতন AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", সাৰিগুলি_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(নূতন Ast_ৰাশিমালা_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_সাৰি(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* ধ্রুৱ elseDeclp
                = নূতন AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", সাৰিগুলি_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(নূতন Ast_ৰাশিমালা_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_সাৰি(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } অন্যথা {
            সাৰি_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    শূন্য visit(AstIf* nodep) override {
        যদি (nodep->user2()) ঘূৰাই;

        UINFO(4, " IF: " << nodep);
        যদি (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ধ্রুৱ তার্কিক elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            যদি (elsif) VN_AS(nodep->elsesp(), If)->user1(সঁচা);
            ধ্রুৱ তার্কিক first_elsif = !nodep->user1() && elsif;
            ধ্রুৱ তার্কিক cont_elsif = nodep->user1() && elsif;
            ধ্রুৱ তার্কিক final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ধ্রুৱ CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_ৰাশিমালা_Stmtsp);
                VL_RESTORER(m_then);
                m_ৰাশিমালা_Stmtsp = nodep;
                m_then = সঁচা;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                সাৰি_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_ৰাশিমালা_Stmtsp);
                VL_RESTORER(m_then);
                m_ৰাশিমালা_Stmtsp = nodep;
                m_then = মিছা;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            যদি (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ধ্রুৱ ifDeclp
                    = নূতন AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", সাৰিগুলি_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_সাৰি(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ধ্রুৱ elseDeclp
                    = নূতন AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", সাৰিগুলি_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_সাৰি(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            অন্যথা যদি (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                যদি (ifState.সাৰি_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ধ্রুৱ elsifDeclp = নূতন AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        সাৰিগুলি_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_সাৰি(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } অন্যথা {
                // Cover as separate blocks (not a branch as is not two-legged)
                যদি (ifState.সাৰি_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ধ্রুৱ ifDeclp = নূতন AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        সাৰিগুলি_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_সাৰি(nodep, "if")));
                }
                যদি (elseState.সাৰি_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ধ্রুৱ elseDeclp = নূতন AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        সাৰিগুলি_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_সাৰি(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = সঁচা;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    শূন্য visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        যদি (m_state.সাৰি_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            যদি (m_state.সাৰি_Coverage_On(nodep)) {  // if the case body didn't disable it
                সাৰি_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ধ্রুৱ declp
                    = নূতন AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", সাৰিগুলি_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_সাৰি(nodep, "case")));
            }
        }
    }
    শূন্য visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = সঁচা;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        যদি (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            সাৰি_Track(nodep);
            AstCoverOtherDecl* ধ্রুৱ declp
                = নূতন AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", সাৰিগুলি_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    শূন্য visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_ৰাশিমালা_Stmtsp);
        m_ৰাশিমালা_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    শূন্য visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = মিছা;
    }
    শূন্য visit(AstStmtPragma* nodep) override {
        যদি (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = মিছা;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            যদি (m_state.m_on) iterateChildren(nodep);
            সাৰি_Track(nodep);
        }
    }
    শূন্য visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        যদি (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        সাৰি_Track(nodep);
    }

    শূন্য visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_ৰাশিমালা_Stmtsp);
        m_ৰাশিমালা_Stmtsp = nodep;
        m_inToggleOff = সঁচা;
        যদি (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        সাৰি_Track(nodep);
    }

    শূন্য abort_ৰাশিমালা_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        যদি (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    তার্কিক checkMaxExprs(size_t additional = 0) {
        যদি (m_seeking != ABORTED
            && static_cast<পূর্ণাঙ্ক>(m_exprs.size() + additional) <= v3Global.opt.coverage_ৰাশিমালা_Max())
            ঘূৰাই মিছা;
        abort_ৰাশিমালা_Coverage();
        ঘূৰাই সঁচা;
    }

    শূন্য add_ৰাশিমালা_Cover_Inc(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, পূর্ণাঙ্ক start = 0) {
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        পূর্ণাঙ্ক গুনতি = start;
        বাবে (Cover_ৰাশিমালা& ৰাশিমালা : m_exprs) {
            ধ্রুৱ string name = "expr_" + std::to_string(গুনতি);
            string comment = "(";
            তার্কিক first = সঁচা;
            Ast_গ্ৰন্থি_ৰাশিমালা* condp = nullptr;
            বাবে (CoverTerm& term : ৰাশিমালা) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_গ্ৰন্থি_ৰাশিমালা* covExprp = nullptr;
                যদি (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ frefp = term.m_exprp;
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = frefp->dtypep();
                    ধ্রুৱ auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    যদি (pair.second) {
                        varp = নূতন AstVar{fl, VVarType::MODULETEMP, m_ৰাশিমালা_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        যদি (m_ftaskp) {
                            varp->funcLocal(সঁচা);
                            m_ftaskp->prependStmtsp(varp);
                        } অন্যথা {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(নূতন Ast_ৰাশিমালা_Stmt{
                            fl, নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            নূতন AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = নূতন AstVarRef{fl, varp, VAccess::READ};
                } অন্যথা {
                    covExprp = term.m_exprp->clone_বৃক্ষ(মিছা);
                }
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ termp
                    = term.m_objective ? covExprp : নূতন AstLogNot{fl, covExprp};
                যদি (condp) {
                    condp = নূতন AstLogAnd{fl, condp, termp};
                } অন্যথা {
                    condp = termp;
                }
                first = মিছা;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ধ্রুৱ declp = নূতন AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_গ্ৰন্থি* ধ্রুৱ newp = newCoverInc(fl, declp, trace_Name_For_সাৰি(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ধ্রুৱ ifp = নূতন AstIf{fl, condp, newp, nullptr};
            ifp->user2(সঁচা);
            ঢোকাও_Proc_Statement(m_ৰাশিমালা_Stmtsp, ifp);
            ++গুনতি;
        }
    }

    শূন্য coverExprs(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (!m_state.ৰাশিমালা_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_ৰাশিমালা_Stmtsp) {
            ঘূৰাই;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = মিছা;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = সঁচা;
        iterate(nodep);
        যদি (checkMaxExprs(falseExprs.size())) ঘূৰাই;
        যদি (m_seeking == ABORTED) ঘূৰাই;

        add_ৰাশিমালা_Cover_Inc(nodep);
        ধ্রুৱ পূর্ণাঙ্ক start = m_exprs.size();
        m_objective = মিছা;
        m_exprs.swap(falseExprs);
        add_ৰাশিমালা_Cover_Inc(nodep, start);
    }

    শূন্য ৰাশিমালা_Either(Ast_গ্ৰন্থি_Biop* nodep, তার্কিক overrideObjective = মিছা, তার্কিক lObjective = মিছা,
                    তার্কিক rObjective = মিছা) {
        VL_RESTORER(m_objective);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp();

        যদি (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        যদি (checkMaxExprs()) ঘূৰাই;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        যদি (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    শূন্য ৰাশিমালা_Both(Ast_গ্ৰন্থি_Biop* nodep, তার্কিক overrideObjective = মিছা, তার্কিক lObjective = মিছা,
                  তার্কিক rObjective = মিছা) {
        VL_RESTORER(m_objective);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp();

        যদি (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        যদি (checkMaxExprs()) ঘূৰাই;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        যদি (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        যদি (checkMaxExprs()) ঘূৰাই;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        বাবে (ধ্রুৱ Cover_ৰাশিমালা& l : lhsExprs) {
            বাবে (ধ্রুৱ Cover_ৰাশিমালা& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_ৰাশিমালা ৰাশিমালা;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                বাবে (ধ্রুৱ CoverTerm& term : l) {
                    যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].ঢোকাও(refp->varp());
                    } অন্যথা {
                        strs[term.m_objective].ঢোকাও(term.m_emitV);
                    }
                    ৰাশিমালা.ঠেলো_back(term);
                }
                তার্কিক impossible = মিছা;
                বাবে (ধ্রুৱ CoverTerm& term : r) {
                    তার্কিক redundant = মিছা;
                    যদি (ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ refp = VN_CAST(term.m_exprp, গ্ৰন্থি_Var_Ref)) {
                        যদি (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = সঁচা;
                        যদি (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = সঁচা;
                    } অন্যথা {
                        যদি (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = সঁচা;
                        যদি (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = সঁচা;
                    }
                    যদি (!redundant) ৰাশিমালা.ঠেলো_back(term);
                }
                যদি (!impossible) m_exprs.ঠেলো_back(std::move(ৰাশিমালা));
                যদি (checkMaxExprs()) ঘূৰাই;
            }
        }
    }

    শূন্য or_ৰাশিমালা(Ast_গ্ৰন্থি_Biop* nodep) {
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা যদি (m_objective) {
            ৰাশিমালা_Either(nodep);
        } অন্যথা {
            ৰাশিমালা_Both(nodep);
        }
        সাৰি_Track(nodep);
    }
    শূন্য visit(AstLogOr* nodep) override { or_ৰাশিমালা(nodep); }
    শূন্য visit(AstOr* nodep) override { or_ৰাশিমালা(nodep); }

    শূন্য and_ৰাশিমালা(Ast_গ্ৰন্থি_Biop* nodep) {
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা যদি (m_objective) {
            ৰাশিমালা_Both(nodep);
        } অন্যথা {
            ৰাশিমালা_Either(nodep);
        }
        সাৰি_Track(nodep);
    }
    শূন্য visit(AstLogAnd* nodep) override { and_ৰাশিমালা(nodep); }
    শূন্য visit(AstAnd* nodep) override { and_ৰাশিমালা(nodep); }

    শূন্য xor_ৰাশিমালা(Ast_গ্ৰন্থি_Biop* nodep) {
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা {
            বাবে (ধ্রুৱ তার্কিক lObjective : {মিছা, সঁচা}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                ধ্রুৱ তার্কিক rObjective = lObjective ^ m_objective;
                ৰাশিমালা_Both(nodep, সঁচা, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                যদি (checkMaxExprs()) ভঙ্গ;
            }
        }
        সাৰি_Track(nodep);
    }
    শূন্য visit(AstXor* nodep) override { xor_ৰাশিমালা(nodep); }

    শূন্য ৰাশিমালা_Not(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        VL_RESTORER(m_objective);
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা {
            m_objective = !m_objective;
            iterateChildren(nodep);
            সাৰি_Track(nodep);
        }
    }
    শূন্য visit(AstNot* nodep) override { ৰাশিমালা_Not(nodep); }
    শূন্য visit(AstLogNot* nodep) override { ৰাশিমালা_Not(nodep); }

    template <typename T_Oper>
    শূন্য ৰাশিমালা_Reduce(Ast_গ্ৰন্থি_Uniop* nodep) {
        যদি (m_seeking != ABORTED) {
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp();
            ধ্রুৱ পূর্ণাঙ্ক width = lhsp->dtypep()->width();
            ধ্রুৱ size_t expected = std::is_same<T_Oper, AstXor>::মান ? 0x1 << width : width + 1;
            যদি (checkMaxExprs(expected)) ঘূৰাই;
            Ast_গ্ৰন্থি_ৰাশিমালা* unrolledp = নূতন AstSel{
                fl, lhsp->clone_বৃক্ষ(মিছা), নূতন AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            বাবে (পূর্ণাঙ্ক bit = width - 2; bit >= 0; bit--) {
                AstSel* ধ্রুৱ selp = নূতন AstSel{fl, lhsp->clone_বৃক্ষ(মিছা),
                                                নূতন AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = নূতন T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ঠেলো_Deletep(unrolledp), unrolledp);
        } অন্যথা {
            iterateChildren(nodep);
            সাৰি_Track(nodep);
        }
    }
    শূন্য visit(AstRedOr* nodep) override { ৰাশিমালা_Reduce<AstOr>(nodep); }
    শূন্য visit(AstRedAnd* nodep) override { ৰাশিমালা_Reduce<AstAnd>(nodep); }
    শূন্য visit(AstRedXor* nodep) override { ৰাশিমালা_Reduce<AstXor>(nodep); }

    শূন্য visit(AstLogIf* nodep) override {
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা যদি (m_objective) {
            ৰাশিমালা_Either(nodep, সঁচা, মিছা, সঁচা);
        } অন্যথা {
            ৰাশিমালা_Both(nodep, সঁচা, সঁচা, মিছা);
        }
        সাৰি_Track(nodep);
    }

    শূন্য visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা {
            m_objective = !m_objective;
            xor_ৰাশিমালা(nodep);
            সাৰি_Track(nodep);
        }
    }

    শূন্য visit(AstFuncRef* nodep) override {
        যদি (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(nodep));
        } অন্যথা {
            ৰাশিমালা_Unsupported(nodep, "non-automatic function");
        }
    }

    শূন্য visit(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) override {
        যদি (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } অন্যথা {
            ৰাশিমালা_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            যদি (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_বৃক্ষ(nodep, emitV);
                // Add new expression with a single term
                Cover_ৰাশিমালা ৰাশিমালা;
                ৰাশিমালা.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.ঠেলো_back(std::move(ৰাশিমালা));
                checkMaxExprs();
            } অন্যথা {
                ৰাশিমালা_Unsupported(nodep, "not coverage eligible");
            }
        }
        সাৰি_Track(nodep);
    }

    // VISITORS - BOTH
    শূন্য visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = মিছা;
        m_inToggleOff = সঁচা;
        iterateChildren(nodep);
    }
    শূন্য visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = মিছা;
        m_inToggleOff = সঁচা;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        iterateChildren(nodep);
        সাৰি_Track(nodep);
    }

    শূন্য ৰাশিমালা_Unsupported(Ast_গ্ৰন্থি* nodep, ধ্রুৱ string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ধ্রুৱ তার্কিক wasSeeking = m_seeking == SEEKING;
        ধ্রুৱ Objective oldSeeking = m_seeking;
        যদি (wasSeeking) abort_ৰাশিমালা_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        সাৰি_Track(nodep);
        যদি (!wasSeeking) m_seeking = oldSeeking;
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_ৰাশিমালা_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Coverage class functions

শূন্য V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("coverage", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
