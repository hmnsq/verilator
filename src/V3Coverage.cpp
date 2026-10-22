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

صنف دەربڕین_Coverage_Eligible_Visitor final : عام VNVisitorConst {
    // STATE
    منطقي m_eligible = true;

    ساكن منطقي elemDTypeEligible(ثابت Ast_گرێ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت Ast_گرێ_DType* ثابت dtp = dtypep->virtRefDTypep()) {
            إذا (!elemDTypeEligible(dtp)) أرجع خطأ;
        }
        إذا (ثابت Ast_گرێ_DType* ثابت dtp = dtypep->virtRefDType2p()) {
            إذا (!elemDTypeEligible(dtp)) أرجع خطأ;
        }
        أرجع !VN_IS(dtypep, ClassRefDType);
    }

    فراغ visit(Ast_گرێ_Var_Ref* nodep) override {
        ثابت Ast_گرێ_DType* ثابت dtypep = nodep->varp()->dtypep();
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

    فراغ visit(Ast_گرێ* nodep) override {
        إذا (!nodep->is_دەربڕین_Coverage_Eligible()) {
            m_eligible = خطأ;
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }

عام:
    // CONSTRUCTORS
    explicit دەربڕین_Coverage_Eligible_Visitor(Ast_گرێ* nodep) { iterateConst(nodep); }
    ~دەربڕین_Coverage_Eligible_Visitor() override = افتراضي;

    منطقي eligible() ثابت { أرجع m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

صنف CoverageVisitor final : عام VNVisitor {
    // TYPES
    using LinenoSet = std::set<صحيح>;

    بنية CoverTerm final {
        Ast_گرێ_دەربڕین* m_exprp;  // Expression branch term
        منطقي m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_گرێ_دەربڕین* exprp, منطقي objective, ثابت string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_دەربڕین = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_دەربڕین>;

    بنية ToggleEnt final {
        ثابت string m_comment;  // Comment for coverage dump
        Ast_گرێ_دەربڕین* m_varRefp;  // How to get to this element
        Ast_گرێ_دەربڕین* m_chgRefp;  // How to get to this element
        ToggleEnt(ثابت string& comment, Ast_گرێ_دەربڕین* vp, Ast_گرێ_دەربڕین* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = افتراضي;
        فراغ cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_درەخت(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_درەخت(), m_chgRefp = nullptr);
        }
    };

    بنية CheckState final {  // State save-restored on each new coverage scope/block
        منطقي m_on = خطأ;  // Should this block get covered?
        منطقي m_inModOff = خطأ;  // In module with no coverage
        صحيح m_handle = 0;  // Opaque handle for index into line tracking
        ثابت Ast_گرێ* m_nodep = nullptr;  // Node establishing this state
        CheckState() = افتراضي;
        منطقي coverageOn(ثابت Ast_گرێ* nodep) ثابت {
            أرجع m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        منطقي هێڵ_Coverage_On(ثابت Ast_گرێ* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverage_هێڵ();
        }
        منطقي branchCoverageOn(ثابت Ast_گرێ* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        منطقي دەربڕین_Coverage_On(ثابت Ast_گرێ* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverage_دەربڕین();
        }
    };

    تعداد Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    V3UniqueNames m_دەربڕین_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_گرێ_دەربڕین*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    صحيح m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_گرێ_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_گرێ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گرێ* m_دەربڕین_Stmtsp = nullptr;  // Node to add expr coverage to
    منطقي m_then = خطأ;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    منطقي m_objective = خطأ;  // Expression objective
    منطقي m_ifCond = خطأ;  // Visiting if condition
    منطقي m_inToggleOff = خطأ;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<صحيح, LinenoSet> m_handle_هێڵەکان;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ثابت حرف* varIgnoreToggle(ثابت AstVar* nodep) {
        ثابت منطقي cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        إذا (!cover) أرجع "Not relevant signal";
        إذا (nodep->isConst()) أرجع "Signal is constant";
        إذا (nodep->isDouble()) أرجع "Signal is double";
        إذا (nodep->isString()) أرجع "Signal is string";
        إذا (!v3Global.opt.coverageUnderscore()) {
            ثابت string prettyName = nodep->prettyName();
            إذا (prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.find("._") != string::npos) أرجع "Inlined leading underscore";
        }
        إذا ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            أرجع "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        أرجع nullptr;
    }

    AstCoverInc* newCoverInc(File_هێڵ* fl, Ast_گرێ_Cover_Decl* ثابت declp,
                             ثابت string& trace_var_name) {
        AstCoverInc* ثابت incp = جديد AstCoverInc{fl, declp};
        إذا (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_هێڵ* ثابت fl_nowarn = جديد File_هێڵ{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
            AstVar* ثابت varp = جديد AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(true);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* ثابت assp = جديد AstAssign{
                incp->fileline(), جديد AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                جديد AstAdd{incp->fileline(), جديد AstVarRef{incp->fileline(), varp, VAccess::READ},
                           جديد AstConst{incp->fileline(), AstConst::Widthed_بەها{}, 32, 1}}};
            Ast_گرێ::addNext<Ast_گرێ, Ast_گرێ>(incp, assp);
        }
        أرجع incp;
    }
    string trace_Name_For_هێڵ(ثابت Ast_گرێ* nodep, ثابت string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        إذا (ثابت uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        أرجع name;
    }

    // Line tracking
    فراغ createHandle(ثابت Ast_گرێ* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = true;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    فراغ هێڵ_Track(ثابت Ast_گرێ* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        إذا ((m_state.هێڵ_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            لكل (صحيح lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_هێڵەکان[m_state.m_handle].زیادکردن(lineno);
            }
        }
    }
    ساكن string هێڵەکان_First_Last(ثابت صحيح first, ثابت صحيح last) {
        إذا (first && first == last) {
            أرجع cvtToStr(first);
        } وإلا إذا (first && last) {
            أرجع cvtToStr(first) + "-" + cvtToStr(last);
        } وإلا {
            أرجع "";
        }
    }
    string هێڵەکان_Cov(ثابت CheckState& state, ثابت Ast_گرێ* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        ثابت LinenoSet& هێڵەکان = m_handle_هێڵەکان[state.m_handle];
        صحيح first = 0;
        صحيح last = 0;
        لكل (ثابت صحيح linen : هێڵەکان) {
            إذا (!first) {
                first = last = linen;
            } وإلا إذا (linen == last + 1) {
                ++last;
            } وإلا {
                إذا (!out.empty()) out += ",";
                out += هێڵەکان_First_Last(first, last);
                first = last = linen;
            }
        }
        إذا (first) {
            إذا (!out.empty()) out += ",";
            out += هێڵەکان_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        أرجع out;
    }

    // VISITORS - BOTH
    فراغ visit(Ast_گرێ_Module* nodep) override {
        ثابت Ast_گرێ_Module* ثابت origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_دەربڕین_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_لووتکە();  // Already made top shell, no coverage for it
        إذا (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_هێڵەکان.clear();
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_دەربڕین_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    فراغ visit(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_دەربڕین_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_دەربڕین_Stmtsp = nodep;
            m_inToggleOff = true;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            أرجع;
        }
        iterateProcedure(nodep);
    }
    فراغ visit(Ast_گرێ_Procedure* nodep) override { iterateProcedure(nodep); }
    فراغ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_دەربڕین_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_دەربڕین_Stmtsp = nodep;
        m_inToggleOff = true;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        إذا (m_state.هێڵ_Coverage_On(nodep)) {
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", هێڵەکان_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_گرێ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_هێڵ(nodep, "block"));
            زیادکردن_Proc_Statement(nodep, newp);
        }
    }
    فراغ visit(AstLoopTest* nodep) override {
        إذا (nodep->user2SetOnce()) أرجع;
        هێڵ_Track(nodep);
        إذا (m_state.هێڵ_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", هێڵەکان_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_گرێ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_هێڵ(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    فراغ visit(Ast_گرێ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_دەربڕین_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        إذا (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    فراغ زیادکردن_Proc_Statement(Ast_گرێ* nodep, Ast_گرێ* stmtp) {
        إذا (Ast_گرێ_Procedure* ثابت itemp = VN_CAST(nodep, گرێ_Procedure)) {
            itemp->addStmtsp(stmtp);
        } وإلا إذا (Ast_گرێ_FTask* ثابت itemp = VN_CAST(nodep, گرێ_FTask)) {
            itemp->addStmtsp(stmtp);
        } وإلا إذا (AstLoop* ثابت itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } وإلا إذا (AstIf* ثابت itemp = VN_CAST(nodep, If)) {
            إذا (m_then) {
                itemp->addThensp(stmtp);
            } وإلا {
                itemp->addElsesp(stmtp);
            }
        } وإلا إذا (AstBegin* ثابت itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } وإلا {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    فراغ iterateProcedure(Ast_گرێ* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_دەربڕین_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_دەربڕین_Stmtsp = nodep;
        m_inToggleOff = true;
        createHandle(nodep);
        iterateChildren(nodep);
        إذا (m_state.هێڵ_Coverage_On(nodep)) {
            هێڵ_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", هێڵەکان_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_گرێ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_هێڵ(nodep, "block"));
            زیادکردن_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    فراغ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
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
                File_هێڵ* ثابت fl_nowarn = جديد File_هێڵ{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
                AstVar* ثابت chgVarp
                    = جديد AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

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
        ثابت std::string hierPrefix
            = (m_beginHier != "") ? Ast_گرێ::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ثابت declp
            = جديد AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* ثابت newp = جديد AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_درەخت(خطأ), above.m_chgRefp->clone_درەخت(خطأ)};
        m_modp->addStmtsp(newp);
    }

    فراغ toggleVarRecurse(ثابت Ast_گرێ_DType* ثابت dtypep, ثابت صحيح depth,  // per-iteration
                          ثابت ToggleEnt& above, ثابت AstVar* ثابت varp) {  // Constant
        إذا (ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            لكل (ثابت صحيح index_docs : adtypep->declRange().seqLoToHi()) {
                ثابت صحيح index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 جديد AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_درەخت(خطأ), index_code},
                                 جديد AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_درەخت(خطأ), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(dtypep, PackArrayDType)) {
            لكل (ثابت صحيح index_docs : adtypep->declRange().seqLoToHi()) {
                ثابت Ast_گرێ_DType* ثابت subtypep = adtypep->subDTypep()->skipRefp();
                ثابت صحيح index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 جديد AstSel{varp->fileline(), above.m_varRefp->clone_درەخت(خطأ),
                                            index_code * subtypep->width(), subtypep->width()},
                                 جديد AstSel{varp->fileline(), above.m_chgRefp->clone_درەخت(خطأ),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } وإلا إذا (ثابت AstStructDType* ثابت adtypep = VN_CAST(dtypep, StructDType)) {
            إذا (adtypep->packed()) {
                لكل (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت Ast_گرێ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    ثابت صحيح index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        جديد AstSel{varp->fileline(), above.m_varRefp->clone_درەخت(خطأ), index_code,
                                   subtypep->width()},
                        جديد AstSel{varp->fileline(), above.m_chgRefp->clone_درەخت(خطأ), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } وإلا {
                لكل (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_گرێ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    Ast_گرێ_دەربڕین* ثابت varRefp = جديد AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_درەخت(خطأ), itemp->name()};
                    Ast_گرێ_دەربڕین* ثابت chgRefp = جديد AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_درەخت(خطأ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وإلا إذا (ثابت AstUnionDType* ثابت adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            إذا (ثابت AstMemberDType* ثابت itemp = adtypep->membersp()) {
                Ast_گرێ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                إذا (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_درەخت(خطأ),
                                     above.m_chgRefp->clone_درەخت(خطأ)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } وإلا {
                    Ast_گرێ_دەربڕین* ثابت varRefp = جديد AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_درەخت(خطأ), itemp->name()};
                    Ast_گرێ_دەربڕین* ثابت chgRefp = جديد AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_درەخت(خطأ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وإلا إذا (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } وإلا {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    منطقي includeCondToBranchRecursive(ثابت Ast_گرێ* ثابت nodep) {
        ثابت Ast_گرێ* ثابت backp = nodep->backp();
        إذا (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            أرجع includeCondToBranchRecursive(backp);
        } وإلا إذا (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            أرجع includeCondToBranchRecursive(backp);
        } وإلا إذا (VN_IS(backp, گرێ_Assign) && VN_AS(backp, گرێ_Assign)->rhsp() == nodep) {
            أرجع true;
        }
        أرجع خطأ;
    }

    // VISITORS - LINE COVERAGE
    فراغ visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        إذا (m_seeking == NONE) coverExprs(nodep->condp());

        إذا (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ثابت CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            هێڵ_Track(nodep);
            Ast_گرێ_دەربڕین* ثابت thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* ثابت thenDeclp
                = جديد AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", هێڵەکان_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(جديد Ast_دەربڕین_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_هێڵ(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_گرێ_دەربڕین* ثابت elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* ثابت elseDeclp
                = جديد AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", هێڵەکان_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(جديد Ast_دەربڕین_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_هێڵ(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } وإلا {
            هێڵ_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    فراغ visit(AstIf* nodep) override {
        إذا (nodep->user2()) أرجع;

        UINFO(4, " IF: " << nodep);
        إذا (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ثابت منطقي elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            إذا (elsif) VN_AS(nodep->elsesp(), If)->user1(true);
            ثابت منطقي first_elsif = !nodep->user1() && elsif;
            ثابت منطقي cont_elsif = nodep->user1() && elsif;
            ثابت منطقي final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ثابت CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_دەربڕین_Stmtsp);
                VL_RESTORER(m_then);
                m_دەربڕین_Stmtsp = nodep;
                m_then = true;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                هێڵ_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_دەربڕین_Stmtsp);
                VL_RESTORER(m_then);
                m_دەربڕین_Stmtsp = nodep;
                m_then = خطأ;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            إذا (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ثابت ifDeclp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", هێڵەکان_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_هێڵ(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ثابت elseDeclp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", هێڵەکان_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_هێڵ(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            وإلا إذا (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                إذا (ifState.هێڵ_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ثابت elsifDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        هێڵەکان_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_هێڵ(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } وإلا {
                // Cover as separate blocks (not a branch as is not two-legged)
                إذا (ifState.هێڵ_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ثابت ifDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        هێڵەکان_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_هێڵ(nodep, "if")));
                }
                إذا (elseState.هێڵ_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ثابت elseDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        هێڵەکان_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_هێڵ(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = true;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    فراغ visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        إذا (m_state.هێڵ_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            إذا (m_state.هێڵ_Coverage_On(nodep)) {  // if the case body didn't disable it
                هێڵ_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ثابت declp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", هێڵەکان_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_هێڵ(nodep, "case")));
            }
        }
    }
    فراغ visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = true;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        إذا (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            هێڵ_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", هێڵەکان_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    فراغ visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_دەربڕین_Stmtsp);
        m_دەربڕین_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    فراغ visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = خطأ;
    }
    فراغ visit(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = خطأ;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
        } وإلا {
            إذا (m_state.m_on) iterateChildren(nodep);
            هێڵ_Track(nodep);
        }
    }
    فراغ visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        إذا (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        هێڵ_Track(nodep);
    }

    فراغ visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_دەربڕین_Stmtsp);
        m_دەربڕین_Stmtsp = nodep;
        m_inToggleOff = true;
        إذا (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        هێڵ_Track(nodep);
    }

    فراغ abort_دەربڕین_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        إذا (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    منطقي checkMaxExprs(size_t additional = 0) {
        إذا (m_seeking != ABORTED
            && static_cast<صحيح>(m_exprs.size() + additional) <= v3Global.opt.coverage_دەربڕین_Max())
            أرجع خطأ;
        abort_دەربڕین_Coverage();
        أرجع true;
    }

    فراغ add_دەربڕین_Cover_Inc(Ast_گرێ_دەربڕین* nodep, صحيح start = 0) {
        File_هێڵ* ثابت fl = nodep->fileline();
        صحيح ژماردن = start;
        لكل (Cover_دەربڕین& دەربڕین : m_exprs) {
            ثابت string name = "expr_" + std::to_string(ژماردن);
            string comment = "(";
            منطقي first = true;
            Ast_گرێ_دەربڕین* condp = nullptr;
            لكل (CoverTerm& term : دەربڕین) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_گرێ_دەربڕین* covExprp = nullptr;
                إذا (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_گرێ_دەربڕین* ثابت frefp = term.m_exprp;
                    Ast_گرێ_DType* ثابت dtypep = frefp->dtypep();
                    ثابت auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    إذا (pair.second) {
                        varp = جديد AstVar{fl, VVarType::MODULETEMP, m_دەربڕین_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        إذا (m_ftaskp) {
                            varp->funcLocal(true);
                            m_ftaskp->prependStmtsp(varp);
                        } وإلا {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(جديد Ast_دەربڕین_Stmt{
                            fl, جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            جديد AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = جديد AstVarRef{fl, varp, VAccess::READ};
                } وإلا {
                    covExprp = term.m_exprp->clone_درەخت(خطأ);
                }
                Ast_گرێ_دەربڕین* ثابت termp
                    = term.m_objective ? covExprp : جديد AstLogNot{fl, covExprp};
                إذا (condp) {
                    condp = جديد AstLogAnd{fl, condp, termp};
                } وإلا {
                    condp = termp;
                }
                first = خطأ;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ثابت declp = جديد AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_گرێ* ثابت newp = newCoverInc(fl, declp, trace_Name_For_هێڵ(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ثابت ifp = جديد AstIf{fl, condp, newp, nullptr};
            ifp->user2(true);
            زیادکردن_Proc_Statement(m_دەربڕین_Stmtsp, ifp);
            ++ژماردن;
        }
    }

    فراغ coverExprs(Ast_گرێ_دەربڕین* nodep) {
        إذا (!m_state.دەربڕین_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_دەربڕین_Stmtsp) {
            أرجع;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = خطأ;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = true;
        iterate(nodep);
        إذا (checkMaxExprs(falseExprs.size())) أرجع;
        إذا (m_seeking == ABORTED) أرجع;

        add_دەربڕین_Cover_Inc(nodep);
        ثابت صحيح start = m_exprs.size();
        m_objective = خطأ;
        m_exprs.swap(falseExprs);
        add_دەربڕین_Cover_Inc(nodep, start);
    }

    فراغ دەربڕین_Either(Ast_گرێ_Biop* nodep, منطقي overrideObjective = خطأ, منطقي lObjective = خطأ,
                    منطقي rObjective = خطأ) {
        VL_RESTORER(m_objective);
        Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp();
        Ast_گرێ_دەربڕین* ثابت rhsp = nodep->rhsp();

        إذا (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        إذا (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    فراغ دەربڕین_Both(Ast_گرێ_Biop* nodep, منطقي overrideObjective = خطأ, منطقي lObjective = خطأ,
                  منطقي rObjective = خطأ) {
        VL_RESTORER(m_objective);
        Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp();
        Ast_گرێ_دەربڕین* ثابت rhsp = nodep->rhsp();

        إذا (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        إذا (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        لكل (ثابت Cover_دەربڕین& l : lhsExprs) {
            لكل (ثابت Cover_دەربڕین& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_دەربڕین دەربڕین;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                لكل (ثابت CoverTerm& term : l) {
                    إذا (ثابت AstVarRef* ثابت refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].زیادکردن(refp->varp());
                    } وإلا {
                        strs[term.m_objective].زیادکردن(term.m_emitV);
                    }
                    دەربڕین.پاڵنان_back(term);
                }
                منطقي impossible = خطأ;
                لكل (ثابت CoverTerm& term : r) {
                    منطقي redundant = خطأ;
                    إذا (ثابت Ast_گرێ_Var_Ref* ثابت refp = VN_CAST(term.m_exprp, گرێ_Var_Ref)) {
                        إذا (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = true;
                        إذا (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = true;
                    } وإلا {
                        إذا (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = true;
                        إذا (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = true;
                    }
                    إذا (!redundant) دەربڕین.پاڵنان_back(term);
                }
                إذا (!impossible) m_exprs.پاڵنان_back(std::move(دەربڕین));
                إذا (checkMaxExprs()) أرجع;
            }
        }
    }

    فراغ or_دەربڕین(Ast_گرێ_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            دەربڕین_Either(nodep);
        } وإلا {
            دەربڕین_Both(nodep);
        }
        هێڵ_Track(nodep);
    }
    فراغ visit(AstLogOr* nodep) override { or_دەربڕین(nodep); }
    فراغ visit(AstOr* nodep) override { or_دەربڕین(nodep); }

    فراغ and_دەربڕین(Ast_گرێ_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            دەربڕین_Both(nodep);
        } وإلا {
            دەربڕین_Either(nodep);
        }
        هێڵ_Track(nodep);
    }
    فراغ visit(AstLogAnd* nodep) override { and_دەربڕین(nodep); }
    فراغ visit(AstAnd* nodep) override { and_دەربڕین(nodep); }

    فراغ xor_دەربڕین(Ast_گرێ_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            لكل (ثابت منطقي lObjective : {خطأ, true}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                ثابت منطقي rObjective = lObjective ^ m_objective;
                دەربڕین_Both(nodep, true, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                إذا (checkMaxExprs()) اكسر;
            }
        }
        هێڵ_Track(nodep);
    }
    فراغ visit(AstXor* nodep) override { xor_دەربڕین(nodep); }

    فراغ دەربڕین_Not(Ast_گرێ_دەربڕین* nodep) {
        VL_RESTORER(m_objective);
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            m_objective = !m_objective;
            iterateChildren(nodep);
            هێڵ_Track(nodep);
        }
    }
    فراغ visit(AstNot* nodep) override { دەربڕین_Not(nodep); }
    فراغ visit(AstLogNot* nodep) override { دەربڕین_Not(nodep); }

    template <typename T_Oper>
    فراغ دەربڕین_Reduce(Ast_گرێ_Uniop* nodep) {
        إذا (m_seeking != ABORTED) {
            File_هێڵ* ثابت fl = nodep->fileline();
            Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp();
            ثابت صحيح width = lhsp->dtypep()->width();
            ثابت size_t expected = std::is_same<T_Oper, AstXor>::بەها ? 0x1 << width : width + 1;
            إذا (checkMaxExprs(expected)) أرجع;
            Ast_گرێ_دەربڕین* unrolledp = جديد AstSel{
                fl, lhsp->clone_درەخت(خطأ), جديد AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            لكل (صحيح bit = width - 2; bit >= 0; bit--) {
                AstSel* ثابت selp = جديد AstSel{fl, lhsp->clone_درەخت(خطأ),
                                                جديد AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = جديد T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(پاڵنان_Deletep(unrolledp), unrolledp);
        } وإلا {
            iterateChildren(nodep);
            هێڵ_Track(nodep);
        }
    }
    فراغ visit(AstRedOr* nodep) override { دەربڕین_Reduce<AstOr>(nodep); }
    فراغ visit(AstRedAnd* nodep) override { دەربڕین_Reduce<AstAnd>(nodep); }
    فراغ visit(AstRedXor* nodep) override { دەربڕین_Reduce<AstXor>(nodep); }

    فراغ visit(AstLogIf* nodep) override {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            دەربڕین_Either(nodep, true, خطأ, true);
        } وإلا {
            دەربڕین_Both(nodep, true, true, خطأ);
        }
        هێڵ_Track(nodep);
    }

    فراغ visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            m_objective = !m_objective;
            xor_دەربڕین(nodep);
            هێڵ_Track(nodep);
        }
    }

    فراغ visit(AstFuncRef* nodep) override {
        إذا (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_گرێ_دەربڕین*>(nodep));
        } وإلا {
            دەربڕین_Unsupported(nodep, "non-automatic function");
        }
    }

    فراغ visit(Ast_گرێ_دەربڕین* nodep) override {
        إذا (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } وإلا {
            دەربڕین_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            إذا (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_درەخت(nodep, emitV);
                // Add new expression with a single term
                Cover_دەربڕین دەربڕین;
                دەربڕین.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.پاڵنان_back(std::move(دەربڕین));
                checkMaxExprs();
            } وإلا {
                دەربڕین_Unsupported(nodep, "not coverage eligible");
            }
        }
        هێڵ_Track(nodep);
    }

    // VISITORS - BOTH
    فراغ visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = خطأ;
        m_inToggleOff = true;
        iterateChildren(nodep);
    }
    فراغ visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = خطأ;
        m_inToggleOff = true;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_گرێ* nodep) override {
        iterateChildren(nodep);
        هێڵ_Track(nodep);
    }

    فراغ دەربڕین_Unsupported(Ast_گرێ* nodep, ثابت string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ثابت منطقي wasSeeking = m_seeking == SEEKING;
        ثابت Objective oldSeeking = m_seeking;
        إذا (wasSeeking) abort_دەربڕین_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        هێڵ_Track(nodep);
        إذا (!wasSeeking) m_seeking = oldSeeking;
    }

عام:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_دەربڕین_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = افتراضي;
};

//######################################################################
// Coverage class functions

فراغ V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درەخت("coverage", 0, dump_درەخت_Either_Level() >= 3);
}
