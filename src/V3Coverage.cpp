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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Coverage.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ ପଦସମଷ୍ଟି_Coverage_Eligible_Visitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE
    ତାର୍କିକ m_eligible = ସତ୍ୟ;

    ସ୍ଥିର ତାର୍କିକ elemDTypeEligible(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = dtypep->virtRefDTypep()) {
            ଯଦି (!elemDTypeEligible(dtp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = dtypep->virtRefDType2p()) {
            ଯଦି (!elemDTypeEligible(dtp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ !VN_IS(dtypep, ClassRefDType);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        ଯଦି (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } ଅନ୍ୟଥା {
            m_eligible = ମିଥ୍ୟା;
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (!nodep->is_ପଦସମଷ୍ଟି_Coverage_Eligible()) {
            m_eligible = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା {
            iterateChildrenConst(nodep);
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ପଦସମଷ୍ଟି_Coverage_Eligible_Visitor(Ast_ଗଣ୍ଠି* nodep) { iterateConst(nodep); }
    ~ପଦସମଷ୍ଟି_Coverage_Eligible_Visitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ତାର୍କିକ eligible() ଅଚଳ { ଫେରନ୍ତୁ m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

ଶ୍ରେଣୀ CoverageVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // TYPES
    using LinenoSet = std::set<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>;

    ସଂରଚନା CoverTerm final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_exprp;  // Expression branch term
        ତାର୍କିକ m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ତାର୍କିକ objective, ଅଚଳ string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_ପଦସମଷ୍ଟି = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_ପଦସମଷ୍ଟି>;

    ସଂରଚନା ToggleEnt final {
        ଅଚଳ string m_comment;  // Comment for coverage dump
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_varRefp;  // How to get to this element
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_chgRefp;  // How to get to this element
        ToggleEnt(ଅଚଳ string& comment, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* vp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ଶୂନ୍ୟ cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_ବୃକ୍ଷ(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_ବୃକ୍ଷ(), m_chgRefp = nullptr);
        }
    };

    ସଂରଚନା CheckState final {  // State save-restored on each new coverage scope/block
        ତାର୍କିକ m_on = ମିଥ୍ୟା;  // Should this block get covered?
        ତାର୍କିକ m_inModOff = ମିଥ୍ୟା;  // In module with no coverage
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_handle = 0;  // Opaque handle for index into line tracking
        ଅଚଳ Ast_ଗଣ୍ଠି* m_nodep = nullptr;  // Node establishing this state
        CheckState() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ତାର୍କିକ coverageOn(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
            ଫେରନ୍ତୁ m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        ତାର୍କିକ ଧାଡ଼ି_Coverage_On(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
            ଫେରନ୍ତୁ coverageOn(nodep) && v3Global.opt.coverage_ଧାଡ଼ି();
        }
        ତାର୍କିକ branchCoverageOn(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
            ଫେରନ୍ତୁ coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        ତାର୍କିକ ପଦସମଷ୍ଟି_Coverage_On(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
            ଫେରନ୍ତୁ coverageOn(nodep) && v3Global.opt.coverage_ପଦସମଷ୍ଟି();
        }
    };

    ଗଣନା Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    V3UniqueNames m_ପଦସମଷ୍ଟି_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ଗଣ୍ଠି* m_ପଦସମଷ୍ଟି_Stmtsp = nullptr;  // Node to add expr coverage to
    ତାର୍କିକ m_then = ମିଥ୍ୟା;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    ତାର୍କିକ m_objective = ମିଥ୍ୟା;  // Expression objective
    ତାର୍କିକ m_ifCond = ମିଥ୍ୟା;  // Visiting if condition
    ତାର୍କିକ m_inToggleOff = ମିଥ୍ୟା;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, LinenoSet> m_handle_ଧାଡ଼ିଗୁଡ଼ିକ;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ଅଚଳ ଅକ୍ଷର* varIgnoreToggle(ଅଚଳ AstVar* nodep) {
        ଅଚଳ ତାର୍କିକ cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        ଯଦି (!cover) ଫେରନ୍ତୁ "Not relevant signal";
        ଯଦି (nodep->isConst()) ଫେରନ୍ତୁ "Signal is constant";
        ଯଦି (nodep->isDouble()) ଫେରନ୍ତୁ "Signal is double";
        ଯଦି (nodep->isString()) ଫେରନ୍ତୁ "Signal is string";
        ଯଦି (!v3Global.opt.coverageUnderscore()) {
            ଅଚଳ string prettyName = nodep->prettyName();
            ଯଦି (prettyName[0] == '_') ଫେରନ୍ତୁ "Leading underscore";
            ଯଦି (prettyName.find("._") != string::npos) ଫେରନ୍ତୁ "Inlined leading underscore";
        }
        ଯଦି ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            ଫେରନ୍ତୁ "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        ଫେରନ୍ତୁ nullptr;
    }

    AstCoverInc* newCoverInc(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ declp,
                             ଅଚଳ string& trace_var_name) {
        AstCoverInc* ଅଚଳ incp = ନୂତନ AstCoverInc{fl, declp};
        ଯଦି (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_ଧାଡ଼ି* ଅଚଳ fl_nowarn = ନୂତନ File_ଧାଡ଼ି{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ସତ୍ୟ);
            AstVar* ଅଚଳ varp = ନୂତନ AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(ସତ୍ୟ);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* ଅଚଳ assp = ନୂତନ AstAssign{
                incp->fileline(), ନୂତନ AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                ନୂତନ AstAdd{incp->fileline(), ନୂତନ AstVarRef{incp->fileline(), varp, VAccess::READ},
                           ନୂତନ AstConst{incp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1}}};
            Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(incp, assp);
        }
        ଫେରନ୍ତୁ incp;
    }
    string trace_Name_For_ଧାଡ଼ି(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        ଯଦି (ଅଚଳ uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        ଫେରନ୍ତୁ name;
    }

    // Line tracking
    ଶୂନ୍ୟ createHandle(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = ସତ୍ୟ;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    ଶୂନ୍ୟ ଧାଡ଼ି_Track(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        ଯଦି ((m_state.ଧାଡ଼ି_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_ଧାଡ଼ିଗୁଡ଼ିକ[m_state.m_handle].ଭର୍ତ୍ତି(lineno);
            }
        }
    }
    ସ୍ଥିର string ଧାଡ଼ିଗୁଡ଼ିକ_First_Last(ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା first, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା last) {
        ଯଦି (first && first == last) {
            ଫେରନ୍ତୁ cvtToStr(first);
        } ଅନ୍ୟଥା ଯଦି (first && last) {
            ଫେରନ୍ତୁ cvtToStr(first) + "-" + cvtToStr(last);
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ "";
        }
    }
    string ଧାଡ଼ିଗୁଡ଼ିକ_Cov(ଅଚଳ CheckState& state, ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        ଅଚଳ LinenoSet& ଧାଡ଼ିଗୁଡ଼ିକ = m_handle_ଧାଡ଼ିଗୁଡ଼ିକ[state.m_handle];
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା first = 0;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା last = 0;
        ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା linen : ଧାଡ଼ିଗୁଡ଼ିକ) {
            ଯଦି (!first) {
                first = last = linen;
            } ଅନ୍ୟଥା ଯଦି (linen == last + 1) {
                ++last;
            } ଅନ୍ୟଥା {
                ଯଦି (!out.empty()) out += ",";
                out += ଧାଡ଼ିଗୁଡ଼ିକ_First_Last(first, last);
                first = last = linen;
            }
        }
        ଯଦି (first) {
            ଯଦି (!out.empty()) out += ",";
            out += ଧାଡ଼ିଗୁଡ଼ିକ_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        ଫେରନ୍ତୁ out;
    }

    // VISITORS - BOTH
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ପଦସମଷ୍ଟି_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_ଶୀର୍ଷ();  // Already made top shell, no coverage for it
        ଯଦି (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_ଧାଡ଼ିଗୁଡ଼ିକ.clear();
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_ପଦସମଷ୍ଟି_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_ପଦସମଷ୍ଟି_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_ପଦସମଷ୍ଟି_Stmtsp = nodep;
            m_inToggleOff = ସତ୍ୟ;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            ଫେରନ୍ତୁ;
        }
        iterateProcedure(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override { iterateProcedure(nodep); }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_ପଦସମଷ୍ଟି_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_ପଦସମଷ୍ଟି_Stmtsp = nodep;
        m_inToggleOff = ସତ୍ୟ;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        ଯଦି (m_state.ଧାଡ଼ି_Coverage_On(nodep)) {
            AstCoverOtherDecl* ଅଚଳ declp
                = ନୂତନ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ଧାଡ଼ି(nodep, "block"));
            ଭର୍ତ୍ତି_Proc_Statement(nodep, newp);
        }
    }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        ଯଦି (nodep->user2SetOnce()) ଫେରନ୍ତୁ;
        ଧାଡ଼ି_Track(nodep);
        ଯଦି (m_state.ଧାଡ଼ି_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ଅଚଳ declp
                = ନୂତନ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ଧାଡ଼ି(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_ପଦସମଷ୍ଟି_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        ଯଦି (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Proc_Statement(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* stmtp) {
        ଯଦି (Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ itemp = VN_CAST(nodep, ଗଣ୍ଠି_Procedure)) {
            itemp->addStmtsp(stmtp);
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ itemp = VN_CAST(nodep, ଗଣ୍ଠି_FTask)) {
            itemp->addStmtsp(stmtp);
        } ଅନ୍ୟଥା ଯଦି (AstLoop* ଅଚଳ itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } ଅନ୍ୟଥା ଯଦି (AstIf* ଅଚଳ itemp = VN_CAST(nodep, If)) {
            ଯଦି (m_then) {
                itemp->addThensp(stmtp);
            } ଅନ୍ୟଥା {
                itemp->addElsesp(stmtp);
            }
        } ଅନ୍ୟଥା ଯଦି (AstBegin* ଅଚଳ itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    ଶୂନ୍ୟ iterateProcedure(Ast_ଗଣ୍ଠି* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_ପଦସମଷ୍ଟି_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_ପଦସମଷ୍ଟି_Stmtsp = nodep;
        m_inToggleOff = ସତ୍ୟ;
        createHandle(nodep);
        iterateChildren(nodep);
        ଯଦି (m_state.ଧାଡ଼ି_Coverage_On(nodep)) {
            ଧାଡ଼ି_Track(nodep);
            AstCoverOtherDecl* ଅଚଳ declp
                = ନୂତନ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_ଧାଡ଼ି(nodep, "block"));
            ଭର୍ତ୍ତି_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ଅଚଳ ଅକ୍ଷର* ଅଚଳ disablep = varIgnoreToggle(nodep);
            ଯଦି (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } ଅନ୍ୟଥା {
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
                ଅଚଳ string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_ଧାଡ଼ି* ଅଚଳ fl_nowarn = ନୂତନ File_ଧାଡ଼ି{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ସତ୍ୟ);
                AstVar* ଅଚଳ chgVarp
                    = ନୂତନ AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, ନୂତନ AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 ନୂତନ AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    ଶୂନ୍ୟ toggleVarBottom(ଅଚଳ ToggleEnt& above, ଅଚଳ AstVar* varp, ଅଚଳ VNumRange& range) {
        ଅଚଳ std::string hierPrefix
            = (m_beginHier != "") ? Ast_ଗଣ୍ଠି::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ଅଚଳ declp
            = ନୂତନ AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* ଅଚଳ newp = ନୂତନ AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), above.m_chgRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
        m_modp->addStmtsp(newp);
    }

    ଶୂନ୍ୟ toggleVarRecurse(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth,  // per-iteration
                          ଅଚଳ ToggleEnt& above, ଅଚଳ AstVar* ଅଚଳ varp) {  // Constant
        ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା index_docs : adtypep->declRange().seqLoToHi()) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 ନୂତନ AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), index_code},
                                 ନୂତନ AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstPackArrayDType* ଅଚଳ adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା index_docs : adtypep->declRange().seqLoToHi()) {
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = adtypep->subDTypep()->skipRefp();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 ନୂତନ AstSel{varp->fileline(), above.m_varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                            index_code * subtypep->width(), subtypep->width()},
                                 ନୂତନ AstSel{varp->fileline(), above.m_chgRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstStructDType* ଅଚଳ adtypep = VN_CAST(dtypep, StructDType)) {
            ଯଦି (adtypep->packed()) {
                ପାଇଁ (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = itemp->subDTypep()->skipRefp();
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        ନୂତନ AstSel{varp->fileline(), above.m_varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), index_code,
                                   subtypep->width()},
                        ନୂତନ AstSel{varp->fileline(), above.m_chgRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } ଅନ୍ୟଥା {
                ପାଇଁ (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = itemp->subDTypep()->skipRefp();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ varRefp = ନୂତନ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), itemp->name()};
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ chgRefp = ନୂତନ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnionDType* ଅଚଳ adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            ଯଦି (ଅଚଳ AstMemberDType* ଅଚଳ itemp = adtypep->membersp()) {
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = itemp->subDTypep()->skipRefp();
                ଯଦି (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                     above.m_chgRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } ଅନ୍ୟଥା {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ varRefp = ନୂତନ AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), itemp->name()};
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ chgRefp = ନୂତନ AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } ଅନ୍ୟଥା {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    ତାର୍କିକ includeCondToBranchRecursive(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ backp = nodep->backp();
        ଯଦି (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            ଫେରନ୍ତୁ includeCondToBranchRecursive(backp);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            ଫେରନ୍ତୁ includeCondToBranchRecursive(backp);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(backp, ଗଣ୍ଠି_Assign) && VN_AS(backp, ଗଣ୍ଠି_Assign)->rhsp() == nodep) {
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // VISITORS - LINE COVERAGE
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        ଯଦି (m_seeking == NONE) coverExprs(nodep->condp());

        ଯଦି (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ଅଚଳ CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            ଧାଡ଼ି_Track(nodep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* ଅଚଳ thenDeclp
                = ନୂତନ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_ଧାଡ଼ି(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* ଅଚଳ elseDeclp
                = ନୂତନ AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_ଧାଡ଼ି(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } ଅନ୍ୟଥା {
            ଧାଡ଼ି_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    ଶୂନ୍ୟ visit(AstIf* nodep) override {
        ଯଦି (nodep->user2()) ଫେରନ୍ତୁ;

        UINFO(4, " IF: " << nodep);
        ଯଦି (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ଅଚଳ ତାର୍କିକ elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            ଯଦି (elsif) VN_AS(nodep->elsesp(), If)->user1(ସତ୍ୟ);
            ଅଚଳ ତାର୍କିକ first_elsif = !nodep->user1() && elsif;
            ଅଚଳ ତାର୍କିକ cont_elsif = nodep->user1() && elsif;
            ଅଚଳ ତାର୍କିକ final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ଅଚଳ CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_ପଦସମଷ୍ଟି_Stmtsp);
                VL_RESTORER(m_then);
                m_ପଦସମଷ୍ଟି_Stmtsp = nodep;
                m_then = ସତ୍ୟ;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                ଧାଡ଼ି_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_ପଦସମଷ୍ଟି_Stmtsp);
                VL_RESTORER(m_then);
                m_ପଦସମଷ୍ଟି_Stmtsp = nodep;
                m_then = ମିଥ୍ୟା;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            ଯଦି (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ଅଚଳ ifDeclp
                    = ନୂତନ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_ଧାଡ଼ି(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ଅଚଳ elseDeclp
                    = ନୂତନ AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_ଧାଡ଼ି(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            ଅନ୍ୟଥା ଯଦି (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                ଯଦି (ifState.ଧାଡ଼ି_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ଅଚଳ elsifDeclp = ନୂତନ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        ଧାଡ଼ିଗୁଡ଼ିକ_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_ଧାଡ଼ି(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } ଅନ୍ୟଥା {
                // Cover as separate blocks (not a branch as is not two-legged)
                ଯଦି (ifState.ଧାଡ଼ି_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ଅଚଳ ifDeclp = ନୂତନ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        ଧାଡ଼ିଗୁଡ଼ିକ_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_ଧାଡ଼ି(nodep, "if")));
                }
                ଯଦି (elseState.ଧାଡ଼ି_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ଅଚଳ elseDeclp = ନୂତନ AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        ଧାଡ଼ିଗୁଡ଼ିକ_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_ଧାଡ଼ି(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = ସତ୍ୟ;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        ଯଦି (m_state.ଧାଡ଼ି_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            ଯଦି (m_state.ଧାଡ଼ି_Coverage_On(nodep)) {  // if the case body didn't disable it
                ଧାଡ଼ି_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ଅଚଳ declp
                    = ନୂତନ AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_ଧାଡ଼ି(nodep, "case")));
            }
        }
    }
    ଶୂନ୍ୟ visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = ସତ୍ୟ;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        ଯଦି (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            ଧାଡ଼ି_Track(nodep);
            AstCoverOtherDecl* ଅଚଳ declp
                = ନୂତନ AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", ଧାଡ଼ିଗୁଡ଼ିକ_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    ଶୂନ୍ୟ visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_ପଦସମଷ୍ଟି_Stmtsp);
        m_ପଦସମଷ୍ଟି_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ visit(AstStmtPragma* nodep) override {
        ଯଦି (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = ମିଥ୍ୟା;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା {
            ଯଦି (m_state.m_on) iterateChildren(nodep);
            ଧାଡ଼ି_Track(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        ଯଦି (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        ଧାଡ଼ି_Track(nodep);
    }

    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_ପଦସମଷ୍ଟି_Stmtsp);
        m_ପଦସମଷ୍ଟି_Stmtsp = nodep;
        m_inToggleOff = ସତ୍ୟ;
        ଯଦି (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        ଧାଡ଼ି_Track(nodep);
    }

    ଶୂନ୍ୟ abort_ପଦସମଷ୍ଟି_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        ଯଦି (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    ତାର୍କିକ checkMaxExprs(size_t additional = 0) {
        ଯଦି (m_seeking != ABORTED
            && static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_exprs.size() + additional) <= v3Global.opt.coverage_ପଦସମଷ୍ଟି_Max())
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        abort_ପଦସମଷ୍ଟି_Coverage();
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ add_ପଦସମଷ୍ଟି_Cover_Inc(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା start = 0) {
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣନା_2 = start;
        ପାଇଁ (Cover_ପଦସମଷ୍ଟି& ପଦସମଷ୍ଟି : m_exprs) {
            ଅଚଳ string name = "expr_" + std::to_string(ଗଣନା_2);
            string comment = "(";
            ତାର୍କିକ first = ସତ୍ୟ;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;
            ପାଇଁ (CoverTerm& term : ପଦସମଷ୍ଟି) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* covExprp = nullptr;
                ଯଦି (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ frefp = term.m_exprp;
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = frefp->dtypep();
                    ଅଚଳ auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    ଯଦି (pair.second) {
                        varp = ନୂତନ AstVar{fl, VVarType::MODULETEMP, m_ପଦସମଷ୍ଟି_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        ଯଦି (m_ftaskp) {
                            varp->funcLocal(ସତ୍ୟ);
                            m_ftaskp->prependStmtsp(varp);
                        } ଅନ୍ୟଥା {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{
                            fl, ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            ନୂତନ AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = ନୂତନ AstVarRef{fl, varp, VAccess::READ};
                } ଅନ୍ୟଥା {
                    covExprp = term.m_exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                }
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp
                    = term.m_objective ? covExprp : ନୂତନ AstLogNot{fl, covExprp};
                ଯଦି (condp) {
                    condp = ନୂତନ AstLogAnd{fl, condp, termp};
                } ଅନ୍ୟଥା {
                    condp = termp;
                }
                first = ମିଥ୍ୟା;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ଅଚଳ declp = ନୂତନ AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = newCoverInc(fl, declp, trace_Name_For_ଧାଡ଼ି(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{fl, condp, newp, nullptr};
            ifp->user2(ସତ୍ୟ);
            ଭର୍ତ୍ତି_Proc_Statement(m_ପଦସମଷ୍ଟି_Stmtsp, ifp);
            ++ଗଣନା_2;
        }
    }

    ଶୂନ୍ୟ coverExprs(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (!m_state.ପଦସମଷ୍ଟି_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_ପଦସମଷ୍ଟି_Stmtsp) {
            ଫେରନ୍ତୁ;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = ମିଥ୍ୟା;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = ସତ୍ୟ;
        iterate(nodep);
        ଯଦି (checkMaxExprs(falseExprs.size())) ଫେରନ୍ତୁ;
        ଯଦି (m_seeking == ABORTED) ଫେରନ୍ତୁ;

        add_ପଦସମଷ୍ଟି_Cover_Inc(nodep);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା start = m_exprs.size();
        m_objective = ମିଥ୍ୟା;
        m_exprs.swap(falseExprs);
        add_ପଦସମଷ୍ଟି_Cover_Inc(nodep, start);
    }

    ଶୂନ୍ୟ ପଦସମଷ୍ଟି_Either(Ast_ଗଣ୍ଠି_Biop* nodep, ତାର୍କିକ overrideObjective = ମିଥ୍ୟା, ତାର୍କିକ lObjective = ମିଥ୍ୟା,
                    ତାର୍କିକ rObjective = ମିଥ୍ୟା) {
        VL_RESTORER(m_objective);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp();

        ଯଦି (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ଯଦି (checkMaxExprs()) ଫେରନ୍ତୁ;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        ଯଦି (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    ଶୂନ୍ୟ ପଦସମଷ୍ଟି_Both(Ast_ଗଣ୍ଠି_Biop* nodep, ତାର୍କିକ overrideObjective = ମିଥ୍ୟା, ତାର୍କିକ lObjective = ମିଥ୍ୟା,
                  ତାର୍କିକ rObjective = ମିଥ୍ୟା) {
        VL_RESTORER(m_objective);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp();

        ଯଦି (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ଯଦି (checkMaxExprs()) ଫେରନ୍ତୁ;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        ଯଦି (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        ଯଦି (checkMaxExprs()) ଫେରନ୍ତୁ;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        ପାଇଁ (ଅଚଳ Cover_ପଦସମଷ୍ଟି& l : lhsExprs) {
            ପାଇଁ (ଅଚଳ Cover_ପଦସମଷ୍ଟି& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_ପଦସମଷ୍ଟି ପଦସମଷ୍ଟି;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                ପାଇଁ (ଅଚଳ CoverTerm& term : l) {
                    ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].ଭର୍ତ୍ତି(refp->varp());
                    } ଅନ୍ୟଥା {
                        strs[term.m_objective].ଭର୍ତ୍ତି(term.m_emitV);
                    }
                    ପଦସମଷ୍ଟି.ଠେଲ_back(term);
                }
                ତାର୍କିକ impossible = ମିଥ୍ୟା;
                ପାଇଁ (ଅଚଳ CoverTerm& term : r) {
                    ତାର୍କିକ redundant = ମିଥ୍ୟା;
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp = VN_CAST(term.m_exprp, ଗଣ୍ଠି_Var_Ref)) {
                        ଯଦି (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = ସତ୍ୟ;
                        ଯଦି (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = ସତ୍ୟ;
                    } ଅନ୍ୟଥା {
                        ଯଦି (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = ସତ୍ୟ;
                        ଯଦି (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = ସତ୍ୟ;
                    }
                    ଯଦି (!redundant) ପଦସମଷ୍ଟି.ଠେଲ_back(term);
                }
                ଯଦି (!impossible) m_exprs.ଠେଲ_back(std::move(ପଦସମଷ୍ଟି));
                ଯଦି (checkMaxExprs()) ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ or_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଯଦି (m_seeking == NONE) {
            coverExprs(nodep);
        } ଅନ୍ୟଥା ଯଦି (m_objective) {
            ପଦସମଷ୍ଟି_Either(nodep);
        } ଅନ୍ୟଥା {
            ପଦସମଷ୍ଟି_Both(nodep);
        }
        ଧାଡ଼ି_Track(nodep);
    }
    ଶୂନ୍ୟ visit(AstLogOr* nodep) override { or_ପଦସମଷ୍ଟି(nodep); }
    ଶୂନ୍ୟ visit(AstOr* nodep) override { or_ପଦସମଷ୍ଟି(nodep); }

    ଶୂନ୍ୟ and_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଯଦି (m_seeking == NONE) {
            coverExprs(nodep);
        } ଅନ୍ୟଥା ଯଦି (m_objective) {
            ପଦସମଷ୍ଟି_Both(nodep);
        } ଅନ୍ୟଥା {
            ପଦସମଷ୍ଟି_Either(nodep);
        }
        ଧାଡ଼ି_Track(nodep);
    }
    ଶୂନ୍ୟ visit(AstLogAnd* nodep) override { and_ପଦସମଷ୍ଟି(nodep); }
    ଶୂନ୍ୟ visit(AstAnd* nodep) override { and_ପଦସମଷ୍ଟି(nodep); }

    ଶୂନ୍ୟ xor_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଯଦି (m_seeking == NONE) {
            coverExprs(nodep);
        } ଅନ୍ୟଥା {
            ପାଇଁ (ଅଚଳ ତାର୍କିକ lObjective : {ମିଥ୍ୟା, ସତ୍ୟ}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                ଅଚଳ ତାର୍କିକ rObjective = lObjective ^ m_objective;
                ପଦସମଷ୍ଟି_Both(nodep, ସତ୍ୟ, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                ଯଦି (checkMaxExprs()) ଭାଙ୍ଗ;
            }
        }
        ଧାଡ଼ି_Track(nodep);
    }
    ଶୂନ୍ୟ visit(AstXor* nodep) override { xor_ପଦସମଷ୍ଟି(nodep); }

    ଶୂନ୍ୟ ପଦସମଷ୍ଟି_Not(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        VL_RESTORER(m_objective);
        ଯଦି (m_seeking == NONE) {
            coverExprs(nodep);
        } ଅନ୍ୟଥା {
            m_objective = !m_objective;
            iterateChildren(nodep);
            ଧାଡ଼ି_Track(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstNot* nodep) override { ପଦସମଷ୍ଟି_Not(nodep); }
    ଶୂନ୍ୟ visit(AstLogNot* nodep) override { ପଦସମଷ୍ଟି_Not(nodep); }

    template <typename T_Oper>
    ଶୂନ୍ୟ ପଦସମଷ୍ଟି_Reduce(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        ଯଦି (m_seeking != ABORTED) {
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = lhsp->dtypep()->width();
            ଅଚଳ size_t expected = std::is_same<T_Oper, AstXor>::ମୂଲ୍ୟ ? 0x1 << width : width + 1;
            ଯଦି (checkMaxExprs(expected)) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* unrolledp = ନୂତନ AstSel{
                fl, lhsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ନୂତନ AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = width - 2; bit >= 0; bit--) {
                AstSel* ଅଚଳ selp = ନୂତନ AstSel{fl, lhsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                                ନୂତନ AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = ନୂତନ T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ଠେଲ_Deletep(unrolledp), unrolledp);
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
            ଧାଡ଼ି_Track(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstRedOr* nodep) override { ପଦସମଷ୍ଟି_Reduce<AstOr>(nodep); }
    ଶୂନ୍ୟ visit(AstRedAnd* nodep) override { ପଦସମଷ୍ଟି_Reduce<AstAnd>(nodep); }
    ଶୂନ୍ୟ visit(AstRedXor* nodep) override { ପଦସମଷ୍ଟି_Reduce<AstXor>(nodep); }

    ଶୂନ୍ୟ visit(AstLogIf* nodep) override {
        ଯଦି (m_seeking == NONE) {
            coverExprs(nodep);
        } ଅନ୍ୟଥା ଯଦି (m_objective) {
            ପଦସମଷ୍ଟି_Either(nodep, ସତ୍ୟ, ମିଥ୍ୟା, ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            ପଦସମଷ୍ଟି_Both(nodep, ସତ୍ୟ, ସତ୍ୟ, ମିଥ୍ୟା);
        }
        ଧାଡ଼ି_Track(nodep);
    }

    ଶୂନ୍ୟ visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        ଯଦି (m_seeking == NONE) {
            coverExprs(nodep);
        } ଅନ୍ୟଥା {
            m_objective = !m_objective;
            xor_ପଦସମଷ୍ଟି(nodep);
            ଧାଡ଼ି_Track(nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstFuncRef* nodep) override {
        ଯଦି (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(nodep));
        } ଅନ୍ୟଥା {
            ପଦସମଷ୍ଟି_Unsupported(nodep, "non-automatic function");
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            ପଦସମଷ୍ଟି_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            ଯଦି (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_ବୃକ୍ଷ(nodep, emitV);
                // Add new expression with a single term
                Cover_ପଦସମଷ୍ଟି ପଦସମଷ୍ଟି;
                ପଦସମଷ୍ଟି.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.ଠେଲ_back(std::move(ପଦସମଷ୍ଟି));
                checkMaxExprs();
            } ଅନ୍ୟଥା {
                ପଦସମଷ୍ଟି_Unsupported(nodep, "not coverage eligible");
            }
        }
        ଧାଡ଼ି_Track(nodep);
    }

    // VISITORS - BOTH
    ଶୂନ୍ୟ visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = ମିଥ୍ୟା;
        m_inToggleOff = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = ମିଥ୍ୟା;
        m_inToggleOff = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        iterateChildren(nodep);
        ଧାଡ଼ି_Track(nodep);
    }

    ଶୂନ୍ୟ ପଦସମଷ୍ଟି_Unsupported(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ଅଚଳ ତାର୍କିକ wasSeeking = m_seeking == SEEKING;
        ଅଚଳ Objective oldSeeking = m_seeking;
        ଯଦି (wasSeeking) abort_ପଦସମଷ୍ଟି_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        ଧାଡ଼ି_Track(nodep);
        ଯଦି (!wasSeeking) m_seeking = oldSeeking;
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_ପଦସମଷ୍ଟି_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Coverage class functions

ଶୂନ୍ୟ V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("coverage", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
