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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Coverage.h"

#شامل "V3EmitV.h"
#شامل "V3UniqueNames.h"

#شامل <list>
#شامل <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت اظہاریہ_Coverage_Eligible_Visitor final : عوامی VNVisitorConst {
    // STATE
    منطقی m_eligible = سچ;

    ساکن منطقی elemDTypeEligible(ثابت Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (ثابت Ast_گرہ_DType* ثابت dtp = dtypep->virtRefDTypep()) {
            اگر (!elemDTypeEligible(dtp)) واپس جھوٹ;
        }
        اگر (ثابت Ast_گرہ_DType* ثابت dtp = dtypep->virtRefDType2p()) {
            اگر (!elemDTypeEligible(dtp)) واپس جھوٹ;
        }
        واپس !VN_IS(dtypep, ClassRefDType);
    }

    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        ثابت Ast_گرہ_DType* ثابت dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        اگر (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } ورنہ {
            m_eligible = جھوٹ;
        }
    }

    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (!nodep->is_اظہاریہ_Coverage_Eligible()) {
            m_eligible = جھوٹ;
        } ورنہ {
            iterateChildrenConst(nodep);
        }
    }

عوامی:
    // CONSTRUCTORS
    صریح اظہاریہ_Coverage_Eligible_Visitor(Ast_گرہ* nodep) { iterateConst(nodep); }
    ~اظہاریہ_Coverage_Eligible_Visitor() override = طےشدہ;

    منطقی eligible() ثابت { واپس m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

جماعت CoverageVisitor final : عوامی VNVisitor {
    // TYPES
    استعمال Lineno_سیٹ = std::سیٹ<صحیح>;

    ساخت Cover_رکن final {
        Ast_گرہ_اظہاریہ* m_exprp;  // Expression branch term
        منطقی m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        Cover_رکن(Ast_گرہ_اظہاریہ* exprp, منطقی objective, ثابت string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    استعمال Cover_اظہاریہ = std::deque<Cover_رکن>;
    استعمال CoverExprs = std::list<Cover_اظہاریہ>;

    ساخت ToggleEnt final {
        ثابت string m_comment;  // Comment for coverage dump
        Ast_گرہ_اظہاریہ* m_varRefp;  // How to get to this element
        Ast_گرہ_اظہاریہ* m_chgRefp;  // How to get to this element
        ToggleEnt(ثابت string& comment, Ast_گرہ_اظہاریہ* vp, Ast_گرہ_اظہاریہ* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = طےشدہ;
        خالی cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_درخت(), m_varRefp = خالی_اشاریہ);
            VL_DO_CLEAR(m_chgRefp->delete_درخت(), m_chgRefp = خالی_اشاریہ);
        }
    };

    ساخت CheckState final {  // State save-restored on each new coverage scope/block
        منطقی m_on = جھوٹ;  // Should this block get covered?
        منطقی m_اندر_Mod_Off = جھوٹ;  // In module with no coverage
        صحیح m_handle = 0;  // Opaque handle for index into line tracking
        ثابت Ast_گرہ* m_nodep = خالی_اشاریہ;  // Node establishing this state
        CheckState() = طےشدہ;
        منطقی coverageOn(ثابت Ast_گرہ* nodep) ثابت {
            واپس m_on && !m_اندر_Mod_Off && nodep->fileline()->coverageOn();
        }
        منطقی سطر_Coverage_On(ثابت Ast_گرہ* nodep) ثابت {
            واپس coverageOn(nodep) && v3Global.opt.coverage_سطر();
        }
        منطقی branchCoverageOn(ثابت Ast_گرہ* nodep) ثابت {
            واپس coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        منطقی اظہاریہ_Coverage_On(ثابت Ast_گرہ* nodep) ثابت {
            واپس coverageOn(nodep) && v3Global.opt.coverage_اظہاریہ();
        }
    };

    شمار Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    V3_Unique_نام m_اظہاریہ_عارضی_نام;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_گرہ_اظہاریہ*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    صحیح m_اگلا_Handle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module to add statement to
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    Ast_گرہ* m_اظہاریہ_Stmtsp = خالی_اشاریہ;  // Node to add expr coverage to
    منطقی m_then = جھوٹ;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    منطقی m_objective = جھوٹ;  // Expression objective
    منطقی m_ifCond = جھوٹ;  // Visiting if condition
    منطقی m_اندر_Toggle_Off = جھوٹ;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<صحیح, Lineno_سیٹ> m_handle_سطریں;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ثابت حرف* varIgnoreToggle(ثابت AstVar* nodep) {
        ثابت منطقی cover = nodep->isIO() || (nodep->isSignal() && nodep->is_ثنائی_ہندسہ_Logic());
        اگر (!cover) واپس "Not relevant signal";
        اگر (nodep->isConst()) واپس "Signal is constant";
        اگر (nodep->isDouble()) واپس "Signal is double";
        اگر (nodep->isString()) واپس "Signal is string";
        اگر (!v3Global.opt.coverageUnderscore()) {
            ثابت string prettyName = nodep->prettyName();
            اگر (prettyName[0] == '_') واپس "Leading underscore";
            اگر (prettyName.ڈھونڈو("._") != string::npos) واپس "Inlined leading underscore";
        }
        اگر ((nodep->چوڑائی() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverage_Max_چوڑائی())) {
            واپس "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        واپس خالی_اشاریہ;
    }

    AstCoverInc* newCoverInc(File_سطر* fl, Ast_گرہ_Cover_Decl* ثابت declp,
                             ثابت string& trace_var_name) {
        AstCoverInc* ثابت incp = نیا AstCoverInc{fl, declp};
        اگر (!trace_var_name.خالی_خانے()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_سطر* ثابت fl_nowarn = نیا File_سطر{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3_Error_رمز::UNUSEDSIGNAL, سچ);
            AstVar* ثابت varp = نیا AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->ڈھونڈو_UInt32_DType()};
            varp->سیٹ_Ignore_Sched_Write();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(سچ);
            m_modp->جوڑو_Stmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            Ast_تفویض* ثابت assp = نیا Ast_تفویض{
                incp->fileline(), نیا AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                نیا Ast_جوڑو{incp->fileline(), نیا AstVarRef{incp->fileline(), varp, VAccess::READ},
                           نیا AstConst{incp->fileline(), AstConst::Widthed_قدر{}, 32, 1}}};
            Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(incp, assp);
        }
        واپس incp;
    }
    string trace_Name_For_سطر(ثابت Ast_گرہ* nodep, ثابت string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvt_مقصد_Str(nodep->fileline()->lineno()) + "_" + type;
        اگر (ثابت uint32_t suffix = m_varnames[name]++) name += "_" + cvt_مقصد_Str(suffix);
        واپس name;
    }

    // Line tracking
    خالی createHandle(ثابت Ast_گرہ* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = سچ;
        m_state.m_handle = ++m_اگلا_Handle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    خالی سطر_Track(ثابت Ast_گرہ* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        اگر ((m_state.سطر_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            برائے (صحیح lineno = nodep->fileline()->پہلا_Lineno();
                 lineno <= nodep->fileline()->آخری_Lineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_سطریں[m_state.m_handle].داخل_کرو(lineno);
            }
        }
    }
    ساکن string سطریں_پہلا_آخری(ثابت صحیح پہلا, ثابت صحیح آخری) {
        اگر (پہلا && پہلا == آخری) {
            واپس cvt_مقصد_Str(پہلا);
        } ورنہ اگر (پہلا && آخری) {
            واپس cvt_مقصد_Str(پہلا) + "-" + cvt_مقصد_Str(آخری);
        } ورنہ {
            واپس "";
        }
    }
    string سطریں_Cov(ثابت CheckState& state, ثابت Ast_گرہ* nodep) {
        // Return comma separated list of ranged numbers
        string برآمد;
        ثابت Lineno_سیٹ& سطریں = m_handle_سطریں[state.m_handle];
        صحیح پہلا = 0;
        صحیح آخری = 0;
        برائے (ثابت صحیح linen : سطریں) {
            اگر (!پہلا) {
                پہلا = آخری = linen;
            } ورنہ اگر (linen == آخری + 1) {
                ++آخری;
            } ورنہ {
                اگر (!برآمد.خالی_خانے()) برآمد += ",";
                برآمد += سطریں_پہلا_آخری(پہلا, آخری);
                پہلا = آخری = linen;
            }
        }
        اگر (پہلا) {
            اگر (!برآمد.خالی_خانے()) برآمد += ",";
            برآمد += سطریں_پہلا_آخری(پہلا, آخری);
        }
        UINFO(9, "lines out " << برآمد << " for h" << state.m_handle << " " << nodep);
        واپس برآمد;
    }

    // VISITORS - BOTH
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        ثابت Ast_گرہ_Module* ثابت origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_اظہاریہ_عارضی_نام);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_اندر_Mod_Off = nodep->is_چوٹی();  // Already made top shell, no coverage for it
        اگر (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_سطریں.clear();
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_اظہاریہ_عارضی_نام);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_اندر_Mod_Off = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    خالی دورہ(AstAlways* nodep) override {
        اگر (nodep->keyword() == VAlwaysKwd::CONT_تفویض) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_اظہاریہ_Stmtsp);
            VL_RESTORER(m_اندر_Toggle_Off);
            m_اظہاریہ_Stmtsp = nodep;
            m_اندر_Toggle_Off = سچ;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            واپس;
        }
        iterateProcedure(nodep);
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override { iterateProcedure(nodep); }
    خالی دورہ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_اظہاریہ_Stmtsp);
        VL_RESTORER(m_اندر_Toggle_Off);
        m_اظہاریہ_Stmtsp = nodep;
        m_اندر_Toggle_Off = سچ;
        createHandle(nodep);
        iterate_And_اگلا_Null(nodep->stmtsp());
        اگر (m_state.سطر_Coverage_On(nodep)) {
            AstCoverOtherDecl* ثابت declp
                = نیا AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطریں_Cov(m_state, nodep), 0};
            m_modp->جوڑو_Stmtsp(declp);
            Ast_گرہ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            داخل_کرو_Proc_Statement(nodep, newp);
        }
    }
    خالی دورہ(AstLoopTest* nodep) override {
        اگر (nodep->user2_سیٹ_Once()) واپس;
        سطر_Track(nodep);
        اگر (m_state.سطر_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ثابت declp
                = نیا AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطریں_Cov(m_state, nodep), 0};
            m_modp->جوڑو_Stmtsp(declp);
            Ast_گرہ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            nodep->جوڑو_یہاں_This_As_اگلا(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_اظہاریہ_عارضی_نام);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        اگر (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    خالی داخل_کرو_Proc_Statement(Ast_گرہ* nodep, Ast_گرہ* stmtp) {
        اگر (Ast_گرہ_Procedure* ثابت itemp = VN_CAST(nodep, گرہ_Procedure)) {
            itemp->جوڑو_Stmtsp(stmtp);
        } ورنہ اگر (Ast_گرہ_FTask* ثابت itemp = VN_CAST(nodep, گرہ_FTask)) {
            itemp->جوڑو_Stmtsp(stmtp);
        } ورنہ اگر (AstLoop* ثابت itemp = VN_CAST(nodep, Loop)) {
            itemp->جوڑو_Stmtsp(stmtp);
        } ورنہ اگر (AstIf* ثابت itemp = VN_CAST(nodep, If)) {
            اگر (m_then) {
                itemp->جوڑو_Thensp(stmtp);
            } ورنہ {
                itemp->جوڑو_Elsesp(stmtp);
            }
        } ورنہ اگر (AstBegin* ثابت itemp = VN_CAST(nodep, Begin)) {
            itemp->جوڑو_Stmtsp(stmtp);
        } ورنہ {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    خالی iterateProcedure(Ast_گرہ* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_اظہاریہ_Stmtsp);
        VL_RESTORER(m_اندر_Toggle_Off);
        m_اظہاریہ_Stmtsp = nodep;
        m_اندر_Toggle_Off = سچ;
        createHandle(nodep);
        iterateChildren(nodep);
        اگر (m_state.سطر_Coverage_On(nodep)) {
            سطر_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = نیا AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطریں_Cov(m_state, nodep), 0};
            m_modp->جوڑو_Stmtsp(declp);
            Ast_گرہ* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            داخل_کرو_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    خالی دورہ(AstVar* nodep) override {
        iterateChildren(nodep);
        اگر (m_modp && !m_اندر_Toggle_Off && !m_state.m_اندر_Mod_Off && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ثابت حرف* ثابت disablep = varIgnoreToggle(nodep);
            اگر (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } ورنہ {
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
                File_سطر* ثابت fl_nowarn = نیا File_سطر{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3_Error_رمز::UNUSEDSIGNAL, سچ);
                AstVar* ثابت chgVarp
                    = نیا AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->جوڑو_Stmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, نیا AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 نیا AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    خالی toggleVarBottom(ثابت ToggleEnt& above, ثابت AstVar* varp, ثابت VNumRange& range) {
        ثابت std::string hier_سابقہ
            = (m_beginHier != "") ? Ast_گرہ::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ثابت declp
            = نیا AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hier_سابقہ + varp->name() + above.m_comment, range};
        m_modp->جوڑو_Stmtsp(declp);
        AstCoverToggle* ثابت newp = نیا AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_درخت(جھوٹ), above.m_chgRefp->clone_درخت(جھوٹ)};
        m_modp->جوڑو_Stmtsp(newp);
    }

    خالی toggleVarRecurse(ثابت Ast_گرہ_DType* ثابت dtypep, ثابت صحیح گہرائی,  // per-iteration
                          ثابت ToggleEnt& above, ثابت AstVar* ثابت varp) {  // Constant
        اگر (ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } ورنہ اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            برائے (ثابت صحیح اشاریہ_docs : adtypep->declRange().seq_Lo_مقصد_Hi()) {
                ثابت صحیح اشاریہ_رمز = اشاریہ_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_مقصد_Str(اشاریہ_docs) + "]",
                                 نیا AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_درخت(جھوٹ), اشاریہ_رمز},
                                 نیا AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_درخت(جھوٹ), اشاریہ_رمز}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), گہرائی + 1, newent, varp);
                newent.cleanup();
            }
        } ورنہ اگر (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(dtypep, PackArrayDType)) {
            برائے (ثابت صحیح اشاریہ_docs : adtypep->declRange().seq_Lo_مقصد_Hi()) {
                ثابت Ast_گرہ_DType* ثابت subtypep = adtypep->subDTypep()->skipRefp();
                ثابت صحیح اشاریہ_رمز = اشاریہ_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvt_مقصد_Str(اشاریہ_docs) + "]",
                                 نیا AstSel{varp->fileline(), above.m_varRefp->clone_درخت(جھوٹ),
                                            اشاریہ_رمز * subtypep->چوڑائی(), subtypep->چوڑائی()},
                                 نیا AstSel{varp->fileline(), above.m_chgRefp->clone_درخت(جھوٹ),
                                            اشاریہ_رمز * subtypep->چوڑائی(), subtypep->چوڑائی()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), گہرائی + 1, newent, varp);
                newent.cleanup();
            }
        } ورنہ اگر (ثابت AstStructDType* ثابت adtypep = VN_CAST(dtypep, StructDType)) {
            اگر (adtypep->packed()) {
                برائے (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    ثابت صحیح اشاریہ_رمز = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        نیا AstSel{varp->fileline(), above.m_varRefp->clone_درخت(جھوٹ), اشاریہ_رمز,
                                   subtypep->چوڑائی()},
                        نیا AstSel{varp->fileline(), above.m_chgRefp->clone_درخت(جھوٹ), اشاریہ_رمز,
                                   subtypep->چوڑائی()}};
                    toggleVarRecurse(subtypep, گہرائی + 1, newent, varp);
                    newent.cleanup();
                }
            } ورنہ {
                برائے (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    Ast_گرہ_اظہاریہ* ثابت varRefp = نیا AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_درخت(جھوٹ), itemp->name()};
                    Ast_گرہ_اظہاریہ* ثابت chgRefp = نیا AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_درخت(جھوٹ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, گہرائی + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ورنہ اگر (ثابت AstUnionDType* ثابت adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            اگر (ثابت AstMemberDType* ثابت itemp = adtypep->membersp()) {
                Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                اگر (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_درخت(جھوٹ),
                                     above.m_chgRefp->clone_درخت(جھوٹ)};
                    toggleVarRecurse(subtypep, گہرائی + 1, newent, varp);
                    newent.cleanup();
                } ورنہ {
                    Ast_گرہ_اظہاریہ* ثابت varRefp = نیا AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_درخت(جھوٹ), itemp->name()};
                    Ast_گرہ_اظہاریہ* ثابت chgRefp = نیا AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_درخت(جھوٹ), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, گہرائی + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ورنہ اگر (VN_IS(dtypep, قطار_DType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } ورنہ {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    منطقی include_Cond_مقصد_Branch_Recursive(ثابت Ast_گرہ* ثابت nodep) {
        ثابت Ast_گرہ* ثابت backp = nodep->backp();
        اگر (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            واپس include_Cond_مقصد_Branch_Recursive(backp);
        } ورنہ اگر (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            واپس include_Cond_مقصد_Branch_Recursive(backp);
        } ورنہ اگر (VN_IS(backp, گرہ_تفویض) && VN_AS(backp, گرہ_تفویض)->rhsp() == nodep) {
            واپس سچ;
        }
        واپس جھوٹ;
    }

    // VISITORS - LINE COVERAGE
    خالی دورہ(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        اگر (m_seeking == NONE) coverExprs(nodep->condp());

        اگر (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && include_Cond_مقصد_Branch_Recursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ثابت CheckState آخری_State = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            سطر_Track(nodep);
            Ast_گرہ_اظہاریہ* ثابت thenp = nodep->thenp()->unlink_Fr_عقب();
            AstCoverOtherDecl* ثابت thenDeclp
                = نیا AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", سطریں_Cov(m_state, nodep), 0};
            m_modp->جوڑو_Stmtsp(thenDeclp);
            nodep->thenp(نیا Ast_اظہاریہ_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_سطر(nodep, "cond_then")),
                thenp});
            m_state = آخری_State;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_گرہ_اظہاریہ* ثابت elsep = nodep->elsep()->unlink_Fr_عقب();
            AstCoverOtherDecl* ثابت elseDeclp
                = نیا AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", سطریں_Cov(m_state, nodep), 1};
            m_modp->جوڑو_Stmtsp(elseDeclp);
            nodep->elsep(نیا Ast_اظہاریہ_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_سطر(nodep, "cond_else")),
                elsep});

            m_state = آخری_State;
        } ورنہ {
            سطر_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    خالی دورہ(AstIf* nodep) override {
        اگر (nodep->user2()) واپس;

        UINFO(4, " IF: " << nodep);
        اگر (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ثابت منطقی elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            اگر (elsif) VN_AS(nodep->elsesp(), If)->user1(سچ);
            ثابت منطقی پہلا_elsif = !nodep->user1() && elsif;
            ثابت منطقی cont_elsif = nodep->user1() && elsif;
            ثابت منطقی final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ثابت CheckState آخری_State = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_اظہاریہ_Stmtsp);
                VL_RESTORER(m_then);
                m_اظہاریہ_Stmtsp = nodep;
                m_then = سچ;
                createHandle(nodep);
                iterate_And_اگلا_Null(nodep->thensp());
                سطر_Track(nodep);
                ifState = m_state;
            }
            m_state = آخری_State;
            {
                VL_RESTORER(m_اظہاریہ_Stmtsp);
                VL_RESTORER(m_then);
                m_اظہاریہ_Stmtsp = nodep;
                m_then = جھوٹ;
                createHandle(nodep);
                iterate_And_اگلا_Null(nodep->elsesp());
                elseState = m_state;
            }
            m_state = آخری_State;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            اگر (!(پہلا_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ثابت ifDeclp
                    = نیا AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", سطریں_Cov(ifState, nodep), 0};
                m_modp->جوڑو_Stmtsp(ifDeclp);
                nodep->جوڑو_Thensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_سطر(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ثابت elseDeclp
                    = نیا AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", سطریں_Cov(elseState, nodep), 1};
                m_modp->جوڑو_Stmtsp(elseDeclp);
                nodep->جوڑو_Elsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_سطر(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            ورنہ اگر (پہلا_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                اگر (ifState.سطر_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ثابت elsifDeclp = نیا AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        سطریں_Cov(ifState, nodep), 0};
                    m_modp->جوڑو_Stmtsp(elsifDeclp);

                    nodep->جوڑو_Thensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_سطر(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } ورنہ {
                // Cover as separate blocks (not a branch as is not two-legged)
                اگر (ifState.سطر_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ثابت ifDeclp = نیا AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        سطریں_Cov(ifState, nodep), 0};
                    m_modp->جوڑو_Stmtsp(ifDeclp);
                    nodep->جوڑو_Thensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_سطر(nodep, "if")));
                }
                اگر (elseState.سطر_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ثابت elseDeclp = نیا AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        سطریں_Cov(elseState, nodep), 1};
                    m_modp->جوڑو_Stmtsp(elseDeclp);

                    nodep->جوڑو_Elsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_سطر(nodep, "else")));
                }
            }
            m_state = آخری_State;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = سچ;
        iterate_And_اگلا_Null(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    خالی دورہ(Ast_Case_شے* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        اگر (m_state.سطر_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterate_And_اگلا_Null(nodep->stmtsp());
            اگر (m_state.سطر_Coverage_On(nodep)) {  // if the case body didn't disable it
                سطر_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ثابت declp
                    = نیا AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", سطریں_Cov(m_state, nodep), 0};
                m_modp->جوڑو_Stmtsp(declp);
                nodep->جوڑو_Stmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "case")));
            }
        }
    }
    خالی دورہ(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = سچ;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        اگر (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            سطر_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = نیا AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", سطریں_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->جوڑو_Stmtsp(declp);
            nodep->جوڑو_Coverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    خالی دورہ(AstPropSpec* nodep) override {
        VL_RESTORER(m_اظہاریہ_Stmtsp);
        m_اظہاریہ_Stmtsp = خالی_اشاریہ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = جھوٹ;
    }
    خالی دورہ(AstStmtPragma* nodep) override {
        اگر (nodep->pragp()->pragType() == VPragmaType::COVERAGE_بلاک_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = جھوٹ;
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ {
            اگر (m_state.m_on) iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    خالی دورہ(Ast_Gen_بلاک* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        اگر (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    خالی دورہ(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_اندر_Toggle_Off);
        VL_RESTORER(m_اظہاریہ_Stmtsp);
        m_اظہاریہ_Stmtsp = nodep;
        m_اندر_Toggle_Off = سچ;
        اگر (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    خالی abort_اظہاریہ_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        اگر (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    منطقی checkMaxExprs(سائز_t additional = 0) {
        اگر (m_seeking != ABORTED
            && static_cast<صحیح>(m_exprs.سائز() + additional) <= v3Global.opt.coverage_اظہاریہ_Max())
            واپس جھوٹ;
        abort_اظہاریہ_Coverage();
        واپس سچ;
    }

    خالی جوڑو_اظہاریہ_Cover_Inc(Ast_گرہ_اظہاریہ* nodep, صحیح آغاز = 0) {
        File_سطر* ثابت fl = nodep->fileline();
        صحیح گنتی = آغاز;
        برائے (Cover_اظہاریہ& اظہاریہ : m_exprs) {
            ثابت string name = "expr_" + std::مقصد_string(گنتی);
            string comment = "(";
            منطقی پہلا = سچ;
            Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;
            برائے (Cover_رکن& رکن : اظہاریہ) {
                comment += (پہلا ? "" : " && ") + رکن.m_emitV
                           + "==" + (رکن.m_objective ? "1" : "0");
                Ast_گرہ_اظہاریہ* covExprp = خالی_اشاریہ;
                اگر (VN_IS(رکن.m_exprp, FuncRef) || رکن.m_exprp->isSystemFunc()) {
                    Ast_گرہ_اظہاریہ* ثابت frefp = رکن.m_exprp;
                    Ast_گرہ_DType* ثابت dtypep = frefp->dtypep();
                    ثابت خودکار pair = m_funcTemps.emplace(frefp, خالی_اشاریہ);
                    AstVar* varp = pair.پہلا->دوسرا;
                    اگر (pair.دوسرا) {
                        varp = نیا AstVar{fl, VVarType::MODULETEMP, m_اظہاریہ_عارضی_نام.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.پہلا->دوسرا = varp;
                        اگر (m_ftaskp) {
                            varp->funcLocal(سچ);
                            m_ftaskp->prependStmtsp(varp);
                        } ورنہ {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlink_Fr_عقب(&relinkHandle);
                        relinkHandle.relink(نیا Ast_اظہاریہ_Stmt{
                            fl, نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            نیا AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = نیا AstVarRef{fl, varp, VAccess::READ};
                } ورنہ {
                    covExprp = رکن.m_exprp->clone_درخت(جھوٹ);
                }
                Ast_گرہ_اظہاریہ* ثابت termp
                    = رکن.m_objective ? covExprp : نیا AstLogNot{fl, covExprp};
                اگر (condp) {
                    condp = نیا AstLogAnd{fl, condp, termp};
                } ورنہ {
                    condp = termp;
                }
                پہلا = جھوٹ;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ثابت declp = نیا AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->جوڑو_Stmtsp(declp);
            Ast_گرہ* ثابت newp = newCoverInc(fl, declp, trace_Name_For_سطر(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ثابت ifp = نیا AstIf{fl, condp, newp, خالی_اشاریہ};
            ifp->user2(سچ);
            داخل_کرو_Proc_Statement(m_اظہاریہ_Stmtsp, ifp);
            ++گنتی;
        }
    }

    خالی coverExprs(Ast_گرہ_اظہاریہ* nodep) {
        اگر (!m_state.اظہاریہ_Coverage_On(nodep) || nodep->dtypep()->چوڑائی() != 1 || !m_اظہاریہ_Stmtsp) {
            واپس;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.خالی_خانے(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = جھوٹ;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.تبادلہ(falseExprs);

        m_objective = سچ;
        iterate(nodep);
        اگر (checkMaxExprs(falseExprs.سائز())) واپس;
        اگر (m_seeking == ABORTED) واپس;

        جوڑو_اظہاریہ_Cover_Inc(nodep);
        ثابت صحیح آغاز = m_exprs.سائز();
        m_objective = جھوٹ;
        m_exprs.تبادلہ(falseExprs);
        جوڑو_اظہاریہ_Cover_Inc(nodep, آغاز);
    }

    خالی اظہاریہ_Either(Ast_گرہ_Biop* nodep, منطقی overrideObjective = جھوٹ, منطقی lObjective = جھوٹ,
                    منطقی rObjective = جھوٹ) {
        VL_RESTORER(m_objective);
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();

        اگر (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        اگر (checkMaxExprs()) واپس;
        CoverExprs lhsExprs;
        m_exprs.تبادلہ(lhsExprs);
        اگر (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.انتہا(), lhsExprs);
        checkMaxExprs();
    }

    خالی اظہاریہ_Both(Ast_گرہ_Biop* nodep, منطقی overrideObjective = جھوٹ, منطقی lObjective = جھوٹ,
                  منطقی rObjective = جھوٹ) {
        VL_RESTORER(m_objective);
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();

        اگر (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        اگر (checkMaxExprs()) واپس;
        CoverExprs lhsExprs;
        m_exprs.تبادلہ(lhsExprs);
        اگر (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        اگر (checkMaxExprs()) واپس;
        CoverExprs rhsExprs;
        m_exprs.تبادلہ(rhsExprs);

        برائے (ثابت Cover_اظہاریہ& l : lhsExprs) {
            برائے (ثابت Cover_اظہاریہ& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::سیٹ<AstVar*>, 2> varps;
                std::array<std::سیٹ<std::string>, 2> strs;

                UASSERT_OBJ(!l.خالی_خانے() && !r.خالی_خانے(), nodep, "Empty coverage expression branch");
                Cover_اظہاریہ اظہاریہ;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                برائے (ثابت Cover_رکن& رکن : l) {
                    اگر (ثابت AstVarRef* ثابت refp = VN_CAST(رکن.m_exprp, VarRef)) {
                        varps[رکن.m_objective].داخل_کرو(refp->varp());
                    } ورنہ {
                        strs[رکن.m_objective].داخل_کرو(رکن.m_emitV);
                    }
                    اظہاریہ.ڈالو_عقب(رکن);
                }
                منطقی impossible = جھوٹ;
                برائے (ثابت Cover_رکن& رکن : r) {
                    منطقی redundant = جھوٹ;
                    اگر (ثابت Ast_گرہ_Var_Ref* ثابت refp = VN_CAST(رکن.m_exprp, گرہ_Var_Ref)) {
                        اگر (varps[رکن.m_objective].ڈھونڈو(refp->varp())
                            != varps[رکن.m_objective].انتہا())
                            redundant = سچ;
                        اگر (varps[!رکن.m_objective].ڈھونڈو(refp->varp())
                            != varps[!رکن.m_objective].انتہا())
                            impossible = سچ;
                    } ورنہ {
                        اگر (strs[رکن.m_objective].ڈھونڈو(رکن.m_emitV)
                            != strs[رکن.m_objective].انتہا())
                            redundant = سچ;
                        اگر (strs[!رکن.m_objective].ڈھونڈو(رکن.m_emitV)
                            != strs[!رکن.m_objective].انتہا())
                            impossible = سچ;
                    }
                    اگر (!redundant) اظہاریہ.ڈالو_عقب(رکن);
                }
                اگر (!impossible) m_exprs.ڈالو_عقب(std::move(اظہاریہ));
                اگر (checkMaxExprs()) واپس;
            }
        }
    }

    خالی or_اظہاریہ(Ast_گرہ_Biop* nodep) {
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } ورنہ اگر (m_objective) {
            اظہاریہ_Either(nodep);
        } ورنہ {
            اظہاریہ_Both(nodep);
        }
        سطر_Track(nodep);
    }
    خالی دورہ(AstLogOr* nodep) override { or_اظہاریہ(nodep); }
    خالی دورہ(AstOr* nodep) override { or_اظہاریہ(nodep); }

    خالی and_اظہاریہ(Ast_گرہ_Biop* nodep) {
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } ورنہ اگر (m_objective) {
            اظہاریہ_Both(nodep);
        } ورنہ {
            اظہاریہ_Either(nodep);
        }
        سطر_Track(nodep);
    }
    خالی دورہ(AstLogAnd* nodep) override { and_اظہاریہ(nodep); }
    خالی دورہ(AstAnd* nodep) override { and_اظہاریہ(nodep); }

    خالی xor_اظہاریہ(Ast_گرہ_Biop* nodep) {
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } ورنہ {
            برائے (ثابت منطقی lObjective : {جھوٹ, سچ}) {
                CoverExprs پچھلا_Exprs;
                m_exprs.تبادلہ(پچھلا_Exprs);
                ثابت منطقی rObjective = lObjective ^ m_objective;
                اظہاریہ_Both(nodep, سچ, lObjective, rObjective);
                m_exprs.splice(m_exprs.انتہا(), پچھلا_Exprs);
                اگر (checkMaxExprs()) توڑو;
            }
        }
        سطر_Track(nodep);
    }
    خالی دورہ(AstXor* nodep) override { xor_اظہاریہ(nodep); }

    خالی اظہاریہ_Not(Ast_گرہ_اظہاریہ* nodep) {
        VL_RESTORER(m_objective);
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } ورنہ {
            m_objective = !m_objective;
            iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    خالی دورہ(AstNot* nodep) override { اظہاریہ_Not(nodep); }
    خالی دورہ(AstLogNot* nodep) override { اظہاریہ_Not(nodep); }

    سانچہ <قسم_نام T_Oper>
    خالی اظہاریہ_Reduce(Ast_گرہ_Uniop* nodep) {
        اگر (m_seeking != ABORTED) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
            ثابت صحیح چوڑائی = lhsp->dtypep()->چوڑائی();
            ثابت سائز_t expected = std::is_same<T_Oper, AstXor>::قدر ? 0x1 << چوڑائی : چوڑائی + 1;
            اگر (checkMaxExprs(expected)) واپس;
            Ast_گرہ_اظہاریہ* unrolledp = نیا AstSel{
                fl, lhsp->clone_درخت(جھوٹ), نیا AstConst{fl, static_cast<uint32_t>(چوڑائی - 1)}, 1};
            برائے (صحیح ثنائی_ہندسہ_2 = چوڑائی - 2; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
                AstSel* ثابت selp = نیا AstSel{fl, lhsp->clone_درخت(جھوٹ),
                                                نیا AstConst{fl, static_cast<uint32_t>(ثنائی_ہندسہ_2)}, 1};
                unrolledp = نیا T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(ڈالو_Deletep(unrolledp), unrolledp);
        } ورنہ {
            iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    خالی دورہ(AstRedOr* nodep) override { اظہاریہ_Reduce<AstOr>(nodep); }
    خالی دورہ(AstRedAnd* nodep) override { اظہاریہ_Reduce<AstAnd>(nodep); }
    خالی دورہ(AstRedXor* nodep) override { اظہاریہ_Reduce<AstXor>(nodep); }

    خالی دورہ(AstLogIf* nodep) override {
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } ورنہ اگر (m_objective) {
            اظہاریہ_Either(nodep, سچ, جھوٹ, سچ);
        } ورنہ {
            اظہاریہ_Both(nodep, سچ, سچ, جھوٹ);
        }
        سطر_Track(nodep);
    }

    خالی دورہ(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } ورنہ {
            m_objective = !m_objective;
            xor_اظہاریہ(nodep);
            سطر_Track(nodep);
        }
    }

    خالی دورہ(AstFuncRef* nodep) override {
        اگر (nodep->taskp()->lifetime().isAutomatic()) {
            دورہ(static_cast<Ast_گرہ_اظہاریہ*>(nodep));
        } ورنہ {
            اظہاریہ_Unsupported(nodep, "non-automatic function");
        }
    }

    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {
        اگر (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } ورنہ {
            اظہاریہ_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            اگر (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_درخت(nodep, emitV);
                // Add new expression with a single term
                Cover_اظہاریہ اظہاریہ;
                اظہاریہ.emplace_عقب(nodep, m_objective, emitV.str());
                m_exprs.ڈالو_عقب(std::move(اظہاریہ));
                checkMaxExprs();
            } ورنہ {
                اظہاریہ_Unsupported(nodep, "not coverage eligible");
            }
        }
        سطر_Track(nodep);
    }

    // VISITORS - BOTH
    خالی دورہ(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_اندر_Toggle_Off);
        m_state.m_on = جھوٹ;
        m_اندر_Toggle_Off = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_اندر_Toggle_Off);
        m_state.m_on = جھوٹ;
        m_اندر_Toggle_Off = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    خالی اظہاریہ_Unsupported(Ast_گرہ* nodep, ثابت string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ثابت منطقی wasSeeking = m_seeking == SEEKING;
        ثابت Objective oldSeeking = m_seeking;
        اگر (wasSeeking) abort_اظہاریہ_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        سطر_Track(nodep);
        اگر (!wasSeeking) m_seeking = oldSeeking;
    }

عوامی:
    // CONSTRUCTORS
    صریح CoverageVisitor(AstNetlist* rootp)
        : m_اظہاریہ_عارضی_نام{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = طےشدہ;
};

//######################################################################
// Coverage class functions

خالی V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("coverage", 0, dump_درخت_Either_Level() >= 3);
}
