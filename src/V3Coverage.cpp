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

صنف اظهار_Coverage_Eligible_Visitor final : عام VNVisitorConst {
    // STATE
    منطقي m_eligible = سچ;

    ساكن منطقي elemDTypeEligible(ثابت Ast_ڳنڍ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت Ast_ڳنڍ_DType* ثابت dtp = dtypep->virtRefDTypep()) {
            إذا (!elemDTypeEligible(dtp)) أرجع خطأ;
        }
        إذا (ثابت Ast_ڳنڍ_DType* ثابت dtp = dtypep->virtRefDType2p()) {
            إذا (!elemDTypeEligible(dtp)) أرجع خطأ;
        }
        أرجع !VN_IS(dtypep, ClassRefDType);
    }

    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = nodep->varp()->dtypep();
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

    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (!nodep->is_اظهار_Coverage_Eligible()) {
            m_eligible = خطأ;
        } وإلا {
            iterateChildrenConst(nodep);
        }
    }

عام:
    // CONSTRUCTORS
    واضح اظهار_Coverage_Eligible_Visitor(Ast_ڳنڍ* nodep) { iterateConst(nodep); }
    ~اظهار_Coverage_Eligible_Visitor() override = افتراضي;

    منطقي eligible() ثابت { أرجع m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

صنف CoverageVisitor final : عام VNVisitor {
    // TYPES
    استعمال Lineno_سيٽ = std::سيٽ<صحيح>;

    بنية Cover_رڪن final {
        Ast_ڳنڍ_اظهار* m_exprp;  // Expression branch term
        منطقي m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        Cover_رڪن(Ast_ڳنڍ_اظهار* exprp, منطقي objective, ثابت string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    استعمال Cover_اظهار = std::deque<Cover_رڪن>;
    استعمال CoverExprs = std::list<Cover_اظهار>;

    بنية ToggleEnt final {
        ثابت string m_comment;  // Comment for coverage dump
        Ast_ڳنڍ_اظهار* m_varRefp;  // How to get to this element
        Ast_ڳنڍ_اظهار* m_chgRefp;  // How to get to this element
        ToggleEnt(ثابت string& comment, Ast_ڳنڍ_اظهار* vp, Ast_ڳنڍ_اظهار* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = افتراضي;
        فراغ cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_وڻ(), m_varRefp = خالي_اشارو);
            VL_DO_CLEAR(m_chgRefp->delete_وڻ(), m_chgRefp = خالي_اشارو);
        }
    };

    بنية CheckState final {  // State save-restored on each new coverage scope/block
        منطقي m_on = خطأ;  // Should this block get covered?
        منطقي m_اندريون_Mod_Off = خطأ;  // In module with no coverage
        صحيح m_handle = 0;  // Opaque handle for index into line tracking
        ثابت Ast_ڳنڍ* m_nodep = خالي_اشارو;  // Node establishing this state
        CheckState() = افتراضي;
        منطقي coverageOn(ثابت Ast_ڳنڍ* nodep) ثابت {
            أرجع m_on && !m_اندريون_Mod_Off && nodep->fileline()->coverageOn();
        }
        منطقي سٽ_Coverage_On(ثابت Ast_ڳنڍ* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverage_سٽ();
        }
        منطقي branchCoverageOn(ثابت Ast_ڳنڍ* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        منطقي اظهار_Coverage_On(ثابت Ast_ڳنڍ* nodep) ثابت {
            أرجع coverageOn(nodep) && v3Global.opt.coverage_اظهار();
        }
    };

    تعداد Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;
    V3_Unique_نالا m_اظهار_عارضي_نالا;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_ڳنڍ_اظهار*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    صحيح m_ايندڙ_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module to add statement to
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Current function/task
    Ast_ڳنڍ* m_اظهار_Stmtsp = خالي_اشارو;  // Node to add expr coverage to
    منطقي m_then = خطأ;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    منطقي m_objective = خطأ;  // Expression objective
    منطقي m_ifCond = خطأ;  // Visiting if condition
    منطقي m_اندريون_Toggle_Off = خطأ;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<صحيح, Lineno_سيٽ> m_handle_سٽون;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ثابت حرف* varIgnoreToggle(ثابت AstVar* nodep) {
        ثابت منطقي cover = nodep->isIO() || (nodep->isSignal() && nodep->is_ٻٽو_انگ_Logic());
        إذا (!cover) أرجع "Not relevant signal";
        إذا (nodep->isConst()) أرجع "Signal is constant";
        إذا (nodep->isDouble()) أرجع "Signal is double";
        إذا (nodep->isString()) أرجع "Signal is string";
        إذا (!v3Global.opt.coverageUnderscore()) {
            ثابت string prettyName = nodep->prettyName();
            إذا (prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.ڳوليو("._") != string::npos) أرجع "Inlined leading underscore";
        }
        إذا ((nodep->ويڪر() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverage_Max_ويڪر())) {
            أرجع "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        أرجع خالي_اشارو;
    }

    AstCoverInc* newCoverInc(File_سٽ* fl, Ast_ڳنڍ_Cover_Decl* ثابت declp,
                             ثابت string& trace_var_name) {
        AstCoverInc* ثابت incp = جديد AstCoverInc{fl, declp};
        إذا (!trace_var_name.خالي_جايون()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_سٽ* ثابت fl_nowarn = جديد File_سٽ{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3_Error_ڪوڊ::UNUSEDSIGNAL, سچ);
            AstVar* ثابت varp = جديد AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->ڳوليو_UInt32_DType()};
            varp->سيٽ_Ignore_Sched_Write();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(سچ);
            m_modp->جوڙيو_Stmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            Ast_سونپيو* ثابت assp = جديد Ast_سونپيو{
                incp->fileline(), جديد AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                جديد Ast_جوڙيو{incp->fileline(), جديد AstVarRef{incp->fileline(), varp, VAccess::READ},
                           جديد AstConst{incp->fileline(), AstConst::Widthed_قدر{}, 32, 1}}};
            Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ, Ast_ڳنڍ>(incp, assp);
        }
        أرجع incp;
    }
    string trace_Name_For_سٽ(ثابت Ast_ڳنڍ* nodep, ثابت string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvt_منزل_Str(nodep->fileline()->lineno()) + "_" + type;
        إذا (ثابت uint32_t suffix = m_varnames[name]++) name += "_" + cvt_منزل_Str(suffix);
        أرجع name;
    }

    // Line tracking
    فراغ createHandle(ثابت Ast_ڳنڍ* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = سچ;
        m_state.m_handle = ++m_ايندڙ_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    فراغ سٽ_Track(ثابت Ast_ڳنڍ* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        إذا ((m_state.سٽ_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            لكل (صحيح lineno = nodep->fileline()->پهريون_Lineno();
                 lineno <= nodep->fileline()->آخري_Lineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_سٽون[m_state.m_handle].وجهو(lineno);
            }
        }
    }
    ساكن string سٽون_پهريون_آخري(ثابت صحيح پهريون, ثابت صحيح آخري) {
        إذا (پهريون && پهريون == آخري) {
            أرجع cvt_منزل_Str(پهريون);
        } وإلا إذا (پهريون && آخري) {
            أرجع cvt_منزل_Str(پهريون) + "-" + cvt_منزل_Str(آخري);
        } وإلا {
            أرجع "";
        }
    }
    string سٽون_Cov(ثابت CheckState& state, ثابت Ast_ڳنڍ* nodep) {
        // Return comma separated list of ranged numbers
        string ٻاهريون;
        ثابت Lineno_سيٽ& سٽون = m_handle_سٽون[state.m_handle];
        صحيح پهريون = 0;
        صحيح آخري = 0;
        لكل (ثابت صحيح linen : سٽون) {
            إذا (!پهريون) {
                پهريون = آخري = linen;
            } وإلا إذا (linen == آخري + 1) {
                ++آخري;
            } وإلا {
                إذا (!ٻاهريون.خالي_جايون()) ٻاهريون += ",";
                ٻاهريون += سٽون_پهريون_آخري(پهريون, آخري);
                پهريون = آخري = linen;
            }
        }
        إذا (پهريون) {
            إذا (!ٻاهريون.خالي_جايون()) ٻاهريون += ",";
            ٻاهريون += سٽون_پهريون_آخري(پهريون, آخري);
        }
        UINFO(9, "lines out " << ٻاهريون << " for h" << state.m_handle << " " << nodep);
        أرجع ٻاهريون;
    }

    // VISITORS - BOTH
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        ثابت Ast_ڳنڍ_Module* ثابت origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_اظهار_عارضي_نالا);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_اندريون_Mod_Off = nodep->is_چوٽي();  // Already made top shell, no coverage for it
        إذا (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_سٽون.clear();
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_اظهار_عارضي_نالا);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_اندريون_Mod_Off = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    فراغ گهمو(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_سونپيو) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_اظهار_Stmtsp);
            VL_RESTORER(m_اندريون_Toggle_Off);
            m_اظهار_Stmtsp = nodep;
            m_اندريون_Toggle_Off = سچ;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            أرجع;
        }
        iterateProcedure(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override { iterateProcedure(nodep); }
    فراغ گهمو(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_اظهار_Stmtsp);
        VL_RESTORER(m_اندريون_Toggle_Off);
        m_اظهار_Stmtsp = nodep;
        m_اندريون_Toggle_Off = سچ;
        createHandle(nodep);
        iterate_And_ايندڙ_Null(nodep->stmtsp());
        إذا (m_state.سٽ_Coverage_On(nodep)) {
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سٽون_Cov(m_state, nodep), 0};
            m_modp->جوڙيو_Stmtsp(declp);
            Ast_ڳنڍ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سٽ(nodep, "block"));
            وجهو_Proc_Statement(nodep, newp);
        }
    }
    فراغ گهمو(AstLoopTest* nodep) override {
        إذا (nodep->user2_سيٽ_Once()) أرجع;
        سٽ_Track(nodep);
        إذا (m_state.سٽ_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سٽون_Cov(m_state, nodep), 0};
            m_modp->جوڙيو_Stmtsp(declp);
            Ast_ڳنڍ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سٽ(nodep, "block"));
            nodep->جوڙيو_هتي_This_As_ايندڙ(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_اظهار_عارضي_نالا);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        إذا (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    فراغ وجهو_Proc_Statement(Ast_ڳنڍ* nodep, Ast_ڳنڍ* stmtp) {
        إذا (Ast_ڳنڍ_Procedure* ثابت itemp = VN_CAST(nodep, ڳنڍ_Procedure)) {
            itemp->جوڙيو_Stmtsp(stmtp);
        } وإلا إذا (Ast_ڳنڍ_FTask* ثابت itemp = VN_CAST(nodep, ڳنڍ_FTask)) {
            itemp->جوڙيو_Stmtsp(stmtp);
        } وإلا إذا (AstLoop* ثابت itemp = VN_CAST(nodep, Loop)) {
            itemp->جوڙيو_Stmtsp(stmtp);
        } وإلا إذا (AstIf* ثابت itemp = VN_CAST(nodep, If)) {
            إذا (m_then) {
                itemp->جوڙيو_Thensp(stmtp);
            } وإلا {
                itemp->جوڙيو_Elsesp(stmtp);
            }
        } وإلا إذا (AstBegin* ثابت itemp = VN_CAST(nodep, Begin)) {
            itemp->جوڙيو_Stmtsp(stmtp);
        } وإلا {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    فراغ iterateProcedure(Ast_ڳنڍ* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_اظهار_Stmtsp);
        VL_RESTORER(m_اندريون_Toggle_Off);
        m_اظهار_Stmtsp = nodep;
        m_اندريون_Toggle_Off = سچ;
        createHandle(nodep);
        iterateChildren(nodep);
        إذا (m_state.سٽ_Coverage_On(nodep)) {
            سٽ_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سٽون_Cov(m_state, nodep), 0};
            m_modp->جوڙيو_Stmtsp(declp);
            Ast_ڳنڍ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سٽ(nodep, "block"));
            وجهو_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    فراغ گهمو(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_modp && !m_اندريون_Toggle_Off && !m_state.m_اندريون_Mod_Off && nodep->fileline()->coverageOn()
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
                File_سٽ* ثابت fl_nowarn = جديد File_سٽ{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3_Error_ڪوڊ::UNUSEDSIGNAL, سچ);
                AstVar* ثابت chgVarp
                    = جديد AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->جوڙيو_Stmtsp(chgVarp);

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
        ثابت std::string hier_اڳياڙي
            = (m_beginHier != "") ? Ast_ڳنڍ::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ثابت declp
            = جديد AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hier_اڳياڙي + varp->name() + above.m_comment, range};
        m_modp->جوڙيو_Stmtsp(declp);
        AstCoverToggle* ثابت newp = جديد AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_وڻ(خطأ), above.m_chgRefp->clone_وڻ(خطأ)};
        m_modp->جوڙيو_Stmtsp(newp);
    }

    فراغ toggleVarRecurse(ثابت Ast_ڳنڍ_DType* ثابت dtypep, ثابت صحيح اونهائي,  // per-iteration
                          ثابت ToggleEnt& above, ثابت AstVar* ثابت varp) {  // Constant
        إذا (ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            لكل (ثابت صحيح اشاريو_docs : adtypep->declRange().seq_Lo_منزل_Hi()) {
                ثابت صحيح اشاريو_ڪوڊ = اشاريو_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_منزل_Str(اشاريو_docs) + "]",
                                 جديد AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_وڻ(خطأ), اشاريو_ڪوڊ},
                                 جديد AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_وڻ(خطأ), اشاريو_ڪوڊ}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), اونهائي + 1, newent, varp);
                newent.cleanup();
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(dtypep, PackArrayDType)) {
            لكل (ثابت صحيح اشاريو_docs : adtypep->declRange().seq_Lo_منزل_Hi()) {
                ثابت Ast_ڳنڍ_DType* ثابت subtypep = adtypep->subDTypep()->skipRefp();
                ثابت صحيح اشاريو_ڪوڊ = اشاريو_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_منزل_Str(اشاريو_docs) + "]",
                                 جديد AstSel{varp->fileline(), above.m_varRefp->clone_وڻ(خطأ),
                                            اشاريو_ڪوڊ * subtypep->ويڪر(), subtypep->ويڪر()},
                                 جديد AstSel{varp->fileline(), above.m_chgRefp->clone_وڻ(خطأ),
                                            اشاريو_ڪوڊ * subtypep->ويڪر(), subtypep->ويڪر()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), اونهائي + 1, newent, varp);
                newent.cleanup();
            }
        } وإلا إذا (ثابت AstStructDType* ثابت adtypep = VN_CAST(dtypep, StructDType)) {
            إذا (adtypep->packed()) {
                لكل (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت Ast_ڳنڍ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    ثابت صحيح اشاريو_ڪوڊ = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        جديد AstSel{varp->fileline(), above.m_varRefp->clone_وڻ(خطأ), اشاريو_ڪوڊ,
                                   subtypep->ويڪر()},
                        جديد AstSel{varp->fileline(), above.m_chgRefp->clone_وڻ(خطأ), اشاريو_ڪوڊ,
                                   subtypep->ويڪر()}};
                    toggleVarRecurse(subtypep, اونهائي + 1, newent, varp);
                    newent.cleanup();
                }
            } وإلا {
                لكل (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_ڳنڍ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    Ast_ڳنڍ_اظهار* ثابت varRefp = جديد AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_وڻ(خطأ), itemp->name()};
                    Ast_ڳنڍ_اظهار* ثابت chgRefp = جديد AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_وڻ(خطأ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, اونهائي + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وإلا إذا (ثابت AstUnionDType* ثابت adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            إذا (ثابت AstMemberDType* ثابت itemp = adtypep->membersp()) {
                Ast_ڳنڍ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                إذا (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_وڻ(خطأ),
                                     above.m_chgRefp->clone_وڻ(خطأ)};
                    toggleVarRecurse(subtypep, اونهائي + 1, newent, varp);
                    newent.cleanup();
                } وإلا {
                    Ast_ڳنڍ_اظهار* ثابت varRefp = جديد AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_وڻ(خطأ), itemp->name()};
                    Ast_ڳنڍ_اظهار* ثابت chgRefp = جديد AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_وڻ(خطأ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, اونهائي + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وإلا إذا (VN_IS(dtypep, قطار_DType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } وإلا {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    منطقي include_Cond_منزل_Branch_Recursive(ثابت Ast_ڳنڍ* ثابت nodep) {
        ثابت Ast_ڳنڍ* ثابت backp = nodep->backp();
        إذا (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            أرجع include_Cond_منزل_Branch_Recursive(backp);
        } وإلا إذا (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            أرجع include_Cond_منزل_Branch_Recursive(backp);
        } وإلا إذا (VN_IS(backp, ڳنڍ_سونپيو) && VN_AS(backp, ڳنڍ_سونپيو)->rhsp() == nodep) {
            أرجع سچ;
        }
        أرجع خطأ;
    }

    // VISITORS - LINE COVERAGE
    فراغ گهمو(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        إذا (m_seeking == NONE) coverExprs(nodep->condp());

        إذا (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && include_Cond_منزل_Branch_Recursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ثابت CheckState آخري_State = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            سٽ_Track(nodep);
            Ast_ڳنڍ_اظهار* ثابت thenp = nodep->thenp()->unlink_Fr_پٺ();
            AstCoverOtherDecl* ثابت thenDeclp
                = جديد AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", سٽون_Cov(m_state, nodep), 0};
            m_modp->جوڙيو_Stmtsp(thenDeclp);
            nodep->thenp(جديد Ast_اظهار_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_سٽ(nodep, "cond_then")),
                thenp});
            m_state = آخري_State;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_ڳنڍ_اظهار* ثابت elsep = nodep->elsep()->unlink_Fr_پٺ();
            AstCoverOtherDecl* ثابت elseDeclp
                = جديد AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", سٽون_Cov(m_state, nodep), 1};
            m_modp->جوڙيو_Stmtsp(elseDeclp);
            nodep->elsep(جديد Ast_اظهار_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_سٽ(nodep, "cond_else")),
                elsep});

            m_state = آخري_State;
        } وإلا {
            سٽ_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    فراغ گهمو(AstIf* nodep) override {
        إذا (nodep->user2()) أرجع;

        UINFO(4, " IF: " << nodep);
        إذا (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ثابت منطقي elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            إذا (elsif) VN_AS(nodep->elsesp(), If)->user1(سچ);
            ثابت منطقي پهريون_elsif = !nodep->user1() && elsif;
            ثابت منطقي cont_elsif = nodep->user1() && elsif;
            ثابت منطقي final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ثابت CheckState آخري_State = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_اظهار_Stmtsp);
                VL_RESTORER(m_then);
                m_اظهار_Stmtsp = nodep;
                m_then = سچ;
                createHandle(nodep);
                iterate_And_ايندڙ_Null(nodep->thensp());
                سٽ_Track(nodep);
                ifState = m_state;
            }
            m_state = آخري_State;
            {
                VL_RESTORER(m_اظهار_Stmtsp);
                VL_RESTORER(m_then);
                m_اظهار_Stmtsp = nodep;
                m_then = خطأ;
                createHandle(nodep);
                iterate_And_ايندڙ_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = آخري_State;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            إذا (!(پهريون_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ثابت ifDeclp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", سٽون_Cov(ifState, nodep), 0};
                m_modp->جوڙيو_Stmtsp(ifDeclp);
                nodep->جوڙيو_Thensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_سٽ(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ثابت elseDeclp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", سٽون_Cov(elseState, nodep), 1};
                m_modp->جوڙيو_Stmtsp(elseDeclp);
                nodep->جوڙيو_Elsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_سٽ(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            وإلا إذا (پهريون_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                إذا (ifState.سٽ_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ثابت elsifDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        سٽون_Cov(ifState, nodep), 0};
                    m_modp->جوڙيو_Stmtsp(elsifDeclp);

                    nodep->جوڙيو_Thensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_سٽ(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } وإلا {
                // Cover as separate blocks (not a branch as is not two-legged)
                إذا (ifState.سٽ_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ثابت ifDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        سٽون_Cov(ifState, nodep), 0};
                    m_modp->جوڙيو_Stmtsp(ifDeclp);
                    nodep->جوڙيو_Thensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_سٽ(nodep, "if")));
                }
                إذا (elseState.سٽ_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ثابت elseDeclp = جديد AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        سٽون_Cov(elseState, nodep), 1};
                    m_modp->جوڙيو_Stmtsp(elseDeclp);

                    nodep->جوڙيو_Elsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_سٽ(nodep, "else")));
                }
            }
            m_state = آخري_State;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = سچ;
        iterate_And_ايندڙ_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    فراغ گهمو(Ast_Case_شي* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        إذا (m_state.سٽ_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_ايندڙ_Null(nodep->stmtsp());
            إذا (m_state.سٽ_Coverage_On(nodep)) {  // if the case body didn't disable it
                سٽ_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ثابت declp
                    = جديد AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", سٽون_Cov(m_state, nodep), 0};
                m_modp->جوڙيو_Stmtsp(declp);
                nodep->جوڙيو_Stmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_سٽ(nodep, "case")));
            }
        }
    }
    فراغ گهمو(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = سچ;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        إذا (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            سٽ_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جديد AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", سٽون_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->جوڙيو_Stmtsp(declp);
            nodep->جوڙيو_Coverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    فراغ گهمو(AstPropSpec* nodep) override {
        VL_RESTORER(m_اظهار_Stmtsp);
        m_اظهار_Stmtsp = خالي_اشارو;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = خطأ;
    }
    فراغ گهمو(AstStmtPragma* nodep) override {
        إذا (nodep->pragp()->pragType() == VPragmaType::COVERAGE_بلاڪ_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = خطأ;
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
        } وإلا {
            إذا (m_state.m_on) iterateChildren(nodep);
            سٽ_Track(nodep);
        }
    }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        إذا (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        سٽ_Track(nodep);
    }

    فراغ گهمو(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_اندريون_Toggle_Off);
        VL_RESTORER(m_اظهار_Stmtsp);
        m_اظهار_Stmtsp = nodep;
        m_اندريون_Toggle_Off = سچ;
        إذا (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        سٽ_Track(nodep);
    }

    فراغ abort_اظهار_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        إذا (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    منطقي checkMaxExprs(سائيز_t additional = 0) {
        إذا (m_seeking != ABORTED
            && static_cast<صحيح>(m_exprs.سائيز() + additional) <= v3Global.opt.coverage_اظهار_Max())
            أرجع خطأ;
        abort_اظهار_Coverage();
        أرجع سچ;
    }

    فراغ جوڙيو_اظهار_Cover_Inc(Ast_ڳنڍ_اظهار* nodep, صحيح شروعات = 0) {
        File_سٽ* ثابت fl = nodep->fileline();
        صحيح ڳڻپ = شروعات;
        لكل (Cover_اظهار& اظهار : m_exprs) {
            ثابت string name = "expr_" + std::منزل_string(ڳڻپ);
            string comment = "(";
            منطقي پهريون = سچ;
            Ast_ڳنڍ_اظهار* condp = خالي_اشارو;
            لكل (Cover_رڪن& رڪن : اظهار) {
                comment += (پهريون ? "" : " && ") + رڪن.m_emitV
                           + "==" + (رڪن.m_objective ? "1" : "0");
                Ast_ڳنڍ_اظهار* covExprp = خالي_اشارو;
                إذا (VN_IS(رڪن.m_exprp, FuncRef) || رڪن.m_exprp->isSystemFunc()) {
                    Ast_ڳنڍ_اظهار* ثابت frefp = رڪن.m_exprp;
                    Ast_ڳنڍ_DType* ثابت dtypep = frefp->dtypep();
                    ثابت خودڪار pair = m_funcTemps.emplace(frefp, خالي_اشارو);
                    AstVar* varp = pair.پهريون->ٻيو;
                    إذا (pair.ٻيو) {
                        varp = جديد AstVar{fl, VVarType::MODULETEMP, m_اظهار_عارضي_نالا.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.پهريون->ٻيو = varp;
                        إذا (m_ftaskp) {
                            varp->funcLocal(سچ);
                            m_ftaskp->prependStmtsp(varp);
                        } وإلا {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlink_Fr_پٺ(&relinkHandle);
                        relinkHandle.relink(جديد Ast_اظهار_Stmt{
                            fl, جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            جديد AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = جديد AstVarRef{fl, varp, VAccess::READ};
                } وإلا {
                    covExprp = رڪن.m_exprp->clone_وڻ(خطأ);
                }
                Ast_ڳنڍ_اظهار* ثابت termp
                    = رڪن.m_objective ? covExprp : جديد AstLogNot{fl, covExprp};
                إذا (condp) {
                    condp = جديد AstLogAnd{fl, condp, termp};
                } وإلا {
                    condp = termp;
                }
                پهريون = خطأ;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ثابت declp = جديد AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->جوڙيو_Stmtsp(declp);
            Ast_ڳنڍ* ثابت newp = newCoverInc(fl, declp, trace_Name_For_سٽ(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ثابت ifp = جديد AstIf{fl, condp, newp, خالي_اشارو};
            ifp->user2(سچ);
            وجهو_Proc_Statement(m_اظهار_Stmtsp, ifp);
            ++ڳڻپ;
        }
    }

    فراغ coverExprs(Ast_ڳنڍ_اظهار* nodep) {
        إذا (!m_state.اظهار_Coverage_On(nodep) || nodep->dtypep()->ويڪر() != 1 || !m_اظهار_Stmtsp) {
            أرجع;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.خالي_جايون(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = خطأ;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.مٽا_سٽا(falseExprs);

        m_objective = سچ;
        iterate(nodep);
        إذا (checkMaxExprs(falseExprs.سائيز())) أرجع;
        إذا (m_seeking == ABORTED) أرجع;

        جوڙيو_اظهار_Cover_Inc(nodep);
        ثابت صحيح شروعات = m_exprs.سائيز();
        m_objective = خطأ;
        m_exprs.مٽا_سٽا(falseExprs);
        جوڙيو_اظهار_Cover_Inc(nodep, شروعات);
    }

    فراغ اظهار_Either(Ast_ڳنڍ_Biop* nodep, منطقي overrideObjective = خطأ, منطقي lObjective = خطأ,
                    منطقي rObjective = خطأ) {
        VL_RESTORER(m_objective);
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp();

        إذا (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs lhsExprs;
        m_exprs.مٽا_سٽا(lhsExprs);
        إذا (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.پڇاڙي(), lhsExprs);
        checkMaxExprs();
    }

    فراغ اظهار_Both(Ast_ڳنڍ_Biop* nodep, منطقي overrideObjective = خطأ, منطقي lObjective = خطأ,
                  منطقي rObjective = خطأ) {
        VL_RESTORER(m_objective);
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp();

        إذا (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs lhsExprs;
        m_exprs.مٽا_سٽا(lhsExprs);
        إذا (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        إذا (checkMaxExprs()) أرجع;
        CoverExprs rhsExprs;
        m_exprs.مٽا_سٽا(rhsExprs);

        لكل (ثابت Cover_اظهار& l : lhsExprs) {
            لكل (ثابت Cover_اظهار& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::سيٽ<AstVar*>, 2> varps;
                std::array<std::سيٽ<std::string>, 2> strs;

                UASSERT_OBJ(!l.خالي_جايون() && !r.خالي_جايون(), nodep, "Empty coverage expression branch");
                Cover_اظهار اظهار;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                لكل (ثابت Cover_رڪن& رڪن : l) {
                    إذا (ثابت AstVarRef* ثابت refp = VN_CAST(رڪن.m_exprp, VarRef)) {
                        varps[رڪن.m_objective].وجهو(refp->varp());
                    } وإلا {
                        strs[رڪن.m_objective].وجهو(رڪن.m_emitV);
                    }
                    اظهار.ڌڪو_پٺ(رڪن);
                }
                منطقي impossible = خطأ;
                لكل (ثابت Cover_رڪن& رڪن : r) {
                    منطقي redundant = خطأ;
                    إذا (ثابت Ast_ڳنڍ_Var_Ref* ثابت refp = VN_CAST(رڪن.m_exprp, ڳنڍ_Var_Ref)) {
                        إذا (varps[رڪن.m_objective].ڳوليو(refp->varp())
                            != varps[رڪن.m_objective].پڇاڙي())
                            redundant = سچ;
                        إذا (varps[!رڪن.m_objective].ڳوليو(refp->varp())
                            != varps[!رڪن.m_objective].پڇاڙي())
                            impossible = سچ;
                    } وإلا {
                        إذا (strs[رڪن.m_objective].ڳوليو(رڪن.m_emitV)
                            != strs[رڪن.m_objective].پڇاڙي())
                            redundant = سچ;
                        إذا (strs[!رڪن.m_objective].ڳوليو(رڪن.m_emitV)
                            != strs[!رڪن.m_objective].پڇاڙي())
                            impossible = سچ;
                    }
                    إذا (!redundant) اظهار.ڌڪو_پٺ(رڪن);
                }
                إذا (!impossible) m_exprs.ڌڪو_پٺ(std::move(اظهار));
                إذا (checkMaxExprs()) أرجع;
            }
        }
    }

    فراغ or_اظهار(Ast_ڳنڍ_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            اظهار_Either(nodep);
        } وإلا {
            اظهار_Both(nodep);
        }
        سٽ_Track(nodep);
    }
    فراغ گهمو(AstLogOr* nodep) override { or_اظهار(nodep); }
    فراغ گهمو(AstOr* nodep) override { or_اظهار(nodep); }

    فراغ and_اظهار(Ast_ڳنڍ_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            اظهار_Both(nodep);
        } وإلا {
            اظهار_Either(nodep);
        }
        سٽ_Track(nodep);
    }
    فراغ گهمو(AstLogAnd* nodep) override { and_اظهار(nodep); }
    فراغ گهمو(AstAnd* nodep) override { and_اظهار(nodep); }

    فراغ xor_اظهار(Ast_ڳنڍ_Biop* nodep) {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            لكل (ثابت منطقي lObjective : {خطأ, سچ}) {
                CoverExprs prevExprs;
                m_exprs.مٽا_سٽا(prevExprs);
                ثابت منطقي rObjective = lObjective ^ m_objective;
                اظهار_Both(nodep, سچ, lObjective, rObjective);
                m_exprs.splice(m_exprs.پڇاڙي(), prevExprs);
                إذا (checkMaxExprs()) اكسر;
            }
        }
        سٽ_Track(nodep);
    }
    فراغ گهمو(AstXor* nodep) override { xor_اظهار(nodep); }

    فراغ اظهار_Not(Ast_ڳنڍ_اظهار* nodep) {
        VL_RESTORER(m_objective);
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            m_objective = !m_objective;
            iterateChildren(nodep);
            سٽ_Track(nodep);
        }
    }
    فراغ گهمو(AstNot* nodep) override { اظهار_Not(nodep); }
    فراغ گهمو(AstLogNot* nodep) override { اظهار_Not(nodep); }

    سانچو <قسم_نالو T_Oper>
    فراغ اظهار_Reduce(Ast_ڳنڍ_Uniop* nodep) {
        إذا (m_seeking != ABORTED) {
            File_سٽ* ثابت fl = nodep->fileline();
            Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
            ثابت صحيح ويڪر = lhsp->dtypep()->ويڪر();
            ثابت سائيز_t expected = std::is_same<T_Oper, AstXor>::قدر ? 0x1 << ويڪر : ويڪر + 1;
            إذا (checkMaxExprs(expected)) أرجع;
            Ast_ڳنڍ_اظهار* unrolledp = جديد AstSel{
                fl, lhsp->clone_وڻ(خطأ), جديد AstConst{fl, static_cast<uint32_t>(ويڪر - 1)}, 1};
            لكل (صحيح ٻٽو_انگ_2 = ويڪر - 2; ٻٽو_انگ_2 >= 0; ٻٽو_انگ_2--) {
                AstSel* ثابت selp = جديد AstSel{fl, lhsp->clone_وڻ(خطأ),
                                                جديد AstConst{fl, static_cast<uint32_t>(ٻٽو_انگ_2)}, 1};
                unrolledp = جديد T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ڌڪو_Deletep(unrolledp), unrolledp);
        } وإلا {
            iterateChildren(nodep);
            سٽ_Track(nodep);
        }
    }
    فراغ گهمو(AstRedOr* nodep) override { اظهار_Reduce<AstOr>(nodep); }
    فراغ گهمو(AstRedAnd* nodep) override { اظهار_Reduce<AstAnd>(nodep); }
    فراغ گهمو(AstRedXor* nodep) override { اظهار_Reduce<AstXor>(nodep); }

    فراغ گهمو(AstLogIf* nodep) override {
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا إذا (m_objective) {
            اظهار_Either(nodep, سچ, خطأ, سچ);
        } وإلا {
            اظهار_Both(nodep, سچ, سچ, خطأ);
        }
        سٽ_Track(nodep);
    }

    فراغ گهمو(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        إذا (m_seeking == NONE) {
            coverExprs(nodep);
        } وإلا {
            m_objective = !m_objective;
            xor_اظهار(nodep);
            سٽ_Track(nodep);
        }
    }

    فراغ گهمو(AstFuncRef* nodep) override {
        إذا (nodep->taskp()->lifetime().isAutomatic()) {
            گهمو(static_cast<Ast_ڳنڍ_اظهار*>(nodep));
        } وإلا {
            اظهار_Unsupported(nodep, "non-automatic function");
        }
    }

    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        إذا (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } وإلا {
            اظهار_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            إذا (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_وڻ(nodep, emitV);
                // Add new expression with a single term
                Cover_اظهار اظهار;
                اظهار.emplace_پٺ(nodep, m_objective, emitV.str());
                m_exprs.ڌڪو_پٺ(std::move(اظهار));
                checkMaxExprs();
            } وإلا {
                اظهار_Unsupported(nodep, "not coverage eligible");
            }
        }
        سٽ_Track(nodep);
    }

    // VISITORS - BOTH
    فراغ گهمو(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_اندريون_Toggle_Off);
        m_state.m_on = خطأ;
        m_اندريون_Toggle_Off = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_اندريون_Toggle_Off);
        m_state.m_on = خطأ;
        m_اندريون_Toggle_Off = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        iterateChildren(nodep);
        سٽ_Track(nodep);
    }

    فراغ اظهار_Unsupported(Ast_ڳنڍ* nodep, ثابت string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ثابت منطقي wasSeeking = m_seeking == SEEKING;
        ثابت Objective oldSeeking = m_seeking;
        إذا (wasSeeking) abort_اظهار_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        سٽ_Track(nodep);
        إذا (!wasSeeking) m_seeking = oldSeeking;
    }

عام:
    // CONSTRUCTORS
    واضح CoverageVisitor(AstNetlist* rootp)
        : m_اظهار_عارضي_نالا{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = افتراضي;
};

//######################################################################
// Coverage class functions

فراغ V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("coverage", 0, dump_وڻ_Either_Level() >= 3);
}
