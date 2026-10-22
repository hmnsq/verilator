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

رده عبارت_Coverage_Eligible_Visitor final : عمومی VNVisitorConst {
    // STATE
    منطقی m_eligible = درست;

    ایستا منطقی elemDTypeEligible(ثابت Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (ثابت Ast_گره_DType* ثابت dtp = dtypep->virtRefDTypep()) {
            اگر (!elemDTypeEligible(dtp)) بازگشت نادرست;
        }
        اگر (ثابت Ast_گره_DType* ثابت dtp = dtypep->virtRefDType2p()) {
            اگر (!elemDTypeEligible(dtp)) بازگشت نادرست;
        }
        بازگشت !VN_IS(dtypep, ClassRefDType);
    }

    تهی visit(Ast_گره_Var_Ref* nodep) override {
        ثابت Ast_گره_DType* ثابت dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        اگر (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } وگرنه {
            m_eligible = نادرست;
        }
    }

    تهی visit(Ast_گره* nodep) override {
        اگر (!nodep->is_عبارت_Coverage_Eligible()) {
            m_eligible = نادرست;
        } وگرنه {
            iterateChildrenConst(nodep);
        }
    }

عمومی:
    // CONSTRUCTORS
    explicit عبارت_Coverage_Eligible_Visitor(Ast_گره* nodep) { iterateConst(nodep); }
    ~عبارت_Coverage_Eligible_Visitor() override = پیشفرض;

    منطقی eligible() ثابت { بازگشت m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

رده CoverageVisitor final : عمومی VNVisitor {
    // TYPES
    using LinenoSet = std::set<صحیح>;

    ساختار CoverTerm final {
        Ast_گره_عبارت* m_exprp;  // Expression branch term
        منطقی m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(Ast_گره_عبارت* exprp, منطقی objective, ثابت string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using Cover_عبارت = std::deque<CoverTerm>;
    using CoverExprs = std::list<Cover_عبارت>;

    ساختار ToggleEnt final {
        ثابت string m_comment;  // Comment for coverage dump
        Ast_گره_عبارت* m_varRefp;  // How to get to this element
        Ast_گره_عبارت* m_chgRefp;  // How to get to this element
        ToggleEnt(ثابت string& comment, Ast_گره_عبارت* vp, Ast_گره_عبارت* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = پیشفرض;
        تهی cleanup() {
            VL_DO_CLEAR(m_varRefp->delete_درخت(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->delete_درخت(), m_chgRefp = nullptr);
        }
    };

    ساختار CheckState final {  // State save-restored on each new coverage scope/block
        منطقی m_on = نادرست;  // Should this block get covered?
        منطقی m_inModOff = نادرست;  // In module with no coverage
        صحیح m_handle = 0;  // Opaque handle for index into line tracking
        ثابت Ast_گره* m_nodep = nullptr;  // Node establishing this state
        CheckState() = پیشفرض;
        منطقی coverageOn(ثابت Ast_گره* nodep) ثابت {
            بازگشت m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        منطقی سطر_Coverage_On(ثابت Ast_گره* nodep) ثابت {
            بازگشت coverageOn(nodep) && v3Global.opt.coverage_سطر();
        }
        منطقی branchCoverageOn(ثابت Ast_گره* nodep) ثابت {
            بازگشت coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        منطقی عبارت_Coverage_On(ثابت Ast_گره* nodep) ثابت {
            بازگشت coverageOn(nodep) && v3Global.opt.coverage_عبارت();
        }
    };

    شمارش Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    V3UniqueNames m_عبارت_Temp_Names;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<Ast_گره_عبارت*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    صحیح m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    Ast_گره_Module* m_modp = nullptr;  // Current module to add statement to
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گره* m_عبارت_Stmtsp = nullptr;  // Node to add expr coverage to
    منطقی m_then = نادرست;  // Whether we're iterating the then or else branch
                          // when m_exprStmtps is an AstIf
    CoverExprs m_exprs;  // List of expressions that can reach objective
    Objective m_seeking = NONE;  // Seeking objective for expression coverage
    منطقی m_objective = نادرست;  // Expression objective
    منطقی m_ifCond = نادرست;  // Visiting if condition
    منطقی m_inToggleOff = نادرست;  // In function/task etc
    string m_beginHier;  // AstBegin hier name for user coverage points

    // STATE - cleared each module
    std::unordered_map<std::string, uint32_t> m_varnames;  // Uniquify inserted variable names
    std::unordered_map<صحیح, LinenoSet> m_handle_سطرها;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    ثابت نویسه* varIgnoreToggle(ثابت AstVar* nodep) {
        ثابت منطقی cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        اگر (!cover) بازگشت "Not relevant signal";
        اگر (nodep->isConst()) بازگشت "Signal is constant";
        اگر (nodep->isDouble()) بازگشت "Signal is double";
        اگر (nodep->isString()) بازگشت "Signal is string";
        اگر (!v3Global.opt.coverageUnderscore()) {
            ثابت string prettyName = nodep->prettyName();
            اگر (prettyName[0] == '_') بازگشت "Leading underscore";
            اگر (prettyName.find("._") != string::npos) بازگشت "Inlined leading underscore";
        }
        اگر ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            بازگشت "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        بازگشت nullptr;
    }

    AstCoverInc* newCoverInc(File_سطر* fl, Ast_گره_Cover_Decl* ثابت declp,
                             ثابت string& trace_var_name) {
        AstCoverInc* ثابت incp = جدید AstCoverInc{fl, declp};
        اگر (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            File_سطر* ثابت fl_nowarn = جدید File_سطر{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, درست);
            AstVar* ثابت varp = جدید AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(درست);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* ثابت assp = جدید AstAssign{
                incp->fileline(), جدید AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                جدید AstAdd{incp->fileline(), جدید AstVarRef{incp->fileline(), varp, VAccess::READ},
                           جدید AstConst{incp->fileline(), AstConst::Widthed_مقدار{}, 32, 1}}};
            Ast_گره::addNext<Ast_گره, Ast_گره>(incp, assp);
        }
        بازگشت incp;
    }
    string trace_Name_For_سطر(ثابت Ast_گره* nodep, ثابت string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        اگر (ثابت uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        بازگشت name;
    }

    // Line tracking
    تهی createHandle(ثابت Ast_گره* nodep) {
        // Start tracking lines for the given handling node
        // If and if's else have separate handles for same nodep,
        // so nodep cannot have a pointer to a unique handle
        m_state.m_on = درست;
        m_state.m_handle = ++m_nextHandle;
        // Ensure line numbers we track are in the same file as this block
        // so track via nodep
        m_state.m_nodep = nodep;
        UINFO(9, "line create h" << m_state.m_handle << " " << nodep);
    }
    تهی سطر_Track(ثابت Ast_گره* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        اگر ((m_state.سطر_Coverage_On(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            برای (صحیح lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handle_سطرها[m_state.m_handle].درج(lineno);
            }
        }
    }
    ایستا string سطرها_First_Last(ثابت صحیح first, ثابت صحیح last) {
        اگر (first && first == last) {
            بازگشت cvtToStr(first);
        } وگرنه اگر (first && last) {
            بازگشت cvtToStr(first) + "-" + cvtToStr(last);
        } وگرنه {
            بازگشت "";
        }
    }
    string سطرها_Cov(ثابت CheckState& state, ثابت Ast_گره* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        ثابت LinenoSet& سطرها = m_handle_سطرها[state.m_handle];
        صحیح first = 0;
        صحیح last = 0;
        برای (ثابت صحیح linen : سطرها) {
            اگر (!first) {
                first = last = linen;
            } وگرنه اگر (linen == last + 1) {
                ++last;
            } وگرنه {
                اگر (!out.empty()) out += ",";
                out += سطرها_First_Last(first, last);
                first = last = linen;
            }
        }
        اگر (first) {
            اگر (!out.empty()) out += ",";
            out += سطرها_First_Last(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        بازگشت out;
    }

    // VISITORS - BOTH
    تهی visit(Ast_گره_Module* nodep) override {
        ثابت Ast_گره_Module* ثابت origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_عبارت_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->is_بالاترین();  // Already made top shell, no coverage for it
        اگر (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handle_سطرها.clear();
        }
        iterateChildren(nodep);
    }
    تهی visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_عبارت_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    تهی visit(AstAlways* nodep) override {
        اگر (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_عبارت_Stmtsp);
            VL_RESTORER(m_inToggleOff);
            m_عبارت_Stmtsp = nodep;
            m_inToggleOff = درست;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            بازگشت;
        }
        iterateProcedure(nodep);
    }
    تهی visit(Ast_گره_Procedure* nodep) override { iterateProcedure(nodep); }
    تهی visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_عبارت_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_عبارت_Stmtsp = nodep;
        m_inToggleOff = درست;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        اگر (m_state.سطر_Coverage_On(nodep)) {
            AstCoverOtherDecl* ثابت declp
                = جدید AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطرها_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_گره* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            درج_Proc_Statement(nodep, newp);
        }
    }
    تهی visit(AstLoopTest* nodep) override {
        اگر (nodep->user2SetOnce()) بازگشت;
        سطر_Track(nodep);
        اگر (m_state.سطر_Coverage_On(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* ثابت declp
                = جدید AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطرها_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_گره* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_عبارت_Temp_Names);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        اگر (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    تهی درج_Proc_Statement(Ast_گره* nodep, Ast_گره* stmtp) {
        اگر (Ast_گره_Procedure* ثابت itemp = VN_CAST(nodep, گره_Procedure)) {
            itemp->addStmtsp(stmtp);
        } وگرنه اگر (Ast_گره_FTask* ثابت itemp = VN_CAST(nodep, گره_FTask)) {
            itemp->addStmtsp(stmtp);
        } وگرنه اگر (AstLoop* ثابت itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } وگرنه اگر (AstIf* ثابت itemp = VN_CAST(nodep, If)) {
            اگر (m_then) {
                itemp->addThensp(stmtp);
            } وگرنه {
                itemp->addElsesp(stmtp);
            }
        } وگرنه اگر (AstBegin* ثابت itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } وگرنه {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    تهی iterateProcedure(Ast_گره* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_عبارت_Stmtsp);
        VL_RESTORER(m_inToggleOff);
        m_عبارت_Stmtsp = nodep;
        m_inToggleOff = درست;
        createHandle(nodep);
        iterateChildren(nodep);
        اگر (m_state.سطر_Coverage_On(nodep)) {
            سطر_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جدید AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", سطرها_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            Ast_گره* ثابت newp
                = newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "block"));
            درج_Proc_Statement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    تهی visit(AstVar* nodep) override {
        iterateChildren(nodep);
        اگر (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            ثابت نویسه* ثابت disablep = varIgnoreToggle(nodep);
            اگر (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } وگرنه {
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
                File_سطر* ثابت fl_nowarn = جدید File_سطر{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, درست);
                AstVar* ثابت chgVarp
                    = جدید AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, جدید AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 جدید AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    تهی toggleVarBottom(ثابت ToggleEnt& above, ثابت AstVar* varp, ثابت VNumRange& range) {
        ثابت std::string hierPrefix
            = (m_beginHier != "") ? Ast_گره::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* ثابت declp
            = جدید AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* ثابت newp = جدید AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->clone_درخت(نادرست), above.m_chgRefp->clone_درخت(نادرست)};
        m_modp->addStmtsp(newp);
    }

    تهی toggleVarRecurse(ثابت Ast_گره_DType* ثابت dtypep, ثابت صحیح depth,  // per-iteration
                          ثابت ToggleEnt& above, ثابت AstVar* ثابت varp) {  // Constant
        اگر (ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } وگرنه اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            برای (ثابت صحیح index_docs : adtypep->declRange().seqLoToHi()) {
                ثابت صحیح index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 جدید AstArraySel{varp->fileline(),
                                                 above.m_varRefp->clone_درخت(نادرست), index_code},
                                 جدید AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->clone_درخت(نادرست), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } وگرنه اگر (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(dtypep, PackArrayDType)) {
            برای (ثابت صحیح index_docs : adtypep->declRange().seqLoToHi()) {
                ثابت Ast_گره_DType* ثابت subtypep = adtypep->subDTypep()->skipRefp();
                ثابت صحیح index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 جدید AstSel{varp->fileline(), above.m_varRefp->clone_درخت(نادرست),
                                            index_code * subtypep->width(), subtypep->width()},
                                 جدید AstSel{varp->fileline(), above.m_chgRefp->clone_درخت(نادرست),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } وگرنه اگر (ثابت AstStructDType* ثابت adtypep = VN_CAST(dtypep, StructDType)) {
            اگر (adtypep->packed()) {
                برای (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    ثابت صحیح index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        جدید AstSel{varp->fileline(), above.m_varRefp->clone_درخت(نادرست), index_code,
                                   subtypep->width()},
                        جدید AstSel{varp->fileline(), above.m_chgRefp->clone_درخت(نادرست), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } وگرنه {
                برای (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                    Ast_گره_عبارت* ثابت varRefp = جدید AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_درخت(نادرست), itemp->name()};
                    Ast_گره_عبارت* ثابت chgRefp = جدید AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_درخت(نادرست), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وگرنه اگر (ثابت AstUnionDType* ثابت adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            اگر (ثابت AstMemberDType* ثابت itemp = adtypep->membersp()) {
                Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skipRefp();
                اگر (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->clone_درخت(نادرست),
                                     above.m_chgRefp->clone_درخت(نادرست)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } وگرنه {
                    Ast_گره_عبارت* ثابت varRefp = جدید AstStructSel{
                        varp->fileline(), above.m_varRefp->clone_درخت(نادرست), itemp->name()};
                    Ast_گره_عبارت* ثابت chgRefp = جدید AstStructSel{
                        varp->fileline(), above.m_chgRefp->clone_درخت(نادرست), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } وگرنه اگر (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } وگرنه {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    منطقی includeCondToBranchRecursive(ثابت Ast_گره* ثابت nodep) {
        ثابت Ast_گره* ثابت backp = nodep->backp();
        اگر (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            بازگشت includeCondToBranchRecursive(backp);
        } وگرنه اگر (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            بازگشت includeCondToBranchRecursive(backp);
        } وگرنه اگر (VN_IS(backp, گره_Assign) && VN_AS(backp, گره_Assign)->rhsp() == nodep) {
            بازگشت درست;
        }
        بازگشت نادرست;
    }

    // VISITORS - LINE COVERAGE
    تهی visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        اگر (m_seeking == NONE) coverExprs(nodep->condp());

        اگر (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            ثابت CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            سطر_Track(nodep);
            Ast_گره_عبارت* ثابت thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* ثابت thenDeclp
                = جدید AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", سطرها_Cov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(جدید Ast_عبارت_Stmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, trace_Name_For_سطر(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            Ast_گره_عبارت* ثابت elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* ثابت elseDeclp
                = جدید AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", سطرها_Cov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(جدید Ast_عبارت_Stmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_سطر(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } وگرنه {
            سطر_Track(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    تهی visit(AstIf* nodep) override {
        اگر (nodep->user2()) بازگشت;

        UINFO(4, " IF: " << nodep);
        اگر (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            ثابت منطقی elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            اگر (elsif) VN_AS(nodep->elsesp(), If)->user1(درست);
            ثابت منطقی first_elsif = !nodep->user1() && elsif;
            ثابت منطقی cont_elsif = nodep->user1() && elsif;
            ثابت منطقی final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            ثابت CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_عبارت_Stmtsp);
                VL_RESTORER(m_then);
                m_عبارت_Stmtsp = nodep;
                m_then = درست;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                سطر_Track(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_عبارت_Stmtsp);
                VL_RESTORER(m_then);
                m_عبارت_Stmtsp = nodep;
                m_then = نادرست;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            اگر (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* ثابت ifDeclp
                    = جدید AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", سطرها_Cov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_سطر(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* ثابت elseDeclp
                    = جدید AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", سطرها_Cov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, trace_Name_For_سطر(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            وگرنه اگر (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                اگر (ifState.سطر_Coverage_On(nodep)) {
                    AstCoverOtherDecl* ثابت elsifDeclp = جدید AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        سطرها_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 trace_Name_For_سطر(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } وگرنه {
                // Cover as separate blocks (not a branch as is not two-legged)
                اگر (ifState.سطر_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* ثابت ifDeclp = جدید AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        سطرها_Cov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, trace_Name_For_سطر(nodep, "if")));
                }
                اگر (elseState.سطر_Coverage_On(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* ثابت elseDeclp = جدید AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        سطرها_Cov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 trace_Name_For_سطر(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = درست;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    تهی visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        اگر (m_state.سطر_Coverage_On(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            اگر (m_state.سطر_Coverage_On(nodep)) {  // if the case body didn't disable it
                سطر_Track(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* ثابت declp
                    = جدید AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", سطرها_Cov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, trace_Name_For_سطر(nodep, "case")));
            }
        }
    }
    تهی visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = درست;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        اگر (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            سطر_Track(nodep);
            AstCoverOtherDecl* ثابت declp
                = جدید AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", سطرها_Cov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    تهی visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_عبارت_Stmtsp);
        m_عبارت_Stmtsp = nullptr;
        iterateChildren(nodep);
    }
    تهی visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = نادرست;
    }
    تهی visit(AstStmtPragma* nodep) override {
        اگر (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = نادرست;
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه {
            اگر (m_state.m_on) iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    تهی visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        اگر (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    تهی visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_عبارت_Stmtsp);
        m_عبارت_Stmtsp = nodep;
        m_inToggleOff = درست;
        اگر (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    تهی abort_عبارت_Coverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        اگر (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    منطقی checkMaxExprs(size_t additional = 0) {
        اگر (m_seeking != ABORTED
            && static_cast<صحیح>(m_exprs.size() + additional) <= v3Global.opt.coverage_عبارت_Max())
            بازگشت نادرست;
        abort_عبارت_Coverage();
        بازگشت درست;
    }

    تهی add_عبارت_Cover_Inc(Ast_گره_عبارت* nodep, صحیح start = 0) {
        File_سطر* ثابت fl = nodep->fileline();
        صحیح شمار = start;
        برای (Cover_عبارت& عبارت : m_exprs) {
            ثابت string name = "expr_" + std::to_string(شمار);
            string comment = "(";
            منطقی first = درست;
            Ast_گره_عبارت* condp = nullptr;
            برای (CoverTerm& term : عبارت) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                Ast_گره_عبارت* covExprp = nullptr;
                اگر (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    Ast_گره_عبارت* ثابت frefp = term.m_exprp;
                    Ast_گره_DType* ثابت dtypep = frefp->dtypep();
                    ثابت auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    اگر (pair.second) {
                        varp = جدید AstVar{fl, VVarType::MODULETEMP, m_عبارت_Temp_Names.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        اگر (m_ftaskp) {
                            varp->funcLocal(درست);
                            m_ftaskp->prependStmtsp(varp);
                        } وگرنه {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(جدید Ast_عبارت_Stmt{
                            fl, جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            جدید AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = جدید AstVarRef{fl, varp, VAccess::READ};
                } وگرنه {
                    covExprp = term.m_exprp->clone_درخت(نادرست);
                }
                Ast_گره_عبارت* ثابت termp
                    = term.m_objective ? covExprp : جدید AstLogNot{fl, covExprp};
                اگر (condp) {
                    condp = جدید AstLogAnd{fl, condp, termp};
                } وگرنه {
                    condp = termp;
                }
                first = نادرست;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* ثابت declp = جدید AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            Ast_گره* ثابت newp = newCoverInc(fl, declp, trace_Name_For_سطر(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* ثابت ifp = جدید AstIf{fl, condp, newp, nullptr};
            ifp->user2(درست);
            درج_Proc_Statement(m_عبارت_Stmtsp, ifp);
            ++شمار;
        }
    }

    تهی coverExprs(Ast_گره_عبارت* nodep) {
        اگر (!m_state.عبارت_Coverage_On(nodep) || nodep->dtypep()->width() != 1 || !m_عبارت_Stmtsp) {
            بازگشت;
        }

        UASSERT_OBJ(m_seeking == NONE, nodep, "recursively covering expressions is not expected");
        UASSERT_OBJ(m_exprs.empty(), nodep, "unexpected expression coverage garbage");
        VL_RESTORER(m_seeking);
        VL_RESTORER(m_objective);
        VL_RESTORER_CLEAR(m_exprs);  // Already asserted above it's empty.

        m_seeking = SEEKING;
        m_objective = نادرست;
        iterate(nodep);
        CoverExprs falseExprs;
        m_exprs.swap(falseExprs);

        m_objective = درست;
        iterate(nodep);
        اگر (checkMaxExprs(falseExprs.size())) بازگشت;
        اگر (m_seeking == ABORTED) بازگشت;

        add_عبارت_Cover_Inc(nodep);
        ثابت صحیح start = m_exprs.size();
        m_objective = نادرست;
        m_exprs.swap(falseExprs);
        add_عبارت_Cover_Inc(nodep, start);
    }

    تهی عبارت_Either(Ast_گره_Biop* nodep, منطقی overrideObjective = نادرست, منطقی lObjective = نادرست,
                    منطقی rObjective = نادرست) {
        VL_RESTORER(m_objective);
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();

        اگر (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        اگر (checkMaxExprs()) بازگشت;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        اگر (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    تهی عبارت_Both(Ast_گره_Biop* nodep, منطقی overrideObjective = نادرست, منطقی lObjective = نادرست,
                  منطقی rObjective = نادرست) {
        VL_RESTORER(m_objective);
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();

        اگر (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        اگر (checkMaxExprs()) بازگشت;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        اگر (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        اگر (checkMaxExprs()) بازگشت;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        برای (ثابت Cover_عبارت& l : lhsExprs) {
            برای (ثابت Cover_عبارت& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                Cover_عبارت عبارت;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                برای (ثابت CoverTerm& term : l) {
                    اگر (ثابت AstVarRef* ثابت refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].درج(refp->varp());
                    } وگرنه {
                        strs[term.m_objective].درج(term.m_emitV);
                    }
                    عبارت.افزودن_back(term);
                }
                منطقی impossible = نادرست;
                برای (ثابت CoverTerm& term : r) {
                    منطقی redundant = نادرست;
                    اگر (ثابت Ast_گره_Var_Ref* ثابت refp = VN_CAST(term.m_exprp, گره_Var_Ref)) {
                        اگر (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = درست;
                        اگر (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = درست;
                    } وگرنه {
                        اگر (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = درست;
                        اگر (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = درست;
                    }
                    اگر (!redundant) عبارت.افزودن_back(term);
                }
                اگر (!impossible) m_exprs.افزودن_back(std::move(عبارت));
                اگر (checkMaxExprs()) بازگشت;
            }
        }
    }

    تهی or_عبارت(Ast_گره_Biop* nodep) {
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } وگرنه اگر (m_objective) {
            عبارت_Either(nodep);
        } وگرنه {
            عبارت_Both(nodep);
        }
        سطر_Track(nodep);
    }
    تهی visit(AstLogOr* nodep) override { or_عبارت(nodep); }
    تهی visit(AstOr* nodep) override { or_عبارت(nodep); }

    تهی and_عبارت(Ast_گره_Biop* nodep) {
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } وگرنه اگر (m_objective) {
            عبارت_Both(nodep);
        } وگرنه {
            عبارت_Either(nodep);
        }
        سطر_Track(nodep);
    }
    تهی visit(AstLogAnd* nodep) override { and_عبارت(nodep); }
    تهی visit(AstAnd* nodep) override { and_عبارت(nodep); }

    تهی xor_عبارت(Ast_گره_Biop* nodep) {
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } وگرنه {
            برای (ثابت منطقی lObjective : {نادرست, درست}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                ثابت منطقی rObjective = lObjective ^ m_objective;
                عبارت_Both(nodep, درست, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                اگر (checkMaxExprs()) بشکن;
            }
        }
        سطر_Track(nodep);
    }
    تهی visit(AstXor* nodep) override { xor_عبارت(nodep); }

    تهی عبارت_Not(Ast_گره_عبارت* nodep) {
        VL_RESTORER(m_objective);
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } وگرنه {
            m_objective = !m_objective;
            iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    تهی visit(AstNot* nodep) override { عبارت_Not(nodep); }
    تهی visit(AstLogNot* nodep) override { عبارت_Not(nodep); }

    template <typename T_Oper>
    تهی عبارت_Reduce(Ast_گره_Uniop* nodep) {
        اگر (m_seeking != ABORTED) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
            ثابت صحیح width = lhsp->dtypep()->width();
            ثابت size_t expected = std::is_same<T_Oper, AstXor>::مقدار ? 0x1 << width : width + 1;
            اگر (checkMaxExprs(expected)) بازگشت;
            Ast_گره_عبارت* unrolledp = جدید AstSel{
                fl, lhsp->clone_درخت(نادرست), جدید AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            برای (صحیح bit = width - 2; bit >= 0; bit--) {
                AstSel* ثابت selp = جدید AstSel{fl, lhsp->clone_درخت(نادرست),
                                                جدید AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = جدید T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(افزودن_Deletep(unrolledp), unrolledp);
        } وگرنه {
            iterateChildren(nodep);
            سطر_Track(nodep);
        }
    }
    تهی visit(AstRedOr* nodep) override { عبارت_Reduce<AstOr>(nodep); }
    تهی visit(AstRedAnd* nodep) override { عبارت_Reduce<AstAnd>(nodep); }
    تهی visit(AstRedXor* nodep) override { عبارت_Reduce<AstXor>(nodep); }

    تهی visit(AstLogIf* nodep) override {
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } وگرنه اگر (m_objective) {
            عبارت_Either(nodep, درست, نادرست, درست);
        } وگرنه {
            عبارت_Both(nodep, درست, درست, نادرست);
        }
        سطر_Track(nodep);
    }

    تهی visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        اگر (m_seeking == NONE) {
            coverExprs(nodep);
        } وگرنه {
            m_objective = !m_objective;
            xor_عبارت(nodep);
            سطر_Track(nodep);
        }
    }

    تهی visit(AstFuncRef* nodep) override {
        اگر (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<Ast_گره_عبارت*>(nodep));
        } وگرنه {
            عبارت_Unsupported(nodep, "non-automatic function");
        }
    }

    تهی visit(Ast_گره_عبارت* nodep) override {
        اگر (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } وگرنه {
            عبارت_Coverage_Eligible_Visitor elgibleVisitor(nodep);
            اگر (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilog_For_درخت(nodep, emitV);
                // Add new expression with a single term
                Cover_عبارت عبارت;
                عبارت.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.افزودن_back(std::move(عبارت));
                checkMaxExprs();
            } وگرنه {
                عبارت_Unsupported(nodep, "not coverage eligible");
            }
        }
        سطر_Track(nodep);
    }

    // VISITORS - BOTH
    تهی visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = نادرست;
        m_inToggleOff = درست;
        iterateChildren(nodep);
    }
    تهی visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = نادرست;
        m_inToggleOff = درست;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره* nodep) override {
        iterateChildren(nodep);
        سطر_Track(nodep);
    }

    تهی عبارت_Unsupported(Ast_گره* nodep, ثابت string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        ثابت منطقی wasSeeking = m_seeking == SEEKING;
        ثابت Objective oldSeeking = m_seeking;
        اگر (wasSeeking) abort_عبارت_Coverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        سطر_Track(nodep);
        اگر (!wasSeeking) m_seeking = oldSeeking;
    }

عمومی:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_عبارت_Temp_Names{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = پیشفرض;
};

//######################################################################
// Coverage class functions

تهی V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("coverage", 0, dump_درخت_Either_Level() >= 3);
}
