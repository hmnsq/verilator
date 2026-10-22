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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Coverage.h"

#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <list>
#അന്തര്ഭാവ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം വ്യഞ്ജകം_Coverage_Eligible_Visitor final : പൊതു VNVisitorConst {
    // STATE
    തർക്കം m_eligible = സത്യം;

    സ്ഥിതം തർക്കം elemDTypeEligible(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtp = dtypep->virtRefDTypep()) {
            എങ്കിൽ (!elemDTypeEligible(dtp)) തിരികെ അസത്യം;
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtp = dtypep->virtRefDType2p()) {
            എങ്കിൽ (!elemDTypeEligible(dtp)) തിരികെ അസത്യം;
        }
        തിരികെ !VN_IS(dtypep, ClassRefDType);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        എങ്കിൽ (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } അല്ലെങ്കിൽ {
            m_eligible = അസത്യം;
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (!nodep->is_വ്യഞ്ജകം_Coverage_Eligible()) {
            m_eligible = അസത്യം;
        } അല്ലെങ്കിൽ {
            iterateChildrenConst(nodep);
        }
    }

പൊതു:
    // CONSTRUCTORS
    explicit വ്യഞ്ജകം_Coverage_Eligible_Visitor(Ast_കണ്ണി* nodep) { iterateConst(nodep); }
    ~വ്യഞ്ജകം_Coverage_Eligible_Visitor() override = സ്ഥിരസ്ഥിതി;

    തർക്കം eligible() സ്ഥിരം { തിരികെ m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

വർഗ്ഗം CoverageVisitor final : പൊതു VNVisitor {
    // TYPES
    using LinenoSet = std::set<പൂർണ്ണസംഖ്യ>;

    ഘടന CoverTerm final {
        Ast_കണ്ണി_വ്യഞ്ജകം* m_exprp;  // Expression branch term
        തർക്കം m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, തർക്കം objective, സ്ഥിരം string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_വ്യഞ്ജകം = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_വ്യഞ്ജകം>;

    ഘടന ToggleEnt final {
        സ്ഥിരം string m_comment;  // Comment for coverage dump
        Ast_കണ്ണി_വ്യഞ്ജകം* m_varRefp;  // How to get to this element
        Ast_കണ്ണി_വ്യഞ്ജകം* m_chgRefp;  // How to get to this element
        ToggleEnt(സ്ഥിരം string& comment, Ast_കണ്ണി_വ്യഞ്ജകം* vp, Ast_കണ്ണി_വ്യഞ്ജകം* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = സ്ഥിരസ്ഥിതി;
        ശൂന്യം cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_വൃക്ഷം(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_വൃക്ഷം(), m_chgRefp = nullptr);
        }
    };

    ഘടന CheckState final {  // State save-restored on each new coverage scope/block
        തർക്കം m_on = അസത്യം;  // Should this block get covered?
        തർക്കം m_inModOff = അസത്യം;  // In module with no coverage
        പൂർണ്ണസംഖ്യ m_handle = 0;  // Opaque handle for index into line tracking
        സ്ഥിരം Ast_കണ്ണി* m_nodep = nullptr;  // Node establishing this state
        CheckState() = സ്ഥിരസ്ഥിതി;
        തർക്കം coverageOn(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം {
            തിരികെ m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        തർക്കം വരി_Coverage_On(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം {
            തിരികെ coverageOn(nodep) && v3Global.opt.coverage_വരി();
        }
        തർക്കം branchCoverageOn(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം {
            തിരികെ coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        തർക്കം വ്യഞ്ജകം_Coverage_On(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം {
            തിരികെ coverageOn(nodep) && v3Global.opt.coverage_വ്യഞ്ജകം();
        }
    };

    എണ്ണൽ Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    V3UniqueNames m_വ്യഞ്ജകം_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_കണ്ണി_വ്യഞ്ജകം*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    പൂർണ്ണസംഖ്യ m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_കണ്ണി* m_വ്യഞ്ജകം_Stmtsp = nullptr;  // Node to add expr coverage to
    തർക്കം m_then = അസത്യം;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    തർക്കം m_objective = അസത്യം;  // Expression objective
    തർക്കം m_ifCond = അസത്യം;  // Visiting if condition
    തർക്കം m_inToggleOff = അസത്യം;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<പൂർണ്ണസംഖ്യ, LinenoSet> m_handle_വരികൾ;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    സ്ഥിരം അക്ഷരം* varIgnoreToggle(സ്ഥിരം AstVar* nodep) {
        സ്ഥിരം തർക്കം cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        എങ്കിൽ (!cover) തിരികെ "Not relevant signal";
        എങ്കിൽ (nodep->isConst()) തിരികെ "Signal is constant";
        എങ്കിൽ (nodep->isDouble()) തിരികെ "Signal is double";
        എങ്കിൽ (nodep->isString()) തിരികെ "Signal is string";
        എങ്കിൽ (!v3Global.opt.coverageUnderscore()) {
            സ്ഥിരം string prettyName = nodep->prettyName();
            എങ്കിൽ (prettyName[0] == '_') തിരികെ "Leading underscore";
            എങ്കിൽ (prettyName.find("._") != string::npos) തിരികെ "Inlined leading underscore";
        }
        എങ്കിൽ ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            തിരികെ "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        തിരികെ nullptr;
    }

    AstCoverInc* newCoverInc(File_വരി* fl, Ast_കണ്ണി_Cover_Decl* സ്ഥിരം declp,
                             സ്ഥിരം string& trace_var_name) {
        AstCoverInc* സ്ഥിരം incp = പുതിയ AstCoverInc{fl, declp};
        എങ്കിൽ (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_വരി* സ്ഥിരം fl_nowarn = പുതിയ File_വരി{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, സത്യം);
            AstVar* സ്ഥിരം varp = പുതിയ AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(സത്യം);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* സ്ഥിരം assp = പുതിയ AstAssign{
                incp->fileline(), പുതിയ AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                പുതിയ AstAdd{incp->fileline(), പുതിയ AstVarRef{incp->fileline(), varp, VAccess::READ},
                           പുതിയ AstConst{incp->fileline(), AstConst::Widthed_മൂല്യം{}, 32, 1}}};
            Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(incp, assp);
        }
        തിരികെ incp;
    }
    string trace_Name_For_വരി(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        എങ്കിൽ (സ്ഥിരം uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        തിരികെ name;
    }

    // Line tracking
    ശൂന്യം createHandle(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = സത്യം;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    ശൂന്യം വരി_Track(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        എങ്കിൽ ((m_state.വരി_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            വേണ്ടി (പൂർണ്ണസംഖ്യ lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_വരികൾ[m_state.m_handle].തിരുകുക(lineno);
            }
        }
    }
    സ്ഥിതം string വരികൾ_First_Last(സ്ഥിരം പൂർണ്ണസംഖ്യ first, സ്ഥിരം പൂർണ്ണസംഖ്യ last) {
        എങ്കിൽ (first && first == last) {
            തിരികെ cvtToStr(first);
        } അല്ലെങ്കിൽ എങ്കിൽ (first && last) {
            തിരികെ cvtToStr(first) + "-" + cvtToStr(last);
        } അല്ലെങ്കിൽ {
            തിരികെ "";
        }
    }
    string വരികൾ_Cov(സ്ഥിരം CheckState& state, സ്ഥിരം Ast_കണ്ണി* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        സ്ഥിരം LinenoSet& വരികൾ = m_handle_വരികൾ[state.m_handle];
        പൂർണ്ണസംഖ്യ first = 0;
        പൂർണ്ണസംഖ്യ last = 0;
        വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ linen : വരികൾ) {
            എങ്കിൽ (!first) {
                first = last = linen;
            } അല്ലെങ്കിൽ എങ്കിൽ (linen == last + 1) {
                ++last;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (!out.empty()) out += ",";
                out += വരികൾ_First_Last(first, last);
                first = last = linen;
            }
        }
        എങ്കിൽ (first) {
            എങ്കിൽ (!out.empty()) out += ",";
            out += വരികൾ_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        തിരികെ out;
    }

    // VISITORS - BOTH
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_വ്യഞ്ജകം_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_മുകൾ();  // Already made top shell, no coverage for it
        എങ്കിൽ (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_വരികൾ.clear();
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_വ്യഞ്ജകം_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_വ്യഞ്ജകം_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_വ്യഞ്ജകം_Stmtsp = nodep;
            m_inToggleOff = സത്യം;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            തിരികെ;
        }
        iterateProcedure(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override { iterateProcedure(nodep); }
    ശൂന്യം visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_വ്യഞ്ജകം_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_വ്യഞ്ജകം_Stmtsp = nodep;
        m_inToggleOff = സത്യം;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        എങ്കിൽ (m_state.വരി_Coverage_On(nodep)) {
            AstCoverOtherDecl* സ്ഥിരം declp
                = പുതിയ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", വരികൾ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_കണ്ണി* സ്ഥിരം newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_വരി(nodep, "block"));
            തിരുകുക_Proc_Statement(nodep, newp);
        }
    }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        എങ്കിൽ (nodep->user2SetOnce()) തിരികെ;
        വരി_Track(nodep);
        എങ്കിൽ (m_state.വരി_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* സ്ഥിരം declp
                = പുതിയ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", വരികൾ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_കണ്ണി* സ്ഥിരം newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_വരി(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_വ്യഞ്ജകം_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        എങ്കിൽ (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    ശൂന്യം തിരുകുക_Proc_Statement(Ast_കണ്ണി* nodep, Ast_കണ്ണി* stmtp) {
        എങ്കിൽ (Ast_കണ്ണി_Procedure* സ്ഥിരം itemp = VN_CAST(nodep, കണ്ണി_Procedure)) {
            itemp->addStmtsp(stmtp);
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം itemp = VN_CAST(nodep, കണ്ണി_FTask)) {
            itemp->addStmtsp(stmtp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstLoop* സ്ഥിരം itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstIf* സ്ഥിരം itemp = VN_CAST(nodep, If)) {
            എങ്കിൽ (m_then) {
                itemp->addThensp(stmtp);
            } അല്ലെങ്കിൽ {
                itemp->addElsesp(stmtp);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstBegin* സ്ഥിരം itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    ശൂന്യം iterateProcedure(Ast_കണ്ണി* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_വ്യഞ്ജകം_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_വ്യഞ്ജകം_Stmtsp = nodep;
        m_inToggleOff = സത്യം;
        createHandle(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (m_state.വരി_Coverage_On(nodep)) {
            വരി_Track(nodep);
            AstCoverOtherDecl* സ്ഥിരം declp
                = പുതിയ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", വരികൾ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_കണ്ണി* സ്ഥിരം newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_വരി(nodep, "block"));
            തിരുകുക_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    ശൂന്യം visit(AstVar* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            സ്ഥിരം അക്ഷരം* സ്ഥിരം disablep = varIgnoreToggle(nodep);
            എങ്കിൽ (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } അല്ലെങ്കിൽ {
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
                സ്ഥിരം string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_വരി* സ്ഥിരം fl_nowarn = പുതിയ File_വരി{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, സത്യം);
                AstVar* സ്ഥിരം chgVarp
                    = പുതിയ AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, പുതിയ AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 പുതിയ AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    ശൂന്യം toggleVarBottom(സ്ഥിരം ToggleEnt& above, സ്ഥിരം AstVar* varp, സ്ഥിരം VNumRange& range) {
        സ്ഥിരം std::string hierPrefix
            = (m_beginHier != "") ? Ast_കണ്ണി::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* സ്ഥിരം declp
            = പുതിയ AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* സ്ഥിരം newp = പുതിയ AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_വൃക്ഷം(അസത്യം), above.m_chgRefp->clone_വൃക്ഷം(അസത്യം)};
        m_modp->addStmtsp(newp);
    }

    ശൂന്യം toggleVarRecurse(സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep, സ്ഥിരം പൂർണ്ണസംഖ്യ depth,  // per-iteration
                          സ്ഥിരം ToggleEnt& above, സ്ഥിരം AstVar* സ്ഥിരം varp) {  // Constant
        എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ index_docs : adtypep->declRange().seqLoToHi()) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 പുതിയ AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_വൃക്ഷം(അസത്യം), index_code},
                                 പുതിയ AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_വൃക്ഷം(അസത്യം), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം adtypep = VN_CAST(dtypep, PackArrayDType)) {
            വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ index_docs : adtypep->declRange().seqLoToHi()) {
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം subtypep = adtypep->subDTypep()->skipRefp();
                സ്ഥിരം പൂർണ്ണസംഖ്യ index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 പുതിയ AstSel{varp->fileline(), above.m_varRefp->clone_വൃക്ഷം(അസത്യം),
                                            index_code * subtypep->width(), subtypep->width()},
                                 പുതിയ AstSel{varp->fileline(), above.m_chgRefp->clone_വൃക്ഷം(അസത്യം),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstStructDType* സ്ഥിരം adtypep = VN_CAST(dtypep, StructDType)) {
            എങ്കിൽ (adtypep->packed()) {
                വേണ്ടി (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം subtypep = itemp->subDTypep()->skipRefp();
                    സ്ഥിരം പൂർണ്ണസംഖ്യ index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        പുതിയ AstSel{varp->fileline(), above.m_varRefp->clone_വൃക്ഷം(അസത്യം), index_code,
                                   subtypep->width()},
                        പുതിയ AstSel{varp->fileline(), above.m_chgRefp->clone_വൃക്ഷം(അസത്യം), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } അല്ലെങ്കിൽ {
                വേണ്ടി (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_കണ്ണി_DType* സ്ഥിരം subtypep = itemp->subDTypep()->skipRefp();
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം varRefp = പുതിയ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_വൃക്ഷം(അസത്യം), itemp->name()};
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം chgRefp = പുതിയ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_വൃക്ഷം(അസത്യം), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnionDType* സ്ഥിരം adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            എങ്കിൽ (സ്ഥിരം AstMemberDType* സ്ഥിരം itemp = adtypep->membersp()) {
                Ast_കണ്ണി_DType* സ്ഥിരം subtypep = itemp->subDTypep()->skipRefp();
                എങ്കിൽ (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_വൃക്ഷം(അസത്യം),
                                     above.m_chgRefp->clone_വൃക്ഷം(അസത്യം)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } അല്ലെങ്കിൽ {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം varRefp = പുതിയ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_വൃക്ഷം(അസത്യം), itemp->name()};
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം chgRefp = പുതിയ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_വൃക്ഷം(അസത്യം), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } അല്ലെങ്കിൽ {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    തർക്കം includeCondToBranchRecursive(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep) {
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം backp = nodep->backp();
        എങ്കിൽ (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            തിരികെ includeCondToBranchRecursive(backp);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            തിരികെ includeCondToBranchRecursive(backp);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(backp, കണ്ണി_Assign) && VN_AS(backp, കണ്ണി_Assign)->rhsp() == nodep) {
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // VISITORS - LINE COVERAGE
    ശൂന്യം visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        എങ്കിൽ (m_seeking == NONE) coverExprs(nodep->condp());

        എങ്കിൽ (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            സ്ഥിരം CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            വരി_Track(nodep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* സ്ഥിരം thenDeclp
                = പുതിയ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", വരികൾ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(പുതിയ Ast_വ്യഞ്ജകം_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_വരി(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* സ്ഥിരം elseDeclp
                = പുതിയ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", വരികൾ_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(പുതിയ Ast_വ്യഞ്ജകം_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_വരി(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } അല്ലെങ്കിൽ {
            വരി_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    ശൂന്യം visit(AstIf* nodep) override {
        എങ്കിൽ (nodep->user2()) തിരികെ;

        UINFO(4, " IF: " << nodep);
        എങ്കിൽ (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            സ്ഥിരം തർക്കം elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            എങ്കിൽ (elsif) VN_AS(nodep->elsesp(), If)->user1(സത്യം);
            സ്ഥിരം തർക്കം first_elsif = !nodep->user1() && elsif;
            സ്ഥിരം തർക്കം cont_elsif = nodep->user1() && elsif;
            സ്ഥിരം തർക്കം final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            സ്ഥിരം CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_വ്യഞ്ജകം_Stmtsp);
                VL_RESTORER(m_then);
                m_വ്യഞ്ജകം_Stmtsp = nodep;
                m_then = സത്യം;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                വരി_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_വ്യഞ്ജകം_Stmtsp);
                VL_RESTORER(m_then);
                m_വ്യഞ്ജകം_Stmtsp = nodep;
                m_then = അസത്യം;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            എങ്കിൽ (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* സ്ഥിരം ifDeclp
                    = പുതിയ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", വരികൾ_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_വരി(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* സ്ഥിരം elseDeclp
                    = പുതിയ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", വരികൾ_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_വരി(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            അല്ലെങ്കിൽ എങ്കിൽ (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                എങ്കിൽ (ifState.വരി_Coverage_On(nodep)) {
                    AstCoverOtherDecl* സ്ഥിരം elsifDeclp = പുതിയ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        വരികൾ_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_വരി(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } അല്ലെങ്കിൽ {
                // Cover as separate blocks (not a branch as is not two-legged)
                എങ്കിൽ (ifState.വരി_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* സ്ഥിരം ifDeclp = പുതിയ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        വരികൾ_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_വരി(nodep, "if")));
                }
                എങ്കിൽ (elseState.വരി_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* സ്ഥിരം elseDeclp = പുതിയ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        വരികൾ_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_വരി(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = സത്യം;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    ശൂന്യം visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        എങ്കിൽ (m_state.വരി_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            എങ്കിൽ (m_state.വരി_Coverage_On(nodep)) {  // if the case body didn't disable it
                വരി_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* സ്ഥിരം declp
                    = പുതിയ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", വരികൾ_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_വരി(nodep, "case")));
            }
        }
    }
    ശൂന്യം visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = സത്യം;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            വരി_Track(nodep);
            AstCoverOtherDecl* സ്ഥിരം declp
                = പുതിയ AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", വരികൾ_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    ശൂന്യം visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_വ്യഞ്ജകം_Stmtsp);
        m_വ്യഞ്ജകം_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = അസത്യം;
    }
    ശൂന്യം visit(AstStmtPragma* nodep) override {
        എങ്കിൽ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = അസത്യം;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_state.m_on) iterateChildren(nodep);
            വരി_Track(nodep);
        }
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        എങ്കിൽ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        വരി_Track(nodep);
    }

    ശൂന്യം visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_വ്യഞ്ജകം_Stmtsp);
        m_വ്യഞ്ജകം_Stmtsp = nodep;
        m_inToggleOff = സത്യം;
        എങ്കിൽ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        വരി_Track(nodep);
    }

    ശൂന്യം abort_വ്യഞ്ജകം_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        എങ്കിൽ (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    തർക്കം checkMaxExprs(size_t additional = 0) {
        എങ്കിൽ (m_seeking != ABORTED
            && static_cast<പൂർണ്ണസംഖ്യ>(m_exprs.size() + additional) <= v3Global.opt.coverage_വ്യഞ്ജകം_Max())
            തിരികെ അസത്യം;
        abort_വ്യഞ്ജകം_Coverage();
        തിരികെ സത്യം;
    }

    ശൂന്യം add_വ്യഞ്ജകം_Cover_Inc(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, പൂർണ്ണസംഖ്യ start = 0) {
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        പൂർണ്ണസംഖ്യ എണ്ണം = start;
        വേണ്ടി (Cover_വ്യഞ്ജകം& വ്യഞ്ജകം : m_exprs) {
            സ്ഥിരം string name = "expr_" + std::to_string(എണ്ണം);
            string comment = "(";
            തർക്കം first = സത്യം;
            Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;
            വേണ്ടി (CoverTerm& term : വ്യഞ്ജകം) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_കണ്ണി_വ്യഞ്ജകം* covExprp = nullptr;
                എങ്കിൽ (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം frefp = term.m_exprp;
                    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = frefp->dtypep();
                    സ്ഥിരം auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    എങ്കിൽ (pair.second) {
                        varp = പുതിയ AstVar{fl, VVarType::MODULETEMP, m_വ്യഞ്ജകം_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        എങ്കിൽ (m_ftaskp) {
                            varp->funcLocal(സത്യം);
                            m_ftaskp->prependStmtsp(varp);
                        } അല്ലെങ്കിൽ {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(പുതിയ Ast_വ്യഞ്ജകം_Stmt{
                            fl, പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            പുതിയ AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = പുതിയ AstVarRef{fl, varp, VAccess::READ};
                } അല്ലെങ്കിൽ {
                    covExprp = term.m_exprp->clone_വൃക്ഷം(അസത്യം);
                }
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp
                    = term.m_objective ? covExprp : പുതിയ AstLogNot{fl, covExprp};
                എങ്കിൽ (condp) {
                    condp = പുതിയ AstLogAnd{fl, condp, termp};
                } അല്ലെങ്കിൽ {
                    condp = termp;
                }
                first = അസത്യം;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* സ്ഥിരം declp = പുതിയ AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_കണ്ണി* സ്ഥിരം newp = newCoverInc(fl, declp, trace_Name_For_വരി(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{fl, condp, newp, nullptr};
            ifp->user2(സത്യം);
            തിരുകുക_Proc_Statement(m_വ്യഞ്ജകം_Stmtsp, ifp);
            ++എണ്ണം;
        }
    }

    ശൂന്യം coverExprs(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (!m_state.വ്യഞ്ജകം_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_വ്യഞ്ജകം_Stmtsp) {
            തിരികെ;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = അസത്യം;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = സത്യം;
        iterate(nodep);
        എങ്കിൽ (checkMaxExprs(falseExprs.size())) തിരികെ;
        എങ്കിൽ (m_seeking == ABORTED) തിരികെ;

        add_വ്യഞ്ജകം_Cover_Inc(nodep);
        സ്ഥിരം പൂർണ്ണസംഖ്യ start = m_exprs.size();
        m_objective = അസത്യം;
        m_exprs.swap(falseExprs);
        add_വ്യഞ്ജകം_Cover_Inc(nodep, start);
    }

    ശൂന്യം വ്യഞ്ജകം_Either(Ast_കണ്ണി_Biop* nodep, തർക്കം overrideObjective = അസത്യം, തർക്കം lObjective = അസത്യം,
                    തർക്കം rObjective = അസത്യം) {
        VL_RESTORER(m_objective);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp();

        എങ്കിൽ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        എങ്കിൽ (checkMaxExprs()) തിരികെ;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        എങ്കിൽ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    ശൂന്യം വ്യഞ്ജകം_Both(Ast_കണ്ണി_Biop* nodep, തർക്കം overrideObjective = അസത്യം, തർക്കം lObjective = അസത്യം,
                  തർക്കം rObjective = അസത്യം) {
        VL_RESTORER(m_objective);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp();

        എങ്കിൽ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        എങ്കിൽ (checkMaxExprs()) തിരികെ;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        എങ്കിൽ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        എങ്കിൽ (checkMaxExprs()) തിരികെ;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        വേണ്ടി (സ്ഥിരം Cover_വ്യഞ്ജകം& l : lhsExprs) {
            വേണ്ടി (സ്ഥിരം Cover_വ്യഞ്ജകം& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_വ്യഞ്ജകം വ്യഞ്ജകം;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                വേണ്ടി (സ്ഥിരം CoverTerm& term : l) {
                    എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].തിരുകുക(refp->varp());
                    } അല്ലെങ്കിൽ {
                        strs[term.m_objective].തിരുകുക(term.m_emitV);
                    }
                    വ്യഞ്ജകം.തള്ളുക_back(term);
                }
                തർക്കം impossible = അസത്യം;
                വേണ്ടി (സ്ഥിരം CoverTerm& term : r) {
                    തർക്കം redundant = അസത്യം;
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp = VN_CAST(term.m_exprp, കണ്ണി_Var_Ref)) {
                        എങ്കിൽ (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = സത്യം;
                        എങ്കിൽ (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = സത്യം;
                    } അല്ലെങ്കിൽ {
                        എങ്കിൽ (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = സത്യം;
                        എങ്കിൽ (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = സത്യം;
                    }
                    എങ്കിൽ (!redundant) വ്യഞ്ജകം.തള്ളുക_back(term);
                }
                എങ്കിൽ (!impossible) m_exprs.തള്ളുക_back(std::move(വ്യഞ്ജകം));
                എങ്കിൽ (checkMaxExprs()) തിരികെ;
            }
        }
    }

    ശൂന്യം or_വ്യഞ്ജകം(Ast_കണ്ണി_Biop* nodep) {
        എങ്കിൽ (m_seeking == NONE) {
            coverExprs(nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_objective) {
            വ്യഞ്ജകം_Either(nodep);
        } അല്ലെങ്കിൽ {
            വ്യഞ്ജകം_Both(nodep);
        }
        വരി_Track(nodep);
    }
    ശൂന്യം visit(AstLogOr* nodep) override { or_വ്യഞ്ജകം(nodep); }
    ശൂന്യം visit(AstOr* nodep) override { or_വ്യഞ്ജകം(nodep); }

    ശൂന്യം and_വ്യഞ്ജകം(Ast_കണ്ണി_Biop* nodep) {
        എങ്കിൽ (m_seeking == NONE) {
            coverExprs(nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_objective) {
            വ്യഞ്ജകം_Both(nodep);
        } അല്ലെങ്കിൽ {
            വ്യഞ്ജകം_Either(nodep);
        }
        വരി_Track(nodep);
    }
    ശൂന്യം visit(AstLogAnd* nodep) override { and_വ്യഞ്ജകം(nodep); }
    ശൂന്യം visit(AstAnd* nodep) override { and_വ്യഞ്ജകം(nodep); }

    ശൂന്യം xor_വ്യഞ്ജകം(Ast_കണ്ണി_Biop* nodep) {
        എങ്കിൽ (m_seeking == NONE) {
            coverExprs(nodep);
        } അല്ലെങ്കിൽ {
            വേണ്ടി (സ്ഥിരം തർക്കം lObjective : {അസത്യം, സത്യം}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                സ്ഥിരം തർക്കം rObjective = lObjective ^ m_objective;
                വ്യഞ്ജകം_Both(nodep, സത്യം, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                എങ്കിൽ (checkMaxExprs()) നിർത്തുക;
            }
        }
        വരി_Track(nodep);
    }
    ശൂന്യം visit(AstXor* nodep) override { xor_വ്യഞ്ജകം(nodep); }

    ശൂന്യം വ്യഞ്ജകം_Not(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        VL_RESTORER(m_objective);
        എങ്കിൽ (m_seeking == NONE) {
            coverExprs(nodep);
        } അല്ലെങ്കിൽ {
            m_objective = !m_objective;
            iterateChildren(nodep);
            വരി_Track(nodep);
        }
    }
    ശൂന്യം visit(AstNot* nodep) override { വ്യഞ്ജകം_Not(nodep); }
    ശൂന്യം visit(AstLogNot* nodep) override { വ്യഞ്ജകം_Not(nodep); }

    template <typename T_Oper>
    ശൂന്യം വ്യഞ്ജകം_Reduce(Ast_കണ്ണി_Uniop* nodep) {
        എങ്കിൽ (m_seeking != ABORTED) {
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = lhsp->dtypep()->width();
            സ്ഥിരം size_t expected = std::is_same<T_Oper, AstXor>::മൂല്യം ? 0x1 << width : width + 1;
            എങ്കിൽ (checkMaxExprs(expected)) തിരികെ;
            Ast_കണ്ണി_വ്യഞ്ജകം* unrolledp = പുതിയ AstSel{
                fl, lhsp->clone_വൃക്ഷം(അസത്യം), പുതിയ AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            വേണ്ടി (പൂർണ്ണസംഖ്യ bit = width - 2; bit >= 0; bit--) {
                AstSel* സ്ഥിരം selp = പുതിയ AstSel{fl, lhsp->clone_വൃക്ഷം(അസത്യം),
                                                പുതിയ AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = പുതിയ T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(തള്ളുക_Deletep(unrolledp), unrolledp);
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
            വരി_Track(nodep);
        }
    }
    ശൂന്യം visit(AstRedOr* nodep) override { വ്യഞ്ജകം_Reduce<AstOr>(nodep); }
    ശൂന്യം visit(AstRedAnd* nodep) override { വ്യഞ്ജകം_Reduce<AstAnd>(nodep); }
    ശൂന്യം visit(AstRedXor* nodep) override { വ്യഞ്ജകം_Reduce<AstXor>(nodep); }

    ശൂന്യം visit(AstLogIf* nodep) override {
        എങ്കിൽ (m_seeking == NONE) {
            coverExprs(nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_objective) {
            വ്യഞ്ജകം_Either(nodep, സത്യം, അസത്യം, സത്യം);
        } അല്ലെങ്കിൽ {
            വ്യഞ്ജകം_Both(nodep, സത്യം, സത്യം, അസത്യം);
        }
        വരി_Track(nodep);
    }

    ശൂന്യം visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        എങ്കിൽ (m_seeking == NONE) {
            coverExprs(nodep);
        } അല്ലെങ്കിൽ {
            m_objective = !m_objective;
            xor_വ്യഞ്ജകം(nodep);
            വരി_Track(nodep);
        }
    }

    ശൂന്യം visit(AstFuncRef* nodep) override {
        എങ്കിൽ (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(nodep));
        } അല്ലെങ്കിൽ {
            വ്യഞ്ജകം_Unsupported(nodep, "non-automatic function");
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            വ്യഞ്ജകം_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            എങ്കിൽ (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_വൃക്ഷം(nodep, emitV);
                // Add new expression with a single term
                Cover_വ്യഞ്ജകം വ്യഞ്ജകം;
                വ്യഞ്ജകം.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.തള്ളുക_back(std::move(വ്യഞ്ജകം));
                checkMaxExprs();
            } അല്ലെങ്കിൽ {
                വ്യഞ്ജകം_Unsupported(nodep, "not coverage eligible");
            }
        }
        വരി_Track(nodep);
    }

    // VISITORS - BOTH
    ശൂന്യം visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = അസത്യം;
        m_inToggleOff = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = അസത്യം;
        m_inToggleOff = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        iterateChildren(nodep);
        വരി_Track(nodep);
    }

    ശൂന്യം വ്യഞ്ജകം_Unsupported(Ast_കണ്ണി* nodep, സ്ഥിരം string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        സ്ഥിരം തർക്കം wasSeeking = m_seeking == SEEKING;
        സ്ഥിരം Objective oldSeeking = m_seeking;
        എങ്കിൽ (wasSeeking) abort_വ്യഞ്ജകം_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        വരി_Track(nodep);
        എങ്കിൽ (!wasSeeking) m_seeking = oldSeeking;
    }

പൊതു:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_വ്യഞ്ജകം_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Coverage class functions

ശൂന്യം V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("coverage", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
