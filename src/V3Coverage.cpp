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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Coverage.h"

#समावेश "V3EmitV.h"
#समावेश "V3UniqueNames.h"

#समावेश <list>
#समावेश <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी व्यंजक_Coverage_Eligible_Visitor final : खुला VNVisitorConst {
    // STATE
    bool m_eligible = true;

    अटल bool elemDTypeEligible(स्थिर Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        अगर (स्थिर Ast_गाँठ_DType* स्थिर dtp = dtypep->virtRefDTypep()) {
            अगर (!elemDTypeEligible(dtp)) वापस false;
        }
        अगर (स्थिर Ast_गाँठ_DType* स्थिर dtp = dtypep->virtRefDType2p()) {
            अगर (!elemDTypeEligible(dtp)) वापस false;
        }
        वापस !VN_IS(dtypep, ClassRefDType);
    }

    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        स्थिर Ast_गाँठ_DType* स्थिर dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        अगर (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            m_eligible = false;
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (!nodep->is_व्यंजक_Coverage_Eligible()) {
            m_eligible = false;
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
    }

खुला:
    // CONSTRUCTORS
    explicit व्यंजक_Coverage_Eligible_Visitor(Ast_गाँठ* nodep) { iterateConst(nodep); }
    ~व्यंजक_Coverage_Eligible_Visitor() override = शेष;

    bool eligible() स्थिर { वापस m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

श्रेणी CoverageVisitor final : खुला VNVisitor {
    // TYPES
    using LinenoSet = std::set<पूर्णांक>;

    काष्ठा CoverTerm final {
        Ast_गाँठ_व्यंजक* m_exprp;  // Expression branch term
        bool m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_गाँठ_व्यंजक* exprp, bool objective, स्थिर string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_व्यंजक = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_व्यंजक>;

    काष्ठा ToggleEnt final {
        स्थिर string m_comment;  // Comment for coverage dump
        Ast_गाँठ_व्यंजक* m_varRefp;  // How to get to this element
        Ast_गाँठ_व्यंजक* m_chgRefp;  // How to get to this element
        ToggleEnt(स्थिर string& comment, Ast_गाँठ_व्यंजक* vp, Ast_गाँठ_व्यंजक* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = शेष;
        व्योम cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_वृक्ष(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_वृक्ष(), m_chgRefp = nullptr);
        }
    };

    काष्ठा CheckState final {  // State save-restored on each new coverage scope/block
        bool m_on = false;  // Should this block get covered?
        bool m_inModOff = false;  // In module with no coverage
        पूर्णांक m_handle = 0;  // Opaque handle for index into line tracking
        स्थिर Ast_गाँठ* m_nodep = nullptr;  // Node establishing this state
        CheckState() = शेष;
        bool coverageOn(स्थिर Ast_गाँठ* nodep) स्थिर {
            वापस m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        bool पंक्ति_Coverage_On(स्थिर Ast_गाँठ* nodep) स्थिर {
            वापस coverageOn(nodep) && v3Global.opt.coverage_पंक्ति();
        }
        bool branchCoverageOn(स्थिर Ast_गाँठ* nodep) स्थिर {
            वापस coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        bool व्यंजक_Coverage_On(स्थिर Ast_गाँठ* nodep) स्थिर {
            वापस coverageOn(nodep) && v3Global.opt.coverage_व्यंजक();
        }
    };

    क्रमागत Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;
    V3UniqueNames m_व्यंजक_अस्थायी_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_गाँठ_व्यंजक*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    पूर्णांक m_अग्र_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाँठ* m_व्यंजक_Stmtsp = nullptr;  // Node to add expr coverage to
    bool m_then = false;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    bool m_objective = false;  // Expression objective
    bool m_ifCond = false;  // Visiting if condition
    bool m_inToggleOff = false;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<पूर्णांक, LinenoSet> m_handle_पंक्तियाँ;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    स्थिर अक्षर* varIgnoreToggle(स्थिर AstVar* nodep) {
        स्थिर bool cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        अगर (!cover) वापस "Not relevant signal";
        अगर (nodep->isConst()) वापस "Signal is constant";
        अगर (nodep->isDouble()) वापस "Signal is double";
        अगर (nodep->isString()) वापस "Signal is string";
        अगर (!v3Global.opt.coverageUnderscore()) {
            स्थिर string prettyName = nodep->prettyName();
            अगर (prettyName[0] == '_') वापस "Leading underscore";
            अगर (prettyName.find("._") != string::npos) वापस "Inlined leading underscore";
        }
        अगर ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            वापस "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        वापस nullptr;
    }

    AstCoverInc* newCoverInc(File_पंक्ति* fl, Ast_गाँठ_Cover_Decl* स्थिर declp,
                             स्थिर string& trace_var_name) {
        AstCoverInc* स्थिर incp = नया AstCoverInc{fl, declp};
        अगर (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_पंक्ति* स्थिर fl_nowarn = नया File_पंक्ति{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
            AstVar* स्थिर varp = नया AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(true);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* स्थिर assp = नया AstAssign{
                incp->fileline(), नया AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                नया AstAdd{incp->fileline(), नया AstVarRef{incp->fileline(), varp, VAccess::READ},
                           नया AstConst{incp->fileline(), AstConst::Widthed_मूल्य{}, 32, 1}}};
            Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(incp, assp);
        }
        वापस incp;
    }
    string trace_Name_For_पंक्ति(स्थिर Ast_गाँठ* nodep, स्थिर string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        अगर (स्थिर uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        वापस name;
    }

    // Line tracking
    व्योम createHandle(स्थिर Ast_गाँठ* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = true;
        m_state.m_handle = ++m_अग्र_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    व्योम पंक्ति_Track(स्थिर Ast_गाँठ* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        अगर ((m_state.पंक्ति_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            क्रम (पूर्णांक lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_पंक्तियाँ[m_state.m_handle].जोड़ो(lineno);
            }
        }
    }
    अटल string पंक्तियाँ_First_Last(स्थिर पूर्णांक first, स्थिर पूर्णांक last) {
        अगर (first && first == last) {
            वापस cvtToStr(first);
        } अन्यथा अगर (first && last) {
            वापस cvtToStr(first) + "-" + cvtToStr(last);
        } अन्यथा {
            वापस "";
        }
    }
    string पंक्तियाँ_Cov(स्थिर CheckState& state, स्थिर Ast_गाँठ* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        स्थिर LinenoSet& पंक्तियाँ = m_handle_पंक्तियाँ[state.m_handle];
        पूर्णांक first = 0;
        पूर्णांक last = 0;
        क्रम (स्थिर पूर्णांक linen : पंक्तियाँ) {
            अगर (!first) {
                first = last = linen;
            } अन्यथा अगर (linen == last + 1) {
                ++last;
            } अन्यथा {
                अगर (!out.empty()) out += ",";
                out += पंक्तियाँ_First_Last(first, last);
                first = last = linen;
            }
        }
        अगर (first) {
            अगर (!out.empty()) out += ",";
            out += पंक्तियाँ_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        वापस out;
    }

    // VISITORS - BOTH
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        स्थिर Ast_गाँठ_Module* स्थिर origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_व्यंजक_अस्थायी_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_शीर्ष();  // Already made top shell, no coverage for it
        अगर (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.सफा();
            m_handle_पंक्तियाँ.सफा();
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_व्यंजक_अस्थायी_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    व्योम visit(AstAlways* nodep) override {
        अगर (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_व्यंजक_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_व्यंजक_Stmtsp = nodep;
            m_inToggleOff = true;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            वापस;
        }
        iterateProcedure(nodep);
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override { iterateProcedure(nodep); }
    व्योम visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_व्यंजक_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_व्यंजक_Stmtsp = nodep;
        m_inToggleOff = true;
        createHandle(nodep);
        iterate_And_अग्र_Null(nodep->stmtsp());
        अगर (m_state.पंक्ति_Coverage_On(nodep)) {
            AstCoverOtherDecl* स्थिर declp
                = नया AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पंक्तियाँ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_गाँठ* स्थिर newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पंक्ति(nodep, "block"));
            जोड़ो_Proc_Statement(nodep, newp);
        }
    }
    व्योम visit(AstLoopTest* nodep) override {
        अगर (nodep->user2SetOnce()) वापस;
        पंक्ति_Track(nodep);
        अगर (m_state.पंक्ति_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* स्थिर declp
                = नया AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पंक्तियाँ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_गाँठ* स्थिर newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पंक्ति(nodep, "block"));
            nodep->add_Here_This_As_अग्र(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_व्यंजक_अस्थायी_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        अगर (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    व्योम जोड़ो_Proc_Statement(Ast_गाँठ* nodep, Ast_गाँठ* stmtp) {
        अगर (Ast_गाँठ_Procedure* स्थिर itemp = VN_CAST(nodep, गाँठ_Procedure)) {
            itemp->addStmtsp(stmtp);
        } अन्यथा अगर (Ast_गाँठ_FTask* स्थिर itemp = VN_CAST(nodep, गाँठ_FTask)) {
            itemp->addStmtsp(stmtp);
        } अन्यथा अगर (AstLoop* स्थिर itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } अन्यथा अगर (AstIf* स्थिर itemp = VN_CAST(nodep, If)) {
            अगर (m_then) {
                itemp->addThensp(stmtp);
            } अन्यथा {
                itemp->addElsesp(stmtp);
            }
        } अन्यथा अगर (AstBegin* स्थिर itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } अन्यथा {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    व्योम iterateProcedure(Ast_गाँठ* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_व्यंजक_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_व्यंजक_Stmtsp = nodep;
        m_inToggleOff = true;
        createHandle(nodep);
        iterateChildren(nodep);
        अगर (m_state.पंक्ति_Coverage_On(nodep)) {
            पंक्ति_Track(nodep);
            AstCoverOtherDecl* स्थिर declp
                = नया AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पंक्तियाँ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_गाँठ* स्थिर newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पंक्ति(nodep, "block"));
            जोड़ो_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    व्योम visit(AstVar* nodep) override {
        iterateChildren(nodep);
        अगर (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            स्थिर अक्षर* स्थिर disablep = varIgnoreToggle(nodep);
            अगर (disablep) {
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
                स्थिर string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                File_पंक्ति* स्थिर fl_nowarn = नया File_पंक्ति{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
                AstVar* स्थिर chgVarp
                    = नया AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, नया AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 नया AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    व्योम toggleVarBottom(स्थिर ToggleEnt& above, स्थिर AstVar* varp, स्थिर VNumRange& range) {
        स्थिर std::string hierPrefix
            = (m_beginHier != "") ? Ast_गाँठ::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* स्थिर declp
            = नया AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* स्थिर newp = नया AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_वृक्ष(false), above.m_chgRefp->clone_वृक्ष(false)};
        m_modp->addStmtsp(newp);
    }

    व्योम toggleVarRecurse(स्थिर Ast_गाँठ_DType* स्थिर dtypep, स्थिर पूर्णांक depth,  // per-iteration
                          स्थिर ToggleEnt& above, स्थिर AstVar* स्थिर varp) {  // Constant
        अगर (स्थिर AstBasicDType* स्थिर basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } अन्यथा अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            क्रम (स्थिर पूर्णांक सूचकांक_docs : adtypep->declRange().seqLoToHi()) {
                स्थिर पूर्णांक सूचकांक_code = सूचकांक_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(सूचकांक_docs) + "]",
                                 नया AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_वृक्ष(false), सूचकांक_code},
                                 नया AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_वृक्ष(false), सूचकांक_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } अन्यथा अगर (स्थिर AstPackArrayDType* स्थिर adtypep = VN_CAST(dtypep, PackArrayDType)) {
            क्रम (स्थिर पूर्णांक सूचकांक_docs : adtypep->declRange().seqLoToHi()) {
                स्थिर Ast_गाँठ_DType* स्थिर subtypep = adtypep->subDTypep()->skipRefp();
                स्थिर पूर्णांक सूचकांक_code = सूचकांक_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(सूचकांक_docs) + "]",
                                 नया AstSel{varp->fileline(), above.m_varRefp->clone_वृक्ष(false),
                                            सूचकांक_code * subtypep->width(), subtypep->width()},
                                 नया AstSel{varp->fileline(), above.m_chgRefp->clone_वृक्ष(false),
                                            सूचकांक_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } अन्यथा अगर (स्थिर AstStructDType* स्थिर adtypep = VN_CAST(dtypep, StructDType)) {
            अगर (adtypep->packed()) {
                क्रम (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    स्थिर Ast_गाँठ_DType* स्थिर subtypep = itemp->subDTypep()->skipRefp();
                    स्थिर पूर्णांक सूचकांक_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        नया AstSel{varp->fileline(), above.m_varRefp->clone_वृक्ष(false), सूचकांक_code,
                                   subtypep->width()},
                        नया AstSel{varp->fileline(), above.m_chgRefp->clone_वृक्ष(false), सूचकांक_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } अन्यथा {
                क्रम (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_गाँठ_DType* स्थिर subtypep = itemp->subDTypep()->skipRefp();
                    Ast_गाँठ_व्यंजक* स्थिर varRefp = नया AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_वृक्ष(false), itemp->name()};
                    Ast_गाँठ_व्यंजक* स्थिर chgRefp = नया AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_वृक्ष(false), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } अन्यथा अगर (स्थिर AstUnionDType* स्थिर adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            अगर (स्थिर AstMemberDType* स्थिर itemp = adtypep->membersp()) {
                Ast_गाँठ_DType* स्थिर subtypep = itemp->subDTypep()->skipRefp();
                अगर (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_वृक्ष(false),
                                     above.m_chgRefp->clone_वृक्ष(false)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } अन्यथा {
                    Ast_गाँठ_व्यंजक* स्थिर varRefp = नया AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_वृक्ष(false), itemp->name()};
                    Ast_गाँठ_व्यंजक* स्थिर chgRefp = नया AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_वृक्ष(false), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } अन्यथा अगर (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } अन्यथा {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    bool includeCondToBranchRecursive(स्थिर Ast_गाँठ* स्थिर nodep) {
        स्थिर Ast_गाँठ* स्थिर backp = nodep->backp();
        अगर (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            वापस includeCondToBranchRecursive(backp);
        } अन्यथा अगर (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            वापस includeCondToBranchRecursive(backp);
        } अन्यथा अगर (VN_IS(backp, गाँठ_Assign) && VN_AS(backp, गाँठ_Assign)->rhsp() == nodep) {
            वापस true;
        }
        वापस false;
    }

    // VISITORS - LINE COVERAGE
    व्योम visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        अगर (m_seeking == NONE) coverExprs(nodep->condp());

        अगर (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            स्थिर CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            पंक्ति_Track(nodep);
            Ast_गाँठ_व्यंजक* स्थिर thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* स्थिर thenDeclp
                = नया AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", पंक्तियाँ_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(नया Ast_व्यंजक_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_पंक्ति(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_गाँठ_व्यंजक* स्थिर elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* स्थिर elseDeclp
                = नया AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", पंक्तियाँ_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(नया Ast_व्यंजक_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_पंक्ति(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } अन्यथा {
            पंक्ति_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    व्योम visit(AstIf* nodep) override {
        अगर (nodep->user2()) वापस;

        UINFO(4, " IF: " << nodep);
        अगर (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            स्थिर bool elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            अगर (elsif) VN_AS(nodep->elsesp(), If)->user1(true);
            स्थिर bool first_elsif = !nodep->user1() && elsif;
            स्थिर bool cont_elsif = nodep->user1() && elsif;
            स्थिर bool final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            स्थिर CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_व्यंजक_Stmtsp);
                VL_RESTORER(m_then);
                m_व्यंजक_Stmtsp = nodep;
                m_then = true;
                createHandle(nodep);
                iterate_And_अग्र_Null(nodep->thensp());
                पंक्ति_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_व्यंजक_Stmtsp);
                VL_RESTORER(m_then);
                m_व्यंजक_Stmtsp = nodep;
                m_then = false;
                createHandle(nodep);
                iterate_And_अग्र_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            अगर (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* स्थिर ifDeclp
                    = नया AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", पंक्तियाँ_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_पंक्ति(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* स्थिर elseDeclp
                    = नया AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", पंक्तियाँ_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_पंक्ति(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            अन्यथा अगर (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                अगर (ifState.पंक्ति_Coverage_On(nodep)) {
                    AstCoverOtherDecl* स्थिर elsifDeclp = नया AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        पंक्तियाँ_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_पंक्ति(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } अन्यथा {
                // Cover as separate blocks (not a branch as is not two-legged)
                अगर (ifState.पंक्ति_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* स्थिर ifDeclp = नया AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        पंक्तियाँ_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_पंक्ति(nodep, "if")));
                }
                अगर (elseState.पंक्ति_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* स्थिर elseDeclp = नया AstCoverOtherDecl{
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
        m_ifCond = true;
        iterate_And_अग्र_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    व्योम visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        अगर (m_state.पंक्ति_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_अग्र_Null(nodep->stmtsp());
            अगर (m_state.पंक्ति_Coverage_On(nodep)) {  // if the case body didn't disable it
                पंक्ति_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* स्थिर declp
                    = नया AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", पंक्तियाँ_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_पंक्ति(nodep, "case")));
            }
        }
    }
    व्योम visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = true;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        अगर (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            पंक्ति_Track(nodep);
            AstCoverOtherDecl* स्थिर declp
                = नया AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", पंक्तियाँ_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    व्योम visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_व्यंजक_Stmtsp);
        m_व्यंजक_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    व्योम visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = false;
    }
    व्योम visit(AstStmtPragma* nodep) override {
        अगर (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = false;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        } अन्यथा {
            अगर (m_state.m_on) iterateChildren(nodep);
            पंक्ति_Track(nodep);
        }
    }
    व्योम visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        अगर (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        पंक्ति_Track(nodep);
    }

    व्योम visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_व्यंजक_Stmtsp);
        m_व्यंजक_Stmtsp = nodep;
        m_inToggleOff = true;
        अगर (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        पंक्ति_Track(nodep);
    }

    व्योम abort_व्यंजक_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        अगर (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.सफा();
    }

    bool checkMaxExprs(माप_प्रकार additional = 0) {
        अगर (m_seeking != ABORTED
            && static_cast<पूर्णांक>(m_exprs.आकार() + additional) <= v3Global.opt.coverage_व्यंजक_Max())
            वापस false;
        abort_व्यंजक_Coverage();
        वापस true;
    }

    व्योम add_व्यंजक_Cover_Inc(Ast_गाँठ_व्यंजक* nodep, पूर्णांक start = 0) {
        File_पंक्ति* स्थिर fl = nodep->fileline();
        पूर्णांक गिनती = start;
        क्रम (Cover_व्यंजक& व्यंजक : m_exprs) {
            स्थिर string name = "expr_" + std::to_string(गिनती);
            string comment = "(";
            bool first = true;
            Ast_गाँठ_व्यंजक* condp = nullptr;
            क्रम (CoverTerm& term : व्यंजक) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_गाँठ_व्यंजक* covExprp = nullptr;
                अगर (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_गाँठ_व्यंजक* स्थिर frefp = term.m_exprp;
                    Ast_गाँठ_DType* स्थिर dtypep = frefp->dtypep();
                    स्थिर स्वतः pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    अगर (pair.second) {
                        varp = नया AstVar{fl, VVarType::MODULETEMP, m_व्यंजक_अस्थायी_Names.लो(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        अगर (m_ftaskp) {
                            varp->funcLocal(true);
                            m_ftaskp->prependStmtsp(varp);
                        } अन्यथा {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(नया Ast_व्यंजक_Stmt{
                            fl, नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            नया AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = नया AstVarRef{fl, varp, VAccess::READ};
                } अन्यथा {
                    covExprp = term.m_exprp->clone_वृक्ष(false);
                }
                Ast_गाँठ_व्यंजक* स्थिर termp
                    = term.m_objective ? covExprp : नया AstLogNot{fl, covExprp};
                अगर (condp) {
                    condp = नया AstLogAnd{fl, condp, termp};
                } अन्यथा {
                    condp = termp;
                }
                first = false;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* स्थिर declp = नया AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_गाँठ* स्थिर newp = newCoverInc(fl, declp, trace_Name_For_पंक्ति(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* स्थिर ifp = नया AstIf{fl, condp, newp, nullptr};
            ifp->user2(true);
            जोड़ो_Proc_Statement(m_व्यंजक_Stmtsp, ifp);
            ++गिनती;
        }
    }

    व्योम coverExprs(Ast_गाँठ_व्यंजक* nodep) {
        अगर (!m_state.व्यंजक_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_व्यंजक_Stmtsp) {
            वापस;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = false;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = true;
        iterate(nodep);
        अगर (checkMaxExprs(falseExprs.आकार())) वापस;
        अगर (m_seeking == ABORTED) वापस;

        add_व्यंजक_Cover_Inc(nodep);
        स्थिर पूर्णांक start = m_exprs.आकार();
        m_objective = false;
        m_exprs.swap(falseExprs);
        add_व्यंजक_Cover_Inc(nodep, start);
    }

    व्योम व्यंजक_Either(Ast_गाँठ_Biop* nodep, bool overrideObjective = false, bool lObjective = false,
                    bool rObjective = false) {
        VL_RESTORER(m_objective);
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp();

        अगर (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        अगर (checkMaxExprs()) वापस;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        अगर (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    व्योम व्यंजक_Both(Ast_गाँठ_Biop* nodep, bool overrideObjective = false, bool lObjective = false,
                  bool rObjective = false) {
        VL_RESTORER(m_objective);
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp();

        अगर (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        अगर (checkMaxExprs()) वापस;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        अगर (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        अगर (checkMaxExprs()) वापस;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        क्रम (स्थिर Cover_व्यंजक& l : lhsExprs) {
            क्रम (स्थिर Cover_व्यंजक& r : rhsExprs) {
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
                क्रम (स्थिर CoverTerm& term : l) {
                    अगर (स्थिर AstVarRef* स्थिर refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].जोड़ो(refp->varp());
                    } अन्यथा {
                        strs[term.m_objective].जोड़ो(term.m_emitV);
                    }
                    व्यंजक.धकेलो_back(term);
                }
                bool impossible = false;
                क्रम (स्थिर CoverTerm& term : r) {
                    bool redundant = false;
                    अगर (स्थिर Ast_गाँठ_Var_Ref* स्थिर refp = VN_CAST(term.m_exprp, गाँठ_Var_Ref)) {
                        अगर (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = true;
                        अगर (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = true;
                    } अन्यथा {
                        अगर (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = true;
                        अगर (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = true;
                    }
                    अगर (!redundant) व्यंजक.धकेलो_back(term);
                }
                अगर (!impossible) m_exprs.धकेलो_back(std::move(व्यंजक));
                अगर (checkMaxExprs()) वापस;
            }
        }
    }

    व्योम or_व्यंजक(Ast_गाँठ_Biop* nodep) {
        अगर (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा अगर (m_objective) {
            व्यंजक_Either(nodep);
        } अन्यथा {
            व्यंजक_Both(nodep);
        }
        पंक्ति_Track(nodep);
    }
    व्योम visit(AstLogOr* nodep) override { or_व्यंजक(nodep); }
    व्योम visit(AstOr* nodep) override { or_व्यंजक(nodep); }

    व्योम and_व्यंजक(Ast_गाँठ_Biop* nodep) {
        अगर (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा अगर (m_objective) {
            व्यंजक_Both(nodep);
        } अन्यथा {
            व्यंजक_Either(nodep);
        }
        पंक्ति_Track(nodep);
    }
    व्योम visit(AstLogAnd* nodep) override { and_व्यंजक(nodep); }
    व्योम visit(AstAnd* nodep) override { and_व्यंजक(nodep); }

    व्योम xor_व्यंजक(Ast_गाँठ_Biop* nodep) {
        अगर (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            क्रम (स्थिर bool lObjective : {false, true}) {
                CoverExprs पूर्व_Exprs;
                m_exprs.swap(पूर्व_Exprs);
                स्थिर bool rObjective = lObjective ^ m_objective;
                व्यंजक_Both(nodep, true, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), पूर्व_Exprs);
                अगर (checkMaxExprs()) अवरोध;
            }
        }
        पंक्ति_Track(nodep);
    }
    व्योम visit(AstXor* nodep) override { xor_व्यंजक(nodep); }

    व्योम व्यंजक_Not(Ast_गाँठ_व्यंजक* nodep) {
        VL_RESTORER(m_objective);
        अगर (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            m_objective = !m_objective;
            iterateChildren(nodep);
            पंक्ति_Track(nodep);
        }
    }
    व्योम visit(AstNot* nodep) override { व्यंजक_Not(nodep); }
    व्योम visit(AstLogNot* nodep) override { व्यंजक_Not(nodep); }

    ढाँचा <typename T_Oper>
    व्योम व्यंजक_Reduce(Ast_गाँठ_Uniop* nodep) {
        अगर (m_seeking != ABORTED) {
            File_पंक्ति* स्थिर fl = nodep->fileline();
            Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp();
            स्थिर पूर्णांक width = lhsp->dtypep()->width();
            स्थिर माप_प्रकार expected = std::is_same<T_Oper, AstXor>::मूल्य ? 0x1 << width : width + 1;
            अगर (checkMaxExprs(expected)) वापस;
            Ast_गाँठ_व्यंजक* unrolledp = नया AstSel{
                fl, lhsp->clone_वृक्ष(false), नया AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            क्रम (पूर्णांक bit = width - 2; bit >= 0; bit--) {
                AstSel* स्थिर selp = नया AstSel{fl, lhsp->clone_वृक्ष(false),
                                                नया AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = नया T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(धकेलो_Deletep(unrolledp), unrolledp);
        } अन्यथा {
            iterateChildren(nodep);
            पंक्ति_Track(nodep);
        }
    }
    व्योम visit(AstRedOr* nodep) override { व्यंजक_Reduce<AstOr>(nodep); }
    व्योम visit(AstRedAnd* nodep) override { व्यंजक_Reduce<AstAnd>(nodep); }
    व्योम visit(AstRedXor* nodep) override { व्यंजक_Reduce<AstXor>(nodep); }

    व्योम visit(AstLogIf* nodep) override {
        अगर (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा अगर (m_objective) {
            व्यंजक_Either(nodep, true, false, true);
        } अन्यथा {
            व्यंजक_Both(nodep, true, true, false);
        }
        पंक्ति_Track(nodep);
    }

    व्योम visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        अगर (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            m_objective = !m_objective;
            xor_व्यंजक(nodep);
            पंक्ति_Track(nodep);
        }
    }

    व्योम visit(AstFuncRef* nodep) override {
        अगर (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_गाँठ_व्यंजक*>(nodep));
        } अन्यथा {
            व्यंजक_Unsupported(nodep, "non-automatic function");
        }
    }

    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {
        अगर (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } अन्यथा {
            व्यंजक_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            अगर (elgibleVisitor.eligible()) {
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
    व्योम visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = false;
        m_inToggleOff = true;
        iterateChildren(nodep);
    }
    व्योम visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = false;
        m_inToggleOff = true;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        iterateChildren(nodep);
        पंक्ति_Track(nodep);
    }

    व्योम व्यंजक_Unsupported(Ast_गाँठ* nodep, स्थिर string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        स्थिर bool wasSeeking = m_seeking == SEEKING;
        स्थिर Objective oldSeeking = m_seeking;
        अगर (wasSeeking) abort_व्यंजक_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        पंक्ति_Track(nodep);
        अगर (!wasSeeking) m_seeking = oldSeeking;
    }

खुला:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_व्यंजक_अस्थायी_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = शेष;
};

//######################################################################
// Coverage class functions

व्योम V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("coverage", 0, dump_वृक्ष_Either_Level() >= 3);
}
