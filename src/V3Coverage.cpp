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

वर्ग व्यंजक_Coverage_Eligible_Visitor final : सार्वजनिक VNVisitorConst {
    // STATE
    तार्किक m_eligible = सत्य;

    स्थिर तार्किक elemDTypeEligible(ध्रुव Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (ध्रुव Ast_गाँठ_DType* ध्रुव dtp = dtypep->virtRefDTypep()) {
            यदि (!elemDTypeEligible(dtp)) प्रत्यागम असत्य;
        }
        यदि (ध्रुव Ast_गाँठ_DType* ध्रुव dtp = dtypep->virtRefDType2p()) {
            यदि (!elemDTypeEligible(dtp)) प्रत्यागम असत्य;
        }
        प्रत्यागम !VN_IS(dtypep, ClassRefDType);
    }

    शून्य visit(Ast_गाँठ_Var_Ref* nodep) override {
        ध्रुव Ast_गाँठ_DType* ध्रुव dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        यदि (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            m_eligible = असत्य;
        }
    }

    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (!nodep->is_व्यंजक_Coverage_Eligible()) {
            m_eligible = असत्य;
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit व्यंजक_Coverage_Eligible_Visitor(Ast_गाँठ* nodep) { iterateConst(nodep); }
    ~व्यंजक_Coverage_Eligible_Visitor() override = स्वभाव;

    तार्किक eligible() ध्रुव { प्रत्यागम m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

वर्ग CoverageVisitor final : सार्वजनिक VNVisitor {
    // TYPES
    using LinenoSet = std::set<पूर्णाङ्क>;

    संरचना CoverTerm final {
        Ast_गाँठ_व्यंजक* m_exprp;  // Expression branch term
        तार्किक m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_गाँठ_व्यंजक* exprp, तार्किक objective, ध्रुव string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_व्यंजक = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_व्यंजक>;

    संरचना ToggleEnt final {
        ध्रुव string m_comment;  // Comment for coverage dump
        Ast_गाँठ_व्यंजक* m_varRefp;  // How to get to this element
        Ast_गाँठ_व्यंजक* m_chgRefp;  // How to get to this element
        ToggleEnt(ध्रुव string& comment, Ast_गाँठ_व्यंजक* vp, Ast_गाँठ_व्यंजक* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = स्वभाव;
        शून्य cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_वृक्ष(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_वृक्ष(), m_chgRefp = nullptr);
        }
    };

    संरचना CheckState final {  // State save-restored on each new coverage scope/block
        तार्किक m_on = असत्य;  // Should this block get covered?
        तार्किक m_inModOff = असत्य;  // In module with no coverage
        पूर्णाङ्क m_handle = 0;  // Opaque handle for index into line tracking
        ध्रुव Ast_गाँठ* m_nodep = nullptr;  // Node establishing this state
        CheckState() = स्वभाव;
        तार्किक coverageOn(ध्रुव Ast_गाँठ* nodep) ध्रुव {
            प्रत्यागम m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        तार्किक पंक्ति_Coverage_On(ध्रुव Ast_गाँठ* nodep) ध्रुव {
            प्रत्यागम coverageOn(nodep) && v3Global.opt.coverage_पंक्ति();
        }
        तार्किक branchCoverageOn(ध्रुव Ast_गाँठ* nodep) ध्रुव {
            प्रत्यागम coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        तार्किक व्यंजक_Coverage_On(ध्रुव Ast_गाँठ* nodep) ध्रुव {
            प्रत्यागम coverageOn(nodep) && v3Global.opt.coverage_व्यंजक();
        }
    };

    गणना Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;
    V3UniqueNames m_व्यंजक_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_गाँठ_व्यंजक*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    पूर्णाङ्क m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाँठ* m_व्यंजक_Stmtsp = nullptr;  // Node to add expr coverage to
    तार्किक m_then = असत्य;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    तार्किक m_objective = असत्य;  // Expression objective
    तार्किक m_ifCond = असत्य;  // Visiting if condition
    तार्किक m_inToggleOff = असत्य;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<पूर्णाङ्क, LinenoSet> m_handle_पंक्तियाँ;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ध्रुव वर्ण* varIgnoreToggle(ध्रुव AstVar* nodep) {
        ध्रुव तार्किक cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        यदि (!cover) प्रत्यागम "Not relevant signal";
        यदि (nodep->isConst()) प्रत्यागम "Signal is constant";
        यदि (nodep->isDouble()) प्रत्यागम "Signal is double";
        यदि (nodep->isString()) प्रत्यागम "Signal is string";
        यदि (!v3Global.opt.coverageUnderscore()) {
            ध्रुव string prettyName = nodep->prettyName();
            यदि (prettyName[0] == '_') प्रत्यागम "Leading underscore";
            यदि (prettyName.find("._") != string::npos) प्रत्यागम "Inlined leading underscore";
        }
        यदि ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            प्रत्यागम "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        प्रत्यागम nullptr;
    }

    AstCoverInc* newCoverInc(File_पंक्ति* fl, Ast_गाँठ_Cover_Decl* ध्रुव declp,
                             ध्रुव string& trace_var_name) {
        AstCoverInc* ध्रुव incp = नूतन AstCoverInc{fl, declp};
        यदि (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_पंक्ति* ध्रुव fl_nowarn = नूतन File_पंक्ति{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, सत्य);
            AstVar* ध्रुव varp = नूतन AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(सत्य);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* ध्रुव assp = नूतन AstAssign{
                incp->fileline(), नूतन AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                नूतन AstAdd{incp->fileline(), नूतन AstVarRef{incp->fileline(), varp, VAccess::READ},
                           नूतन AstConst{incp->fileline(), AstConst::Widthed_मूल्य{}, 32, 1}}};
            Ast_गाँठ::addNext<Ast_गाँठ, Ast_गाँठ>(incp, assp);
        }
        प्रत्यागम incp;
    }
    string trace_Name_For_पंक्ति(ध्रुव Ast_गाँठ* nodep, ध्रुव string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        यदि (ध्रुव uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        प्रत्यागम name;
    }

    // Line tracking
    शून्य createHandle(ध्रुव Ast_गाँठ* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = सत्य;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    शून्य पंक्ति_Track(ध्रुव Ast_गाँठ* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        यदि ((m_state.पंक्ति_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            प्रति (पूर्णाङ्क lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_पंक्तियाँ[m_state.m_handle].जोड़ो(lineno);
            }
        }
    }
    स्थिर string पंक्तियाँ_First_Last(ध्रुव पूर्णाङ्क first, ध्रुव पूर्णाङ्क last) {
        यदि (first && first == last) {
            प्रत्यागम cvtToStr(first);
        } अन्यथा यदि (first && last) {
            प्रत्यागम cvtToStr(first) + "-" + cvtToStr(last);
        } अन्यथा {
            प्रत्यागम "";
        }
    }
    string पंक्तियाँ_Cov(ध्रुव CheckState& state, ध्रुव Ast_गाँठ* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        ध्रुव LinenoSet& पंक्तियाँ = m_handle_पंक्तियाँ[state.m_handle];
        पूर्णाङ्क first = 0;
        पूर्णाङ्क last = 0;
        प्रति (ध्रुव पूर्णाङ्क linen : पंक्तियाँ) {
            यदि (!first) {
                first = last = linen;
            } अन्यथा यदि (linen == last + 1) {
                ++last;
            } अन्यथा {
                यदि (!out.empty()) out += ",";
                out += पंक्तियाँ_First_Last(first, last);
                first = last = linen;
            }
        }
        यदि (first) {
            यदि (!out.empty()) out += ",";
            out += पंक्तियाँ_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        प्रत्यागम out;
    }

    // VISITORS - BOTH
    शून्य visit(Ast_गाँठ_Module* nodep) override {
        ध्रुव Ast_गाँठ_Module* ध्रुव origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_व्यंजक_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_शीर्ष();  // Already made top shell, no coverage for it
        यदि (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_पंक्तियाँ.clear();
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_व्यंजक_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    शून्य visit(AstAlways* nodep) override {
        यदि (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_व्यंजक_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_व्यंजक_Stmtsp = nodep;
            m_inToggleOff = सत्य;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            प्रत्यागम;
        }
        iterateProcedure(nodep);
    }
    शून्य visit(Ast_गाँठ_Procedure* nodep) override { iterateProcedure(nodep); }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_व्यंजक_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_व्यंजक_Stmtsp = nodep;
        m_inToggleOff = सत्य;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        यदि (m_state.पंक्ति_Coverage_On(nodep)) {
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पंक्तियाँ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_गाँठ* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पंक्ति(nodep, "block"));
            जोड़ो_Proc_Statement(nodep, newp);
        }
    }
    शून्य visit(AstLoopTest* nodep) override {
        यदि (nodep->user2SetOnce()) प्रत्यागम;
        पंक्ति_Track(nodep);
        यदि (m_state.पंक्ति_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पंक्तियाँ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_गाँठ* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पंक्ति(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_व्यंजक_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        यदि (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    शून्य जोड़ो_Proc_Statement(Ast_गाँठ* nodep, Ast_गाँठ* stmtp) {
        यदि (Ast_गाँठ_Procedure* ध्रुव itemp = VN_CAST(nodep, गाँठ_Procedure)) {
            itemp->addStmtsp(stmtp);
        } अन्यथा यदि (Ast_गाँठ_FTask* ध्रुव itemp = VN_CAST(nodep, गाँठ_FTask)) {
            itemp->addStmtsp(stmtp);
        } अन्यथा यदि (AstLoop* ध्रुव itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } अन्यथा यदि (AstIf* ध्रुव itemp = VN_CAST(nodep, If)) {
            यदि (m_then) {
                itemp->addThensp(stmtp);
            } अन्यथा {
                itemp->addElsesp(stmtp);
            }
        } अन्यथा यदि (AstBegin* ध्रुव itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } अन्यथा {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    शून्य iterateProcedure(Ast_गाँठ* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_व्यंजक_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_व्यंजक_Stmtsp = nodep;
        m_inToggleOff = सत्य;
        createHandle(nodep);
        iterateChildren(nodep);
        यदि (m_state.पंक्ति_Coverage_On(nodep)) {
            पंक्ति_Track(nodep);
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पंक्तियाँ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_गाँठ* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पंक्ति(nodep, "block"));
            जोड़ो_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    शून्य visit(AstVar* nodep) override {
        iterateChildren(nodep);
        यदि (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ध्रुव वर्ण* ध्रुव disablep = varIgnoreToggle(nodep);
            यदि (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } अन्यथा {
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
                File_पंक्ति* ध्रुव fl_nowarn = नूतन File_पंक्ति{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, सत्य);
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
            = (m_beginHier != "") ? Ast_गाँठ::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ध्रुव declp
            = नूतन AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* ध्रुव newp = नूतन AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_वृक्ष(असत्य), above.m_chgRefp->clone_वृक्ष(असत्य)};
        m_modp->addStmtsp(newp);
    }

    शून्य toggleVarRecurse(ध्रुव Ast_गाँठ_DType* ध्रुव dtypep, ध्रुव पूर्णाङ्क depth,  // per-iteration
                          ध्रुव ToggleEnt& above, ध्रुव AstVar* ध्रुव varp) {  // Constant
        यदि (ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } अन्यथा यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            प्रति (ध्रुव पूर्णाङ्क index_docs : adtypep->declRange().seqLoToHi()) {
                ध्रुव पूर्णाङ्क index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 नूतन AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_वृक्ष(असत्य), index_code},
                                 नूतन AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_वृक्ष(असत्य), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } अन्यथा यदि (ध्रुव AstPackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, PackArrayDType)) {
            प्रति (ध्रुव पूर्णाङ्क index_docs : adtypep->declRange().seqLoToHi()) {
                ध्रुव Ast_गाँठ_DType* ध्रुव subtypep = adtypep->subDTypep()->skipRefp();
                ध्रुव पूर्णाङ्क index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 नूतन AstSel{varp->fileline(), above.m_varRefp->clone_वृक्ष(असत्य),
                                            index_code * subtypep->width(), subtypep->width()},
                                 नूतन AstSel{varp->fileline(), above.m_chgRefp->clone_वृक्ष(असत्य),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } अन्यथा यदि (ध्रुव AstStructDType* ध्रुव adtypep = VN_CAST(dtypep, StructDType)) {
            यदि (adtypep->packed()) {
                प्रति (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ध्रुव Ast_गाँठ_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                    ध्रुव पूर्णाङ्क index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        नूतन AstSel{varp->fileline(), above.m_varRefp->clone_वृक्ष(असत्य), index_code,
                                   subtypep->width()},
                        नूतन AstSel{varp->fileline(), above.m_chgRefp->clone_वृक्ष(असत्य), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } अन्यथा {
                प्रति (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_गाँठ_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                    Ast_गाँठ_व्यंजक* ध्रुव varRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_वृक्ष(असत्य), itemp->name()};
                    Ast_गाँठ_व्यंजक* ध्रुव chgRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_वृक्ष(असत्य), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } अन्यथा यदि (ध्रुव AstUnionDType* ध्रुव adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            यदि (ध्रुव AstMemberDType* ध्रुव itemp = adtypep->membersp()) {
                Ast_गाँठ_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                यदि (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_वृक्ष(असत्य),
                                     above.m_chgRefp->clone_वृक्ष(असत्य)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } अन्यथा {
                    Ast_गाँठ_व्यंजक* ध्रुव varRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_वृक्ष(असत्य), itemp->name()};
                    Ast_गाँठ_व्यंजक* ध्रुव chgRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_वृक्ष(असत्य), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } अन्यथा यदि (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } अन्यथा {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    तार्किक includeCondToBranchRecursive(ध्रुव Ast_गाँठ* ध्रुव nodep) {
        ध्रुव Ast_गाँठ* ध्रुव backp = nodep->backp();
        यदि (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            प्रत्यागम includeCondToBranchRecursive(backp);
        } अन्यथा यदि (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            प्रत्यागम includeCondToBranchRecursive(backp);
        } अन्यथा यदि (VN_IS(backp, गाँठ_Assign) && VN_AS(backp, गाँठ_Assign)->rhsp() == nodep) {
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // VISITORS - LINE COVERAGE
    शून्य visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        यदि (m_seeking == NONE) coverExprs(nodep->condp());

        यदि (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ध्रुव CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            पंक्ति_Track(nodep);
            Ast_गाँठ_व्यंजक* ध्रुव thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* ध्रुव thenDeclp
                = नूतन AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", पंक्तियाँ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(नूतन Ast_व्यंजक_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_पंक्ति(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_गाँठ_व्यंजक* ध्रुव elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* ध्रुव elseDeclp
                = नूतन AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", पंक्तियाँ_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(नूतन Ast_व्यंजक_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_पंक्ति(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } अन्यथा {
            पंक्ति_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    शून्य visit(AstIf* nodep) override {
        यदि (nodep->user2()) प्रत्यागम;

        UINFO(4, " IF: " << nodep);
        यदि (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ध्रुव तार्किक elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            यदि (elsif) VN_AS(nodep->elsesp(), If)->user1(सत्य);
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
                VL_RESTORER(m_व्यंजक_Stmtsp);
                VL_RESTORER(m_then);
                m_व्यंजक_Stmtsp = nodep;
                m_then = सत्य;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                पंक्ति_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_व्यंजक_Stmtsp);
                VL_RESTORER(m_then);
                m_व्यंजक_Stmtsp = nodep;
                m_then = असत्य;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            यदि (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ध्रुव ifDeclp
                    = नूतन AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", पंक्तियाँ_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_पंक्ति(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ध्रुव elseDeclp
                    = नूतन AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", पंक्तियाँ_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_पंक्ति(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            अन्यथा यदि (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                यदि (ifState.पंक्ति_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ध्रुव elsifDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        पंक्तियाँ_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_पंक्ति(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } अन्यथा {
                // Cover as separate blocks (not a branch as is not two-legged)
                यदि (ifState.पंक्ति_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ध्रुव ifDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        पंक्तियाँ_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_पंक्ति(nodep, "if")));
                }
                यदि (elseState.पंक्ति_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ध्रुव elseDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        पंक्तियाँ_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_पंक्ति(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = सत्य;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    शून्य visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        यदि (m_state.पंक्ति_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            यदि (m_state.पंक्ति_Coverage_On(nodep)) {  // if the case body didn't disable it
                पंक्ति_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ध्रुव declp
                    = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", पंक्तियाँ_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_पंक्ति(nodep, "case")));
            }
        }
    }
    शून्य visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = सत्य;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        यदि (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            पंक्ति_Track(nodep);
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", पंक्तियाँ_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    शून्य visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_व्यंजक_Stmtsp);
        m_व्यंजक_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    शून्य visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = असत्य;
    }
    शून्य visit(AstStmtPragma* nodep) override {
        यदि (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = असत्य;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        } अन्यथा {
            यदि (m_state.m_on) iterateChildren(nodep);
            पंक्ति_Track(nodep);
        }
    }
    शून्य visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        यदि (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        पंक्ति_Track(nodep);
    }

    शून्य visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_व्यंजक_Stmtsp);
        m_व्यंजक_Stmtsp = nodep;
        m_inToggleOff = सत्य;
        यदि (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        पंक्ति_Track(nodep);
    }

    शून्य abort_व्यंजक_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        यदि (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    तार्किक checkMaxExprs(size_t additional = 0) {
        यदि (m_seeking != ABORTED
            && static_cast<पूर्णाङ्क>(m_exprs.size() + additional) <= v3Global.opt.coverage_व्यंजक_Max())
            प्रत्यागम असत्य;
        abort_व्यंजक_Coverage();
        प्रत्यागम सत्य;
    }

    शून्य add_व्यंजक_Cover_Inc(Ast_गाँठ_व्यंजक* nodep, पूर्णाङ्क start = 0) {
        File_पंक्ति* ध्रुव fl = nodep->fileline();
        पूर्णाङ्क गिनती = start;
        प्रति (Cover_व्यंजक& व्यंजक : m_exprs) {
            ध्रुव string name = "expr_" + std::to_string(गिनती);
            string comment = "(";
            तार्किक first = सत्य;
            Ast_गाँठ_व्यंजक* condp = nullptr;
            प्रति (CoverTerm& term : व्यंजक) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_गाँठ_व्यंजक* covExprp = nullptr;
                यदि (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_गाँठ_व्यंजक* ध्रुव frefp = term.m_exprp;
                    Ast_गाँठ_DType* ध्रुव dtypep = frefp->dtypep();
                    ध्रुव auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    यदि (pair.second) {
                        varp = नूतन AstVar{fl, VVarType::MODULETEMP, m_व्यंजक_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        यदि (m_ftaskp) {
                            varp->funcLocal(सत्य);
                            m_ftaskp->prependStmtsp(varp);
                        } अन्यथा {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(नूतन Ast_व्यंजक_Stmt{
                            fl, नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            नूतन AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = नूतन AstVarRef{fl, varp, VAccess::READ};
                } अन्यथा {
                    covExprp = term.m_exprp->clone_वृक्ष(असत्य);
                }
                Ast_गाँठ_व्यंजक* ध्रुव termp
                    = term.m_objective ? covExprp : नूतन AstLogNot{fl, covExprp};
                यदि (condp) {
                    condp = नूतन AstLogAnd{fl, condp, termp};
                } अन्यथा {
                    condp = termp;
                }
                first = असत्य;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ध्रुव declp = नूतन AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_गाँठ* ध्रुव newp = newCoverInc(fl, declp, trace_Name_For_पंक्ति(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ध्रुव ifp = नूतन AstIf{fl, condp, newp, nullptr};
            ifp->user2(सत्य);
            जोड़ो_Proc_Statement(m_व्यंजक_Stmtsp, ifp);
            ++गिनती;
        }
    }

    शून्य coverExprs(Ast_गाँठ_व्यंजक* nodep) {
        यदि (!m_state.व्यंजक_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_व्यंजक_Stmtsp) {
            प्रत्यागम;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = असत्य;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = सत्य;
        iterate(nodep);
        यदि (checkMaxExprs(falseExprs.size())) प्रत्यागम;
        यदि (m_seeking == ABORTED) प्रत्यागम;

        add_व्यंजक_Cover_Inc(nodep);
        ध्रुव पूर्णाङ्क start = m_exprs.size();
        m_objective = असत्य;
        m_exprs.swap(falseExprs);
        add_व्यंजक_Cover_Inc(nodep, start);
    }

    शून्य व्यंजक_Either(Ast_गाँठ_Biop* nodep, तार्किक overrideObjective = असत्य, तार्किक lObjective = असत्य,
                    तार्किक rObjective = असत्य) {
        VL_RESTORER(m_objective);
        Ast_गाँठ_व्यंजक* ध्रुव lhsp = nodep->lhsp();
        Ast_गाँठ_व्यंजक* ध्रुव rhsp = nodep->rhsp();

        यदि (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        यदि (checkMaxExprs()) प्रत्यागम;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        यदि (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    शून्य व्यंजक_Both(Ast_गाँठ_Biop* nodep, तार्किक overrideObjective = असत्य, तार्किक lObjective = असत्य,
                  तार्किक rObjective = असत्य) {
        VL_RESTORER(m_objective);
        Ast_गाँठ_व्यंजक* ध्रुव lhsp = nodep->lhsp();
        Ast_गाँठ_व्यंजक* ध्रुव rhsp = nodep->rhsp();

        यदि (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        यदि (checkMaxExprs()) प्रत्यागम;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        यदि (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        यदि (checkMaxExprs()) प्रत्यागम;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        प्रति (ध्रुव Cover_व्यंजक& l : lhsExprs) {
            प्रति (ध्रुव Cover_व्यंजक& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_व्यंजक व्यंजक;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                प्रति (ध्रुव CoverTerm& term : l) {
                    यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].जोड़ो(refp->varp());
                    } अन्यथा {
                        strs[term.m_objective].जोड़ो(term.m_emitV);
                    }
                    व्यंजक.धकेलो_back(term);
                }
                तार्किक impossible = असत्य;
                प्रति (ध्रुव CoverTerm& term : r) {
                    तार्किक redundant = असत्य;
                    यदि (ध्रुव Ast_गाँठ_Var_Ref* ध्रुव refp = VN_CAST(term.m_exprp, गाँठ_Var_Ref)) {
                        यदि (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = सत्य;
                        यदि (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = सत्य;
                    } अन्यथा {
                        यदि (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = सत्य;
                        यदि (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = सत्य;
                    }
                    यदि (!redundant) व्यंजक.धकेलो_back(term);
                }
                यदि (!impossible) m_exprs.धकेलो_back(std::move(व्यंजक));
                यदि (checkMaxExprs()) प्रत्यागम;
            }
        }
    }

    शून्य or_व्यंजक(Ast_गाँठ_Biop* nodep) {
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा यदि (m_objective) {
            व्यंजक_Either(nodep);
        } अन्यथा {
            व्यंजक_Both(nodep);
        }
        पंक्ति_Track(nodep);
    }
    शून्य visit(AstLogOr* nodep) override { or_व्यंजक(nodep); }
    शून्य visit(AstOr* nodep) override { or_व्यंजक(nodep); }

    शून्य and_व्यंजक(Ast_गाँठ_Biop* nodep) {
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा यदि (m_objective) {
            व्यंजक_Both(nodep);
        } अन्यथा {
            व्यंजक_Either(nodep);
        }
        पंक्ति_Track(nodep);
    }
    शून्य visit(AstLogAnd* nodep) override { and_व्यंजक(nodep); }
    शून्य visit(AstAnd* nodep) override { and_व्यंजक(nodep); }

    शून्य xor_व्यंजक(Ast_गाँठ_Biop* nodep) {
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            प्रति (ध्रुव तार्किक lObjective : {असत्य, सत्य}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                ध्रुव तार्किक rObjective = lObjective ^ m_objective;
                व्यंजक_Both(nodep, सत्य, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                यदि (checkMaxExprs()) भङ्ग;
            }
        }
        पंक्ति_Track(nodep);
    }
    शून्य visit(AstXor* nodep) override { xor_व्यंजक(nodep); }

    शून्य व्यंजक_Not(Ast_गाँठ_व्यंजक* nodep) {
        VL_RESTORER(m_objective);
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            m_objective = !m_objective;
            iterateChildren(nodep);
            पंक्ति_Track(nodep);
        }
    }
    शून्य visit(AstNot* nodep) override { व्यंजक_Not(nodep); }
    शून्य visit(AstLogNot* nodep) override { व्यंजक_Not(nodep); }

    template <typename T_Oper>
    शून्य व्यंजक_Reduce(Ast_गाँठ_Uniop* nodep) {
        यदि (m_seeking != ABORTED) {
            File_पंक्ति* ध्रुव fl = nodep->fileline();
            Ast_गाँठ_व्यंजक* ध्रुव lhsp = nodep->lhsp();
            ध्रुव पूर्णाङ्क width = lhsp->dtypep()->width();
            ध्रुव size_t expected = std::is_same<T_Oper, AstXor>::मूल्य ? 0x1 << width : width + 1;
            यदि (checkMaxExprs(expected)) प्रत्यागम;
            Ast_गाँठ_व्यंजक* unrolledp = नूतन AstSel{
                fl, lhsp->clone_वृक्ष(असत्य), नूतन AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            प्रति (पूर्णाङ्क bit = width - 2; bit >= 0; bit--) {
                AstSel* ध्रुव selp = नूतन AstSel{fl, lhsp->clone_वृक्ष(असत्य),
                                                नूतन AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = नूतन T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(धकेलो_Deletep(unrolledp), unrolledp);
        } अन्यथा {
            iterateChildren(nodep);
            पंक्ति_Track(nodep);
        }
    }
    शून्य visit(AstRedOr* nodep) override { व्यंजक_Reduce<AstOr>(nodep); }
    शून्य visit(AstRedAnd* nodep) override { व्यंजक_Reduce<AstAnd>(nodep); }
    शून्य visit(AstRedXor* nodep) override { व्यंजक_Reduce<AstXor>(nodep); }

    शून्य visit(AstLogIf* nodep) override {
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा यदि (m_objective) {
            व्यंजक_Either(nodep, सत्य, असत्य, सत्य);
        } अन्यथा {
            व्यंजक_Both(nodep, सत्य, सत्य, असत्य);
        }
        पंक्ति_Track(nodep);
    }

    शून्य visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            m_objective = !m_objective;
            xor_व्यंजक(nodep);
            पंक्ति_Track(nodep);
        }
    }

    शून्य visit(AstFuncRef* nodep) override {
        यदि (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_गाँठ_व्यंजक*>(nodep));
        } अन्यथा {
            व्यंजक_Unsupported(nodep, "non-automatic function");
        }
    }

    शून्य visit(Ast_गाँठ_व्यंजक* nodep) override {
        यदि (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } अन्यथा {
            व्यंजक_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            यदि (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_वृक्ष(nodep, emitV);
                // Add new expression with a single term
                Cover_व्यंजक व्यंजक;
                व्यंजक.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.धकेलो_back(std::move(व्यंजक));
                checkMaxExprs();
            } अन्यथा {
                व्यंजक_Unsupported(nodep, "not coverage eligible");
            }
        }
        पंक्ति_Track(nodep);
    }

    // VISITORS - BOTH
    शून्य visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = असत्य;
        m_inToggleOff = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = असत्य;
        m_inToggleOff = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठ* nodep) override {
        iterateChildren(nodep);
        पंक्ति_Track(nodep);
    }

    शून्य व्यंजक_Unsupported(Ast_गाँठ* nodep, ध्रुव string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ध्रुव तार्किक wasSeeking = m_seeking == SEEKING;
        ध्रुव Objective oldSeeking = m_seeking;
        यदि (wasSeeking) abort_व्यंजक_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        पंक्ति_Track(nodep);
        यदि (!wasSeeking) m_seeking = oldSeeking;
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_व्यंजक_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = स्वभाव;
};

//######################################################################
// Coverage class functions

शून्य V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("coverage", 0, dump_वृक्ष_Either_Level() >= 3);
}
