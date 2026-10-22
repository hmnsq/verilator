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
    तार्किक m_eligible = सत्य;

    स्थिर तार्किक elemDTypeEligible(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtp = dtypep->virtRefDTypep()) {
            यदि (!elemDTypeEligible(dtp)) प्रत्यागम असत्य;
        }
        यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtp = dtypep->virtRefDType2p()) {
            यदि (!elemDTypeEligible(dtp)) प्रत्यागम असत्य;
        }
        प्रत्यागम !VN_IS(dtypep, ClassRefDType);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->varp()->dtypep();
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

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (!nodep->is_व्यञ्जकम्_Coverage_Eligible()) {
            m_eligible = असत्य;
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट व्यञ्जकम्_Coverage_Eligible_Visitor(Ast_ग्रन्थिः* nodep) { iterateConst(nodep); }
    ~व्यञ्जकम्_Coverage_Eligible_Visitor() override = स्वभाव;

    तार्किक eligible() ध्रुव { प्रत्यागम m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

वर्ग CoverageVisitor final : सार्वजनिक VNVisitor {
    // TYPES
    उपयुज्य Lineno_समुच्चयः = std::समुच्चयः<पूर्णाङ्क>;

    संरचना Cover_पदांशः final {
        Ast_ग्रन्थिः_व्यञ्जकम्* m_exprp;  // Expression branch term
        तार्किक m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        Cover_पदांशः(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, तार्किक objective, ध्रुव string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    उपयुज्य Cover_व्यञ्जकम् = std::deque<Cover_पदांशः>;
    उपयुज्य CoverExprs = std::list<Cover_व्यञ्जकम्>;

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
            VL_DO_CLEAR(m_varRefp->delete_वृक्षः(), m_varRefp = शून्यसूचकः);
            VL_DO_CLEAR(m_chgRefp->delete_वृक्षः(), m_chgRefp = शून्यसूचकः);
        }
    };

    संरचना CheckState final {  // State save-restored on each new coverage scope/block
        तार्किक m_on = असत्य;  // Should this block get covered?
        तार्किक m_अन्तर्_Mod_Off = असत्य;  // In module with no coverage
        पूर्णाङ्क m_handle = 0;  // Opaque handle for index into line tracking
        ध्रुव Ast_ग्रन्थिः* m_nodep = शून्यसूचकः;  // Node establishing this state
        CheckState() = स्वभाव;
        तार्किक coverageOn(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {
            प्रत्यागम m_on && !m_अन्तर्_Mod_Off && nodep->fileline()->coverageOn();
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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;
    V3_Unique_नामानि m_व्यञ्जकम्_अस्थायी_नामानि;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_ग्रन्थिः_व्यञ्जकम्*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    पूर्णाङ्क m_अग्र_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module to add statement to
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current function/task
    Ast_ग्रन्थिः* m_व्यञ्जकम्_Stmtsp = शून्यसूचकः;  // Node to add expr coverage to
    तार्किक m_then = असत्य;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    तार्किक m_objective = असत्य;  // Expression objective
    तार्किक m_ifCond = असत्य;  // Visiting if condition
    तार्किक m_अन्तर्_Toggle_Off = असत्य;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<पूर्णाङ्क, Lineno_समुच्चयः> m_handle_पङ्क्तयः;  // Line numbers for given m_stateHandle

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
        यदि ((nodep->विस्तारः() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverage_Max_विस्तारः())) {
            प्रत्यागम "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        प्रत्यागम शून्यसूचकः;
    }

    AstCoverInc* newCoverInc(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_Cover_Decl* ध्रुव declp,
                             ध्रुव string& trace_var_name) {
        AstCoverInc* ध्रुव incp = नूतन AstCoverInc{fl, declp};
        यदि (!trace_var_name.रिक्तानि()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_पङ्क्तिः* ध्रुव fl_nowarn = नूतन File_पङ्क्तिः{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3_Error_संहिता::UNUSEDSIGNAL, सत्य);
            AstVar* ध्रुव varp = नूतन AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->समुच्चयः_Ignore_Sched_Write();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(सत्य);
            m_modp->सङ्कलय_Stmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            Ast_नियोजय* ध्रुव assp = नूतन Ast_नियोजय{
                incp->fileline(), नूतन AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                नूतन Ast_सङ्कलय{incp->fileline(), नूतन AstVarRef{incp->fileline(), varp, VAccess::READ},
                           नूतन AstConst{incp->fileline(), AstConst::Widthed_मानम्{}, 32, 1}}};
            Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(incp, assp);
        }
        प्रत्यागम incp;
    }
    string trace_Name_For_पङ्क्तिः(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()) + "_" + type;
        यदि (ध्रुव uint32_t suffix = m_varnames[name]++) name += "_" + cvt_गन्तव्यम्_Str(suffix);
        प्रत्यागम name;
    }

    // Line tracking
    शून्य createHandle(ध्रुव Ast_ग्रन्थिः* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = सत्य;
        m_state.m_handle = ++m_अग्र_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    शून्य पङ्क्तिः_Track(ध्रुव Ast_ग्रन्थिः* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        यदि ((m_state.पङ्क्तिः_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            प्रति (पूर्णाङ्क lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->चरमः_Lineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_पङ्क्तयः[m_state.m_handle].निवेशय(lineno);
            }
        }
    }
    स्थिर string पङ्क्तयः_First_चरमः(ध्रुव पूर्णाङ्क first, ध्रुव पूर्णाङ्क चरमः) {
        यदि (first && first == चरमः) {
            प्रत्यागम cvt_गन्तव्यम्_Str(first);
        } अन्यथा यदि (first && चरमः) {
            प्रत्यागम cvt_गन्तव्यम्_Str(first) + "-" + cvt_गन्तव्यम्_Str(चरमः);
        } अन्यथा {
            प्रत्यागम "";
        }
    }
    string पङ्क्तयः_Cov(ध्रुव CheckState& state, ध्रुव Ast_ग्रन्थिः* nodep) {
        // Return comma separated list of ranged numbers
        string बहिर्गमनम्;
        ध्रुव Lineno_समुच्चयः& पङ्क्तयः = m_handle_पङ्क्तयः[state.m_handle];
        पूर्णाङ्क first = 0;
        पूर्णाङ्क चरमः = 0;
        प्रति (ध्रुव पूर्णाङ्क linen : पङ्क्तयः) {
            यदि (!first) {
                first = चरमः = linen;
            } अन्यथा यदि (linen == चरमः + 1) {
                ++चरमः;
            } अन्यथा {
                यदि (!बहिर्गमनम्.रिक्तानि()) बहिर्गमनम् += ",";
                बहिर्गमनम् += पङ्क्तयः_First_चरमः(first, चरमः);
                first = चरमः = linen;
            }
        }
        यदि (first) {
            यदि (!बहिर्गमनम्.रिक्तानि()) बहिर्गमनम् += ",";
            बहिर्गमनम् += पङ्क्तयः_First_चरमः(first, चरमः);
        }
        UINFO(9, "lines out " << बहिर्गमनम् << " for h" << state.m_handle << " " << nodep);
        प्रत्यागम बहिर्गमनम्;
    }

    // VISITORS - BOTH
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_व्यञ्जकम्_अस्थायी_नामानि);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_अन्तर्_Mod_Off = nodep->is_शिखरम्();  // Already made top shell, no coverage for it
        यदि (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_पङ्क्तयः.clear();
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_व्यञ्जकम्_अस्थायी_नामानि);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_अन्तर्_Mod_Off = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstAlways* nodep) override {
        यदि (nodep->keyword() == VAlwaysKwd::CONT_नियोजय) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
            VL_RESTORER(m_अन्तर्_Toggle_Off);
            m_व्यञ्जकम्_Stmtsp = nodep;
            m_अन्तर्_Toggle_Off = सत्य;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            प्रत्यागम;
        }
        iterateProcedure(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override { iterateProcedure(nodep); }
    शून्य दर्शय(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
        VL_RESTORER(m_अन्तर्_Toggle_Off);
        m_व्यञ्जकम्_Stmtsp = nodep;
        m_अन्तर्_Toggle_Off = सत्य;
        createHandle(nodep);
        iterate_And_अग्र_Null(nodep->stmtsp());
        यदि (m_state.पङ्क्तिः_Coverage_On(nodep)) {
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पङ्क्तयः_Cov(m_state, nodep), 0};
            m_modp->सङ्कलय_Stmtsp(declp);
            Ast_ग्रन्थिः* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पङ्क्तिः(nodep, "block"));
            निवेशय_Proc_Statement(nodep, newp);
        }
    }
    शून्य दर्शय(AstLoopTest* nodep) override {
        यदि (nodep->user2_समुच्चयः_Once()) प्रत्यागम;
        पङ्क्तिः_Track(nodep);
        यदि (m_state.पङ्क्तिः_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पङ्क्तयः_Cov(m_state, nodep), 0};
            m_modp->सङ्कलय_Stmtsp(declp);
            Ast_ग्रन्थिः* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पङ्क्तिः(nodep, "block"));
            nodep->सङ्कलय_अत्र_This_As_अग्र(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_व्यञ्जकम्_अस्थायी_नामानि);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        यदि (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    शून्य निवेशय_Proc_Statement(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* stmtp) {
        यदि (Ast_ग्रन्थिः_Procedure* ध्रुव itemp = VN_CAST(nodep, ग्रन्थिः_Procedure)) {
            itemp->सङ्कलय_Stmtsp(stmtp);
        } अन्यथा यदि (Ast_ग्रन्थिः_FTask* ध्रुव itemp = VN_CAST(nodep, ग्रन्थिः_FTask)) {
            itemp->सङ्कलय_Stmtsp(stmtp);
        } अन्यथा यदि (AstLoop* ध्रुव itemp = VN_CAST(nodep, Loop)) {
            itemp->सङ्कलय_Stmtsp(stmtp);
        } अन्यथा यदि (AstIf* ध्रुव itemp = VN_CAST(nodep, If)) {
            यदि (m_then) {
                itemp->सङ्कलय_Thensp(stmtp);
            } अन्यथा {
                itemp->सङ्कलय_Elsesp(stmtp);
            }
        } अन्यथा यदि (AstBegin* ध्रुव itemp = VN_CAST(nodep, Begin)) {
            itemp->सङ्कलय_Stmtsp(stmtp);
        } अन्यथा {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    शून्य iterateProcedure(Ast_ग्रन्थिः* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
        VL_RESTORER(m_अन्तर्_Toggle_Off);
        m_व्यञ्जकम्_Stmtsp = nodep;
        m_अन्तर्_Toggle_Off = सत्य;
        createHandle(nodep);
        iterateChildren(nodep);
        यदि (m_state.पङ्क्तिः_Coverage_On(nodep)) {
            पङ्क्तिः_Track(nodep);
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", पङ्क्तयः_Cov(m_state, nodep), 0};
            m_modp->सङ्कलय_Stmtsp(declp);
            Ast_ग्रन्थिः* ध्रुव newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_पङ्क्तिः(nodep, "block"));
            निवेशय_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    शून्य दर्शय(AstVar* nodep) override {
        iterateChildren(nodep);
        यदि (m_modp && !m_अन्तर्_Toggle_Off && !m_state.m_अन्तर्_Mod_Off && nodep->fileline()->coverageOn()
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
                File_पङ्क्तिः* ध्रुव fl_nowarn = नूतन File_पङ्क्तिः{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3_Error_संहिता::UNUSEDSIGNAL, सत्य);
                AstVar* ध्रुव chgVarp
                    = नूतन AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->सङ्कलय_Stmtsp(chgVarp);

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
        ध्रुव std::string hier_उपसर्गः
            = (m_beginHier != "") ? Ast_ग्रन्थिः::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ध्रुव declp
            = नूतन AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hier_उपसर्गः + varp->name() + above.m_comment, range};
        m_modp->सङ्कलय_Stmtsp(declp);
        AstCoverToggle* ध्रुव newp = नूतन AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_वृक्षः(असत्य), above.m_chgRefp->clone_वृक्षः(असत्य)};
        m_modp->सङ्कलय_Stmtsp(newp);
    }

    शून्य toggleVarRecurse(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep, ध्रुव पूर्णाङ्क गभीरता,  // per-iteration
                          ध्रुव ToggleEnt& above, ध्रुव AstVar* ध्रुव varp) {  // Constant
        यदि (ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } अन्यथा यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            प्रति (ध्रुव पूर्णाङ्क सूचकांक_docs : adtypep->declRange().seq_Lo_गन्तव्यम्_Hi()) {
                ध्रुव पूर्णाङ्क सूचकांक_संहिता = सूचकांक_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_गन्तव्यम्_Str(सूचकांक_docs) + "]",
                                 नूतन AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_वृक्षः(असत्य), सूचकांक_संहिता},
                                 नूतन AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_वृक्षः(असत्य), सूचकांक_संहिता}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), गभीरता + 1, newent, varp);
                newent.cleanup();
            }
        } अन्यथा यदि (ध्रुव AstPackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, PackArrayDType)) {
            प्रति (ध्रुव पूर्णाङ्क सूचकांक_docs : adtypep->declRange().seq_Lo_गन्तव्यम्_Hi()) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subtypep = adtypep->subDTypep()->skipRefp();
                ध्रुव पूर्णाङ्क सूचकांक_संहिता = सूचकांक_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_गन्तव्यम्_Str(सूचकांक_docs) + "]",
                                 नूतन AstSel{varp->fileline(), above.m_varRefp->clone_वृक्षः(असत्य),
                                            सूचकांक_संहिता * subtypep->विस्तारः(), subtypep->विस्तारः()},
                                 नूतन AstSel{varp->fileline(), above.m_chgRefp->clone_वृक्षः(असत्य),
                                            सूचकांक_संहिता * subtypep->विस्तारः(), subtypep->विस्तारः()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), गभीरता + 1, newent, varp);
                newent.cleanup();
            }
        } अन्यथा यदि (ध्रुव AstStructDType* ध्रुव adtypep = VN_CAST(dtypep, StructDType)) {
            यदि (adtypep->packed()) {
                प्रति (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                    ध्रुव पूर्णाङ्क सूचकांक_संहिता = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        नूतन AstSel{varp->fileline(), above.m_varRefp->clone_वृक्षः(असत्य), सूचकांक_संहिता,
                                   subtypep->विस्तारः()},
                        नूतन AstSel{varp->fileline(), above.m_chgRefp->clone_वृक्षः(असत्य), सूचकांक_संहिता,
                                   subtypep->विस्तारः()}};
                    toggleVarRecurse(subtypep, गभीरता + 1, newent, varp);
                    newent.cleanup();
                }
            } अन्यथा {
                प्रति (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_वृक्षः(असत्य), itemp->name()};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव chgRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_वृक्षः(असत्य), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, गभीरता + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } अन्यथा यदि (ध्रुव AstUnionDType* ध्रुव adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            यदि (ध्रुव AstMemberDType* ध्रुव itemp = adtypep->membersp()) {
                Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skipRefp();
                यदि (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_वृक्षः(असत्य),
                                     above.m_chgRefp->clone_वृक्षः(असत्य)};
                    toggleVarRecurse(subtypep, गभीरता + 1, newent, varp);
                    newent.cleanup();
                } अन्यथा {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_वृक्षः(असत्य), itemp->name()};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव chgRefp = नूतन AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_वृक्षः(असत्य), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, गभीरता + 1, newent, varp);
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
    तार्किक include_Cond_गन्तव्यम्_Branch_Recursive(ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
        ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();
        यदि (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            प्रत्यागम include_Cond_गन्तव्यम्_Branch_Recursive(backp);
        } अन्यथा यदि (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            प्रत्यागम include_Cond_गन्तव्यम्_Branch_Recursive(backp);
        } अन्यथा यदि (VN_IS(backp, ग्रन्थिः_नियोजय) && VN_AS(backp, ग्रन्थिः_नियोजय)->rhsp() == nodep) {
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // VISITORS - LINE COVERAGE
    शून्य दर्शय(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        यदि (m_seeking == NONE) coverExprs(nodep->condp());

        यदि (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && include_Cond_गन्तव्यम्_Branch_Recursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ध्रुव CheckState चरमः_State = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            पङ्क्तिः_Track(nodep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenp = nodep->thenp()->unlink_Fr_पृष्ठभागः();
            AstCoverOtherDecl* ध्रुव thenDeclp
                = नूतन AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", पङ्क्तयः_Cov(m_state, nodep), 0};
            m_modp->सङ्कलय_Stmtsp(thenDeclp);
            nodep->thenp(नूतन Ast_व्यञ्जकम्_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_पङ्क्तिः(nodep, "cond_then")),
                thenp});
            m_state = चरमः_State;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elsep = nodep->elsep()->unlink_Fr_पृष्ठभागः();
            AstCoverOtherDecl* ध्रुव elseDeclp
                = नूतन AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", पङ्क्तयः_Cov(m_state, nodep), 1};
            m_modp->सङ्कलय_Stmtsp(elseDeclp);
            nodep->elsep(नूतन Ast_व्यञ्जकम्_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_पङ्क्तिः(nodep, "cond_else")),
                elsep});

            m_state = चरमः_State;
        } अन्यथा {
            पङ्क्तिः_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    शून्य दर्शय(AstIf* nodep) override {
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
            ध्रुव CheckState चरमः_State = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
                VL_RESTORER(m_then);
                m_व्यञ्जकम्_Stmtsp = nodep;
                m_then = सत्य;
                createHandle(nodep);
                iterate_And_अग्र_Null(nodep->thensp());
                पङ्क्तिः_Track(nodep);
                ifState = m_state;
            }
            m_state = चरमः_State;
            {
                VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
                VL_RESTORER(m_then);
                m_व्यञ्जकम्_Stmtsp = nodep;
                m_then = असत्य;
                createHandle(nodep);
                iterate_And_अग्र_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = चरमः_State;
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
                                            "if", पङ्क्तयः_Cov(ifState, nodep), 0};
                m_modp->सङ्कलय_Stmtsp(ifDeclp);
                nodep->सङ्कलय_Thensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_पङ्क्तिः(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ध्रुव elseDeclp
                    = नूतन AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", पङ्क्तयः_Cov(elseState, nodep), 1};
                m_modp->सङ्कलय_Stmtsp(elseDeclp);
                nodep->सङ्कलय_Elsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_पङ्क्तिः(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            अन्यथा यदि (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                यदि (ifState.पङ्क्तिः_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ध्रुव elsifDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        पङ्क्तयः_Cov(ifState, nodep), 0};
                    m_modp->सङ्कलय_Stmtsp(elsifDeclp);

                    nodep->सङ्कलय_Thensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_पङ्क्तिः(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } अन्यथा {
                // Cover as separate blocks (not a branch as is not two-legged)
                यदि (ifState.पङ्क्तिः_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ध्रुव ifDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        पङ्क्तयः_Cov(ifState, nodep), 0};
                    m_modp->सङ्कलय_Stmtsp(ifDeclp);
                    nodep->सङ्कलय_Thensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_पङ्क्तिः(nodep, "if")));
                }
                यदि (elseState.पङ्क्तिः_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ध्रुव elseDeclp = नूतन AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        पङ्क्तयः_Cov(elseState, nodep), 1};
                    m_modp->सङ्कलय_Stmtsp(elseDeclp);

                    nodep->सङ्कलय_Elsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_पङ्क्तिः(nodep, "else")));
                }
            }
            m_state = चरमः_State;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = सत्य;
        iterate_And_अग्र_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    शून्य दर्शय(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        यदि (m_state.पङ्क्तिः_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_अग्र_Null(nodep->stmtsp());
            यदि (m_state.पङ्क्तिः_Coverage_On(nodep)) {  // if the case body didn't disable it
                पङ्क्तिः_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ध्रुव declp
                    = नूतन AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", पङ्क्तयः_Cov(m_state, nodep), 0};
                m_modp->सङ्कलय_Stmtsp(declp);
                nodep->सङ्कलय_Stmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_पङ्क्तिः(nodep, "case")));
            }
        }
    }
    शून्य दर्शय(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = सत्य;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        यदि (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            पङ्क्तिः_Track(nodep);
            AstCoverOtherDecl* ध्रुव declp
                = नूतन AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", पङ्क्तयः_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->सङ्कलय_Stmtsp(declp);
            nodep->सङ्कलय_Coverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    शून्य दर्शय(AstPropSpec* nodep) override {
        VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
        m_व्यञ्जकम्_Stmtsp = शून्यसूचकः;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = असत्य;
    }
    शून्य दर्शय(AstStmtPragma* nodep) override {
        यदि (nodep->pragp()->pragType() == VPragmaType::COVERAGE_खण्डम्_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = असत्य;
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा {
            यदि (m_state.m_on) iterateChildren(nodep);
            पङ्क्तिः_Track(nodep);
        }
    }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        यदि (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        पङ्क्तिः_Track(nodep);
    }

    शून्य दर्शय(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_अन्तर्_Toggle_Off);
        VL_RESTORER(m_व्यञ्जकम्_Stmtsp);
        m_व्यञ्जकम्_Stmtsp = nodep;
        m_अन्तर्_Toggle_Off = सत्य;
        यदि (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        पङ्क्तिः_Track(nodep);
    }

    शून्य abort_व्यञ्जकम्_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        यदि (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    तार्किक checkMaxExprs(आकार_t additional = 0) {
        यदि (m_seeking != ABORTED
            && static_cast<पूर्णाङ्क>(m_exprs.आकार() + additional) <= v3Global.opt.coverage_व्यञ्जकम्_Max())
            प्रत्यागम असत्य;
        abort_व्यञ्जकम्_Coverage();
        प्रत्यागम सत्य;
    }

    शून्य सङ्कलय_व्यञ्जकम्_Cover_Inc(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क start = 0) {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        पूर्णाङ्क गणनम् = start;
        प्रति (Cover_व्यञ्जकम्& व्यञ्जकम् : m_exprs) {
            ध्रुव string name = "expr_" + std::गन्तव्यम्_string(गणनम्);
            string comment = "(";
            तार्किक first = सत्य;
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः;
            प्रति (Cover_पदांशः& पदांशः : व्यञ्जकम्) {
                comment += (first ? "" : " && ") + पदांशः.m_emitV
                           + "==" + (पदांशः.m_objective ? "1" : "0");
                Ast_ग्रन्थिः_व्यञ्जकम्* covExprp = शून्यसूचकः;
                यदि (VN_IS(पदांशः.m_exprp, FuncRef) || पदांशः.m_exprp->isSystemFunc()) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव frefp = पदांशः.m_exprp;
                    Ast_ग्रन्थिः_DType* ध्रुव dtypep = frefp->dtypep();
                    ध्रुव स्वचालित pair = m_funcTemps.emplace(frefp, शून्यसूचकः);
                    AstVar* varp = pair.first->second;
                    यदि (pair.second) {
                        varp = नूतन AstVar{fl, VVarType::MODULETEMP, m_व्यञ्जकम्_अस्थायी_नामानि.get(frefp),
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
                        frefp->unlink_Fr_पृष्ठभागः(&relinkHandle);
                        relinkHandle.relink(नूतन Ast_व्यञ्जकम्_Stmt{
                            fl, नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            नूतन AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = नूतन AstVarRef{fl, varp, VAccess::READ};
                } अन्यथा {
                    covExprp = पदांशः.m_exprp->clone_वृक्षः(असत्य);
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp
                    = पदांशः.m_objective ? covExprp : नूतन AstLogNot{fl, covExprp};
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
            m_modp->सङ्कलय_Stmtsp(declp);
            Ast_ग्रन्थिः* ध्रुव newp = newCoverInc(fl, declp, trace_Name_For_पङ्क्तिः(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ध्रुव ifp = नूतन AstIf{fl, condp, newp, शून्यसूचकः};
            ifp->user2(सत्य);
            निवेशय_Proc_Statement(m_व्यञ्जकम्_Stmtsp, ifp);
            ++गणनम्;
        }
    }

    शून्य coverExprs(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (!m_state.व्यञ्जकम्_Coverage_On(nodep) || nodep->dtypep()->विस्तारः() != 1 || !m_व्यञ्जकम्_Stmtsp) {
            प्रत्यागम;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.रिक्तानि(), nodep, "unexpected expression coverage garbage");
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
        यदि (checkMaxExprs(falseExprs.आकार())) प्रत्यागम;
        यदि (m_seeking == ABORTED) प्रत्यागम;

        सङ्कलय_व्यञ्जकम्_Cover_Inc(nodep);
        ध्रुव पूर्णाङ्क start = m_exprs.आकार();
        m_objective = असत्य;
        m_exprs.swap(falseExprs);
        सङ्कलय_व्यञ्जकम्_Cover_Inc(nodep, start);
    }

    शून्य व्यञ्जकम्_Either(Ast_ग्रन्थिः_Biop* nodep, तार्किक overrideObjective = असत्य, तार्किक lObjective = असत्य,
                    तार्किक rObjective = असत्य) {
        VL_RESTORER(m_objective);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();

        यदि (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        यदि (checkMaxExprs()) प्रत्यागम;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        यदि (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.अन्तः_2(), lhsExprs);
        checkMaxExprs();
    }

    शून्य व्यञ्जकम्_Both(Ast_ग्रन्थिः_Biop* nodep, तार्किक overrideObjective = असत्य, तार्किक lObjective = असत्य,
                  तार्किक rObjective = असत्य) {
        VL_RESTORER(m_objective);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();

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

        प्रति (ध्रुव Cover_व्यञ्जकम्& l : lhsExprs) {
            प्रति (ध्रुव Cover_व्यञ्जकम्& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::समुच्चयः<AstVar*>, 2> varps;
                std::array<std::समुच्चयः<std::string>, 2> strs;

                UASSERT_OBJ(!l.रिक्तानि() && !r.रिक्तानि(), nodep, "Empty coverage expression branch");
                Cover_व्यञ्जकम् व्यञ्जकम्;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                प्रति (ध्रुव Cover_पदांशः& पदांशः : l) {
                    यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(पदांशः.m_exprp, VarRef)) {
                        varps[पदांशः.m_objective].निवेशय(refp->varp());
                    } अन्यथा {
                        strs[पदांशः.m_objective].निवेशय(पदांशः.m_emitV);
                    }
                    व्यञ्जकम्.प्रक्षिप_पृष्ठभागः(पदांशः);
                }
                तार्किक impossible = असत्य;
                प्रति (ध्रुव Cover_पदांशः& पदांशः : r) {
                    तार्किक redundant = असत्य;
                    यदि (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(पदांशः.m_exprp, ग्रन्थिः_Var_Ref)) {
                        यदि (varps[पदांशः.m_objective].find(refp->varp())
                            != varps[पदांशः.m_objective].अन्तः_2())
                            redundant = सत्य;
                        यदि (varps[!पदांशः.m_objective].find(refp->varp())
                            != varps[!पदांशः.m_objective].अन्तः_2())
                            impossible = सत्य;
                    } अन्यथा {
                        यदि (strs[पदांशः.m_objective].find(पदांशः.m_emitV)
                            != strs[पदांशः.m_objective].अन्तः_2())
                            redundant = सत्य;
                        यदि (strs[!पदांशः.m_objective].find(पदांशः.m_emitV)
                            != strs[!पदांशः.m_objective].अन्तः_2())
                            impossible = सत्य;
                    }
                    यदि (!redundant) व्यञ्जकम्.प्रक्षिप_पृष्ठभागः(पदांशः);
                }
                यदि (!impossible) m_exprs.प्रक्षिप_पृष्ठभागः(std::move(व्यञ्जकम्));
                यदि (checkMaxExprs()) प्रत्यागम;
            }
        }
    }

    शून्य or_व्यञ्जकम्(Ast_ग्रन्थिः_Biop* nodep) {
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा यदि (m_objective) {
            व्यञ्जकम्_Either(nodep);
        } अन्यथा {
            व्यञ्जकम्_Both(nodep);
        }
        पङ्क्तिः_Track(nodep);
    }
    शून्य दर्शय(AstLogOr* nodep) override { or_व्यञ्जकम्(nodep); }
    शून्य दर्शय(AstOr* nodep) override { or_व्यञ्जकम्(nodep); }

    शून्य and_व्यञ्जकम्(Ast_ग्रन्थिः_Biop* nodep) {
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा यदि (m_objective) {
            व्यञ्जकम्_Both(nodep);
        } अन्यथा {
            व्यञ्जकम्_Either(nodep);
        }
        पङ्क्तिः_Track(nodep);
    }
    शून्य दर्शय(AstLogAnd* nodep) override { and_व्यञ्जकम्(nodep); }
    शून्य दर्शय(AstAnd* nodep) override { and_व्यञ्जकम्(nodep); }

    शून्य xor_व्यञ्जकम्(Ast_ग्रन्थिः_Biop* nodep) {
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            प्रति (ध्रुव तार्किक lObjective : {असत्य, सत्य}) {
                CoverExprs पूर्व_Exprs;
                m_exprs.swap(पूर्व_Exprs);
                ध्रुव तार्किक rObjective = lObjective ^ m_objective;
                व्यञ्जकम्_Both(nodep, सत्य, lObjective, rObjective);
                m_exprs.splice(m_exprs.अन्तः_2(), पूर्व_Exprs);
                यदि (checkMaxExprs()) भङ्ग;
            }
        }
        पङ्क्तिः_Track(nodep);
    }
    शून्य दर्शय(AstXor* nodep) override { xor_व्यञ्जकम्(nodep); }

    शून्य व्यञ्जकम्_Not(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        VL_RESTORER(m_objective);
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            m_objective = !m_objective;
            iterateChildren(nodep);
            पङ्क्तिः_Track(nodep);
        }
    }
    शून्य दर्शय(AstNot* nodep) override { व्यञ्जकम्_Not(nodep); }
    शून्य दर्शय(AstLogNot* nodep) override { व्यञ्जकम्_Not(nodep); }

    प्रतिरूपम् <प्रकारनाम T_Oper>
    शून्य व्यञ्जकम्_Reduce(Ast_ग्रन्थिः_Uniop* nodep) {
        यदि (m_seeking != ABORTED) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
            ध्रुव पूर्णाङ्क विस्तारः = lhsp->dtypep()->विस्तारः();
            ध्रुव आकार_t expected = std::is_same<T_Oper, AstXor>::मानम् ? 0x1 << विस्तारः : विस्तारः + 1;
            यदि (checkMaxExprs(expected)) प्रत्यागम;
            Ast_ग्रन्थिः_व्यञ्जकम्* unrolledp = नूतन AstSel{
                fl, lhsp->clone_वृक्षः(असत्य), नूतन AstConst{fl, static_cast<uint32_t>(विस्तारः - 1)}, 1};
            प्रति (पूर्णाङ्क bit = विस्तारः - 2; bit >= 0; bit--) {
                AstSel* ध्रुव selp = नूतन AstSel{fl, lhsp->clone_वृक्षः(असत्य),
                                                नूतन AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = नूतन T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(unrolledp), unrolledp);
        } अन्यथा {
            iterateChildren(nodep);
            पङ्क्तिः_Track(nodep);
        }
    }
    शून्य दर्शय(AstRedOr* nodep) override { व्यञ्जकम्_Reduce<AstOr>(nodep); }
    शून्य दर्शय(AstRedAnd* nodep) override { व्यञ्जकम्_Reduce<AstAnd>(nodep); }
    शून्य दर्शय(AstRedXor* nodep) override { व्यञ्जकम्_Reduce<AstXor>(nodep); }

    शून्य दर्शय(AstLogIf* nodep) override {
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा यदि (m_objective) {
            व्यञ्जकम्_Either(nodep, सत्य, असत्य, सत्य);
        } अन्यथा {
            व्यञ्जकम्_Both(nodep, सत्य, सत्य, असत्य);
        }
        पङ्क्तिः_Track(nodep);
    }

    शून्य दर्शय(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        यदि (m_seeking == NONE) {
            coverExprs(nodep);
        } अन्यथा {
            m_objective = !m_objective;
            xor_व्यञ्जकम्(nodep);
            पङ्क्तिः_Track(nodep);
        }
    }

    शून्य दर्शय(AstFuncRef* nodep) override {
        यदि (nodep->taskp()->lifetime().isAutomatic()) {
            दर्शय(static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(nodep));
        } अन्यथा {
            व्यञ्जकम्_Unsupported(nodep, "non-automatic function");
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        यदि (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } अन्यथा {
            व्यञ्जकम्_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            यदि (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_वृक्षः(nodep, emitV);
                // Add new expression with a single term
                Cover_व्यञ्जकम् व्यञ्जकम्;
                व्यञ्जकम्.emplace_पृष्ठभागः(nodep, m_objective, emitV.str());
                m_exprs.प्रक्षिप_पृष्ठभागः(std::move(व्यञ्जकम्));
                checkMaxExprs();
            } अन्यथा {
                व्यञ्जकम्_Unsupported(nodep, "not coverage eligible");
            }
        }
        पङ्क्तिः_Track(nodep);
    }

    // VISITORS - BOTH
    शून्य दर्शय(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_अन्तर्_Toggle_Off);
        m_state.m_on = असत्य;
        m_अन्तर्_Toggle_Off = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_अन्तर्_Toggle_Off);
        m_state.m_on = असत्य;
        m_अन्तर्_Toggle_Off = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        iterateChildren(nodep);
        पङ्क्तिः_Track(nodep);
    }

    शून्य व्यञ्जकम्_Unsupported(Ast_ग्रन्थिः* nodep, ध्रुव string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ध्रुव तार्किक wasSeeking = m_seeking == SEEKING;
        ध्रुव Objective oldSeeking = m_seeking;
        यदि (wasSeeking) abort_व्यञ्जकम्_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        पङ्क्तिः_Track(nodep);
        यदि (!wasSeeking) m_seeking = oldSeeking;
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट CoverageVisitor(AstNetlist* rootp)
        : m_व्यञ्जकम्_अस्थायी_नामानि{"__VExpr"} {
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
