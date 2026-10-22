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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Coverage.h"

#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <list>
#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग व्यञ्जकम्_Coverage_Eligible_Visitor final : सार्वजनिक VNVisitorConst {
    // STATE
    तार्किक m_eligible = सच्च;

    स्थिर तार्किक elemDTypeEligible(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        जइ (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtp = dtypep->virtRefDTypep()) {
            जइ (!elemDTypeEligible(dtp)) प्रत्यागम मिच्छ;
        }
        जइ (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtp = dtypep->virtRefDType2p()) {
            जइ (!elemDTypeEligible(dtp)) प्रत्यागम मिच्छ;
        }
        प्रत्यागम !VN_IS(dtypep, ClassRefDType);
    }

    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        जइ (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } अण्णहा {
            m_eligible = मिच्छ;
        }
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        जइ (!nodep->is_व्यञ्जकम्_Coverage_Eligible()) {
            m_eligible = मिच्छ;
        } अण्णहा {
            iterateChildrenConst(nodep);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit व्यञ्जकम्_Coverage_Eligible_Visitor(Ast_ग्रन्थिः* nodep) { iterateConst(nodep); }
    ~व्यञ्जकम्_Coverage_Eligible_Visitor() override = स्वभाव;

    तार्किक eligible() ध्रुव { प्रत्यागम m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

वर्ग CoverageVisitor final : सार्वजनिक VNVisitor {
    // TYPES
    using LinenoSet = std::set<पूर्णाङ्क>;

    संरचना CoverTerm final {
        Ast_ग्रन्थिः_व्यञ्जकम्* m_exprp;  // Expression branch term
        तार्किक m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक objective, ध्रुव string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_व्यञ्जकम् = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_व्यञ्जकम्>;

    संरचना ToggleEnt final {
        ध्रुव string m_comment;  // Comment for coverage dump
        Ast_ग्रन्थिः_व्यञ्जकम्* m_varRefp;  // How to get to this element
        Ast_ग्रन्थिः_व्यञ्जकम्* m_chgRefp;  // How to get to this element
        ToggleEnt(ध्रुव string& comment, Ast_ग्रन्थिः_व्यञ्जकम्* vp, Ast_ग्रन्थिः_व्यञ्जकम्* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = स्वभाव;
        शून्य cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_वृक्षः(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_वृक्षः(), m_chgRefp = nullptr);
        }
    };

    संरचना CheckState final {  // State save-restored on each new coverage scope/block
        तार्किक m_on = मिच्छ;  // Should this block get covered?
        तार्किक m_inModOff = मिच्छ;  // In module with no coverage
        पूर्णाङ्क m_handle = 0;  // Opaque handle for index into line tracking
        ध्रुव Ast_ग्रन्थिः* m_nodep = nullptr;  // Node establishing this state
        CheckState() = स्वभाव;
        तार्किक coverageOn(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {
            प्रत्यागम m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        तार्किक पङ्क्तिः_Coverage_On(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {
            प्रत्यागम coverageOn(nodep) && v3Global.opt.coverage_पङ्क्तिः();
        }
        तार्किक branchCoverageOn(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {
            प्रत्यागम coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        तार्किक व्यञ्जकम्_Coverage_On(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {
            प्रत्यागम coverageOn(nodep) && v3Global.opt.coverage_व्यञ्जकम्();
        }
    };

    गणना Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;
    V3UniqueNames m_व्यञ्जकम्_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_ग्रन्थिः_व्यञ्जकम्*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    पूर्णाङ्क m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_ग्रन्थिः_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ग्रन्थिः* m_व्यञ्जकम्_Stmtsp = nullptr;  // Node to add expr coverage to
    तार्किक m_then = मिच्छ;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    तार्किक m_objective = मिच्छ;  // Expression objective
    तार्किक m_ifCond = मिच्छ;  // Visiting if condition
    तार्किक m_inToggleOff = मिच्छ;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<पूर्णाङ्क, LinenoSet> m_handle_पङ्क्तयः;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ध्रुव वर्ण* varIgnoreToggle(ध्रुव AstVar* nodep) {
        ध्रुव तार्किक cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        जइ (!cover) प्रत्यागम "Not relevant signal";
        जइ (nodep->isConst()) प्रत्यागम "Signal is constant";
        जइ (nodep->isDouble()) प्रत्यागम "Signal is double";
        जइ (nodep->isString()) प्रत्यागम "Signal is string";
        जइ (!v3Global.opt.coverageUnderscore()) {
            ध्रुव string prettyName = nodep->prettyName();
            जइ (prettyName[0] == '_') प्रत्यागम "Leading underscore";
            जइ (prettyName.find("._") != string::npos) प्रत्यागम "Inlined leading underscore";
        }
        जइ ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            प्रत्यागम "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        प्रत्यागम nullptr;
    }

    AstCoverInc* newCoverInc(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp,
                             ध्रुव string& trace_var_name) {
        AstCoverInc* ध्रुव incp = नूतन AstCoverInc{fl, declp};
        जइ (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_पङ्क्तिः* ध्रुव fl_nowarn = नूतन File_पङ्क्तिः{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, सच्च);
            AstVar* ध्रुव varp = नूतन AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(सच्च);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* ध्रुव assp = नूतन AstAssign{
                incp->fileline(), नूतन AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                नूतन AstAdd{incp->fileline(), नूतन AstVarRef{incp->fileline(), varp, VAccess::READ},
                           नूतन AstConst{incp->fileline(), AstConst::Widthed_मानम्{}, 32, 1}}};
            Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(incp, assp);
        }
        प्रत्यागम incp;
    }
    string trace_Name_For_पङ्क्तिः(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        जइ (ध्रुव uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        प्रत्यागम name;
    }

    // Line tracking
    शून्य createHandle(ध्रुव Ast_ग्रन्थिः* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = सच्च;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    शून्य पङ्क्तिः_Track(ध्रुव Ast_ग्रन्थिः* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        जइ ((m_state.पङ्क्तिः_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            प्रति (पूर्णाङ्क lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_पङ्क्तयः[m_state.m_handle].निवेशय(lineno);
            }
        }
    }
    स्थिर string पङ्क्तयः_First_Last(ध्रुव पूर्णाङ्क first, ध्रुव पूर्णाङ्क last) {
        जइ (first && first == last) {
            प्रत्यागम cvtToStr(first);
        } अण्णहा जइ (first && last) {
            प्रत्यागम cvtToStr(first) + "-" + cvtToStr(last);
        } अण्णहा {
            प्रत्यागम "";
        }
    }
    string पङ्क्तयः_Cov(ध्रुव CheckState& state, ध्रुव Ast_ग्रन्थिः* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        ध्रुव LinenoSet& पङ्क्तयः = m_handle_पङ्क्तयः[state.m_handle];
        पूर्णाङ्क first = 0;
        पूर्णाङ्क last = 0;
        प्रति (ध्रुव पूर्णाङ्क linen : पङ्क्तयः) {
            जइ (!first) {
                first = last = linen;
            } अण्णहा जइ (linen == last + 1) {
                ++last;
            } अण्णहा {
                जइ (!out.empty()) out += ",";
                out += पङ्क्तयः_First_Last(first, last);
                first = last = linen;
            }
        }
        जइ (first) {
            जइ (!out.empty()) out += ",";
            out += पङ्क्तयः_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        प्रत्यागम out;
    }

    // VISITORS - BOTH
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_व्यञ्जकम्_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_शिखरम्();  // Already made top shell, no coverage for it
        जइ (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_पङ्क्तयः.clear();
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_व्यञ्जकम्_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    शून्य visit(AstAlways* nodep) override {
        जइ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_व्यञ्जकम्_Stmtsp = nodep;
            m_inToggleOff = सच्च;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            प्रत्यागम;
        }
        iterateProcedure(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override { iterateProcedure(nodep); }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_व्यञ्जकम्_Stmtsp = nodep;
        m_inToggleOff = सच्च;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        जइ (m_state.पङ्क्तिः_Coverage_On(nodep)) {
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पङ्क्तयः_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ग्रन्थिः* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पङ्क्तिः(nodep, "block"));
            निवेशय_Proc_Statement(nodep, newp);
        }
    }
    शून्य visit(AstLoopTest* nodep) override {
        जइ (nodep->user2SetOnce()) प्रत्यागम;
        पङ्क्तिः_Track(nodep);
        जइ (m_state.पङ्क्तिः_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पङ्क्तयः_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ग्रन्थिः* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पङ्क्तिः(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_व्यञ्जकम्_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        जइ (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    शून्य निवेशय_Proc_Statement(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* stmtp) {
        जइ (Ast_ग्रन्थिः_Procedure* ध्रुव itemp = VN_CAST(nodep, ग्रन्थिः_Procedure)) {
            itemp->addStmtsp(stmtp);
        } अण्णहा जइ (Ast_ग्रन्थिः_FTask* ध्रुव itemp = VN_CAST(nodep, ग्रन्थिः_FTask)) {
            itemp->addStmtsp(stmtp);
        } अण्णहा जइ (AstLoop* ध्रुव itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } अण्णहा जइ (AstIf* ध्रुव itemp = VN_CAST(nodep, If)) {
            जइ (m_then) {
                itemp->addThensp(stmtp);
            } अण्णहा {
                itemp->addElsesp(stmtp);
            }
        } अण्णहा जइ (AstBegin* ध्रुव itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } अण्णहा {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    शून्य iterateProcedure(Ast_ग्रन्थिः* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_व्यञ्जकम्_Stmtsp = nodep;
        m_inToggleOff = सच्च;
        createHandle(nodep);
        iterateChildren(nodep);
        जइ (m_state.पङ्क्तिः_Coverage_On(nodep)) {
            पङ्क्तिः_Track(nodep);
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पङ्क्तयः_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_ग्रन्थिः* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पङ्क्तिः(nodep, "block"));
            निवेशय_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    शून्य visit(AstVar* nodep) override {
        iterateChildren(nodep);
        जइ (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ध्रुव वर्ण* ध्रुव disablep = varIgnoreToggle(nodep);
            जइ (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } अण्णहा {
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
                ध्रुव string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_पङ्क्तिः* ध्रुव fl_nowarn = नूतन File_पङ्क्तिः{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, सच्च);
                AstVar* ध्रुव chgVarp
                    = नूतन AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, नूतन AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 नूतन AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    शून्य toggleVarBottom(ध्रुव ToggleEnt& above, ध्रुव AstVar* varp, ध्रुव VNumRange& range) {
        ध्रुव std::string hierPrefix
            = (m_beginHier != "") ? Ast_ग्रन्थिः::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ध्रुव declp
            = नूतन AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* ध्रुव newp = नूतन AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_वृक्षः(मिच्छ), above.m_chgRefp->clone_वृक्षः(मिच्छ)};
        m_modp->addStmtsp(newp);
    }

    शून्य toggleVarRecurse(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep, ध्रुव पूर्णाङ्क depth,  // per-iteration
                          ध्रुव ToggleEnt& above, ध्रुव AstVar* ध्रुव varp) {  // Constant
        जइ (ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } अण्णहा जइ (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            प्रति (ध्रुव पूर्णाङ्क index_docs : adtypep->declRange().seqLoToHi()) {
                ध्रुव पूर्णाङ्क index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 नूतन AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_वृक्षः(मिच्छ), index_code},
                                 नूतन AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_वृक्षः(मिच्छ), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } अण्णहा जइ (ध्रुव AstPackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, PackArrayDType)) {
            प्रति (ध्रुव पूर्णाङ्क index_docs : adtypep->declRange().seqLoToHi()) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subtypep = adtypep->subDTypep()->skipRefp();
                ध्रुव पूर्णाङ्क index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 नूतन AstSel{varp->fileline(), above.m_varRefp->clone_वृक्षः(मिच्छ),
                                            index_code * subtypep->width(), subtypep->width()},
                                 नूतन AstSel{varp->fileline(), above.m_chgRefp->clone_वृक्षः(मिच्छ),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } अण्णहा जइ (ध्रुव AstStructDType* ध्रुव adtypep = VN_CAST(dtypep, StructDType)) {
            जइ (adtypep->packed()) {
                प्रति (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                    ध्रुव पूर्णाङ्क index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        नूतन AstSel{varp->fileline(), above.m_varRefp->clone_वृक्षः(मिच्छ), index_code,
                                   subtypep->width()},
                        नूतन AstSel{varp->fileline(), above.m_chgRefp->clone_वृक्षः(मिच्छ), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } अण्णहा {
                प्रति (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_वृक्षः(मिच्छ), itemp->name()};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव chgRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_वृक्षः(मिच्छ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } अण्णहा जइ (ध्रुव AstUnionDType* ध्रुव adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            जइ (ध्रुव AstMemberDType* ध्रुव itemp = adtypep->membersp()) {
                Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                जइ (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_वृक्षः(मिच्छ),
                                     above.m_chgRefp->clone_वृक्षः(मिच्छ)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } अण्णहा {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_वृक्षः(मिच्छ), itemp->name()};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव chgRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_वृक्षः(मिच्छ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } अण्णहा जइ (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } अण्णहा {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    तार्किक includeCondToBranchRecursive(ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
        ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();
        जइ (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            प्रत्यागम includeCondToBranchRecursive(backp);
        } अण्णहा जइ (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            प्रत्यागम includeCondToBranchRecursive(backp);
        } अण्णहा जइ (VN_IS(backp, ग्रन्थिः_Assign) && VN_AS(backp, ग्रन्थिः_Assign)->rhsp() == nodep) {
            प्रत्यागम सच्च;
        }
        प्रत्यागम मिच्छ;
    }

    // VISITORS - LINE COVERAGE
    शून्य visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        जइ (m_seeking == NONE) coverExprs(nodep->condp());

        जइ (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ध्रुव CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            पङ्क्तिः_Track(nodep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* ध्रुव thenDeclp
                = नूतन AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", पङ्क्तयः_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(नूतन Ast_व्यञ्जकम्_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_पङ्क्तिः(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* ध्रुव elseDeclp
                = नूतन AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", पङ्क्तयः_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(नूतन Ast_व्यञ्जकम्_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_पङ्क्तिः(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } अण्णहा {
            पङ्क्तिः_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    शून्य visit(AstIf* nodep) override {
        जइ (nodep->user2()) प्रत्यागम;

        UINFO(4, " IF: " << nodep);
        जइ (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ध्रुव तार्किक elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            जइ (elsif) VN_AS(nodep->elsesp(), If)->user1(सच्च);
            ध्रुव तार्किक first_elsif = !nodep->user1() && elsif;
            ध्रुव तार्किक cont_elsif = nodep->user1() && elsif;
            ध्रुव तार्किक final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ध्रुव CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
                VL_RESTORER(m_then);
                m_व्यञ्जकम्_Stmtsp = nodep;
                m_then = सच्च;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                पङ्क्तिः_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
                VL_RESTORER(m_then);
                m_व्यञ्जकम्_Stmtsp = nodep;
                m_then = मिच्छ;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            जइ (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ध्रुव ifDeclp
                    = नूतन AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", पङ्क्तयः_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_पङ्क्तिः(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ध्रुव elseDeclp
                    = नूतन AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", पङ्क्तयः_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_पङ्क्तिः(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            अण्णहा जइ (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                जइ (ifState.पङ्क्तिः_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ध्रुव elsifDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        पङ्क्तयः_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_पङ्क्तिः(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } अण्णहा {
                // Cover as separate blocks (not a branch as is not two-legged)
                जइ (ifState.पङ्क्तिः_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ध्रुव ifDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        पङ्क्तयः_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_पङ्क्तिः(nodep, "if")));
                }
                जइ (elseState.पङ्क्तिः_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ध्रुव elseDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        पङ्क्तयः_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_पङ्क्तिः(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = सच्च;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    शून्य visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        जइ (m_state.पङ्क्तिः_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            जइ (m_state.पङ्क्तिः_Coverage_On(nodep)) {  // if the case body didn't disable it
                पङ्क्तिः_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ध्रुव declp
                    = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", पङ्क्तयः_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_पङ्क्तिः(nodep, "case")));
            }
        }
    }
    शून्य visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = सच्च;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        जइ (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            पङ्क्तिः_Track(nodep);
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", पङ्क्तयः_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    शून्य visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
        m_व्यञ्जकम्_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    शून्य visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = मिच्छ;
    }
    शून्य visit(AstStmtPragma* nodep) override {
        जइ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = मिच्छ;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
        } अण्णहा {
            जइ (m_state.m_on) iterateChildren(nodep);
            पङ्क्तिः_Track(nodep);
        }
    }
    शून्य visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        जइ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        पङ्क्तिः_Track(nodep);
    }

    शून्य visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
        m_व्यञ्जकम्_Stmtsp = nodep;
        m_inToggleOff = सच्च;
        जइ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        पङ्क्तिः_Track(nodep);
    }

    शून्य abort_व्यञ्जकम्_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        जइ (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    तार्किक checkMaxExprs(size_t additional = 0) {
        जइ (m_seeking != ABORTED
            && static_cast<पूर्णाङ्क>(m_exprs.size() + additional) <= v3Global.opt.coverage_व्यञ्जकम्_Max())
            प्रत्यागम मिच्छ;
        abort_व्यञ्जकम्_Coverage();
        प्रत्यागम सच्च;
    }

    शून्य add_व्यञ्जकम्_Cover_Inc(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क start = 0) {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        पूर्णाङ्क गणनम् = start;
        प्रति (Cover_व्यञ्जकम्& व्यञ्जकम् : m_exprs) {
            ध्रुव string name = "expr_" + std::to_string(गणनम्);
            string comment = "(";
            तार्किक first = सच्च;
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;
            प्रति (CoverTerm& term : व्यञ्जकम्) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_ग्रन्थिः_व्यञ्जकम्* covExprp = nullptr;
                जइ (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव frefp = term.m_exprp;
                    Ast_ग्रन्थिः_DType* ध्रुव dtypep = frefp->dtypep();
                    ध्रुव auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    जइ (pair.second) {
                        varp = नूतन AstVar{fl, VVarType::MODULETEMP, m_व्यञ्जकम्_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        जइ (m_ftaskp) {
                            varp->funcLocal(सच्च);
                            m_ftaskp->prependStmtsp(varp);
                        } अण्णहा {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(नूतन Ast_व्यञ्जकम्_Stmt{
                            fl, नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            नूतन AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = नूतन AstVarRef{fl, varp, VAccess::READ};
                } अण्णहा {
                    covExprp = term.m_exprp->clone_वृक्षः(मिच्छ);
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp
                    = term.m_objective ? covExprp : नूतन AstLogNot{fl, covExprp};
                जइ (condp) {
                    condp = नूतन AstLogAnd{fl, condp, termp};
                } अण्णहा {
                    condp = termp;
                }
                first = मिच्छ;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ध्रुव declp = नूतन AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_ग्रन्थिः* ध्रुव newp = newCoverInc(fl, declp, trace_Name_For_पङ्क्तिः(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ध्रुव ifp = नूतन AstIf{fl, condp, newp, nullptr};
            ifp->user2(सच्च);
            निवेशय_Proc_Statement(m_व्यञ्जकम्_Stmtsp, ifp);
            ++गणनम्;
        }
    }

    शून्य coverExprs(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        जइ (!m_state.व्यञ्जकम्_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_व्यञ्जकम्_Stmtsp) {
            प्रत्यागम;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = मिच्छ;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = सच्च;
        iterate(nodep);
        जइ (checkMaxExprs(falseExprs.size())) प्रत्यागम;
        जइ (m_seeking == ABORTED) प्रत्यागम;

        add_व्यञ्जकम्_Cover_Inc(nodep);
        ध्रुव पूर्णाङ्क start = m_exprs.size();
        m_objective = मिच्छ;
        m_exprs.swap(falseExprs);
        add_व्यञ्जकम्_Cover_Inc(nodep, start);
    }

    शून्य व्यञ्जकम्_Either(Ast_ग्रन्थिः_Biop* nodep, तार्किक overrideObjective = मिच्छ, तार्किक lObjective = मिच्छ,
                    तार्किक rObjective = मिच्छ) {
        VL_RESTORER(m_objective);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();

        जइ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        जइ (checkMaxExprs()) प्रत्यागम;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        जइ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    शून्य व्यञ्जकम्_Both(Ast_ग्रन्थिः_Biop* nodep, तार्किक overrideObjective = मिच्छ, तार्किक lObjective = मिच्छ,
                  तार्किक rObjective = मिच्छ) {
        VL_RESTORER(m_objective);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();

        जइ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        जइ (checkMaxExprs()) प्रत्यागम;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        जइ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        जइ (checkMaxExprs()) प्रत्यागम;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        प्रति (ध्रुव Cover_व्यञ्जकम्& l : lhsExprs) {
            प्रति (ध्रुव Cover_व्यञ्जकम्& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_व्यञ्जकम् व्यञ्जकम्;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                प्रति (ध्रुव CoverTerm& term : l) {
                    जइ (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].निवेशय(refp->varp());
                    } अण्णहा {
                        strs[term.m_objective].निवेशय(term.m_emitV);
                    }
                    व्यञ्जकम्.प्रक्षिप_back(term);
                }
                तार्किक impossible = मिच्छ;
                प्रति (ध्रुव CoverTerm& term : r) {
                    तार्किक redundant = मिच्छ;
                    जइ (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(term.m_exprp, ग्रन्थिः_Var_Ref)) {
                        जइ (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = सच्च;
                        जइ (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = सच्च;
                    } अण्णहा {
                        जइ (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = सच्च;
                        जइ (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = सच्च;
                    }
                    जइ (!redundant) व्यञ्जकम्.प्रक्षिप_back(term);
                }
                जइ (!impossible) m_exprs.प्रक्षिप_back(std::move(व्यञ्जकम्));
                जइ (checkMaxExprs()) प्रत्यागम;
            }
        }
    }

    शून्य or_व्यञ्जकम्(Ast_ग्रन्थिः_Biop* nodep) {
        जइ (m_seeking == NONE) {
            coverExprs(nodep);
        } अण्णहा जइ (m_objective) {
            व्यञ्जकम्_Either(nodep);
        } अण्णहा {
            व्यञ्जकम्_Both(nodep);
        }
        पङ्क्तिः_Track(nodep);
    }
    शून्य visit(AstLogOr* nodep) override { or_व्यञ्जकम्(nodep); }
    शून्य visit(AstOr* nodep) override { or_व्यञ्जकम्(nodep); }

    शून्य and_व्यञ्जकम्(Ast_ग्रन्थिः_Biop* nodep) {
        जइ (m_seeking == NONE) {
            coverExprs(nodep);
        } अण्णहा जइ (m_objective) {
            व्यञ्जकम्_Both(nodep);
        } अण्णहा {
            व्यञ्जकम्_Either(nodep);
        }
        पङ्क्तिः_Track(nodep);
    }
    शून्य visit(AstLogAnd* nodep) override { and_व्यञ्जकम्(nodep); }
    शून्य visit(AstAnd* nodep) override { and_व्यञ्जकम्(nodep); }

    शून्य xor_व्यञ्जकम्(Ast_ग्रन्थिः_Biop* nodep) {
        जइ (m_seeking == NONE) {
            coverExprs(nodep);
        } अण्णहा {
            प्रति (ध्रुव तार्किक lObjective : {मिच्छ, सच्च}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                ध्रुव तार्किक rObjective = lObjective ^ m_objective;
                व्यञ्जकम्_Both(nodep, सच्च, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                जइ (checkMaxExprs()) भङ्ग;
            }
        }
        पङ्क्तिः_Track(nodep);
    }
    शून्य visit(AstXor* nodep) override { xor_व्यञ्जकम्(nodep); }

    शून्य व्यञ्जकम्_Not(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        VL_RESTORER(m_objective);
        जइ (m_seeking == NONE) {
            coverExprs(nodep);
        } अण्णहा {
            m_objective = !m_objective;
            iterateChildren(nodep);
            पङ्क्तिः_Track(nodep);
        }
    }
    शून्य visit(AstNot* nodep) override { व्यञ्जकम्_Not(nodep); }
    शून्य visit(AstLogNot* nodep) override { व्यञ्जकम्_Not(nodep); }

    template <typename T_Oper>
    शून्य व्यञ्जकम्_Reduce(Ast_ग्रन्थिः_Uniop* nodep) {
        जइ (m_seeking != ABORTED) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
            ध्रुव पूर्णाङ्क width = lhsp->dtypep()->width();
            ध्रुव size_t expected = std::is_same<T_Oper, AstXor>::मानम् ? 0x1 << width : width + 1;
            जइ (checkMaxExprs(expected)) प्रत्यागम;
            Ast_ग्रन्थिः_व्यञ्जकम्* unrolledp = नूतन AstSel{
                fl, lhsp->clone_वृक्षः(मिच्छ), नूतन AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            प्रति (पूर्णाङ्क bit = width - 2; bit >= 0; bit--) {
                AstSel* ध्रुव selp = नूतन AstSel{fl, lhsp->clone_वृक्षः(मिच्छ),
                                                नूतन AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = नूतन T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(unrolledp), unrolledp);
        } अण्णहा {
            iterateChildren(nodep);
            पङ्क्तिः_Track(nodep);
        }
    }
    शून्य visit(AstRedOr* nodep) override { व्यञ्जकम्_Reduce<AstOr>(nodep); }
    शून्य visit(AstRedAnd* nodep) override { व्यञ्जकम्_Reduce<AstAnd>(nodep); }
    शून्य visit(AstRedXor* nodep) override { व्यञ्जकम्_Reduce<AstXor>(nodep); }

    शून्य visit(AstLogIf* nodep) override {
        जइ (m_seeking == NONE) {
            coverExprs(nodep);
        } अण्णहा जइ (m_objective) {
            व्यञ्जकम्_Either(nodep, सच्च, मिच्छ, सच्च);
        } अण्णहा {
            व्यञ्जकम्_Both(nodep, सच्च, सच्च, मिच्छ);
        }
        पङ्क्तिः_Track(nodep);
    }

    शून्य visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        जइ (m_seeking == NONE) {
            coverExprs(nodep);
        } अण्णहा {
            m_objective = !m_objective;
            xor_व्यञ्जकम्(nodep);
            पङ्क्तिः_Track(nodep);
        }
    }

    शून्य visit(AstFuncRef* nodep) override {
        जइ (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(nodep));
        } अण्णहा {
            व्यञ्जकम्_Unsupported(nodep, "non-automatic function");
        }
    }

    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        जइ (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } अण्णहा {
            व्यञ्जकम्_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            जइ (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_वृक्षः(nodep, emitV);
                // Add new expression with a single term
                Cover_व्यञ्जकम् व्यञ्जकम्;
                व्यञ्जकम्.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.प्रक्षिप_back(std::move(व्यञ्जकम्));
                checkMaxExprs();
            } अण्णहा {
                व्यञ्जकम्_Unsupported(nodep, "not coverage eligible");
            }
        }
        पङ्क्तिः_Track(nodep);
    }

    // VISITORS - BOTH
    शून्य visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = मिच्छ;
        m_inToggleOff = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = मिच्छ;
        m_inToggleOff = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        iterateChildren(nodep);
        पङ्क्तिः_Track(nodep);
    }

    शून्य व्यञ्जकम्_Unsupported(Ast_ग्रन्थिः* nodep, ध्रुव string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ध्रुव तार्किक wasSeeking = m_seeking == SEEKING;
        ध्रुव Objective oldSeeking = m_seeking;
        जइ (wasSeeking) abort_व्यञ्जकम्_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        पङ्क्तिः_Track(nodep);
        जइ (!wasSeeking) m_seeking = oldSeeking;
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_व्यञ्जकम्_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = स्वभाव;
};

//######################################################################
// Coverage class functions

शून्य V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("coverage", 0, dump_वृक्षः_Either_Level() >= 3);
}
