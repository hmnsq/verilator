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

صنف عبارت_Coverage_Eligible_Visitor final : عام VNVisitorConst {
    // STATE
    منطقي m_eligible = درست;

    ساكن منطقي elemDTypeEligible(ثابت Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت Ast_گره_DType* ثابت dtp = dtypep->virtRefDTypep()) {
            إذا (!elemDTypeEligible(dtp)) أرجع خطأ;
        }
        إذا (ثابت Ast_گره_DType* ثابت dtp = dtypep->virtRefDType2p()) {
            إذا (!elemDTypeEligible(dtp)) أرجع خطأ;
        }
        أرجع !VN_IS(dtypep, ClassRefDType);
    }

    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        ثابت Ast_گره_DType* ثابت dtypep = nodep->varp()->dtypep();
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

    فراغ بازدید(Ast_گره* nodep) override {
        إذا (!nodep->is_عبارت_Coverage_Eligible()) {
            m_eligible = خطأ;
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }

عام:
    // CONSTRUCTORS
    صریح عبارت_Coverage_Eligible_Visitor(Ast_گره* nodep) { iterateConst(nodep); }
    ~عبارت_Coverage_Eligible_Visitor() override = افتراضي;

    منطقي eligible() ثابت { أرجع m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

صنف CoverageVisitor final : عام VNVisitor {
    // TYPES
    استفاده Lineno_مجموعه = std::مجموعه<صحيح>;

    بنية Cover_جمله final {
        Ast_گره_عبارت* m_exprp;  // Expression branch term
        منطقي m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        Cover_جمله(Ast_گره_عبارت* exprp, منطقي objective, ثابت string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    استفاده Cover_عبارت = std::deque<Cover_جمله>;
    استفاده CoverExprs = std::list<Cover_عبارت>;

    بنية ToggleEnt final {
        ثابت string m_comment;  // Comment for coverage dump
        Ast_گره_عبارت* m_varRefp;  // How to get to this element
        Ast_گره_عبارت* m_chgRefp;  // How to get to this element
        ToggleEnt(ثابت string& comment, Ast_گره_عبارت* vp, Ast_گره_عبارت* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = افتراضي;
        فراغ cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_درخت(), m_varRefp = اشاره_تهی);
            VL_DO_CLEAR(m_chgRefp->delete_درخت(), m_chgRefp = اشاره_تهی);
        }
    };

    بنية CheckState final {  // State save-restored on each new coverage scope/block
        منطقي m_on = خطأ;  // Should this block get covered?
        منطقي m_درون_Mod_Off = خطأ;  // In module with no coverage
        صحيح m_handle = 0;  // Opaque handle for index into line tracking
        ثابت Ast_گره* m_nodep = اشاره_تهی;  // Node establishing this state
        CheckState() = افتراضي;
        منطقي coverageOn(ثابت Ast_گره* nodep) ثابت {
            أرجع m_on && !m_درون_Mod_Off && nodep->fileline()->coverageOn();
        }
        منطقي سطر_Coverage_On(ثابت Ast_گره* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverage_سطر();
        }
        منطقي branchCoverageOn(ثابت Ast_گره* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        منطقي عبارت_Coverage_On(ثابت Ast_گره* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverage_عبارت();
        }
    };

    تعداد Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;
    V3_Unique_نامها m_عبارت_موقت_نامها;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_گره_عبارت*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    صحيح m_بعدی_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module to add statement to
    Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Current function/task
    Ast_گره* m_عبارت_Stmtsp = اشاره_تهی;  // Node to add expr coverage to
    منطقي m_then = خطأ;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    منطقي m_objective = خطأ;  // Expression objective
    منطقي m_ifCond = خطأ;  // Visiting if condition
    منطقي m_درون_Toggle_Off = خطأ;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<صحيح, Lineno_مجموعه> m_handle_سطرها;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ثابت حرف* varIgnoreToggle(ثابت AstVar* nodep) {
        ثابت منطقي cover = nodep->isIO() || (nodep->isSignal() && nodep->is_بیت_Logic());
        إذا (!cover) أرجع "Not relevant signal";
        إذا (nodep->isConst()) أرجع "Signal is constant";
        إذا (nodep->isDouble()) أرجع "Signal is double";
        إذا (nodep->isString()) أرجع "Signal is string";
        إذا (!v3Global.opt.coverageUnderscore()) {
            ثابت string prettyName = nodep->prettyName();
            إذا (prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.پیدا_کن("._") != string::npos) أرجع "Inlined leading underscore";
        }
        إذا ((nodep->عرض() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverage_Max_عرض())) {
            أرجع "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        أرجع اشاره_تهی;
    }

    AstCoverInc* newCoverInc(File_سطر* fl, Ast_گره_Cover_Decl* ثابت declp,
                             ثابت string& trace_var_name) {
        AstCoverInc* ثابت incp = جديد AstCoverInc{fl, declp};
        إذا (!trace_var_name.خالی()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_سطر* ثابت fl_nowarn = جديد File_سطر{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3_Error_کد::UNUSEDSIGNAL, درست);
            AstVar* ثابت varp = جديد AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->پیدا_کن_UInt32_DType()};
            varp->مجموعه_Ignore_Sched_Write();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(درست);
            m_modp->جمع_کن_Stmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            Ast_تخصیص_بده* ثابت assp = جديد Ast_تخصیص_بده{
                incp->fileline(), جديد AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                جديد Ast_جمع_کن{incp->fileline(), جديد AstVarRef{incp->fileline(), varp, VAccess::READ},
                           جديد AstConst{incp->fileline(), AstConst::Widthed_مقدار{}, 32, 1}}};
            Ast_گره::جمع_کن_بعدی<Ast_گره, Ast_گره>(incp, assp);
        }
        أرجع incp;
    }
    string trace_Name_For_سطر(ثابت Ast_گره* nodep, ثابت string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvt_مقصد_Str(nodep->fileline()->lineno()) + "_" + type;
        إذا (ثابت uint32_t suffix = m_varnames[name]++) name += "_" + cvt_مقصد_Str(suffix);
        أرجع name;
    }

    // Line tracking
    فراغ createHandle(ثابت Ast_گره* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = درست;
        m_state.m_handle = ++m_بعدی_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    فراغ سطر_Track(ثابت Ast_گره* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        إذا ((m_state.سطر_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            لكل (صحيح lineno = nodep->fileline()->اولی_Lineno();
                 lineno <= nodep->fileline()->آخرین_Lineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_سطرها[m_state.m_handle].درج(lineno);
            }
        }
    }
    ساكن string سطرها_اولی_آخرین(ثابت صحيح اولی, ثابت صحيح آخرین) {
        إذا (اولی && اولی == آخرین) {
            أرجع cvt_مقصد_Str(اولی);
        } وإلا إذا (اولی && آخرین) {
            أرجع cvt_مقصد_Str(اولی) + "-" + cvt_مقصد_Str(آخرین);
        } وإلا {
            أرجع "";
        }
    }
    string سطرها_Cov(ثابت CheckState& state, ثابت Ast_گره* nodep) {
        // Return comma separated list of ranged numbers
        string بیرون;
        ثابت Lineno_مجموعه& سطرها = m_handle_سطرها[state.m_handle];
        صحيح اولی = 0;
        صحيح آخرین = 0;
        لكل (ثابت صحيح linen : سطرها) {
            إذا (!اولی) {
                اولی = آخرین = linen;
            } وإلا إذا (linen == آخرین + 1) {
                ++آخرین;
            } وإلا {
                إذا (!بیرون.خالی()) بیرون += ",";
                بیرون += سطرها_اولی_آخرین(اولی, آخرین);
                اولی = آخرین = linen;
            }
        }
        إذا (اولی) {
            إذا (!بیرون.خالی()) بیرون += ",";
            بیرون += سطرها_اولی_آخرین(اولی, آخرین);
        }
        UINFO(9, "lines out " << بیرون << " for h" << state.m_handle << " " << nodep);
        أرجع بیرون;
    }

    // VISITORS - BOTH
    فراغ بازدید(Ast_گره_Module* nodep) override {
        ثابت Ast_گره_Module* ثابت origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_عبارت_موقت_نامها);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_درون_Mod_Off = nodep->is_بالاترین();  // Already made top shell, no coverage for it
        إذا (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_سطرها.clear();
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_عبارت_موقت_نامها);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_درون_Mod_Off = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    فراغ بازدید(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_تخصیص_بده) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_عبارت_Stmtsp);
            VL_RESTORER(m_درون_Toggle_Off);
            m_عبارت_Stmtsp = nodep;
            m_درون_Toggle_Off = درست;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            أرجع;
        }
        iterateProcedure(nodep);
    }
    فراغ بازدید(Ast_گره_Procedure* nodep) override { iterateProcedure(nodep); }
    فراغ بازدید(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_عبارت_Stmtsp);
        VL_RESTORER(m_درون_Toggle_Off);
        m_عبارت_Stmtsp = nodep;
        m_درون_Toggle_Off = درست;
        createHandle(nodep);
        iterate_And_بعدی_Null(nodep->stmtsp());
        إذا (m_state.سطر_Coverage_On(nodep)) {
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطرها_Cov(m_state, nodep), 0};
            m_modp->جمع_کن_Stmtsp(declp);
            Ast_گره* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            درج_Proc_Statement(nodep, newp);
        }
    }
    فراغ بازدید(AstLoopTest* nodep) override {
        إذا (nodep->user2_مجموعه_Once()) أرجع;
        سطر_Track(nodep);
        إذا (m_state.سطر_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطرها_Cov(m_state, nodep), 0};
            m_modp->جمع_کن_Stmtsp(declp);
            Ast_گره* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            nodep->جمع_کن_اینجا_This_As_بعدی(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_عبارت_موقت_نامها);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        إذا (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    فراغ درج_Proc_Statement(Ast_گره* nodep, Ast_گره* stmtp) {
        إذا (Ast_گره_Procedure* ثابت itemp = VN_CAST(nodep, گره_Procedure)) {
            itemp->جمع_کن_Stmtsp(stmtp);
        } وإلا إذا (Ast_گره_FTask* ثابت itemp = VN_CAST(nodep, گره_FTask)) {
            itemp->جمع_کن_Stmtsp(stmtp);
        } وإلا إذا (AstLoop* ثابت itemp = VN_CAST(nodep, Loop)) {
            itemp->جمع_کن_Stmtsp(stmtp);
        } وإلا إذا (AstIf* ثابت itemp = VN_CAST(nodep, If)) {
            إذا (m_then) {
                itemp->جمع_کن_Thensp(stmtp);
            } وإلا {
                itemp->جمع_کن_Elsesp(stmtp);
            }
        } وإلا إذا (AstBegin* ثابت itemp = VN_CAST(nodep, Begin)) {
            itemp->جمع_کن_Stmtsp(stmtp);
        } وإلا {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    فراغ iterateProcedure(Ast_گره* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_عبارت_Stmtsp);
        VL_RESTORER(m_درون_Toggle_Off);
        m_عبارت_Stmtsp = nodep;
        m_درون_Toggle_Off = درست;
        createHandle(nodep);
        iterateChildren(nodep);
        إذا (m_state.سطر_Coverage_On(nodep)) {
            سطر_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطرها_Cov(m_state, nodep), 0};
            m_modp->جمع_کن_Stmtsp(declp);
            Ast_گره* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            درج_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    فراغ بازدید(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_modp && !m_درون_Toggle_Off && !m_state.m_درون_Mod_Off && nodep->fileline()->coverageOn()
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
                File_سطر* ثابت fl_nowarn = جديد File_سطر{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3_Error_کد::UNUSEDSIGNAL, درست);
                AstVar* ثابت chgVarp
                    = جديد AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->جمع_کن_Stmtsp(chgVarp);

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
        ثابت std::string hier_پیشوند
            = (m_beginHier != "") ? Ast_گره::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ثابت declp
            = جديد AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hier_پیشوند + varp->name() + above.m_comment, range};
        m_modp->جمع_کن_Stmtsp(declp);
        AstCoverToggle* ثابت newp = جديد AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_درخت(خطأ), above.m_chgRefp->clone_درخت(خطأ)};
        m_modp->جمع_کن_Stmtsp(newp);
    }

    فراغ toggleVarRecurse(ثابت Ast_گره_DType* ثابت dtypep, ثابت صحيح عمق,  // per-iteration
                          ثابت ToggleEnt& above, ثابت AstVar* ثابت varp) {  // Constant
        إذا (ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            لكل (ثابت صحيح شاخص_docs : adtypep->declRange().seq_Lo_مقصد_Hi()) {
                ثابت صحيح شاخص_کد = شاخص_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_مقصد_Str(شاخص_docs) + "]",
                                 جديد AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_درخت(خطأ), شاخص_کد},
                                 جديد AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_درخت(خطأ), شاخص_کد}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), عمق + 1, newent, varp);
                newent.cleanup();
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(dtypep, PackArrayDType)) {
            لكل (ثابت صحيح شاخص_docs : adtypep->declRange().seq_Lo_مقصد_Hi()) {
                ثابت Ast_گره_DType* ثابت subtypep = adtypep->subDTypep()->skipRefp();
                ثابت صحيح شاخص_کد = شاخص_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_مقصد_Str(شاخص_docs) + "]",
                                 جديد AstSel{varp->fileline(), above.m_varRefp->clone_درخت(خطأ),
                                            شاخص_کد * subtypep->عرض(), subtypep->عرض()},
                                 جديد AstSel{varp->fileline(), above.m_chgRefp->clone_درخت(خطأ),
                                            شاخص_کد * subtypep->عرض(), subtypep->عرض()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), عمق + 1, newent, varp);
                newent.cleanup();
            }
        } وإلا إذا (ثابت AstStructDType* ثابت adtypep = VN_CAST(dtypep, StructDType)) {
            إذا (adtypep->packed()) {
                لكل (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    ثابت صحيح شاخص_کد = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        جديد AstSel{varp->fileline(), above.m_varRefp->clone_درخت(خطأ), شاخص_کد,
                                   subtypep->عرض()},
                        جديد AstSel{varp->fileline(), above.m_chgRefp->clone_درخت(خطأ), شاخص_کد,
                                   subtypep->عرض()}};
                    toggleVarRecurse(subtypep, عمق + 1, newent, varp);
                    newent.cleanup();
                }
            } وإلا {
                لكل (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    Ast_گره_عبارت* ثابت varRefp = جديد AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_درخت(خطأ), itemp->name()};
                    Ast_گره_عبارت* ثابت chgRefp = جديد AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_درخت(خطأ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, عمق + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وإلا إذا (ثابت AstUnionDType* ثابت adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            إذا (ثابت AstMemberDType* ثابت itemp = adtypep->membersp()) {
                Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                إذا (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_درخت(خطأ),
                                     above.m_chgRefp->clone_درخت(خطأ)};
                    toggleVarRecurse(subtypep, عمق + 1, newent, varp);
                    newent.cleanup();
                } وإلا {
                    Ast_گره_عبارت* ثابت varRefp = جديد AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_درخت(خطأ), itemp->name()};
                    Ast_گره_عبارت* ثابت chgRefp = جديد AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_درخت(خطأ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, عمق + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وإلا إذا (VN_IS(dtypep, صف_DType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } وإلا {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    منطقي include_Cond_مقصد_Branch_Recursive(ثابت Ast_گره* ثابت nodep) {
        ثابت Ast_گره* ثابت backp = nodep->backp();
        إذا (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            أرجع include_Cond_مقصد_Branch_Recursive(backp);
        } وإلا إذا (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            أرجع include_Cond_مقصد_Branch_Recursive(backp);
        } وإلا إذا (VN_IS(backp, گره_تخصیص_بده) && VN_AS(backp, گره_تخصیص_بده)->rhsp() == nodep) {
            أرجع درست;
        }
        أرجع خطأ;
    }

    // VISITORS - LINE COVERAGE
    فراغ بازدید(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        إذا (m_seeking == NONE) coverExprs(nodep->condp());

        إذا (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && include_Cond_مقصد_Branch_Recursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ثابت CheckState آخرین_State = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            سطر_Track(nodep);
            Ast_گره_عبارت* ثابت thenp = nodep->thenp()->unlink_Fr_پشت();
            AstCoverOtherDecl* ثابت thenDeclp
                = جديد AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", سطرها_Cov(m_state, nodep), 0};
            m_modp->جمع_کن_Stmtsp(thenDeclp);
            nodep->thenp(جديد Ast_عبارت_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_سطر(nodep, "cond_then")),
                thenp});
            m_state = آخرین_State;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_گره_عبارت* ثابت elsep = nodep->elsep()->unlink_Fr_پشت();
            AstCoverOtherDecl* ثابت elseDeclp
                = جديد AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", سطرها_Cov(m_state, nodep), 1};
            m_modp->جمع_کن_Stmtsp(elseDeclp);
            nodep->elsep(جديد Ast_عبارت_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_سطر(nodep, "cond_else")),
                elsep});

            m_state = آخرین_State;
        } وإلا {
            سطر_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    فراغ بازدید(AstIf* nodep) override {
        إذا (nodep->user2()) أرجع;

        UINFO(4, " IF: " << nodep);
        إذا (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ثابت منطقي elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            إذا (elsif) VN_AS(nodep->elsesp(), If)->user1(درست);
            ثابت منطقي اولی_elsif = !nodep->user1() && elsif;
            ثابت منطقي cont_elsif = nodep->user1() && elsif;
            ثابت منطقي final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ثابت CheckState آخرین_State = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_عبارت_Stmtsp);
                VL_RESTORER(m_then);
                m_عبارت_Stmtsp = nodep;
                m_then = درست;
                createHandle(nodep);
                iterate_And_بعدی_Null(nodep->thensp());
                سطر_Track(nodep);
                ifState = m_state;
            }
            m_state = آخرین_State;
            {
                VL_RESTORER(m_عبارت_Stmtsp);
                VL_RESTORER(m_then);
                m_عبارت_Stmtsp = nodep;
                m_then = خطأ;
                createHandle(nodep);
                iterate_And_بعدی_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = آخرین_State;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            إذا (!(اولی_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ثابت ifDeclp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", سطرها_Cov(ifState, nodep), 0};
                m_modp->جمع_کن_Stmtsp(ifDeclp);
                nodep->جمع_کن_Thensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_سطر(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ثابت elseDeclp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", سطرها_Cov(elseState, nodep), 1};
                m_modp->جمع_کن_Stmtsp(elseDeclp);
                nodep->جمع_کن_Elsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_سطر(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            وإلا إذا (اولی_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                إذا (ifState.سطر_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ثابت elsifDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        سطرها_Cov(ifState, nodep), 0};
                    m_modp->جمع_کن_Stmtsp(elsifDeclp);

                    nodep->جمع_کن_Thensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_سطر(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } وإلا {
                // Cover as separate blocks (not a branch as is not two-legged)
                إذا (ifState.سطر_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ثابت ifDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        سطرها_Cov(ifState, nodep), 0};
                    m_modp->جمع_کن_Stmtsp(ifDeclp);
                    nodep->جمع_کن_Thensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_سطر(nodep, "if")));
                }
                إذا (elseState.سطر_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ثابت elseDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        سطرها_Cov(elseState, nodep), 1};
                    m_modp->جمع_کن_Stmtsp(elseDeclp);

                    nodep->جمع_کن_Elsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_سطر(nodep, "else")));
                }
            }
            m_state = آخرین_State;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = درست;
        iterate_And_بعدی_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    فراغ بازدید(Ast_Case_قلم* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        إذا (m_state.سطر_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_بعدی_Null(nodep->stmtsp());
            إذا (m_state.سطر_Coverage_On(nodep)) {  // if the case body didn't disable it
                سطر_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ثابت declp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", سطرها_Cov(m_state, nodep), 0};
                m_modp->جمع_کن_Stmtsp(declp);
                nodep->جمع_کن_Stmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "case")));
            }
        }
    }
    فراغ بازدید(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = درست;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        إذا (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            سطر_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", سطرها_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->جمع_کن_Stmtsp(declp);
            nodep->جمع_کن_Coverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    فراغ بازدید(AstPropSpec* nodep) override {
        VL_RESTORER(m_عبارت_Stmtsp);
        m_عبارت_Stmtsp = اشاره_تهی;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = خطأ;
    }
    فراغ بازدید(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::COVERAGE_بلوک_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = خطأ;
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
        } وإلا {
            إذا (m_state.m_on) iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    فراغ بازدید(Ast_Gen_بلوک* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        إذا (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    فراغ بازدید(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_درون_Toggle_Off);
        VL_RESTORER(m_عبارت_Stmtsp);
        m_عبارت_Stmtsp = nodep;
        m_درون_Toggle_Off = درست;
        إذا (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    فراغ abort_عبارت_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        إذا (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    منطقي checkMaxExprs(اندازه_t additional = 0) {
        إذا (m_seeking != ABORTED
            && static_cast<صحيح>(m_exprs.اندازه() + additional) <= v3Global.opt.coverage_عبارت_Max())
            أرجع خطأ;
        abort_عبارت_Coverage();
        أرجع درست;
    }

    فراغ جمع_کن_عبارت_Cover_Inc(Ast_گره_عبارت* nodep, صحيح شروع = 0) {
        File_سطر* ثابت fl = nodep->fileline();
        صحيح شمار = شروع;
        لكل (Cover_عبارت& عبارت : m_exprs) {
            ثابت string name = "expr_" + std::مقصد_string(شمار);
            string comment = "(";
            منطقي اولی = درست;
            Ast_گره_عبارت* condp = اشاره_تهی;
            لكل (Cover_جمله& جمله : عبارت) {
                comment += (اولی ? "" : " && ") + جمله.m_emitV
                           + "==" + (جمله.m_objective ? "1" : "0");
                Ast_گره_عبارت* covExprp = اشاره_تهی;
                إذا (VN_IS(جمله.m_exprp, FuncRef) || جمله.m_exprp->isSystemFunc()) {
                    Ast_گره_عبارت* ثابت frefp = جمله.m_exprp;
                    Ast_گره_DType* ثابت dtypep = frefp->dtypep();
                    ثابت خودکار pair = m_funcTemps.emplace(frefp, اشاره_تهی);
                    AstVar* varp = pair.اولی->دومی;
                    إذا (pair.دومی) {
                        varp = جديد AstVar{fl, VVarType::MODULETEMP, m_عبارت_موقت_نامها.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.اولی->دومی = varp;
                        إذا (m_ftaskp) {
                            varp->funcLocal(درست);
                            m_ftaskp->prependStmtsp(varp);
                        } وإلا {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlink_Fr_پشت(&relinkHandle);
                        relinkHandle.relink(جديد Ast_عبارت_Stmt{
                            fl, جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            جديد AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = جديد AstVarRef{fl, varp, VAccess::READ};
                } وإلا {
                    covExprp = جمله.m_exprp->clone_درخت(خطأ);
                }
                Ast_گره_عبارت* ثابت termp
                    = جمله.m_objective ? covExprp : جديد AstLogNot{fl, covExprp};
                إذا (condp) {
                    condp = جديد AstLogAnd{fl, condp, termp};
                } وإلا {
                    condp = termp;
                }
                اولی = خطأ;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ثابت declp = جديد AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->جمع_کن_Stmtsp(declp);
            Ast_گره* ثابت newp = newCoverInc(fl, declp, trace_Name_For_سطر(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ثابت ifp = جديد AstIf{fl, condp, newp, اشاره_تهی};
            ifp->user2(درست);
            درج_Proc_Statement(m_عبارت_Stmtsp, ifp);
            ++شمار;
        }
    }

    فراغ coverExprs(Ast_گره_عبارت* nodep) {
        إذا (!m_state.عبارت_Coverage_On(nodep) || nodep->dtypep()->عرض() != 1 || !m_عبارت_Stmtsp) {
            أرجع;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.خالی(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = خطأ;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.جابجا(falseExprs);

        m_objective = درست;
        iterate(nodep);
        إذا (checkMaxExprs(falseExprs.اندازه())) أرجع;
        إذا (m_seeking == ABORTED) أرجع;

        جمع_کن_عبارت_Cover_Inc(nodep);
        ثابت صحيح شروع = m_exprs.اندازه();
        m_objective = خطأ;
        m_exprs.جابجا(falseExprs);
        جمع_کن_عبارت_Cover_Inc(nodep, شروع);
    }

    فراغ عبارت_Either(Ast_گره_Biop* nodep, منطقي overrideObjective = خطأ, منطقي lObjective = خطأ,
                    منطقي rObjective = خطأ) {
        VL_RESTORER(m_objective);
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();

        إذا (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs lhsExprs;
        m_exprs.جابجا(lhsExprs);
        إذا (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.انتها(), lhsExprs);
        checkMaxExprs();
    }

    فراغ عبارت_Both(Ast_گره_Biop* nodep, منطقي overrideObjective = خطأ, منطقي lObjective = خطأ,
                  منطقي rObjective = خطأ) {
        VL_RESTORER(m_objective);
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();

        إذا (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs lhsExprs;
        m_exprs.جابجا(lhsExprs);
        إذا (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs rhsExprs;
        m_exprs.جابجا(rhsExprs);

        لكل (ثابت Cover_عبارت& l : lhsExprs) {
            لكل (ثابت Cover_عبارت& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::مجموعه<AstVar*>, 2> varps;
                std::array<std::مجموعه<std::string>, 2> strs;

                UASSERT_OBJ(!l.خالی() && !r.خالی(), nodep, "Empty coverage expression branch");
                Cover_عبارت عبارت;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                لكل (ثابت Cover_جمله& جمله : l) {
                    إذا (ثابت AstVarRef* ثابت refp = VN_CAST(جمله.m_exprp, VarRef)) {
                        varps[جمله.m_objective].درج(refp->varp());
                    } وإلا {
                        strs[جمله.m_objective].درج(جمله.m_emitV);
                    }
                    عبارت.افزودن_پشت(جمله);
                }
                منطقي impossible = خطأ;
                لكل (ثابت Cover_جمله& جمله : r) {
                    منطقي redundant = خطأ;
                    إذا (ثابت Ast_گره_Var_Ref* ثابت refp = VN_CAST(جمله.m_exprp, گره_Var_Ref)) {
                        إذا (varps[جمله.m_objective].پیدا_کن(refp->varp())
                            != varps[جمله.m_objective].انتها())
                            redundant = درست;
                        إذا (varps[!جمله.m_objective].پیدا_کن(refp->varp())
                            != varps[!جمله.m_objective].انتها())
                            impossible = درست;
                    } وإلا {
                        إذا (strs[جمله.m_objective].پیدا_کن(جمله.m_emitV)
                            != strs[جمله.m_objective].انتها())
                            redundant = درست;
                        إذا (strs[!جمله.m_objective].پیدا_کن(جمله.m_emitV)
                            != strs[!جمله.m_objective].انتها())
                            impossible = درست;
                    }
                    إذا (!redundant) عبارت.افزودن_پشت(جمله);
                }
                إذا (!impossible) m_exprs.افزودن_پشت(std::move(عبارت));
                إذا (checkMaxExprs()) أرجع;
            }
        }
    }

    فراغ or_عبارت(Ast_گره_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            عبارت_Either(nodep);
        } وإلا {
            عبارت_Both(nodep);
        }
        سطر_Track(nodep);
    }
    فراغ بازدید(AstLogOr* nodep) override { or_عبارت(nodep); }
    فراغ بازدید(AstOr* nodep) override { or_عبارت(nodep); }

    فراغ and_عبارت(Ast_گره_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            عبارت_Both(nodep);
        } وإلا {
            عبارت_Either(nodep);
        }
        سطر_Track(nodep);
    }
    فراغ بازدید(AstLogAnd* nodep) override { and_عبارت(nodep); }
    فراغ بازدید(AstAnd* nodep) override { and_عبارت(nodep); }

    فراغ xor_عبارت(Ast_گره_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            لكل (ثابت منطقي lObjective : {خطأ, درست}) {
                CoverExprs prevExprs;
                m_exprs.جابجا(prevExprs);
                ثابت منطقي rObjective = lObjective ^ m_objective;
                عبارت_Both(nodep, درست, lObjective, rObjective);
                m_exprs.splice(m_exprs.انتها(), prevExprs);
                إذا (checkMaxExprs()) اكسر;
            }
        }
        سطر_Track(nodep);
    }
    فراغ بازدید(AstXor* nodep) override { xor_عبارت(nodep); }

    فراغ عبارت_Not(Ast_گره_عبارت* nodep) {
        VL_RESTORER(m_objective);
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            m_objective = !m_objective;
            iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    فراغ بازدید(AstNot* nodep) override { عبارت_Not(nodep); }
    فراغ بازدید(AstLogNot* nodep) override { عبارت_Not(nodep); }

    قالب <نام_نوع T_Oper>
    فراغ عبارت_Reduce(Ast_گره_Uniop* nodep) {
        إذا (m_seeking != ABORTED) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
            ثابت صحيح عرض = lhsp->dtypep()->عرض();
            ثابت اندازه_t expected = std::is_same<T_Oper, AstXor>::مقدار ? 0x1 << عرض : عرض + 1;
            إذا (checkMaxExprs(expected)) أرجع;
            Ast_گره_عبارت* unrolledp = جديد AstSel{
                fl, lhsp->clone_درخت(خطأ), جديد AstConst{fl, static_cast<uint32_t>(عرض - 1)}, 1};
            لكل (صحيح بیت_2 = عرض - 2; بیت_2 >= 0; بیت_2--) {
                AstSel* ثابت selp = جديد AstSel{fl, lhsp->clone_درخت(خطأ),
                                                جديد AstConst{fl, static_cast<uint32_t>(بیت_2)}, 1};
                unrolledp = جديد T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(افزودن_Deletep(unrolledp), unrolledp);
        } وإلا {
            iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    فراغ بازدید(AstRedOr* nodep) override { عبارت_Reduce<AstOr>(nodep); }
    فراغ بازدید(AstRedAnd* nodep) override { عبارت_Reduce<AstAnd>(nodep); }
    فراغ بازدید(AstRedXor* nodep) override { عبارت_Reduce<AstXor>(nodep); }

    فراغ بازدید(AstLogIf* nodep) override {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            عبارت_Either(nodep, درست, خطأ, درست);
        } وإلا {
            عبارت_Both(nodep, درست, درست, خطأ);
        }
        سطر_Track(nodep);
    }

    فراغ بازدید(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            m_objective = !m_objective;
            xor_عبارت(nodep);
            سطر_Track(nodep);
        }
    }

    فراغ بازدید(AstFuncRef* nodep) override {
        إذا (nodep->taskp()->lifetime().isAutomatic()) {
            بازدید(static_cast<Ast_گره_عبارت*>(nodep));
        } وإلا {
            عبارت_Unsupported(nodep, "non-automatic function");
        }
    }

    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        إذا (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } وإلا {
            عبارت_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            إذا (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_درخت(nodep, emitV);
                // Add new expression with a single term
                Cover_عبارت عبارت;
                عبارت.emplace_پشت(nodep, m_objective, emitV.str());
                m_exprs.افزودن_پشت(std::move(عبارت));
                checkMaxExprs();
            } وإلا {
                عبارت_Unsupported(nodep, "not coverage eligible");
            }
        }
        سطر_Track(nodep);
    }

    // VISITORS - BOTH
    فراغ بازدید(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_درون_Toggle_Off);
        m_state.m_on = خطأ;
        m_درون_Toggle_Off = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_درون_Toggle_Off);
        m_state.m_on = خطأ;
        m_درون_Toggle_Off = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override {
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    فراغ عبارت_Unsupported(Ast_گره* nodep, ثابت string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ثابت منطقي wasSeeking = m_seeking == SEEKING;
        ثابت Objective oldSeeking = m_seeking;
        إذا (wasSeeking) abort_عبارت_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        سطر_Track(nodep);
        إذا (!wasSeeking) m_seeking = oldSeeking;
    }

عام:
    // CONSTRUCTORS
    صریح CoverageVisitor(AstNetlist* rootp)
        : m_عبارت_موقت_نامها{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = افتراضي;
};

//######################################################################
// Coverage class functions

فراغ V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("coverage", 0, dump_درخت_Either_Level() >= 3);
}
