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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Coverage.h"

#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <list>
#அந்தர்பாவ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு கோவை_Coverage_Eligible_Visitor final : பொது VNVisitorConst {
    // STATE
    தருக்க m_eligible = உண்மை;

    நிலையான தருக்க elemDTypeEligible(மாறிலி Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        எனில் (மாறிலி Ast_முனை_DType* மாறிலி dtp = dtypep->virtRefDTypep()) {
            எனில் (!elemDTypeEligible(dtp)) திரும்பு பொய்;
        }
        எனில் (மாறிலி Ast_முனை_DType* மாறிலி dtp = dtypep->virtRefDType2p()) {
            எனில் (!elemDTypeEligible(dtp)) திரும்பு பொய்;
        }
        திரும்பு !VN_IS(dtypep, ClassRefDType);
    }

    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        எனில் (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } இல்லையெனில் {
            m_eligible = பொய்;
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (!nodep->is_கோவை_Coverage_Eligible()) {
            m_eligible = பொய்;
        } இல்லையெனில் {
            iterateChildrenConst(nodep);
        }
    }

பொது:
    // CONSTRUCTORS
    explicit கோவை_Coverage_Eligible_Visitor(Ast_முனை* nodep) { iterateConst(nodep); }
    ~கோவை_Coverage_Eligible_Visitor() override = இயல்பு;

    தருக்க eligible() மாறிலி { திரும்பு m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

வகுப்பு CoverageVisitor final : பொது VNVisitor {
    // TYPES
    using LinenoSet = std::set<முழு>;

    கட்டமைப்பு CoverTerm final {
        Ast_முனை_கோவை* m_exprp;  // Expression branch term
        தருக்க m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_முனை_கோவை* exprp, தருக்க objective, மாறிலி string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_கோவை = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_கோவை>;

    கட்டமைப்பு ToggleEnt final {
        மாறிலி string m_comment;  // Comment for coverage dump
        Ast_முனை_கோவை* m_varRefp;  // How to get to this element
        Ast_முனை_கோவை* m_chgRefp;  // How to get to this element
        ToggleEnt(மாறிலி string& comment, Ast_முனை_கோவை* vp, Ast_முனை_கோவை* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = இயல்பு;
        வெறுமை cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_மரம்(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_மரம்(), m_chgRefp = nullptr);
        }
    };

    கட்டமைப்பு CheckState final {  // State save-restored on each new coverage scope/block
        தருக்க m_on = பொய்;  // Should this block get covered?
        தருக்க m_inModOff = பொய்;  // In module with no coverage
        முழு m_handle = 0;  // Opaque handle for index into line tracking
        மாறிலி Ast_முனை* m_nodep = nullptr;  // Node establishing this state
        CheckState() = இயல்பு;
        தருக்க coverageOn(மாறிலி Ast_முனை* nodep) மாறிலி {
            திரும்பு m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        தருக்க வரி_Coverage_On(மாறிலி Ast_முனை* nodep) மாறிலி {
            திரும்பு coverageOn(nodep) && v3Global.opt.coverage_வரி();
        }
        தருக்க branchCoverageOn(மாறிலி Ast_முனை* nodep) மாறிலி {
            திரும்பு coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        தருக்க கோவை_Coverage_On(மாறிலி Ast_முனை* nodep) மாறிலி {
            திரும்பு coverageOn(nodep) && v3Global.opt.coverage_கோவை();
        }
    };

    எண்ணிக்கை Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    V3UniqueNames m_கோவை_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_முனை_கோவை*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    முழு m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_முனை_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_முனை* m_கோவை_Stmtsp = nullptr;  // Node to add expr coverage to
    தருக்க m_then = பொய்;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    தருக்க m_objective = பொய்;  // Expression objective
    தருக்க m_ifCond = பொய்;  // Visiting if condition
    தருக்க m_inToggleOff = பொய்;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<முழு, LinenoSet> m_handle_வரிகள்;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    மாறிலி எழுத்து* varIgnoreToggle(மாறிலி AstVar* nodep) {
        மாறிலி தருக்க cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        எனில் (!cover) திரும்பு "Not relevant signal";
        எனில் (nodep->isConst()) திரும்பு "Signal is constant";
        எனில் (nodep->isDouble()) திரும்பு "Signal is double";
        எனில் (nodep->isString()) திரும்பு "Signal is string";
        எனில் (!v3Global.opt.coverageUnderscore()) {
            மாறிலி string prettyName = nodep->prettyName();
            எனில் (prettyName[0] == '_') திரும்பு "Leading underscore";
            எனில் (prettyName.find("._") != string::npos) திரும்பு "Inlined leading underscore";
        }
        எனில் ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            திரும்பு "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        திரும்பு nullptr;
    }

    AstCoverInc* newCoverInc(File_வரி* fl, Ast_முனை_Cover_Decl* மாறிலி declp,
                             மாறிலி string& trace_var_name) {
        AstCoverInc* மாறிலி incp = புதிய AstCoverInc{fl, declp};
        எனில் (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_வரி* மாறிலி fl_nowarn = புதிய File_வரி{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, உண்மை);
            AstVar* மாறிலி varp = புதிய AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(உண்மை);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* மாறிலி assp = புதிய AstAssign{
                incp->fileline(), புதிய AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                புதிய AstAdd{incp->fileline(), புதிய AstVarRef{incp->fileline(), varp, VAccess::READ},
                           புதிய AstConst{incp->fileline(), AstConst::Widthed_மதிப்பு{}, 32, 1}}};
            Ast_முனை::addNext<Ast_முனை, Ast_முனை>(incp, assp);
        }
        திரும்பு incp;
    }
    string trace_Name_For_வரி(மாறிலி Ast_முனை* nodep, மாறிலி string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        எனில் (மாறிலி uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        திரும்பு name;
    }

    // Line tracking
    வெறுமை createHandle(மாறிலி Ast_முனை* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = உண்மை;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    வெறுமை வரி_Track(மாறிலி Ast_முனை* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        எனில் ((m_state.வரி_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            க்கு (முழு lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_வரிகள்[m_state.m_handle].செருகு(lineno);
            }
        }
    }
    நிலையான string வரிகள்_First_Last(மாறிலி முழு first, மாறிலி முழு last) {
        எனில் (first && first == last) {
            திரும்பு cvtToStr(first);
        } இல்லையெனில் எனில் (first && last) {
            திரும்பு cvtToStr(first) + "-" + cvtToStr(last);
        } இல்லையெனில் {
            திரும்பு "";
        }
    }
    string வரிகள்_Cov(மாறிலி CheckState& state, மாறிலி Ast_முனை* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        மாறிலி LinenoSet& வரிகள் = m_handle_வரிகள்[state.m_handle];
        முழு first = 0;
        முழு last = 0;
        க்கு (மாறிலி முழு linen : வரிகள்) {
            எனில் (!first) {
                first = last = linen;
            } இல்லையெனில் எனில் (linen == last + 1) {
                ++last;
            } இல்லையெனில் {
                எனில் (!out.empty()) out += ",";
                out += வரிகள்_First_Last(first, last);
                first = last = linen;
            }
        }
        எனில் (first) {
            எனில் (!out.empty()) out += ",";
            out += வரிகள்_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        திரும்பு out;
    }

    // VISITORS - BOTH
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        மாறிலி Ast_முனை_Module* மாறிலி origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_கோவை_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_உச்சி();  // Already made top shell, no coverage for it
        எனில் (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_வரிகள்.clear();
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_கோவை_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    வெறுமை visit(AstAlways* nodep) override {
        எனில் (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_கோவை_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_கோவை_Stmtsp = nodep;
            m_inToggleOff = உண்மை;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            திரும்பு;
        }
        iterateProcedure(nodep);
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override { iterateProcedure(nodep); }
    வெறுமை visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_கோவை_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_கோவை_Stmtsp = nodep;
        m_inToggleOff = உண்மை;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        எனில் (m_state.வரி_Coverage_On(nodep)) {
            AstCoverOtherDecl* மாறிலி declp
                = புதிய AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", வரிகள்_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_முனை* மாறிலி newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_வரி(nodep, "block"));
            செருகு_Proc_Statement(nodep, newp);
        }
    }
    வெறுமை visit(AstLoopTest* nodep) override {
        எனில் (nodep->user2SetOnce()) திரும்பு;
        வரி_Track(nodep);
        எனில் (m_state.வரி_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* மாறிலி declp
                = புதிய AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", வரிகள்_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_முனை* மாறிலி newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_வரி(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_கோவை_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        எனில் (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    வெறுமை செருகு_Proc_Statement(Ast_முனை* nodep, Ast_முனை* stmtp) {
        எனில் (Ast_முனை_Procedure* மாறிலி itemp = VN_CAST(nodep, முனை_Procedure)) {
            itemp->addStmtsp(stmtp);
        } இல்லையெனில் எனில் (Ast_முனை_FTask* மாறிலி itemp = VN_CAST(nodep, முனை_FTask)) {
            itemp->addStmtsp(stmtp);
        } இல்லையெனில் எனில் (AstLoop* மாறிலி itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } இல்லையெனில் எனில் (AstIf* மாறிலி itemp = VN_CAST(nodep, If)) {
            எனில் (m_then) {
                itemp->addThensp(stmtp);
            } இல்லையெனில் {
                itemp->addElsesp(stmtp);
            }
        } இல்லையெனில் எனில் (AstBegin* மாறிலி itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } இல்லையெனில் {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    வெறுமை iterateProcedure(Ast_முனை* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_கோவை_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_கோவை_Stmtsp = nodep;
        m_inToggleOff = உண்மை;
        createHandle(nodep);
        iterateChildren(nodep);
        எனில் (m_state.வரி_Coverage_On(nodep)) {
            வரி_Track(nodep);
            AstCoverOtherDecl* மாறிலி declp
                = புதிய AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", வரிகள்_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_முனை* மாறிலி newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_வரி(nodep, "block"));
            செருகு_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    வெறுமை visit(AstVar* nodep) override {
        iterateChildren(nodep);
        எனில் (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            மாறிலி எழுத்து* மாறிலி disablep = varIgnoreToggle(nodep);
            எனில் (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } இல்லையெனில் {
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
                மாறிலி string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_வரி* மாறிலி fl_nowarn = புதிய File_வரி{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, உண்மை);
                AstVar* மாறிலி chgVarp
                    = புதிய AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, புதிய AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 புதிய AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    வெறுமை toggleVarBottom(மாறிலி ToggleEnt& above, மாறிலி AstVar* varp, மாறிலி VNumRange& range) {
        மாறிலி std::string hierPrefix
            = (m_beginHier != "") ? Ast_முனை::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* மாறிலி declp
            = புதிய AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* மாறிலி newp = புதிய AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_மரம்(பொய்), above.m_chgRefp->clone_மரம்(பொய்)};
        m_modp->addStmtsp(newp);
    }

    வெறுமை toggleVarRecurse(மாறிலி Ast_முனை_DType* மாறிலி dtypep, மாறிலி முழு depth,  // per-iteration
                          மாறிலி ToggleEnt& above, மாறிலி AstVar* மாறிலி varp) {  // Constant
        எனில் (மாறிலி AstBasicDType* மாறிலி basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } இல்லையெனில் எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            க்கு (மாறிலி முழு index_docs : adtypep->declRange().seqLoToHi()) {
                மாறிலி முழு index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 புதிய AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_மரம்(பொய்), index_code},
                                 புதிய AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_மரம்(பொய்), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } இல்லையெனில் எனில் (மாறிலி AstPackArrayDType* மாறிலி adtypep = VN_CAST(dtypep, PackArrayDType)) {
            க்கு (மாறிலி முழு index_docs : adtypep->declRange().seqLoToHi()) {
                மாறிலி Ast_முனை_DType* மாறிலி subtypep = adtypep->subDTypep()->skipRefp();
                மாறிலி முழு index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 புதிய AstSel{varp->fileline(), above.m_varRefp->clone_மரம்(பொய்),
                                            index_code * subtypep->width(), subtypep->width()},
                                 புதிய AstSel{varp->fileline(), above.m_chgRefp->clone_மரம்(பொய்),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } இல்லையெனில் எனில் (மாறிலி AstStructDType* மாறிலி adtypep = VN_CAST(dtypep, StructDType)) {
            எனில் (adtypep->packed()) {
                க்கு (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    மாறிலி Ast_முனை_DType* மாறிலி subtypep = itemp->subDTypep()->skipRefp();
                    மாறிலி முழு index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        புதிய AstSel{varp->fileline(), above.m_varRefp->clone_மரம்(பொய்), index_code,
                                   subtypep->width()},
                        புதிய AstSel{varp->fileline(), above.m_chgRefp->clone_மரம்(பொய்), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } இல்லையெனில் {
                க்கு (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_முனை_DType* மாறிலி subtypep = itemp->subDTypep()->skipRefp();
                    Ast_முனை_கோவை* மாறிலி varRefp = புதிய AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_மரம்(பொய்), itemp->name()};
                    Ast_முனை_கோவை* மாறிலி chgRefp = புதிய AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_மரம்(பொய்), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } இல்லையெனில் எனில் (மாறிலி AstUnionDType* மாறிலி adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            எனில் (மாறிலி AstMemberDType* மாறிலி itemp = adtypep->membersp()) {
                Ast_முனை_DType* மாறிலி subtypep = itemp->subDTypep()->skipRefp();
                எனில் (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_மரம்(பொய்),
                                     above.m_chgRefp->clone_மரம்(பொய்)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } இல்லையெனில் {
                    Ast_முனை_கோவை* மாறிலி varRefp = புதிய AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_மரம்(பொய்), itemp->name()};
                    Ast_முனை_கோவை* மாறிலி chgRefp = புதிய AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_மரம்(பொய்), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } இல்லையெனில் எனில் (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } இல்லையெனில் {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    தருக்க includeCondToBranchRecursive(மாறிலி Ast_முனை* மாறிலி nodep) {
        மாறிலி Ast_முனை* மாறிலி backp = nodep->backp();
        எனில் (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            திரும்பு includeCondToBranchRecursive(backp);
        } இல்லையெனில் எனில் (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            திரும்பு includeCondToBranchRecursive(backp);
        } இல்லையெனில் எனில் (VN_IS(backp, முனை_Assign) && VN_AS(backp, முனை_Assign)->rhsp() == nodep) {
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    // VISITORS - LINE COVERAGE
    வெறுமை visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        எனில் (m_seeking == NONE) coverExprs(nodep->condp());

        எனில் (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            மாறிலி CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            வரி_Track(nodep);
            Ast_முனை_கோவை* மாறிலி thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* மாறிலி thenDeclp
                = புதிய AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", வரிகள்_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(புதிய Ast_கோவை_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_வரி(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_முனை_கோவை* மாறிலி elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* மாறிலி elseDeclp
                = புதிய AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", வரிகள்_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(புதிய Ast_கோவை_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_வரி(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } இல்லையெனில் {
            வரி_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    வெறுமை visit(AstIf* nodep) override {
        எனில் (nodep->user2()) திரும்பு;

        UINFO(4, " IF: " << nodep);
        எனில் (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            மாறிலி தருக்க elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            எனில் (elsif) VN_AS(nodep->elsesp(), If)->user1(உண்மை);
            மாறிலி தருக்க first_elsif = !nodep->user1() && elsif;
            மாறிலி தருக்க cont_elsif = nodep->user1() && elsif;
            மாறிலி தருக்க final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            மாறிலி CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_கோவை_Stmtsp);
                VL_RESTORER(m_then);
                m_கோவை_Stmtsp = nodep;
                m_then = உண்மை;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                வரி_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_கோவை_Stmtsp);
                VL_RESTORER(m_then);
                m_கோவை_Stmtsp = nodep;
                m_then = பொய்;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            எனில் (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* மாறிலி ifDeclp
                    = புதிய AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", வரிகள்_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_வரி(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* மாறிலி elseDeclp
                    = புதிய AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", வரிகள்_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_வரி(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            இல்லையெனில் எனில் (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                எனில் (ifState.வரி_Coverage_On(nodep)) {
                    AstCoverOtherDecl* மாறிலி elsifDeclp = புதிய AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        வரிகள்_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_வரி(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } இல்லையெனில் {
                // Cover as separate blocks (not a branch as is not two-legged)
                எனில் (ifState.வரி_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* மாறிலி ifDeclp = புதிய AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        வரிகள்_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_வரி(nodep, "if")));
                }
                எனில் (elseState.வரி_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* மாறிலி elseDeclp = புதிய AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        வரிகள்_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_வரி(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = உண்மை;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    வெறுமை visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        எனில் (m_state.வரி_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            எனில் (m_state.வரி_Coverage_On(nodep)) {  // if the case body didn't disable it
                வரி_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* மாறிலி declp
                    = புதிய AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", வரிகள்_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_வரி(nodep, "case")));
            }
        }
    }
    வெறுமை visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = உண்மை;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        எனில் (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            வரி_Track(nodep);
            AstCoverOtherDecl* மாறிலி declp
                = புதிய AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", வரிகள்_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    வெறுமை visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_கோவை_Stmtsp);
        m_கோவை_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = பொய்;
    }
    வெறுமை visit(AstStmtPragma* nodep) override {
        எனில் (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = பொய்;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் {
            எனில் (m_state.m_on) iterateChildren(nodep);
            வரி_Track(nodep);
        }
    }
    வெறுமை visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        எனில் (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        வரி_Track(nodep);
    }

    வெறுமை visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_கோவை_Stmtsp);
        m_கோவை_Stmtsp = nodep;
        m_inToggleOff = உண்மை;
        எனில் (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        வரி_Track(nodep);
    }

    வெறுமை abort_கோவை_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        எனில் (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    தருக்க checkMaxExprs(size_t additional = 0) {
        எனில் (m_seeking != ABORTED
            && static_cast<முழு>(m_exprs.size() + additional) <= v3Global.opt.coverage_கோவை_Max())
            திரும்பு பொய்;
        abort_கோவை_Coverage();
        திரும்பு உண்மை;
    }

    வெறுமை add_கோவை_Cover_Inc(Ast_முனை_கோவை* nodep, முழு start = 0) {
        File_வரி* மாறிலி fl = nodep->fileline();
        முழு மொத்தம் = start;
        க்கு (Cover_கோவை& கோவை : m_exprs) {
            மாறிலி string name = "expr_" + std::to_string(மொத்தம்);
            string comment = "(";
            தருக்க first = உண்மை;
            Ast_முனை_கோவை* condp = nullptr;
            க்கு (CoverTerm& term : கோவை) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_முனை_கோவை* covExprp = nullptr;
                எனில் (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_முனை_கோவை* மாறிலி frefp = term.m_exprp;
                    Ast_முனை_DType* மாறிலி dtypep = frefp->dtypep();
                    மாறிலி auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    எனில் (pair.second) {
                        varp = புதிய AstVar{fl, VVarType::MODULETEMP, m_கோவை_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        எனில் (m_ftaskp) {
                            varp->funcLocal(உண்மை);
                            m_ftaskp->prependStmtsp(varp);
                        } இல்லையெனில் {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(புதிய Ast_கோவை_Stmt{
                            fl, புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            புதிய AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = புதிய AstVarRef{fl, varp, VAccess::READ};
                } இல்லையெனில் {
                    covExprp = term.m_exprp->clone_மரம்(பொய்);
                }
                Ast_முனை_கோவை* மாறிலி termp
                    = term.m_objective ? covExprp : புதிய AstLogNot{fl, covExprp};
                எனில் (condp) {
                    condp = புதிய AstLogAnd{fl, condp, termp};
                } இல்லையெனில் {
                    condp = termp;
                }
                first = பொய்;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* மாறிலி declp = புதிய AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_முனை* மாறிலி newp = newCoverInc(fl, declp, trace_Name_For_வரி(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* மாறிலி ifp = புதிய AstIf{fl, condp, newp, nullptr};
            ifp->user2(உண்மை);
            செருகு_Proc_Statement(m_கோவை_Stmtsp, ifp);
            ++மொத்தம்;
        }
    }

    வெறுமை coverExprs(Ast_முனை_கோவை* nodep) {
        எனில் (!m_state.கோவை_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_கோவை_Stmtsp) {
            திரும்பு;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = பொய்;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = உண்மை;
        iterate(nodep);
        எனில் (checkMaxExprs(falseExprs.size())) திரும்பு;
        எனில் (m_seeking == ABORTED) திரும்பு;

        add_கோவை_Cover_Inc(nodep);
        மாறிலி முழு start = m_exprs.size();
        m_objective = பொய்;
        m_exprs.swap(falseExprs);
        add_கோவை_Cover_Inc(nodep, start);
    }

    வெறுமை கோவை_Either(Ast_முனை_Biop* nodep, தருக்க overrideObjective = பொய், தருக்க lObjective = பொய்,
                    தருக்க rObjective = பொய்) {
        VL_RESTORER(m_objective);
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp();

        எனில் (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        எனில் (checkMaxExprs()) திரும்பு;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        எனில் (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    வெறுமை கோவை_Both(Ast_முனை_Biop* nodep, தருக்க overrideObjective = பொய், தருக்க lObjective = பொய்,
                  தருக்க rObjective = பொய்) {
        VL_RESTORER(m_objective);
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp();

        எனில் (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        எனில் (checkMaxExprs()) திரும்பு;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        எனில் (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        எனில் (checkMaxExprs()) திரும்பு;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        க்கு (மாறிலி Cover_கோவை& l : lhsExprs) {
            க்கு (மாறிலி Cover_கோவை& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_கோவை கோவை;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                க்கு (மாறிலி CoverTerm& term : l) {
                    எனில் (மாறிலி AstVarRef* மாறிலி refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].செருகு(refp->varp());
                    } இல்லையெனில் {
                        strs[term.m_objective].செருகு(term.m_emitV);
                    }
                    கோவை.தள்ளு_back(term);
                }
                தருக்க impossible = பொய்;
                க்கு (மாறிலி CoverTerm& term : r) {
                    தருக்க redundant = பொய்;
                    எனில் (மாறிலி Ast_முனை_Var_Ref* மாறிலி refp = VN_CAST(term.m_exprp, முனை_Var_Ref)) {
                        எனில் (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = உண்மை;
                        எனில் (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = உண்மை;
                    } இல்லையெனில் {
                        எனில் (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = உண்மை;
                        எனில் (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = உண்மை;
                    }
                    எனில் (!redundant) கோவை.தள்ளு_back(term);
                }
                எனில் (!impossible) m_exprs.தள்ளு_back(std::move(கோவை));
                எனில் (checkMaxExprs()) திரும்பு;
            }
        }
    }

    வெறுமை or_கோவை(Ast_முனை_Biop* nodep) {
        எனில் (m_seeking == NONE) {
            coverExprs(nodep);
        } இல்லையெனில் எனில் (m_objective) {
            கோவை_Either(nodep);
        } இல்லையெனில் {
            கோவை_Both(nodep);
        }
        வரி_Track(nodep);
    }
    வெறுமை visit(AstLogOr* nodep) override { or_கோவை(nodep); }
    வெறுமை visit(AstOr* nodep) override { or_கோவை(nodep); }

    வெறுமை and_கோவை(Ast_முனை_Biop* nodep) {
        எனில் (m_seeking == NONE) {
            coverExprs(nodep);
        } இல்லையெனில் எனில் (m_objective) {
            கோவை_Both(nodep);
        } இல்லையெனில் {
            கோவை_Either(nodep);
        }
        வரி_Track(nodep);
    }
    வெறுமை visit(AstLogAnd* nodep) override { and_கோவை(nodep); }
    வெறுமை visit(AstAnd* nodep) override { and_கோவை(nodep); }

    வெறுமை xor_கோவை(Ast_முனை_Biop* nodep) {
        எனில் (m_seeking == NONE) {
            coverExprs(nodep);
        } இல்லையெனில் {
            க்கு (மாறிலி தருக்க lObjective : {பொய், உண்மை}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                மாறிலி தருக்க rObjective = lObjective ^ m_objective;
                கோவை_Both(nodep, உண்மை, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                எனில் (checkMaxExprs()) நிறுத்து;
            }
        }
        வரி_Track(nodep);
    }
    வெறுமை visit(AstXor* nodep) override { xor_கோவை(nodep); }

    வெறுமை கோவை_Not(Ast_முனை_கோவை* nodep) {
        VL_RESTORER(m_objective);
        எனில் (m_seeking == NONE) {
            coverExprs(nodep);
        } இல்லையெனில் {
            m_objective = !m_objective;
            iterateChildren(nodep);
            வரி_Track(nodep);
        }
    }
    வெறுமை visit(AstNot* nodep) override { கோவை_Not(nodep); }
    வெறுமை visit(AstLogNot* nodep) override { கோவை_Not(nodep); }

    template <typename T_Oper>
    வெறுமை கோவை_Reduce(Ast_முனை_Uniop* nodep) {
        எனில் (m_seeking != ABORTED) {
            File_வரி* மாறிலி fl = nodep->fileline();
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp();
            மாறிலி முழு width = lhsp->dtypep()->width();
            மாறிலி size_t expected = std::is_same<T_Oper, AstXor>::மதிப்பு ? 0x1 << width : width + 1;
            எனில் (checkMaxExprs(expected)) திரும்பு;
            Ast_முனை_கோவை* unrolledp = புதிய AstSel{
                fl, lhsp->clone_மரம்(பொய்), புதிய AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            க்கு (முழு bit = width - 2; bit >= 0; bit--) {
                AstSel* மாறிலி selp = புதிய AstSel{fl, lhsp->clone_மரம்(பொய்),
                                                புதிய AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = புதிய T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(தள்ளு_Deletep(unrolledp), unrolledp);
        } இல்லையெனில் {
            iterateChildren(nodep);
            வரி_Track(nodep);
        }
    }
    வெறுமை visit(AstRedOr* nodep) override { கோவை_Reduce<AstOr>(nodep); }
    வெறுமை visit(AstRedAnd* nodep) override { கோவை_Reduce<AstAnd>(nodep); }
    வெறுமை visit(AstRedXor* nodep) override { கோவை_Reduce<AstXor>(nodep); }

    வெறுமை visit(AstLogIf* nodep) override {
        எனில் (m_seeking == NONE) {
            coverExprs(nodep);
        } இல்லையெனில் எனில் (m_objective) {
            கோவை_Either(nodep, உண்மை, பொய், உண்மை);
        } இல்லையெனில் {
            கோவை_Both(nodep, உண்மை, உண்மை, பொய்);
        }
        வரி_Track(nodep);
    }

    வெறுமை visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        எனில் (m_seeking == NONE) {
            coverExprs(nodep);
        } இல்லையெனில் {
            m_objective = !m_objective;
            xor_கோவை(nodep);
            வரி_Track(nodep);
        }
    }

    வெறுமை visit(AstFuncRef* nodep) override {
        எனில் (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_முனை_கோவை*>(nodep));
        } இல்லையெனில் {
            கோவை_Unsupported(nodep, "non-automatic function");
        }
    }

    வெறுமை visit(Ast_முனை_கோவை* nodep) override {
        எனில் (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } இல்லையெனில் {
            கோவை_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            எனில் (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_மரம்(nodep, emitV);
                // Add new expression with a single term
                Cover_கோவை கோவை;
                கோவை.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.தள்ளு_back(std::move(கோவை));
                checkMaxExprs();
            } இல்லையெனில் {
                கோவை_Unsupported(nodep, "not coverage eligible");
            }
        }
        வரி_Track(nodep);
    }

    // VISITORS - BOTH
    வெறுமை visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = பொய்;
        m_inToggleOff = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = பொய்;
        m_inToggleOff = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        iterateChildren(nodep);
        வரி_Track(nodep);
    }

    வெறுமை கோவை_Unsupported(Ast_முனை* nodep, மாறிலி string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        மாறிலி தருக்க wasSeeking = m_seeking == SEEKING;
        மாறிலி Objective oldSeeking = m_seeking;
        எனில் (wasSeeking) abort_கோவை_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        வரி_Track(nodep);
        எனில் (!wasSeeking) m_seeking = oldSeeking;
    }

பொது:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_கோவை_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = இயல்பு;
};

//######################################################################
// Coverage class functions

வெறுமை V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("coverage", 0, dump_மரம்_Either_Level() >= 3);
}
