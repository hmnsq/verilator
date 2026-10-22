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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Coverage.h"

#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <list>
#অন্তর্ভাব <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ রাশিমালা_Coverage_Eligible_Visitor final : সার্বজনিক VNVisitorConst {
    // STATE
    তার্কিক m_eligible = সত্য;

    স্থির তার্কিক elemDTypeEligible(ধ্রুব Ast_গ্রন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtp = dtypep->virtRefDTypep()) {
            যদি (!elemDTypeEligible(dtp)) প্রত্যাগম অসত্য;
        }
        যদি (ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtp = dtypep->virtRefDType2p()) {
            যদি (!elemDTypeEligible(dtp)) প্রত্যাগম অসত্য;
        }
        প্রত্যাগম !VN_IS(dtypep, ClassRefDType);
    }

    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        যদি (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            m_eligible = অসত্য;
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (!nodep->is_রাশিমালা_Coverage_Eligible()) {
            m_eligible = অসত্য;
        } অন্যথা {
            iterateChildrenConst(nodep);
        }
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit রাশিমালা_Coverage_Eligible_Visitor(Ast_গ্রন্থি* nodep) { iterateConst(nodep); }
    ~রাশিমালা_Coverage_Eligible_Visitor() override = স্বভাব;

    তার্কিক eligible() ধ্রুব { প্রত্যাগম m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

বর্গ CoverageVisitor final : সার্বজনিক VNVisitor {
    // TYPES
    using LinenoSet = std::set<পূর্ণাঙ্ক>;

    সংরচনা CoverTerm final {
        Ast_গ্রন্থি_রাশিমালা* m_exprp;  // Expression branch term
        তার্কিক m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_গ্রন্থি_রাশিমালা* exprp, তার্কিক objective, ধ্রুব string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_রাশিমালা = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_রাশিমালা>;

    সংরচনা ToggleEnt final {
        ধ্রুব string m_comment;  // Comment for coverage dump
        Ast_গ্রন্থি_রাশিমালা* m_varRefp;  // How to get to this element
        Ast_গ্রন্থি_রাশিমালা* m_chgRefp;  // How to get to this element
        ToggleEnt(ধ্রুব string& comment, Ast_গ্রন্থি_রাশিমালা* vp, Ast_গ্রন্থি_রাশিমালা* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = স্বভাব;
        শূন্য cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_বৃক্ষ(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_বৃক্ষ(), m_chgRefp = nullptr);
        }
    };

    সংরচনা CheckState final {  // State save-restored on each new coverage scope/block
        তার্কিক m_on = অসত্য;  // Should this block get covered?
        তার্কিক m_inModOff = অসত্য;  // In module with no coverage
        পূর্ণাঙ্ক m_handle = 0;  // Opaque handle for index into line tracking
        ধ্রুব Ast_গ্রন্থি* m_nodep = nullptr;  // Node establishing this state
        CheckState() = স্বভাব;
        তার্কিক coverageOn(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব {
            প্রত্যাগম m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        তার্কিক সারি_Coverage_On(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব {
            প্রত্যাগম coverageOn(nodep) && v3Global.opt.coverage_সারি();
        }
        তার্কিক branchCoverageOn(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব {
            প্রত্যাগম coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        তার্কিক রাশিমালা_Coverage_On(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব {
            প্রত্যাগম coverageOn(nodep) && v3Global.opt.coverage_রাশিমালা();
        }
    };

    গণনা Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;
    V3UniqueNames m_রাশিমালা_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_গ্রন্থি_রাশিমালা*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    পূর্ণাঙ্ক m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্রন্থি* m_রাশিমালা_Stmtsp = nullptr;  // Node to add expr coverage to
    তার্কিক m_then = অসত্য;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    তার্কিক m_objective = অসত্য;  // Expression objective
    তার্কিক m_ifCond = অসত্য;  // Visiting if condition
    তার্কিক m_inToggleOff = অসত্য;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<পূর্ণাঙ্ক, LinenoSet> m_handle_সারিগুলি;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ধ্রুব বর্ণ* varIgnoreToggle(ধ্রুব AstVar* nodep) {
        ধ্রুব তার্কিক cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        যদি (!cover) প্রত্যাগম "Not relevant signal";
        যদি (nodep->isConst()) প্রত্যাগম "Signal is constant";
        যদি (nodep->isDouble()) প্রত্যাগম "Signal is double";
        যদি (nodep->isString()) প্রত্যাগম "Signal is string";
        যদি (!v3Global.opt.coverageUnderscore()) {
            ধ্রুব string prettyName = nodep->prettyName();
            যদি (prettyName[0] == '_') প্রত্যাগম "Leading underscore";
            যদি (prettyName.find("._") != string::npos) প্রত্যাগম "Inlined leading underscore";
        }
        যদি ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            প্রত্যাগম "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        প্রত্যাগম nullptr;
    }

    AstCoverInc* newCoverInc(File_সারি* fl, Ast_গ্রন্থি_Cover_Decl* ধ্রুব declp,
                             ধ্রুব string& trace_var_name) {
        AstCoverInc* ধ্রুব incp = নূতন AstCoverInc{fl, declp};
        যদি (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_সারি* ধ্রুব fl_nowarn = নূতন File_সারি{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, সত্য);
            AstVar* ধ্রুব varp = নূতন AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(সত্য);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* ধ্রুব assp = নূতন AstAssign{
                incp->fileline(), নূতন AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                নূতন AstAdd{incp->fileline(), নূতন AstVarRef{incp->fileline(), varp, VAccess::READ},
                           নূতন AstConst{incp->fileline(), AstConst::Widthed_মান{}, 32, 1}}};
            Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(incp, assp);
        }
        প্রত্যাগম incp;
    }
    string trace_Name_For_সারি(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        যদি (ধ্রুব uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        প্রত্যাগম name;
    }

    // Line tracking
    শূন্য createHandle(ধ্রুব Ast_গ্রন্থি* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = সত্য;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    শূন্য সারি_Track(ধ্রুব Ast_গ্রন্থি* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        যদি ((m_state.সারি_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            প্রতি (পূর্ণাঙ্ক lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_সারিগুলি[m_state.m_handle].ঢোকাও(lineno);
            }
        }
    }
    স্থির string সারিগুলি_First_Last(ধ্রুব পূর্ণাঙ্ক first, ধ্রুব পূর্ণাঙ্ক last) {
        যদি (first && first == last) {
            প্রত্যাগম cvtToStr(first);
        } অন্যথা যদি (first && last) {
            প্রত্যাগম cvtToStr(first) + "-" + cvtToStr(last);
        } অন্যথা {
            প্রত্যাগম "";
        }
    }
    string সারিগুলি_Cov(ধ্রুব CheckState& state, ধ্রুব Ast_গ্রন্থি* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        ধ্রুব LinenoSet& সারিগুলি = m_handle_সারিগুলি[state.m_handle];
        পূর্ণাঙ্ক first = 0;
        পূর্ণাঙ্ক last = 0;
        প্রতি (ধ্রুব পূর্ণাঙ্ক linen : সারিগুলি) {
            যদি (!first) {
                first = last = linen;
            } অন্যথা যদি (linen == last + 1) {
                ++last;
            } অন্যথা {
                যদি (!out.empty()) out += ",";
                out += সারিগুলি_First_Last(first, last);
                first = last = linen;
            }
        }
        যদি (first) {
            যদি (!out.empty()) out += ",";
            out += সারিগুলি_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        প্রত্যাগম out;
    }

    // VISITORS - BOTH
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_রাশিমালা_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_শীর্ষ();  // Already made top shell, no coverage for it
        যদি (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_সারিগুলি.clear();
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_রাশিমালা_Temp_Names);
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
            VL_RESTORER(m_রাশিমালা_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_রাশিমালা_Stmtsp = nodep;
            m_inToggleOff = সত্য;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            প্রত্যাগম;
        }
        iterateProcedure(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override { iterateProcedure(nodep); }
    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_রাশিমালা_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_রাশিমালা_Stmtsp = nodep;
        m_inToggleOff = সত্য;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        যদি (m_state.সারি_Coverage_On(nodep)) {
            AstCoverOtherDecl* ধ্রুব declp
                = নূতন AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", সারিগুলি_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_গ্রন্থি* ধ্রুব newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_সারি(nodep, "block"));
            ঢোকাও_Proc_Statement(nodep, newp);
        }
    }
    শূন্য visit(AstLoopTest* nodep) override {
        যদি (nodep->user2SetOnce()) প্রত্যাগম;
        সারি_Track(nodep);
        যদি (m_state.সারি_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ধ্রুব declp
                = নূতন AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", সারিগুলি_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_গ্রন্থি* ধ্রুব newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_সারি(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_রাশিমালা_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        যদি (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    শূন্য ঢোকাও_Proc_Statement(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* stmtp) {
        যদি (Ast_গ্রন্থি_Procedure* ধ্রুব itemp = VN_CAST(nodep, গ্রন্থি_Procedure)) {
            itemp->addStmtsp(stmtp);
        } অন্যথা যদি (Ast_গ্রন্থি_FTask* ধ্রুব itemp = VN_CAST(nodep, গ্রন্থি_FTask)) {
            itemp->addStmtsp(stmtp);
        } অন্যথা যদি (AstLoop* ধ্রুব itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } অন্যথা যদি (AstIf* ধ্রুব itemp = VN_CAST(nodep, If)) {
            যদি (m_then) {
                itemp->addThensp(stmtp);
            } অন্যথা {
                itemp->addElsesp(stmtp);
            }
        } অন্যথা যদি (AstBegin* ধ্রুব itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } অন্যথা {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    শূন্য iterateProcedure(Ast_গ্রন্থি* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_রাশিমালা_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_রাশিমালা_Stmtsp = nodep;
        m_inToggleOff = সত্য;
        createHandle(nodep);
        iterateChildren(nodep);
        যদি (m_state.সারি_Coverage_On(nodep)) {
            সারি_Track(nodep);
            AstCoverOtherDecl* ধ্রুব declp
                = নূতন AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", সারিগুলি_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_গ্রন্থি* ধ্রুব newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_সারি(nodep, "block"));
            ঢোকাও_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    শূন্য visit(AstVar* nodep) override {
        iterateChildren(nodep);
        যদি (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ধ্রুব বর্ণ* ধ্রুব disablep = varIgnoreToggle(nodep);
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
                ধ্রুব string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_সারি* ধ্রুব fl_nowarn = নূতন File_সারি{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, সত্য);
                AstVar* ধ্রুব chgVarp
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

    শূন্য toggleVarBottom(ধ্রুব ToggleEnt& above, ধ্রুব AstVar* varp, ধ্রুব VNumRange& range) {
        ধ্রুব std::string hierPrefix
            = (m_beginHier != "") ? Ast_গ্রন্থি::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ধ্রুব declp
            = নূতন AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* ধ্রুব newp = নূতন AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_বৃক্ষ(অসত্য), above.m_chgRefp->clone_বৃক্ষ(অসত্য)};
        m_modp->addStmtsp(newp);
    }

    শূন্য toggleVarRecurse(ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep, ধ্রুব পূর্ণাঙ্ক depth,  // per-iteration
                          ধ্রুব ToggleEnt& above, ধ্রুব AstVar* ধ্রুব varp) {  // Constant
        যদি (ধ্রুব AstBasicDType* ধ্রুব basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } অন্যথা যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            প্রতি (ধ্রুব পূর্ণাঙ্ক index_docs : adtypep->declRange().seqLoToHi()) {
                ধ্রুব পূর্ণাঙ্ক index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 নূতন AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_বৃক্ষ(অসত্য), index_code},
                                 নূতন AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_বৃক্ষ(অসত্য), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } অন্যথা যদি (ধ্রুব AstPackArrayDType* ধ্রুব adtypep = VN_CAST(dtypep, PackArrayDType)) {
            প্রতি (ধ্রুব পূর্ণাঙ্ক index_docs : adtypep->declRange().seqLoToHi()) {
                ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব subtypep = adtypep->subDTypep()->skipRefp();
                ধ্রুব পূর্ণাঙ্ক index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 নূতন AstSel{varp->fileline(), above.m_varRefp->clone_বৃক্ষ(অসত্য),
                                            index_code * subtypep->width(), subtypep->width()},
                                 নূতন AstSel{varp->fileline(), above.m_chgRefp->clone_বৃক্ষ(অসত্য),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } অন্যথা যদি (ধ্রুব AstStructDType* ধ্রুব adtypep = VN_CAST(dtypep, StructDType)) {
            যদি (adtypep->packed()) {
                প্রতি (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব subtypep = itemp->subDTypep()->skipRefp();
                    ধ্রুব পূর্ণাঙ্ক index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        নূতন AstSel{varp->fileline(), above.m_varRefp->clone_বৃক্ষ(অসত্য), index_code,
                                   subtypep->width()},
                        নূতন AstSel{varp->fileline(), above.m_chgRefp->clone_বৃক্ষ(অসত্য), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } অন্যথা {
                প্রতি (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_গ্রন্থি_DType* ধ্রুব subtypep = itemp->subDTypep()->skipRefp();
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব varRefp = নূতন AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_বৃক্ষ(অসত্য), itemp->name()};
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব chgRefp = নূতন AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_বৃক্ষ(অসত্য), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } অন্যথা যদি (ধ্রুব AstUnionDType* ধ্রুব adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            যদি (ধ্রুব AstMemberDType* ধ্রুব itemp = adtypep->membersp()) {
                Ast_গ্রন্থি_DType* ধ্রুব subtypep = itemp->subDTypep()->skipRefp();
                যদি (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_বৃক্ষ(অসত্য),
                                     above.m_chgRefp->clone_বৃক্ষ(অসত্য)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } অন্যথা {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব varRefp = নূতন AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_বৃক্ষ(অসত্য), itemp->name()};
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব chgRefp = নূতন AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_বৃক্ষ(অসত্য), itemp->name()};
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
    তার্কিক includeCondToBranchRecursive(ধ্রুব Ast_গ্রন্থি* ধ্রুব nodep) {
        ধ্রুব Ast_গ্রন্থি* ধ্রুব backp = nodep->backp();
        যদি (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            প্রত্যাগম includeCondToBranchRecursive(backp);
        } অন্যথা যদি (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            প্রত্যাগম includeCondToBranchRecursive(backp);
        } অন্যথা যদি (VN_IS(backp, গ্রন্থি_Assign) && VN_AS(backp, গ্রন্থি_Assign)->rhsp() == nodep) {
            প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
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

            ধ্রুব CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            সারি_Track(nodep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* ধ্রুব thenDeclp
                = নূতন AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", সারিগুলি_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(নূতন Ast_রাশিমালা_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_সারি(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* ধ্রুব elseDeclp
                = নূতন AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", সারিগুলি_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(নূতন Ast_রাশিমালা_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_সারি(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } অন্যথা {
            সারি_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    শূন্য visit(AstIf* nodep) override {
        যদি (nodep->user2()) প্রত্যাগম;

        UINFO(4, " IF: " << nodep);
        যদি (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ধ্রুব তার্কিক elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            যদি (elsif) VN_AS(nodep->elsesp(), If)->user1(সত্য);
            ধ্রুব তার্কিক first_elsif = !nodep->user1() && elsif;
            ধ্রুব তার্কিক cont_elsif = nodep->user1() && elsif;
            ধ্রুব তার্কিক final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ধ্রুব CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_রাশিমালা_Stmtsp);
                VL_RESTORER(m_then);
                m_রাশিমালা_Stmtsp = nodep;
                m_then = সত্য;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                সারি_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_রাশিমালা_Stmtsp);
                VL_RESTORER(m_then);
                m_রাশিমালা_Stmtsp = nodep;
                m_then = অসত্য;
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
                AstCoverOtherDecl* ধ্রুব ifDeclp
                    = নূতন AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", সারিগুলি_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_সারি(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ধ্রুব elseDeclp
                    = নূতন AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", সারিগুলি_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_সারি(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            অন্যথা যদি (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                যদি (ifState.সারি_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ধ্রুব elsifDeclp = নূতন AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        সারিগুলি_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_সারি(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } অন্যথা {
                // Cover as separate blocks (not a branch as is not two-legged)
                যদি (ifState.সারি_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ধ্রুব ifDeclp = নূতন AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        সারিগুলি_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_সারি(nodep, "if")));
                }
                যদি (elseState.সারি_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ধ্রুব elseDeclp = নূতন AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        সারিগুলি_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_সারি(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = সত্য;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    শূন্য visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        যদি (m_state.সারি_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            যদি (m_state.সারি_Coverage_On(nodep)) {  // if the case body didn't disable it
                সারি_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ধ্রুব declp
                    = নূতন AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", সারিগুলি_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_সারি(nodep, "case")));
            }
        }
    }
    শূন্য visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = সত্য;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        যদি (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            সারি_Track(nodep);
            AstCoverOtherDecl* ধ্রুব declp
                = নূতন AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", সারিগুলি_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    শূন্য visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_রাশিমালা_Stmtsp);
        m_রাশিমালা_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    শূন্য visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = অসত্য;
    }
    শূন্য visit(AstStmtPragma* nodep) override {
        যদি (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = অসত্য;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            যদি (m_state.m_on) iterateChildren(nodep);
            সারি_Track(nodep);
        }
    }
    শূন্য visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        যদি (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        সারি_Track(nodep);
    }

    শূন্য visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_রাশিমালা_Stmtsp);
        m_রাশিমালা_Stmtsp = nodep;
        m_inToggleOff = সত্য;
        যদি (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        সারি_Track(nodep);
    }

    শূন্য abort_রাশিমালা_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        যদি (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    তার্কিক checkMaxExprs(size_t additional = 0) {
        যদি (m_seeking != ABORTED
            && static_cast<পূর্ণাঙ্ক>(m_exprs.size() + additional) <= v3Global.opt.coverage_রাশিমালা_Max())
            প্রত্যাগম অসত্য;
        abort_রাশিমালা_Coverage();
        প্রত্যাগম সত্য;
    }

    শূন্য add_রাশিমালা_Cover_Inc(Ast_গ্রন্থি_রাশিমালা* nodep, পূর্ণাঙ্ক start = 0) {
        File_সারি* ধ্রুব fl = nodep->fileline();
        পূর্ণাঙ্ক গুনতি = start;
        প্রতি (Cover_রাশিমালা& রাশিমালা : m_exprs) {
            ধ্রুব string name = "expr_" + std::to_string(গুনতি);
            string comment = "(";
            তার্কিক first = সত্য;
            Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
            প্রতি (CoverTerm& term : রাশিমালা) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_গ্রন্থি_রাশিমালা* covExprp = nullptr;
                যদি (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব frefp = term.m_exprp;
                    Ast_গ্রন্থি_DType* ধ্রুব dtypep = frefp->dtypep();
                    ধ্রুব auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    যদি (pair.second) {
                        varp = নূতন AstVar{fl, VVarType::MODULETEMP, m_রাশিমালা_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        যদি (m_ftaskp) {
                            varp->funcLocal(সত্য);
                            m_ftaskp->prependStmtsp(varp);
                        } অন্যথা {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(নূতন Ast_রাশিমালা_Stmt{
                            fl, নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            নূতন AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = নূতন AstVarRef{fl, varp, VAccess::READ};
                } অন্যথা {
                    covExprp = term.m_exprp->clone_বৃক্ষ(অসত্য);
                }
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব termp
                    = term.m_objective ? covExprp : নূতন AstLogNot{fl, covExprp};
                যদি (condp) {
                    condp = নূতন AstLogAnd{fl, condp, termp};
                } অন্যথা {
                    condp = termp;
                }
                first = অসত্য;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ধ্রুব declp = নূতন AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_গ্রন্থি* ধ্রুব newp = newCoverInc(fl, declp, trace_Name_For_সারি(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ধ্রুব ifp = নূতন AstIf{fl, condp, newp, nullptr};
            ifp->user2(সত্য);
            ঢোকাও_Proc_Statement(m_রাশিমালা_Stmtsp, ifp);
            ++গুনতি;
        }
    }

    শূন্য coverExprs(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (!m_state.রাশিমালা_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_রাশিমালা_Stmtsp) {
            প্রত্যাগম;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = অসত্য;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = সত্য;
        iterate(nodep);
        যদি (checkMaxExprs(falseExprs.size())) প্রত্যাগম;
        যদি (m_seeking == ABORTED) প্রত্যাগম;

        add_রাশিমালা_Cover_Inc(nodep);
        ধ্রুব পূর্ণাঙ্ক start = m_exprs.size();
        m_objective = অসত্য;
        m_exprs.swap(falseExprs);
        add_রাশিমালা_Cover_Inc(nodep, start);
    }

    শূন্য রাশিমালা_Either(Ast_গ্রন্থি_Biop* nodep, তার্কিক overrideObjective = অসত্য, তার্কিক lObjective = অসত্য,
                    তার্কিক rObjective = অসত্য) {
        VL_RESTORER(m_objective);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = nodep->lhsp();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp();

        যদি (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        যদি (checkMaxExprs()) প্রত্যাগম;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        যদি (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    শূন্য রাশিমালা_Both(Ast_গ্রন্থি_Biop* nodep, তার্কিক overrideObjective = অসত্য, তার্কিক lObjective = অসত্য,
                  তার্কিক rObjective = অসত্য) {
        VL_RESTORER(m_objective);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = nodep->lhsp();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp();

        যদি (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        যদি (checkMaxExprs()) প্রত্যাগম;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        যদি (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        যদি (checkMaxExprs()) প্রত্যাগম;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        প্রতি (ধ্রুব Cover_রাশিমালা& l : lhsExprs) {
            প্রতি (ধ্রুব Cover_রাশিমালা& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_রাশিমালা রাশিমালা;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                প্রতি (ধ্রুব CoverTerm& term : l) {
                    যদি (ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].ঢোকাও(refp->varp());
                    } অন্যথা {
                        strs[term.m_objective].ঢোকাও(term.m_emitV);
                    }
                    রাশিমালা.ঠেলো_back(term);
                }
                তার্কিক impossible = অসত্য;
                প্রতি (ধ্রুব CoverTerm& term : r) {
                    তার্কিক redundant = অসত্য;
                    যদি (ধ্রুব Ast_গ্রন্থি_Var_Ref* ধ্রুব refp = VN_CAST(term.m_exprp, গ্রন্থি_Var_Ref)) {
                        যদি (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = সত্য;
                        যদি (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = সত্য;
                    } অন্যথা {
                        যদি (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = সত্য;
                        যদি (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = সত্য;
                    }
                    যদি (!redundant) রাশিমালা.ঠেলো_back(term);
                }
                যদি (!impossible) m_exprs.ঠেলো_back(std::move(রাশিমালা));
                যদি (checkMaxExprs()) প্রত্যাগম;
            }
        }
    }

    শূন্য or_রাশিমালা(Ast_গ্রন্থি_Biop* nodep) {
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা যদি (m_objective) {
            রাশিমালা_Either(nodep);
        } অন্যথা {
            রাশিমালা_Both(nodep);
        }
        সারি_Track(nodep);
    }
    শূন্য visit(AstLogOr* nodep) override { or_রাশিমালা(nodep); }
    শূন্য visit(AstOr* nodep) override { or_রাশিমালা(nodep); }

    শূন্য and_রাশিমালা(Ast_গ্রন্থি_Biop* nodep) {
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা যদি (m_objective) {
            রাশিমালা_Both(nodep);
        } অন্যথা {
            রাশিমালা_Either(nodep);
        }
        সারি_Track(nodep);
    }
    শূন্য visit(AstLogAnd* nodep) override { and_রাশিমালা(nodep); }
    শূন্য visit(AstAnd* nodep) override { and_রাশিমালা(nodep); }

    শূন্য xor_রাশিমালা(Ast_গ্রন্থি_Biop* nodep) {
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা {
            প্রতি (ধ্রুব তার্কিক lObjective : {অসত্য, সত্য}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                ধ্রুব তার্কিক rObjective = lObjective ^ m_objective;
                রাশিমালা_Both(nodep, সত্য, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                যদি (checkMaxExprs()) ভঙ্গ;
            }
        }
        সারি_Track(nodep);
    }
    শূন্য visit(AstXor* nodep) override { xor_রাশিমালা(nodep); }

    শূন্য রাশিমালা_Not(Ast_গ্রন্থি_রাশিমালা* nodep) {
        VL_RESTORER(m_objective);
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা {
            m_objective = !m_objective;
            iterateChildren(nodep);
            সারি_Track(nodep);
        }
    }
    শূন্য visit(AstNot* nodep) override { রাশিমালা_Not(nodep); }
    শূন্য visit(AstLogNot* nodep) override { রাশিমালা_Not(nodep); }

    template <typename T_Oper>
    শূন্য রাশিমালা_Reduce(Ast_গ্রন্থি_Uniop* nodep) {
        যদি (m_seeking != ABORTED) {
            File_সারি* ধ্রুব fl = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = nodep->lhsp();
            ধ্রুব পূর্ণাঙ্ক width = lhsp->dtypep()->width();
            ধ্রুব size_t expected = std::is_same<T_Oper, AstXor>::মান ? 0x1 << width : width + 1;
            যদি (checkMaxExprs(expected)) প্রত্যাগম;
            Ast_গ্রন্থি_রাশিমালা* unrolledp = নূতন AstSel{
                fl, lhsp->clone_বৃক্ষ(অসত্য), নূতন AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            প্রতি (পূর্ণাঙ্ক bit = width - 2; bit >= 0; bit--) {
                AstSel* ধ্রুব selp = নূতন AstSel{fl, lhsp->clone_বৃক্ষ(অসত্য),
                                                নূতন AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = নূতন T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ঠেলো_Deletep(unrolledp), unrolledp);
        } অন্যথা {
            iterateChildren(nodep);
            সারি_Track(nodep);
        }
    }
    শূন্য visit(AstRedOr* nodep) override { রাশিমালা_Reduce<AstOr>(nodep); }
    শূন্য visit(AstRedAnd* nodep) override { রাশিমালা_Reduce<AstAnd>(nodep); }
    শূন্য visit(AstRedXor* nodep) override { রাশিমালা_Reduce<AstXor>(nodep); }

    শূন্য visit(AstLogIf* nodep) override {
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা যদি (m_objective) {
            রাশিমালা_Either(nodep, সত্য, অসত্য, সত্য);
        } অন্যথা {
            রাশিমালা_Both(nodep, সত্য, সত্য, অসত্য);
        }
        সারি_Track(nodep);
    }

    শূন্য visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        যদি (m_seeking == NONE) {
            coverExprs(nodep);
        } অন্যথা {
            m_objective = !m_objective;
            xor_রাশিমালা(nodep);
            সারি_Track(nodep);
        }
    }

    শূন্য visit(AstFuncRef* nodep) override {
        যদি (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_গ্রন্থি_রাশিমালা*>(nodep));
        } অন্যথা {
            রাশিমালা_Unsupported(nodep, "non-automatic function");
        }
    }

    শূন্য visit(Ast_গ্রন্থি_রাশিমালা* nodep) override {
        যদি (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } অন্যথা {
            রাশিমালা_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            যদি (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_বৃক্ষ(nodep, emitV);
                // Add new expression with a single term
                Cover_রাশিমালা রাশিমালা;
                রাশিমালা.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.ঠেলো_back(std::move(রাশিমালা));
                checkMaxExprs();
            } অন্যথা {
                রাশিমালা_Unsupported(nodep, "not coverage eligible");
            }
        }
        সারি_Track(nodep);
    }

    // VISITORS - BOTH
    শূন্য visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = অসত্য;
        m_inToggleOff = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = অসত্য;
        m_inToggleOff = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        iterateChildren(nodep);
        সারি_Track(nodep);
    }

    শূন্য রাশিমালা_Unsupported(Ast_গ্রন্থি* nodep, ধ্রুব string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ধ্রুব তার্কিক wasSeeking = m_seeking == SEEKING;
        ধ্রুব Objective oldSeeking = m_seeking;
        যদি (wasSeeking) abort_রাশিমালা_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        সারি_Track(nodep);
        যদি (!wasSeeking) m_seeking = oldSeeking;
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_রাশিমালা_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = স্বভাব;
};

//######################################################################
// Coverage class functions

শূন্য V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("coverage", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
