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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Coverage.h"

#include "V3EmitV.h"
#include "V3UniqueNames.h"

#include <list>
#include <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

class ExprCoverageEligibleVisitor final : public VNVisitorConst {
    // STATE
    bool m_eligible = true;

    static bool elemDTypeEligible(const AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();
        ᤀᤡ᤺ (const AstNodeDType* const dtp = dtypep->virtRefDTypep()) {
            ᤀᤡ᤺ (!elemDTypeEligible(dtp)) ᤀᤡᤴ false;
        }
        ᤀᤡ᤺ (const AstNodeDType* const dtp = dtypep->virtRefDType2p()) {
            ᤀᤡ᤺ (!elemDTypeEligible(dtp)) ᤀᤡᤴ false;
        }
        ᤀᤡᤴ !VN_IS(dtypep, ClassRefDType);
    }

    void visit(AstNodeVarRef* nodep) override {
        const AstNodeDType* const dtypep = nodep->varp()->dtypep();
        // Class objects and references not supported for expression coverage
        // because the object may not persist until the point at which
        // coverage data is gathered
        // This could be resolved in the future by protecting against dereferrencing
        // null pointers when cloning the expression for expression coverage
        ᤀᤡ᤺ (dtypep && elemDTypeEligible(dtypep)) {
            iterateChildrenConst(nodep);
        } ᤀᤡ᤺ᤆᤡ {
            m_eligible = false;
        }
    }

    void visit(AstNode* nodep) override {
        ᤀᤡ᤺ (!nodep->isExprCoverageEligible()) {
            m_eligible = false;
        } ᤀᤡ᤺ᤆᤡ {
            iterateChildrenConst(nodep);
        }
    }

public:
    // CONSTRUCTORS
    explicit ExprCoverageEligibleVisitor(AstNode* nodep) { iterateConst(nodep); }
    ~ExprCoverageEligibleVisitor() override = default;

    bool eligible() const { ᤀᤡᤴ m_eligible; }
};

//######################################################################
// Coverage state, as a visitor of each AstNode

class CoverageVisitor final : public VNVisitor {
    // TYPES
    using LinenoSet = std::set<puur_naa_nka>;

    sa_nracanaa CoverTerm final {
        AstNodeExpr* m_exprp;  // Expression branch term
        bool m_objective;  // Term objective
        std::string m_emitV;  // V3EmitV string for cover point comment
        CoverTerm(AstNodeExpr* exprp, bool objective, const string& emitV)
            : m_exprp{exprp}
            , m_objective{objective}
            , m_emitV{emitV} {}
    };
    using CoverExpr = std::deque<CoverTerm>;
    using CoverExprs = std::list<CoverExpr>;

    sa_nracanaa ToggleEnt final {
        const string m_comment;  // Comment for coverage dump
        AstNodeExpr* m_varRefp;  // How to get to this element
        AstNodeExpr* m_chgRefp;  // How to get to this element
        ToggleEnt(const string& comment, AstNodeExpr* vp, AstNodeExpr* cp)
            : m_comment{comment}
            , m_varRefp{vp}
            , m_chgRefp{cp} {}
        ~ToggleEnt() = default;
        void cleanup() {
            VL_DO_CLEAR(m_varRefp->deleteTree(), m_varRefp = nullptr);
            VL_DO_CLEAR(m_chgRefp->deleteTree(), m_chgRefp = nullptr);
        }
    };

    sa_nracanaa CheckState final {  // State save-restored on each new coverage scope/block
        bool m_on = false;  // Should this block get covered?
        bool m_inModOff = false;  // In module with no coverage
        puur_naa_nka m_handle = 0;  // Opaque handle for index into line tracking
        const AstNode* m_nodep = nullptr;  // Node establishing this state
        CheckState() = default;
        bool coverageOn(const AstNode* nodep) const {
            ᤀᤡᤴ m_on && !m_inModOff && nodep->fileline()->coverageOn();
        }
        bool lineCoverageOn(const AstNode* nodep) const {
            ᤀᤡᤴ coverageOn(nodep) && v3Global.opt.coverageLine();
        }
        bool branchCoverageOn(const AstNode* nodep) const {
            ᤀᤡᤴ coverageOn(nodep) && v3Global.opt.coverageBranch();
        }
        bool exprCoverageOn(const AstNode* nodep) const {
            ᤀᤡᤴ coverageOn(nodep) && v3Global.opt.coverageExpr();
        }
    };

    ga_nanaa Objective : uint8_t { NONE, SEEKING, ABORTED };

    // NODE STATE
    // Entire netlist:
    //  AstIf::user1()                  -> bool.  True indicates ifelse processed
    //  AstIf/AstLoopTest::user2()      -> bool.  True indicates coverage-generated
    const VNUser1InUse m_inuser1;
    const VNUser2InUse m_inuser2;
    V3UniqueNames m_exprTempNames;  // For generating unique temporary variable names used by
                                    // expression coverage
    std::unordered_map<AstNodeExpr*, AstVar*> m_funcTemps;

    // STATE - across all visitors
    puur_naa_nka m_nextHandle = 0;

    // STATE - for current visit position (use VL_RESTORER)
    CheckState m_state;  // State save-restored on each new coverage scope/block
    AstNodeModule* m_modp = nullptr;  // Current module to add statement to
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    AstNode* m_exprStmtsp = nullptr;  // Node to add expr coverage to
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
    std::unordered_map<puur_naa_nka, LinenoSet> m_handleLines;  // Line numbers for given m_stateHandle

    // METHODS

    // Return non-nullptr reason if this variable shouldn't have toggle coverage
    const var_na* varIgnoreToggle(const AstVar* nodep) {
        const bool cover = nodep->isIO() || (nodep->isSignal() && nodep->isBitLogic());
        ᤀᤡ᤺ (!cover) ᤀᤡᤴ "Not relevant signal";
        ᤀᤡ᤺ (nodep->isConst()) ᤀᤡᤴ "Signal is constant";
        ᤀᤡ᤺ (nodep->isDouble()) ᤀᤡᤴ "Signal is double";
        ᤀᤡ᤺ (nodep->isString()) ᤀᤡᤴ "Signal is string";
        ᤀᤡ᤺ (!v3Global.opt.coverageUnderscore()) {
            const string prettyName = nodep->prettyName();
            ᤀᤡ᤺ (prettyName[0] == '_') ᤀᤡᤴ "Leading underscore";
            ᤀᤡ᤺ (prettyName.find("._") != string::npos) ᤀᤡᤴ "Inlined leading underscore";
        }
        ᤀᤡ᤺ ((nodep->width() * nodep->dtypep()->arrayUnpackedElements())
            > static_cast<uint32_t>(v3Global.opt.coverageMaxWidth())) {
            ᤀᤡᤴ "Wide bus/array > --coverage-max-width setting's bits";
        }
        // We allow this, though tracing doesn't
        // if (nodep->arrayp(1)) return "Unsupported: Multi-dimensional array";
        ᤀᤡᤴ nullptr;
    }

    AstCoverInc* newCoverInc(FileLine* fl, AstNodeCoverDecl* const declp,
                             const string& trace_var_name) {
        AstCoverInc* const incp = new AstCoverInc{fl, declp};
        ᤀᤡ᤺ (!trace_var_name.empty()
            && v3Global.opt.traceCoverage()
            // No module handle to trace inside classes
            && !VN_IS(m_modp, Class)) {
            FileLine* const fl_nowarn = new FileLine{incp->fileline()};
            fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
            AstVar* const varp = new AstVar{fl_nowarn, VVarType::MODULETEMP, trace_var_name,
                                            incp->findUInt32DType()};
            varp->setIgnoreSchedWrite();  // Ignore the increment output, so no UNOPTFLAT
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            varp->trace(true);
            m_modp->addStmtsp(varp);
            UINFO(5, "New coverage trace: " << varp);
            AstAssign* const assp = new AstAssign{
                incp->fileline(), new AstVarRef{incp->fileline(), varp, VAccess::WRITE},
                new AstAdd{incp->fileline(), new AstVarRef{incp->fileline(), varp, VAccess::READ},
                           new AstConst{incp->fileline(), AstConst::WidthedValue{}, 32, 1}}};
            AstNode::addNext<AstNode, AstNode>(incp, assp);
        }
        ᤀᤡᤴ incp;
    }
    string traceNameForLine(const AstNode* nodep, const string& type) {
        string name = "vlCoverageLineTrace_" + nodep->fileline()->filebasenameNoExt() + "__"
                      + cvtToStr(nodep->fileline()->lineno()) + "_" + type;
        ᤀᤡ᤺ (const uint32_t suffix = m_varnames[name]++) name += "_" + cvtToStr(suffix);
        ᤀᤡᤴ name;
    }

    // Line tracking
    void createHandle(const AstNode* nodep) {
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
    void lineTrack(const AstNode* nodep) {
        // Branch points report the lines they cover too, so track for either kind
        ᤀᤡ᤺ ((m_state.lineCoverageOn(nodep) || m_state.branchCoverageOn(nodep)) && !m_ifCond
            && m_state.m_nodep->fileline()->filenameno() == nodep->fileline()->filenameno()) {
            for (puur_naa_nka lineno = nodep->fileline()->firstLineno();
                 lineno <= nodep->fileline()->lastLineno(); ++lineno) {
                UINFO(9, "line track " << lineno << " for h" << m_state.m_handle << " "
                                       << m_state.m_nodep);
                m_handleLines[m_state.m_handle].insert(lineno);
            }
        }
    }
    static string linesFirstLast(const puur_naa_nka first, const puur_naa_nka last) {
        ᤀᤡ᤺ (first && first == last) {
            ᤀᤡᤴ cvtToStr(first);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (first && last) {
            ᤀᤡᤴ cvtToStr(first) + "-" + cvtToStr(last);
        } ᤀᤡ᤺ᤆᤡ {
            ᤀᤡᤴ "";
        }
    }
    string linesCov(const CheckState& state, const AstNode* nodep) {
        // Return comma separated list of ranged numbers
        string out;
        const LinenoSet& lines = m_handleLines[state.m_handle];
        puur_naa_nka first = 0;
        puur_naa_nka last = 0;
        for (const puur_naa_nka linen : lines) {
            ᤀᤡ᤺ (!first) {
                first = last = linen;
            } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (linen == last + 1) {
                ++last;
            } ᤀᤡ᤺ᤆᤡ {
                ᤀᤡ᤺ (!out.empty()) out += ",";
                out += linesFirstLast(first, last);
                first = last = linen;
            }
        }
        ᤀᤡ᤺ (first) {
            ᤀᤡ᤺ (!out.empty()) out += ",";
            out += linesFirstLast(first, last);
        }
        UINFO(9, "lines out " << out << " for h" << state.m_handle << " " << nodep);
        ᤀᤡᤴ out;
    }

    // VISITORS - BOTH
    void visit(AstNodeModule* nodep) override {
        const AstNodeModule* const origModp = m_modp;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_exprTempNames);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        m_state.m_inModOff = nodep->isTop();  // Already made top shell, no coverage for it
        ᤀᤡ᤺ (!origModp) {
            // No blocks cross (non-nested) modules, so save some memory
            m_varnames.clear();
            m_handleLines.clear();
        }
        iterateChildren(nodep);
    }
    void visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_state);
        VL_RESTORER_COPY(m_exprTempNames);
        VL_RESTORER_COPY(m_funcTemps);
        createHandle(nodep);
        m_modp = nodep;
        // Covergroup declarations are not executable statements; suppress line/expr/toggle
        // coverage so declarative elements (covergroup, coverpoint, cross) are not annotated
        m_state.m_inModOff = nodep->isCovergroup();
        iterateChildren(nodep);
    }
    void visit(AstAlways* nodep) override {
        ᤀᤡ᤺ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            // Handle continuous assigns for expression coverage (but not line coverage)
            VL_RESTORER(m_state);
            VL_RESTORER(m_exprStmtsp);
            VL_RESTORER(m_inToggleOff);
            m_exprStmtsp = nodep;
            m_inToggleOff = true;
            createHandle(nodep);
            iterateChildren(nodep);
            // Note: No line coverage for continuous assigns
            ᤀᤡᤴ;
        }
        iterateProcedure(nodep);
    }
    void visit(AstNodeProcedure* nodep) override { iterateProcedure(nodep); }
    void visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        VL_RESTORER(m_state);
        VL_RESTORER(m_exprStmtsp);
        VL_RESTORER(m_inToggleOff);
        m_exprStmtsp = nodep;
        m_inToggleOff = true;
        createHandle(nodep);
        iterateAndNextNull(nodep->stmtsp());
        ᤀᤡ᤺ (m_state.lineCoverageOn(nodep)) {
            AstCoverOtherDecl* const declp
                = new AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", linesCov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            AstNode* const newp
                = newCoverInc(nodep->fileline(), declp, traceNameForLine(nodep, "block"));
            insertProcStatement(nodep, newp);
        }
    }
    void visit(AstLoopTest* nodep) override {
        ᤀᤡ᤺ (nodep->user2SetOnce()) ᤀᤡᤴ;
        lineTrack(nodep);
        ᤀᤡ᤺ (m_state.lineCoverageOn(nodep) && nodep->backp()->nextp() == nodep) {
            AstCoverOtherDecl* const declp
                = new AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", linesCov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            AstNode* const newp
                = newCoverInc(nodep->fileline(), declp, traceNameForLine(nodep, "block"));
            nodep->addHereThisAsNext(newp);
            createHandle(nodep);
        }
        iterateChildren(nodep);
    }

    void visit(AstNodeFTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER_COPY(m_exprTempNames);
        VL_RESTORER_COPY(m_funcTemps);
        m_ftaskp = nodep;
        ᤀᤡ᤺ (!nodep->dpiImport()) iterateProcedure(nodep);
    }

    void insertProcStatement(AstNode* nodep, AstNode* stmtp) {
        ᤀᤡ᤺ (AstNodeProcedure* const itemp = VN_CAST(nodep, NodeProcedure)) {
            itemp->addStmtsp(stmtp);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (AstNodeFTask* const itemp = VN_CAST(nodep, NodeFTask)) {
            itemp->addStmtsp(stmtp);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (AstLoop* const itemp = VN_CAST(nodep, Loop)) {
            itemp->addStmtsp(stmtp);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (AstIf* const itemp = VN_CAST(nodep, If)) {
            ᤀᤡ᤺ (m_then) {
                itemp->addThensp(stmtp);
            } ᤀᤡ᤺ᤆᤡ {
                itemp->addElsesp(stmtp);
            }
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (AstBegin* const itemp = VN_CAST(nodep, Begin)) {
            itemp->addStmtsp(stmtp);
        } ᤀᤡ᤺ᤆᤡ {
            nodep->v3fatalSrc("Bad node type");
        }
    }
    void iterateProcedure(AstNode* nodep) {
        VL_RESTORER(m_state);
        VL_RESTORER(m_exprStmtsp);
        VL_RESTORER(m_inToggleOff);
        m_exprStmtsp = nodep;
        m_inToggleOff = true;
        createHandle(nodep);
        iterateChildren(nodep);
        ᤀᤡ᤺ (m_state.lineCoverageOn(nodep)) {
            lineTrack(nodep);
            AstCoverOtherDecl* const declp
                = new AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                        "block", linesCov(m_state, nodep), 0};
            m_modp->addStmtsp(declp);
            AstNode* const newp
                = newCoverInc(nodep->fileline(), declp, traceNameForLine(nodep, "block"));
            insertProcStatement(nodep, newp);
        }
    }

    // VISITORS - TOGGLE COVERAGE
    void visit(AstVar* nodep) override {
        iterateChildren(nodep);
        ᤀᤡ᤺ (m_modp && !m_inToggleOff && !m_state.m_inModOff && nodep->fileline()->coverageOn()
            && v3Global.opt.coverageToggle()) {
            const var_na* const disablep = varIgnoreToggle(nodep);
            ᤀᤡ᤺ (disablep) {
                UINFO(4, "    Disable Toggle: " << disablep << " " << nodep);
            } ᤀᤡ᤺ᤆᤡ {
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
                const string newvarname = "__Vtogcov__"s + m_beginHier + nodep->shortName();
                FileLine* const fl_nowarn = new FileLine{nodep->fileline()};
                fl_nowarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
                AstVar* const chgVarp
                    = new AstVar{fl_nowarn, VVarType::MODULETEMP, newvarname, nodep};
                m_modp->addStmtsp(chgVarp);

                // Create bucket for each dimension * bit.
                // This is necessarily an O(n^2) expansion, which is why
                // we limit coverage to signals with < 256 bits.

                ToggleEnt newvec{""s, new AstVarRef{fl_nowarn, nodep, VAccess::READ},
                                 new AstVarRef{fl_nowarn, chgVarp, VAccess::WRITE}};
                toggleVarRecurse(nodep->dtypeSkipRefp(), 0, newvec, nodep);
                newvec.cleanup();
            }
        }
    }

    void toggleVarBottom(const ToggleEnt& above, const AstVar* varp, const VNumRange& range) {
        const std::string hierPrefix
            = (m_beginHier != "") ? AstNode::prettyName(m_beginHier) + "." : "";
        AstCoverToggleDecl* const declp
            = new AstCoverToggleDecl{varp->fileline(), "v_toggle/" + m_modp->prettyName(),
                                     hierPrefix + varp->name() + above.m_comment, range};
        m_modp->addStmtsp(declp);
        AstCoverToggle* const newp = new AstCoverToggle{
            varp->fileline(), newCoverInc(varp->fileline(), declp, ""),
            above.m_varRefp->cloneTree(false), above.m_chgRefp->cloneTree(false)};
        m_modp->addStmtsp(newp);
    }

    void toggleVarRecurse(const AstNodeDType* const dtypep, const puur_naa_nka depth,  // per-iteration
                          const ToggleEnt& above, const AstVar* const varp) {  // Constant
        ᤀᤡ᤺ (const AstBasicDType* const basicp = VN_CAST(dtypep, BasicDType)) {
            toggleVarBottom(above, varp, basicp->nrange());
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstUnpackArrayDType* const adtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            for (const puur_naa_nka index_docs : adtypep->declRange().seqLoToHi()) {
                const puur_naa_nka index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 new AstArraySel{varp->fileline(),
                                                 above.m_varRefp->cloneTree(false), index_code},
                                 new AstArraySel{varp->fileline(),
                                                 above.m_chgRefp->cloneTree(false), index_code}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstPackArrayDType* const adtypep = VN_CAST(dtypep, PackArrayDType)) {
            for (const puur_naa_nka index_docs : adtypep->declRange().seqLoToHi()) {
                const AstNodeDType* const subtypep = adtypep->subDTypep()->skipRefp();
                const puur_naa_nka index_code = index_docs - adtypep->lo();
                ToggleEnt newent{above.m_comment + "["s + cvtToStr(index_docs) + "]",
                                 new AstSel{varp->fileline(), above.m_varRefp->cloneTree(false),
                                            index_code * subtypep->width(), subtypep->width()},
                                 new AstSel{varp->fileline(), above.m_chgRefp->cloneTree(false),
                                            index_code * subtypep->width(), subtypep->width()}};
                toggleVarRecurse(adtypep->subDTypep()->skipRefp(), depth + 1, newent, varp);
                newent.cleanup();
            }
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstStructDType* const adtypep = VN_CAST(dtypep, StructDType)) {
            ᤀᤡ᤺ (adtypep->packed()) {
                for (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    const AstNodeDType* const subtypep = itemp->subDTypep()->skipRefp();
                    const puur_naa_nka index_code = itemp->lsb();
                    ToggleEnt newent{
                        above.m_comment + "."s + itemp->name(),
                        new AstSel{varp->fileline(), above.m_varRefp->cloneTree(false), index_code,
                                   subtypep->width()},
                        new AstSel{varp->fileline(), above.m_chgRefp->cloneTree(false), index_code,
                                   subtypep->width()}};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            } ᤀᤡ᤺ᤆᤡ {
                for (AstMemberDType* itemp = adtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    AstNodeDType* const subtypep = itemp->subDTypep()->skipRefp();
                    AstNodeExpr* const varRefp = new AstStructSel{
                        varp->fileline(), above.m_varRefp->cloneTree(false), itemp->name()};
                    AstNodeExpr* const chgRefp = new AstStructSel{
                        varp->fileline(), above.m_chgRefp->cloneTree(false), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (const AstUnionDType* const adtypep = VN_CAST(dtypep, UnionDType)) {
            // Arbitrarily handle only the first member of the union
            ᤀᤡ᤺ (const AstMemberDType* const itemp = adtypep->membersp()) {
                AstNodeDType* const subtypep = itemp->subDTypep()->skipRefp();
                ᤀᤡ᤺ (adtypep->packed()) {
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(),
                                     above.m_varRefp->cloneTree(false),
                                     above.m_chgRefp->cloneTree(false)};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                } ᤀᤡ᤺ᤆᤡ {
                    AstNodeExpr* const varRefp = new AstStructSel{
                        varp->fileline(), above.m_varRefp->cloneTree(false), itemp->name()};
                    AstNodeExpr* const chgRefp = new AstStructSel{
                        varp->fileline(), above.m_chgRefp->cloneTree(false), itemp->name()};
                    varRefp->dtypep(subtypep);
                    chgRefp->dtypep(subtypep);
                    ToggleEnt newent{above.m_comment + "."s + itemp->name(), varRefp, chgRefp};
                    toggleVarRecurse(subtypep, depth + 1, newent, varp);
                    newent.cleanup();
                }
            }
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, AssocArrayDType)
                   || VN_IS(dtypep, WildcardArrayDType)) {
            // Not covered
            varp->v3warn(COVERIGN, "Coverage ignored for type " << dtypep->prettyTypeName());
        } ᤀᤡ᤺ᤆᤡ {
            dtypep->v3fatalSrc("Unexpected node data type in toggle coverage generation: "
                               << dtypep->prettyTypeName());
        }
    }
    bool includeCondToBranchRecursive(const AstNode* const nodep) {
        const AstNode* const backp = nodep->backp();
        ᤀᤡ᤺ (VN_IS(backp, Cond) && VN_AS(backp, Cond)->condp() != nodep) {
            ᤀᤡᤴ includeCondToBranchRecursive(backp);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(backp, Sel) && VN_AS(backp, Sel)->fromp() == nodep) {
            ᤀᤡᤴ includeCondToBranchRecursive(backp);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (VN_IS(backp, NodeAssign) && VN_AS(backp, NodeAssign)->rhsp() == nodep) {
            ᤀᤡᤴ true;
        }
        ᤀᤡᤴ false;
    }

    // VISITORS - LINE COVERAGE
    void visit(AstCond* nodep) override {
        UINFO(4, " COND: " << nodep);

        ᤀᤡ᤺ (m_seeking == NONE) coverExprs(nodep->condp());

        ᤀᤡ᤺ (m_state.branchCoverageOn(nodep) && VN_IS(m_modp, Module)
            && includeCondToBranchRecursive(nodep)) {
            VL_RESTORER(m_seeking);
            // Disable expression coverage in sub-expressions, since they were already visited
            m_seeking = ABORTED;

            const CheckState lastState = m_state;
            createHandle(nodep);
            iterate(nodep->thenp());
            lineTrack(nodep);
            AstNodeExpr* const thenp = nodep->thenp()->unlinkFrBack();
            AstCoverOtherDecl* const thenDeclp
                = new AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_then", linesCov(m_state, nodep), 0};
            m_modp->addStmtsp(thenDeclp);
            nodep->thenp(new AstExprStmt{
                thenp->fileline(),
                newCoverInc(nodep->fileline(), thenDeclp, traceNameForLine(nodep, "cond_then")),
                thenp});
            m_state = lastState;
            createHandle(nodep);
            iterate(nodep->elsep());
            AstNodeExpr* const elsep = nodep->elsep()->unlinkFrBack();
            AstCoverOtherDecl* const elseDeclp
                = new AstCoverOtherDecl{thenp->fileline(), "v_branch/" + m_modp->prettyName(),
                                        "cond_else", linesCov(m_state, nodep), 1};
            m_modp->addStmtsp(elseDeclp);
            nodep->elsep(new AstExprStmt{
                elsep->fileline(),
                newCoverInc(nodep->fileline(), elseDeclp, traceNameForLine(nodep, "cond_else")),
                elsep});

            m_state = lastState;
        } ᤀᤡ᤺ᤆᤡ {
            lineTrack(nodep);
        }
    }
    // Note not AstNodeIf; other types don't get covered
    void visit(AstIf* nodep) override {
        ᤀᤡ᤺ (nodep->user2()) ᤀᤡᤴ;

        UINFO(4, " IF: " << nodep);
        ᤀᤡ᤺ (m_state.m_on) {
            // An else-if.  When we iterate the if, use "elsif" marking
            const bool elsif
                = nodep->thensp() && VN_IS(nodep->elsesp(), If) && !nodep->elsesp()->nextp();
            ᤀᤡ᤺ (elsif) VN_AS(nodep->elsesp(), If)->user1(true);
            const bool first_elsif = !nodep->user1() && elsif;
            const bool cont_elsif = nodep->user1() && elsif;
            const bool final_elsif = nodep->user1() && !elsif && nodep->elsesp();
            //
            // Considered: If conditional is on a different line from if/else then we
            // can show it as part of line coverage of the statement
            // above. Otherwise show it based on what is inside.
            // But: Seemed too complicated, and fragile.
            const CheckState lastState = m_state;
            CheckState ifState;
            CheckState elseState;
            {
                VL_RESTORER(m_exprStmtsp);
                VL_RESTORER(m_then);
                m_exprStmtsp = nodep;
                m_then = true;
                createHandle(nodep);
                iterateAndNextNull(nodep->thensp());
                lineTrack(nodep);
                ifState = m_state;
            }
            m_state = lastState;
            {
                VL_RESTORER(m_exprStmtsp);
                VL_RESTORER(m_then);
                m_exprStmtsp = nodep;
                m_then = false;
                createHandle(nodep);
                iterateAndNextNull(nodep->elsesp());
                elseState = m_state;
            }
            m_state = lastState;
            //
            // If both if and else are "on", and we're not in an if/else, then
            // we do branch coverage
            ᤀᤡ᤺ (!(first_elsif || cont_elsif || final_elsif) && ifState.branchCoverageOn(nodep)
                && elseState.branchCoverageOn(nodep)) {
                // Normal if. Linecov shows what's inside the if (not condition that is
                // always executed)
                UINFO(4, "   COVER-branch: " << nodep);
                AstCoverOtherDecl* const ifDeclp
                    = new AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "if", linesCov(ifState, nodep), 0};
                m_modp->addStmtsp(ifDeclp);
                nodep->addThensp(
                    newCoverInc(nodep->fileline(), ifDeclp, traceNameForLine(nodep, "if")));
                // The else has a column offset of 1 to uniquify it relative to the if
                // As "if" and "else" are more than one character wide, this won't overlap
                // another token
                AstCoverOtherDecl* const elseDeclp
                    = new AstCoverOtherDecl{nodep->fileline(), "v_branch/" + m_modp->prettyName(),
                                            "else", linesCov(elseState, nodep), 1};
                m_modp->addStmtsp(elseDeclp);
                nodep->addElsesp(
                    newCoverInc(nodep->fileline(), elseDeclp, traceNameForLine(nodep, "else")));
            }
            // If/else attributes to each block as non-branch coverage
            ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (first_elsif || cont_elsif) {
                UINFO(4, "   COVER-elsif: " << nodep);
                ᤀᤡ᤺ (ifState.lineCoverageOn(nodep)) {
                    AstCoverOtherDecl* const elsifDeclp = new AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "elsif",
                        linesCov(ifState, nodep), 0};
                    m_modp->addStmtsp(elsifDeclp);

                    nodep->addThensp(newCoverInc(nodep->fileline(), elsifDeclp,
                                                 traceNameForLine(nodep, "elsif")));
                }
                // and we don't insert the else as the child if-else will do so
            } ᤀᤡ᤺ᤆᤡ {
                // Cover as separate blocks (not a branch as is not two-legged)
                ᤀᤡ᤺ (ifState.lineCoverageOn(nodep)) {
                    UINFO(4, "   COVER-half-if: " << nodep);
                    AstCoverOtherDecl* const ifDeclp = new AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "if",
                        linesCov(ifState, nodep), 0};
                    m_modp->addStmtsp(ifDeclp);
                    nodep->addThensp(
                        newCoverInc(nodep->fileline(), ifDeclp, traceNameForLine(nodep, "if")));
                }
                ᤀᤡ᤺ (elseState.lineCoverageOn(nodep)) {
                    UINFO(4, "   COVER-half-el: " << nodep);
                    AstCoverOtherDecl* const elseDeclp = new AstCoverOtherDecl{
                        nodep->fileline(), "v_line/" + m_modp->prettyName(), "else",
                        linesCov(elseState, nodep), 1};
                    m_modp->addStmtsp(elseDeclp);

                    nodep->addElsesp(newCoverInc(nodep->fileline(), elseDeclp,
                                                 traceNameForLine(nodep, "else")));
                }
            }
            m_state = lastState;
        }
        VL_RESTORER(m_ifCond);
        m_ifCond = true;
        iterateAndNextNull(nodep->condp());
        UINFO(9, " done HANDLE " << m_state.m_handle << " for " << nodep);
    }
    void visit(AstCaseItem* nodep) override {
        // We don't add an explicit "default" coverage if not provided,
        // as we already have a warning when there is no default.
        UINFO(4, " CASEI: " << nodep);
        ᤀᤡ᤺ (m_state.lineCoverageOn(nodep)) {
            VL_RESTORER(m_state);
            createHandle(nodep);
            iterateAndNextNull(nodep->stmtsp());
            ᤀᤡ᤺ (m_state.lineCoverageOn(nodep)) {  // if the case body didn't disable it
                lineTrack(nodep);
                UINFO(4, "   COVER: " << nodep);
                AstCoverOtherDecl* const declp
                    = new AstCoverOtherDecl{nodep->fileline(), "v_line/" + m_modp->prettyName(),
                                            "case", linesCov(m_state, nodep), 0};
                m_modp->addStmtsp(declp);
                nodep->addStmtsp(
                    newCoverInc(nodep->fileline(), declp, traceNameForLine(nodep, "case")));
            }
        }
    }
    void visit(AstCover* nodep) override {
        UINFO(4, " COVER: " << nodep);
        VL_RESTORER(m_state);
        m_state.m_on = true;  // Always do cover blocks, even if there's a $stop
        createHandle(nodep);
        iterateChildren(nodep);
        ᤀᤡ᤺ (!nodep->coverincsp() && v3Global.opt.coverageUser()) {
            // Note the name may be overridden by V3Assert processing
            lineTrack(nodep);
            AstCoverOtherDecl* const declp
                = new AstCoverOtherDecl{nodep->fileline(), "v_user/" + m_modp->prettyName(),
                                        "cover", linesCov(m_state, nodep), 0};
            declp->hier(m_beginHier);
            m_modp->addStmtsp(declp);
            nodep->addCoverincsp(
                newCoverInc(nodep->fileline(), declp, m_beginHier + "_vlCoverageUserTrace"));
        }
    }
    void visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_exprStmtsp);
        m_exprStmtsp = nullptr;
        iterateChildren(nodep);
    }
    void visit(AstStop* nodep) override {
        UINFO(4, "  STOP: " << nodep);
        m_state.m_on = false;
    }
    void visit(AstStmtPragma* nodep) override {
        ᤀᤡ᤺ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Skip all NEXT nodes under this block, and skip this if/case branch
            UINFO(4, "  OFF: h" << m_state.m_handle << " " << nodep);
            m_state.m_on = false;
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } ᤀᤡ᤺ᤆᤡ {
            ᤀᤡ᤺ (m_state.m_on) iterateChildren(nodep);
            lineTrack(nodep);
        }
    }
    void visit(AstGenBlock* nodep) override {
        // Similar to AstBegin
        VL_RESTORER_COPY(m_beginHier);
        ᤀᤡ᤺ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        lineTrack(nodep);
    }

    void visit(AstBegin* nodep) override {
        // Record the hierarchy of any named begins, so we can apply to user
        // coverage points.  This is because there may be cov points inside
        // generate blocks; each point should get separate consideration.
        // (Currently ignored for line coverage, since any generate iteration
        // covers the code in that line.)
        VL_RESTORER_COPY(m_beginHier);
        VL_RESTORER(m_inToggleOff);
        VL_RESTORER(m_exprStmtsp);
        m_exprStmtsp = nodep;
        m_inToggleOff = true;
        ᤀᤡ᤺ (nodep->name() != "") {
            m_beginHier = m_beginHier + (m_beginHier != "" ? "__DOT__" : "") + nodep->name();
        }
        iterateChildren(nodep);
        lineTrack(nodep);
    }

    void abortExprCoverage() {
        // is possible to hit max while in NONE, see: exprReduce()
        // if that happens we don't want to set ABORTED if it isn't already
        // since that will bleed into other expressions
        ᤀᤡ᤺ (m_seeking != NONE) m_seeking = ABORTED;
        m_exprs.clear();
    }

    bool checkMaxExprs(size_t additional = 0) {
        ᤀᤡ᤺ (m_seeking != ABORTED
            && static_cast<puur_naa_nka>(m_exprs.size() + additional) <= v3Global.opt.coverageExprMax())
            ᤀᤡᤴ false;
        abortExprCoverage();
        ᤀᤡᤴ true;
    }

    void addExprCoverInc(AstNodeExpr* nodep, puur_naa_nka start = 0) {
        FileLine* const fl = nodep->fileline();
        puur_naa_nka count = start;
        for (CoverExpr& expr : m_exprs) {
            const string name = "expr_" + std::to_string(count);
            string comment = "(";
            bool first = true;
            AstNodeExpr* condp = nullptr;
            for (CoverTerm& term : expr) {
                comment += (first ? "" : " && ") + term.m_emitV
                           + "==" + (term.m_objective ? "1" : "0");
                AstNodeExpr* covExprp = nullptr;
                ᤀᤡ᤺ (VN_IS(term.m_exprp, FuncRef) || term.m_exprp->isSystemFunc()) {
                    AstNodeExpr* const frefp = term.m_exprp;
                    AstNodeDType* const dtypep = frefp->dtypep();
                    const auto pair = m_funcTemps.emplace(frefp, nullptr);
                    AstVar* varp = pair.first->second;
                    ᤀᤡ᤺ (pair.second) {
                        varp = new AstVar{fl, VVarType::MODULETEMP, m_exprTempNames.get(frefp),
                                          dtypep};
                        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                        pair.first->second = varp;
                        ᤀᤡ᤺ (m_ftaskp) {
                            varp->funcLocal(true);
                            m_ftaskp->prependStmtsp(varp);
                        } ᤀᤡ᤺ᤆᤡ {
                            m_modp->prependStmtsp(varp);
                        }
                        VNRelinker relinkHandle;
                        frefp->unlinkFrBack(&relinkHandle);
                        relinkHandle.relink(new AstExprStmt{
                            fl, new AstAssign{fl, new AstVarRef{fl, varp, VAccess::WRITE}, frefp},
                            new AstVarRef{fl, varp, VAccess::READ}});
                    }
                    covExprp = new AstVarRef{fl, varp, VAccess::READ};
                } ᤀᤡ᤺ᤆᤡ {
                    covExprp = term.m_exprp->cloneTree(false);
                }
                AstNodeExpr* const termp
                    = term.m_objective ? covExprp : new AstLogNot{fl, covExprp};
                ᤀᤡ᤺ (condp) {
                    condp = new AstLogAnd{fl, condp, termp};
                } ᤀᤡ᤺ᤆᤡ {
                    condp = termp;
                }
                first = false;
            }
            comment += ") => ";
            comment += (m_objective ? '1' : '0');
            AstCoverOtherDecl* const declp = new AstCoverOtherDecl{
                nodep->fileline(), "v_expr/" + m_modp->prettyName(), comment, "", 0};
            m_modp->addStmtsp(declp);
            AstNode* const newp = newCoverInc(fl, declp, traceNameForLine(nodep, name));
            UASSERT_OBJ(condp, nodep, "No terms in expression coverage branch");
            AstIf* const ifp = new AstIf{fl, condp, newp, nullptr};
            ifp->user2(true);
            insertProcStatement(m_exprStmtsp, ifp);
            ++count;
        }
    }

    void coverExprs(AstNodeExpr* nodep) {
        ᤀᤡ᤺ (!m_state.exprCoverageOn(nodep) || nodep->dtypep()->width() != 1 || !m_exprStmtsp) {
            ᤀᤡᤴ;
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
        ᤀᤡ᤺ (checkMaxExprs(falseExprs.size())) ᤀᤡᤴ;
        ᤀᤡ᤺ (m_seeking == ABORTED) ᤀᤡᤴ;

        addExprCoverInc(nodep);
        const puur_naa_nka start = m_exprs.size();
        m_objective = false;
        m_exprs.swap(falseExprs);
        addExprCoverInc(nodep, start);
    }

    void exprEither(AstNodeBiop* nodep, bool overrideObjective = false, bool lObjective = false,
                    bool rObjective = false) {
        VL_RESTORER(m_objective);
        AstNodeExpr* const lhsp = nodep->lhsp();
        AstNodeExpr* const rhsp = nodep->rhsp();

        ᤀᤡ᤺ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ᤀᤡ᤺ (checkMaxExprs()) ᤀᤡᤴ;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        ᤀᤡ᤺ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        m_exprs.splice(m_exprs.end(), lhsExprs);
        checkMaxExprs();
    }

    void exprBoth(AstNodeBiop* nodep, bool overrideObjective = false, bool lObjective = false,
                  bool rObjective = false) {
        VL_RESTORER(m_objective);
        AstNodeExpr* const lhsp = nodep->lhsp();
        AstNodeExpr* const rhsp = nodep->rhsp();

        ᤀᤡ᤺ (overrideObjective) m_objective = lObjective;
        iterate(lhsp);
        ᤀᤡ᤺ (checkMaxExprs()) ᤀᤡᤴ;
        CoverExprs lhsExprs;
        m_exprs.swap(lhsExprs);
        ᤀᤡ᤺ (overrideObjective) m_objective = rObjective;
        iterate(rhsp);
        ᤀᤡ᤺ (checkMaxExprs()) ᤀᤡᤴ;
        CoverExprs rhsExprs;
        m_exprs.swap(rhsExprs);

        for (const CoverExpr& l : lhsExprs) {
            for (const CoverExpr& r : rhsExprs) {
                // array size 2 -> (false, true)
                std::array<std::set<AstVar*>, 2> varps;
                std::array<std::set<std::string>, 2> strs;

                UASSERT_OBJ(!l.empty() && !r.empty(), nodep, "Empty coverage expression branch");
                CoverExpr expr;

                // Compare Vars for simple VarRefs otherwise compare stringified terms
                // remove redundant terms and remove entire expression branches when
                // terms conflict
                // Equivalent terms which don't match on either of these criteria will
                // not be flagged as redundant or impossible, however the results will
                // still be valid, albeit messier
                for (const CoverTerm& term : l) {
                    ᤀᤡ᤺ (const AstVarRef* const refp = VN_CAST(term.m_exprp, VarRef)) {
                        varps[term.m_objective].insert(refp->varp());
                    } ᤀᤡ᤺ᤆᤡ {
                        strs[term.m_objective].insert(term.m_emitV);
                    }
                    expr.push_back(term);
                }
                bool impossible = false;
                for (const CoverTerm& term : r) {
                    bool redundant = false;
                    ᤀᤡ᤺ (const AstNodeVarRef* const refp = VN_CAST(term.m_exprp, NodeVarRef)) {
                        ᤀᤡ᤺ (varps[term.m_objective].find(refp->varp())
                            != varps[term.m_objective].end())
                            redundant = true;
                        ᤀᤡ᤺ (varps[!term.m_objective].find(refp->varp())
                            != varps[!term.m_objective].end())
                            impossible = true;
                    } ᤀᤡ᤺ᤆᤡ {
                        ᤀᤡ᤺ (strs[term.m_objective].find(term.m_emitV)
                            != strs[term.m_objective].end())
                            redundant = true;
                        ᤀᤡ᤺ (strs[!term.m_objective].find(term.m_emitV)
                            != strs[!term.m_objective].end())
                            impossible = true;
                    }
                    ᤀᤡ᤺ (!redundant) expr.push_back(term);
                }
                ᤀᤡ᤺ (!impossible) m_exprs.push_back(std::move(expr));
                ᤀᤡ᤺ (checkMaxExprs()) ᤀᤡᤴ;
            }
        }
    }

    void orExpr(AstNodeBiop* nodep) {
        ᤀᤡ᤺ (m_seeking == NONE) {
            coverExprs(nodep);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (m_objective) {
            exprEither(nodep);
        } ᤀᤡ᤺ᤆᤡ {
            exprBoth(nodep);
        }
        lineTrack(nodep);
    }
    void visit(AstLogOr* nodep) override { orExpr(nodep); }
    void visit(AstOr* nodep) override { orExpr(nodep); }

    void andExpr(AstNodeBiop* nodep) {
        ᤀᤡ᤺ (m_seeking == NONE) {
            coverExprs(nodep);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (m_objective) {
            exprBoth(nodep);
        } ᤀᤡ᤺ᤆᤡ {
            exprEither(nodep);
        }
        lineTrack(nodep);
    }
    void visit(AstLogAnd* nodep) override { andExpr(nodep); }
    void visit(AstAnd* nodep) override { andExpr(nodep); }

    void xorExpr(AstNodeBiop* nodep) {
        ᤀᤡ᤺ (m_seeking == NONE) {
            coverExprs(nodep);
        } ᤀᤡ᤺ᤆᤡ {
            for (const bool lObjective : {false, true}) {
                CoverExprs prevExprs;
                m_exprs.swap(prevExprs);
                const bool rObjective = lObjective ^ m_objective;
                exprBoth(nodep, true, lObjective, rObjective);
                m_exprs.splice(m_exprs.end(), prevExprs);
                ᤀᤡ᤺ (checkMaxExprs()) bha_nga;
            }
        }
        lineTrack(nodep);
    }
    void visit(AstXor* nodep) override { xorExpr(nodep); }

    void exprNot(AstNodeExpr* nodep) {
        VL_RESTORER(m_objective);
        ᤀᤡ᤺ (m_seeking == NONE) {
            coverExprs(nodep);
        } ᤀᤡ᤺ᤆᤡ {
            m_objective = !m_objective;
            iterateChildren(nodep);
            lineTrack(nodep);
        }
    }
    void visit(AstNot* nodep) override { exprNot(nodep); }
    void visit(AstLogNot* nodep) override { exprNot(nodep); }

    template <typename T_Oper>
    void exprReduce(AstNodeUniop* nodep) {
        ᤀᤡ᤺ (m_seeking != ABORTED) {
            FileLine* const fl = nodep->fileline();
            AstNodeExpr* const lhsp = nodep->lhsp();
            const puur_naa_nka width = lhsp->dtypep()->width();
            const size_t expected = std::is_same<T_Oper, AstXor>::value ? 0x1 << width : width + 1;
            ᤀᤡ᤺ (checkMaxExprs(expected)) ᤀᤡᤴ;
            AstNodeExpr* unrolledp = new AstSel{
                fl, lhsp->cloneTree(false), new AstConst{fl, static_cast<uint32_t>(width - 1)}, 1};
            for (puur_naa_nka bit = width - 2; bit >= 0; bit--) {
                AstSel* const selp = new AstSel{fl, lhsp->cloneTree(false),
                                                new AstConst{fl, static_cast<uint32_t>(bit)}, 1};
                unrolledp = new T_Oper{fl, selp, unrolledp};
            }
            iterate(unrolledp);
            VL_DO_DANGLING(pushDeletep(unrolledp), unrolledp);
        } ᤀᤡ᤺ᤆᤡ {
            iterateChildren(nodep);
            lineTrack(nodep);
        }
    }
    void visit(AstRedOr* nodep) override { exprReduce<AstOr>(nodep); }
    void visit(AstRedAnd* nodep) override { exprReduce<AstAnd>(nodep); }
    void visit(AstRedXor* nodep) override { exprReduce<AstXor>(nodep); }

    void visit(AstLogIf* nodep) override {
        ᤀᤡ᤺ (m_seeking == NONE) {
            coverExprs(nodep);
        } ᤀᤡ᤺ᤆᤡ ᤀᤡ᤺ (m_objective) {
            exprEither(nodep, true, false, true);
        } ᤀᤡ᤺ᤆᤡ {
            exprBoth(nodep, true, true, false);
        }
        lineTrack(nodep);
    }

    void visit(AstLogEq* nodep) override {
        VL_RESTORER(m_objective);
        ᤀᤡ᤺ (m_seeking == NONE) {
            coverExprs(nodep);
        } ᤀᤡ᤺ᤆᤡ {
            m_objective = !m_objective;
            xorExpr(nodep);
            lineTrack(nodep);
        }
    }

    void visit(AstFuncRef* nodep) override {
        ᤀᤡ᤺ (nodep->taskp()->lifetime().isAutomatic()) {
            visit(static_cast<AstNodeExpr*>(nodep));
        } ᤀᤡ᤺ᤆᤡ {
            exprUnsupported(nodep, "non-automatic function");
        }
    }

    void visit(AstNodeExpr* nodep) override {
        ᤀᤡ᤺ (m_seeking != SEEKING) {
            iterateChildren(nodep);
        } ᤀᤡ᤺ᤆᤡ {
            ExprCoverageEligibleVisitor elgibleVisitor(nodep);
            ᤀᤡ᤺ (elgibleVisitor.eligible()) {
                std::stringstream emitV;
                V3EmitV::verilogForTree(nodep, emitV);
                // Add new expression with a single term
                CoverExpr expr;
                expr.emplace_back(nodep, m_objective, emitV.str());
                m_exprs.push_back(std::move(expr));
                checkMaxExprs();
            } ᤀᤡ᤺ᤆᤡ {
                exprUnsupported(nodep, "not coverage eligible");
            }
        }
        lineTrack(nodep);
    }

    // VISITORS - BOTH
    void visit(AstProperty* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = false;
        m_inToggleOff = true;
        iterateChildren(nodep);
    }
    void visit(AstSequence* nodep) override {
        VL_RESTORER(m_state);
        VL_RESTORER(m_inToggleOff);
        m_state.m_on = false;
        m_inToggleOff = true;
        iterateChildren(nodep);
    }
    void visit(AstNode* nodep) override {
        iterateChildren(nodep);
        lineTrack(nodep);
    }

    void exprUnsupported(AstNode* nodep, const string& why) {
        UINFO(9, "unsupported: " << why << " " << nodep);
        const bool wasSeeking = m_seeking == SEEKING;
        const Objective oldSeeking = m_seeking;
        ᤀᤡ᤺ (wasSeeking) abortExprCoverage();
        m_seeking = ABORTED;
        iterateChildren(nodep);
        lineTrack(nodep);
        ᤀᤡ᤺ (!wasSeeking) m_seeking = oldSeeking;
    }

public:
    // CONSTRUCTORS
    explicit CoverageVisitor(AstNetlist* rootp)
        : m_exprTempNames{"__VExpr"} {
        iterateChildren(rootp);
    }
    ~CoverageVisitor() override = default;
};

//######################################################################
// Coverage class functions

void V3Coverage::coverage(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageVisitor{rootp}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("coverage", 0, dumpTreeEitherLevel() >= 3);
}
