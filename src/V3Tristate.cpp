// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Deals with tristate logic
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
// V3Tristate's Transformations:
//
// Modify the design to expand tristate logic into its
// corresponding two state representation. At the lowest levels,
//
// In detail:
//
// Over each module, from child to parent:
//   Build a graph, connecting signals together so we can propagate tristates
//     Variable becomes tristate with
//       VAR->isInout
//       VAR->isPullup/isPulldown (converted to AstPullup/AstPulldown
//       BufIf0/1
//   All variables on the LHS need to become tristate when there is:
//       CONST-> with Z value on the RHS of an assignment
//       AstPin with lower connection a tristate
//       A tristate signal on the RHS
//       (this can't generally be determined until that signal is resolved)
//   When LHS becomes tristate, then mark all RHS nodes as tristate
//   so any tristate varrefs on the right will propagate.
//
// Walk graph's tristate indication on each logic block with tristates
// propagating downstream to every other logic block.
//
// Expressions that have Z in them are converted into two state
// drivers and corresponding output enable signals are generated.
// These enable signals get transformed and regenerated through any
// logic that they may go through until they hit the module level.  At
// the module level, all the output enable signals from what can be
// many tristate drivers are combined together to produce a single
// driver and output enable. If the signal propagates up into higher
// modules, then new ports are created for the signal with
// suffixes __en and __out. The original port is turned from an inout
// to an input and the __out port carries the output driver signal and
// the __en port carried the output enable for that driver.
//
// Note 1800-2012 adds user defined resolution functions. This suggests
// long term this code should be scoped-based and resolve all nodes at once
// rather than hierarchically. If/when that is done, make sure to avoid
// duplicating vars and logic that is common between each instance of a
// module.
//
//----------------------------------------------------------------------
//
// Another thing done in this phase is signal strength handling.
//
// Currently they are only supported in assignments and gates parsed as assignments (see verilog.y)
// when any of the cases occurs:
// - it is possible to statically resolve all drivers,
// - all assignments that passed the static resolution have symmetric strengths (the same strength
// is related to both 0 and 1 values).
//
// It is possible to statically resolve all drivers when the strongest assignment has RHS marked as
// non-tristate. If the RHS is equal to z, that assignment has to be skipped. Since the value may
// be not known at Verilation time, cases with tristates on RHS can't be handled statically.
//
// Static resolution is split into 2 parts.
// First part can be done before tristate propagation. It is about removing assignments that are
// weaker or equally strong as the strongest assignment with constant on RHS that has all bits
// the same (equal to 0 or 1). It is done in the following way:
// - The assignment of value 0 (size may be greater than 1), that has greatest strength (the
// one corresponding to 0) of all other assignments of 0, has to be found.
// - The same is done for value '1 and strength corresponding to value 1.
// - The greater of these two strengths is chosen. If they are equal the one that corresponds
// to 1 is taken as the greatest.
// - All assignments, that have strengths weaker or equal to the one that was found before, are
// removed. They are the assignments with constants on the RHS and also all assignments that have
// both strengths non-greater that the one was found, because they are weaker no matter what is on
// RHS.
//
// Second part of static resolution is done after tristate propagation.
// At that moment it is known that some expressions can't be equal to z. The exact value is
// unknown (except the ones with constants that were handled before), so weaker of both strengths
// has to be taken into account. All weaker assignments can be safely removed. It is done in
// similar way to the first part:
// - The assignment with non-tristate RHS with the greatest weaker strength has to be found.
// - Then all not stronger assignments can be removed.
//
// All assignments that are stronger than the strongest with non-tristate RHS are then tried to be
// handled dynamically. Currently it is supported only on assignments with symmetric strengths.
// In this case, the exact value of the RHS doesn't matter. It only matters if it equals z or not.
// Such assignments are handled by changing the values to z of these bits that are overwritten by
// stronger assignments. Then all assignments can be aggregated as they would have equal strengths
// (by | on them and their __en expressions). To change the value to z, the RHS should be & with
// negation of __en expression of stronger assignments. Changing RHS's __en expression is not
// needed, because it will be then aggregated with __en expression of stronger assignments using |,
// so & with the negation can be safely skipped.
// So the values of overwritten bits are actually changed to 0, which doesn't affect stronger
// assignments, because | operation was used.
//
// Dynamic handling is implemented in the following way:
// - group the assignments by their strengths,
// - handle assignments of the same strength by aggregating values with |
// - assign results to var__strength and var__strength__en variables
// - aggregate the results:
// orp = orp | (var__strength & ~enp)
// enp = enp | var__strength__en,
// where orp is aggregated value and enp is aggregated __en value.
//
// There is a possible problem with equally strong assignments, because multiple assignments with
// the same strength, but different values should result in x value, but these values are
// unsupported.
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Tristate.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Graph.h"
#ضمن "V3Inst.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف TristateBaseVisitor VL_NOT_FINAL : عام VNVisitor {
عام:
    // METHODS
};

//######################################################################
// Graph support classes

صنف TristateVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_تۈگۈن* ثابت m_nodep;
    منطقي m_isTristate = خطأ;  // Logic indicates a tristate
    منطقي m_feedsTri = خطأ;  // Propagates to a tristate node (on RHS)
    منطقي m_processed = خطأ;  // Tristating was cleaned up
عام:
    TristateVertex(V3Graph* graphp, Ast_تۈگۈن* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = افتراضي;
    // ACCESSORS
    Ast_تۈگۈن* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
    ثابت AstVar* varp() ثابت { أرجع VN_CAST(nodep(), Var); }
    string name() ثابت override {
        أرجع ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvtToHex(nodep())));
    }
    string dotColor() ثابت override {
        أرجع (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_قۇر* fileline() ثابت override { أرجع nodep()->fileline(); }
    منطقي isTristate() ثابت VL_MT_SAFE { أرجع m_isTristate; }
    فراغ isTristate(منطقي flag) { m_isTristate = flag; }
    منطقي feedsTri() ثابت VL_MT_SAFE { أرجع m_feedsTri; }
    فراغ feedsTri(منطقي flag) { m_feedsTri = flag; }
    منطقي processed() ثابت { أرجع m_processed; }
    فراغ processed(منطقي flag) { m_processed = flag; }
};

//######################################################################

صنف TristateGraph final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
عام:
    using VarVec = std::vector<AstVar*>;

خاص:
    // MEMBERS
    V3Graph m_graph;  // Logic graph

عام:
    // CONSTRUCTORS
    TristateGraph() { clearAndCheck(); }
    virtual ~TristateGraph() { clearAndCheck(); }
    VL_UNCOPYABLE(TristateGraph);

خاص:
    // METHODS

    TristateVertex* makeVertex(Ast_تۈگۈن* nodep) {
        TristateVertex* vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        إذا (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = جديد TristateVertex{&m_graph, nodep};
            nodep->user4p(vertexp);
        }
        أرجع vertexp;
    }

    // METHODS - Graph optimization
    فراغ graphWalkRecurseFwd(TristateVertex* vtxp, صحيح level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        إذا (!vtxp->isTristate()) أرجع;  // tristate involved
        إذا (vtxp->user() == 1) أرجع;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        إذا (!vtxp->varp()) {  // not a var where we stop the recursion
            لكل (V3GraphEdge& edge : vtxp->outEdges()) {
                TristateVertex* ثابت vvertexp = static_cast<TristateVertex*>(edge.چوققا());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                إذا (!vvertexp->isTristate()) {
                    vvertexp->isTristate(true);
                    graphWalkRecurseFwd(vvertexp, level + 1);
                }
            }
        } وإلا {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            لكل (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ثابت vvertexp = static_cast<TristateVertex*>(edge.fromp());
                إذا (ثابت AstVarRef* ثابت refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    إذا (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(true);
                        graphWalkRecurseFwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    فراغ graphWalkRecurseBack(TristateVertex* vtxp, صحيح level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        إذا (!(vtxp->isTristate() || vtxp->feedsTri())) أرجع;  // tristate involved
        إذا (vtxp->user() == 3) أرجع;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        إذا (!vtxp->varp()) {  // not a var where we stop the recursion
            لكل (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ثابت vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                إذا (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(true);
                    graphWalkRecurseBack(vvertexp, level + 1);
                }
            }
        }
    }

عام:
    // METHODS
    منطقي empty() ثابت { أرجع m_graph.empty(); }
    فراغ clearAndCheck() {
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            ثابت TristateVertex& vvertex = static_cast<TristateVertex&>(vtx);
            إذا (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_graph.clear();
        Ast_تۈگۈن::user4_Clear_دەرەخ();  // Wipe all node user4p's that point to vertexes
    }
    فراغ graphWalk(Ast_تۈگۈن_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseBack(static_cast<TristateVertex*>(&vtx), 0);
        }
        إذا (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    فراغ associate(Ast_تۈگۈن* fromp, Ast_تۈگۈن* چوققا) {
        جديد V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(چوققا), 1};
    }
    فراغ deleteVerticesFromSubtreeRecurse(Ast_تۈگۈن* nodep) {
        إذا (!nodep) أرجع;
        // Skip vars, because they may be connected to more than one varref
        إذا (!VN_IS(nodep, Var)) {
            TristateVertex* ثابت vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            إذا (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        إذا (Ast_تۈگۈن* ثابت refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        إذا (Ast_تۈگۈن* ثابت refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        إذا (Ast_تۈگۈن* ثابت refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        إذا (Ast_تۈگۈن* ثابت refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    فراغ setTristate(Ast_تۈگۈن* nodep) { makeVertex(nodep)->isTristate(true); }
    منطقي isTristate(ثابت Ast_تۈگۈن* nodep) {
        ثابت TristateVertex* ثابت vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        أرجع vertexp && vertexp->isTristate();
    }
    منطقي feedsTri(ثابت Ast_تۈگۈن* nodep) {
        ثابت TristateVertex* ثابت vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        أرجع vertexp && vertexp->feedsTri();
    }
    فراغ didProcess(Ast_تۈگۈن* nodep, منطقي quiet = خطأ) {
        إذا (TristateVertex* ثابت vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(true);
        } وإلا إذا (!quiet) {
            // Not v3errorSrc as no reason to stop the world
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct (not in propagation graph): "
                              << nodep->prettyTypeName());
        }
    }
    // ITERATOR METHODS
    VarVec tristateVars() {
        // Return all tristate variables
        VarVec v;
        لكل (ثابت V3GraphVertex& vtx : m_graph.vertices()) {
            ثابت TristateVertex& vvertex = static_cast<ثابت TristateVertex&>(vtx);
            إذا (vvertex.isTristate()) {
                إذا (AstVar* ثابت nodep = VN_CAST(vvertex.nodep(), Var)) v.ئىتتىرىش_back(nodep);
            }
        }
        أرجع v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

صنف TristatePinVisitor final : عام TristateBaseVisitor {
    TristateGraph& m_tgraph;
    ثابت منطقي m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    فراغ visit(Ast_تۈگۈن_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        إذا (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } وإلا إذا (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.setTristate(nodep->varp());
        }
    }
    فراغ visit(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    فراغ visit(AstSliceSel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_تۈگۈن* nodep, TristateGraph& tgraph, منطقي lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = افتراضي;
};

//######################################################################

صنف TristateVisitor final : عام TristateBaseVisitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser3InUse m_inuser3;
    ثابت VNUser4InUse m_inuser4;

    بنية AuxAstVar final {
        AstPull* pullp = nullptr;  // pullup/pulldown direction (whole variable)
        AstVar* outVarp = nullptr;  // output __out var
        منطقي ifaceTristate = خطأ;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<صحيح, صحيح>
            bitPulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    بنية RefStrength final {
        Ast_تۈگۈن_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_تۈگۈن_Assign* m_assignp;  // Assignment containing m_varrefp
        ثابت Ast_تۈگۈن_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_تۈگۈن_ئىپادە* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_تۈگۈن_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_تۈگۈن_ئىپادە* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_تۈگۈن_Var_Ref* varrefp, Ast_تۈگۈن_Assign* assignp,
                    ثابت Ast_تۈگۈن_Procedure* procedurep, Ast_تۈگۈن_ئىپادە* enLhsp,
                    Ast_تۈگۈن_Var_Ref* enVarrefp, Ast_تۈگۈن_ئىپادە* enRhsp, VStrength strength,
                    AstDelay* delayp)
            : m_varrefp{varrefp}
            , m_assignp{assignp}
            , m_procedurep{procedurep}
            , m_enLhsp{enLhsp}
            , m_enVarrefp{enVarrefp}
            , m_enRhsp{enRhsp}
            , m_strength{strength}
            , m_delayp{delayp} {}
    };
    بنية LhsProjection final {
        Ast_تۈگۈن_ئىپادە* m_lhsp;  // LHS giving the enable assign
        Ast_تۈگۈن_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_تۈگۈن_ئىپادە* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    بنية ProceduralEnable final {
        Ast_تۈگۈن_ئىپادە* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_تۈگۈن_ئىپادە* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    using RefStrengthVec = std::vector<RefStrength>;
    using VarMap = std::map<AstVar*, RefStrengthVec*>;
    using Assigns = std::vector<AstAssignW*>;
    using VarToAssignsMap = std::map<AstVar*, Assigns>;
    using AssignEnableMap = std::map<Ast_تۈگۈن_Assign*, Ast_تۈگۈن_ئىپادە*>;
    using ProceduralEnableMap = std::map<Ast_تۈگۈن_Assign*, std::vector<ProceduralEnable>>;
    تعداد : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    منطقي m_graphing = خطأ;  // Major mode - creating graph
    //
    Ast_تۈگۈن_Module* m_modp = nullptr;  // Current module
    AstCell* m_cellp = nullptr;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    VarToAssignsMap m_assigns;  // Assigns in current module
    AssignEnableMap m_assignEnps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    صحيح m_unique = 0;
    منطقي m_alhs = خطأ;  // On LHS of assignment
    منطقي m_inAlias = خطأ;  // Inside alias statement
    منطقي m_processedIfaces = خطأ;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = nullptr;  // Delay of assignment currently collecting LHS drivers
    ثابت Ast_تۈگۈن* m_logicp = nullptr;  // Current logic being built
    Ast_تۈگۈن_Assign* m_assignp = nullptr;  // Current assignment
    ثابت Ast_تۈگۈن_Procedure* m_procedurep = nullptr;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::vector<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ثابت {
        string o = (m_graphing ? " gr " : " ng ");
        إذا (m_alhs) o += "alhs ";
        أرجع o;
    }
    فراغ modAddStmtp(Ast_تۈگۈن* nodep, Ast_تۈگۈن* newp) {
        إذا (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } وإلا {
            m_modp->addStmtsp(newp);
        }
    }
    فراغ associateLogic(Ast_تۈگۈن* fromp, Ast_تۈگۈن* چوققا) {
        إذا (m_logicp) m_tgraph.associate(fromp, چوققا);
    }
    AstConst* newAllZerosOrOnes(Ast_تۈگۈن* nodep, منطقي ones) {
        V3_سان رەقەم{nodep, nodep->width()};
        إذا (ones) رەقەم.setAllBits1();
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), رەقەم};
        أرجع newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_قۇر* fl, AstVar* varp, ثابت string& dotted, VAccess access,
                           ثابت string& inlinedDots) {
        AstVarXRef* ثابت xrefp = جديد AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        أرجع xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_تۈگۈن_Module* modp, ثابت string& dottedPath) {
        إذا (dottedPath.empty()) أرجع nullptr;
        ثابت string firstComp = dottedPath.substr(0, dottedPath.find('.'));
        لكل (Ast_تۈگۈن* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت ivarp = VN_CAST(stmtp, Var);
            إذا (!ivarp || ivarp->name() != firstComp) استمر;
            ثابت AstIfaceRefDType* ثابت ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            إذا (ifaceDtp && ifaceDtp->modportp()) أرجع ifaceDtp->modportp();
            اكسر;
        }
        أرجع nullptr;
    }
    Ast_تۈگۈن_ئىپادە* getEnp(Ast_تۈگۈن* nodep) {
        إذا (nodep->user1p()) {
            إذا (Ast_تۈگۈن_Var_Ref* ثابت refp = VN_CAST(nodep, تۈگۈن_Var_Ref)) {
                إذا (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    أرجع newAllZerosOrOnes(nodep, true);
                }
            }
        } وإلا {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, true));
        }
        // Otherwise return the previous output enable
        أرجع VN_AS(nodep->user1p(), تۈگۈن_ئىپادە);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    فراغ addModportVarRefIfMissing(AstModport* modportp, AstVar* varp, VDirection dir) {
        لكل (Ast_تۈگۈن* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ثابت mvrp = VN_CAST(mrp, ModportVarRef);
            إذا (mvrp && mvrp->varp() == varp) أرجع;  // Already present
        }
        AstModportVarRef* ثابت mvrp = جديد AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->addVarsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, منطقي is_چوققا) {
        // Return the master __en for the specified input variable
        إذا (!invarp->user1p()) {
            AstVar* ثابت newp
                = جديد AstVar{invarp->fileline(), is_چوققا ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_تۈگۈن_Module* ثابت ownerModp = findParentModule(invarp);
            إذا (ownerModp && ownerModp != m_modp) {
                ownerModp->addStmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                لكل (Ast_تۈگۈن* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ثابت modportp = VN_CAST(stmtp, Modport);
                    إذا (!modportp) استمر;
                    لكل (Ast_تۈگۈن* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ثابت mvrp = VN_CAST(mrp, ModportVarRef);
                        إذا (mvrp && mvrp->varp() == invarp) {
                            addModportVarRefIfMissing(modportp, newp, VDirection::كىرگۈزۈش);
                            اكسر;
                        }
                    }
                }
            } وإلا {
                modAddStmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        أرجع VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ثابت constp) {
        File_قۇر* ثابت fl = constp->fileline();
        V3_سان numz{constp, constp->width()};
        numz.opBitsZ(constp->رەقەم());  // Z->1, else 0
        V3_سان numz0{constp, constp->width()};
        numz0.opNot(numz);  // Z->0, else 1
        أرجع جديد AstConst{fl, numz0};
    }
    Ast_تۈگۈن_ئىپادە* get_En_ئىپادە_Based_On_Originalp(Ast_تۈگۈن_ئىپادە* ثابت nodep) {
        إذا (AstVarRef* ثابت varrefp = VN_CAST(nodep, VarRef)) {
            أرجع جديد AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), خطأ),
                                 VAccess::READ};
        } وإلا إذا (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ثابت enVarp = getCreateEnVarp(xrefp->varp(), خطأ);
            أرجع newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } وإلا إذا (AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            أرجع getNonZConstp(constp);
        } وإلا إذا (AstExtend* ثابت extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_تۈگۈن_ئىپادە* ثابت lhsp = extendp->lhsp()->unlinkFrBack();
            AstExtend* ثابت enExtendp = extendp->clone_دەرەخ(خطأ);
            extendp->lhsp(lhsp);
            Ast_تۈگۈن_ئىپادە* ثابت enLhsp = get_En_ئىپادە_Based_On_Originalp(lhsp);
            enExtendp->lhsp(جديد AstNot{enLhsp->fileline(), enLhsp});
            أرجع جديد AstNot{enExtendp->fileline(), enExtendp};
        } وإلا إذا (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            Ast_تۈگۈن_ئىپادە* ثابت fromp = selp->fromp()->unlinkFrBack();
            AstSel* ثابت enSelp = selp->clone_دەرەخ(خطأ);
            selp->fromp(fromp);
            Ast_تۈگۈن_ئىپادە* ثابت enFromp = get_En_ئىپادە_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            أرجع enSelp;
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            أرجع nullptr;
        }
    }
    AstVar* getCreateOutVarp(AstVar* invarp, منطقي is_چوققا) {
        // Return the master __out for the specified input variable
        إذا (!m_varAux(invarp).outVarp) {
            AstVar* ثابت newp
                = جديد AstVar{invarp->fileline(), is_چوققا ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            modAddStmtp(invarp, newp);
            m_varAux(invarp).outVarp = newp;  // find outvar given invarp
        }
        أرجع m_varAux(invarp).outVarp;
    }
    AstVar* getCreateUnconnVarp(Ast_تۈگۈن* fromp, Ast_تۈگۈن_DType* dtypep) {
        AstVar* ثابت newp = جديد AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvtToStr(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        modAddStmtp(newp, newp);
        أرجع newp;
    }

    LhsProjection newLhsProjection(Ast_تۈگۈن_ئىپادە* lhsp, Ast_تۈگۈن_Var_Ref* targetp, Ast_تۈگۈن_ئىپادە* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_تۈگۈن_ئىپادە* ثابت newLhsp = lhsp->clone_دەرەخ_Pure(خطأ);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_تۈگۈن* nodep) { nodep->user1p(nullptr); });
        Ast_تۈگۈن_Var_Ref* ثابت newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        أرجع LhsProjection{newLhsp, newVarrefp, enp};
    }

    فراغ map_قىستۇرۇش_Lhs_Var_Ref(Ast_تۈگۈن_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ثابت ئاچقۇچ = nodep->varp();
        ثابت auto pair = m_lhsmap.emplace(ئاچقۇچ, nullptr);
        إذا (pair.second) pair.first->second = جديد RefStrengthVec;
        Ast_تۈگۈن_ئىپادە* enLhsp = nullptr;
        Ast_تۈگۈن_Var_Ref* enVarrefp = nullptr;
        Ast_تۈگۈن_ئىپادە* enRhsp = nullptr;
        ثابت auto enIt = m_assignEnps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ثابت منطقي canCombineProcedural = !VN_IS(ئاچقۇچ->dtypep()->skipRefp(), تۈگۈن_UOr_Struct_DType);
        ثابت Ast_تۈگۈن_Procedure* ثابت procedurep
            = enIt != m_assignEnps.end() && canCombineProcedural ? m_procedurep : nullptr;
        إذا (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_دەرەخ_Pure(خطأ));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->ئىتتىرىش_back(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_تۈگۈن_ئىپادە* newEnableDeposit(AstSel* selp, Ast_تۈگۈن_ئىپادە* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_تۈگۈن_ئىپادە* ثابت newp = جديد AstShiftL{
            selp->fileline(), جديد AstExtend{selp->fileline(), enp, selp->fromp()->width()},
            selp->lsbp()->clone_دەرەخ_Pure(خطأ), selp->fromp()->width()};
        أرجع newp;
    }

    فراغ setPullDirection(AstVar* varp, AstPull* pullp) {
        ثابت AstPull* ثابت oldpullp = m_varAux(varp).pullp;
        إذا (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } وإلا {
            إذا (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    فراغ setBitPullDirection(AstVar* varp, صحيح bitIndex, صحيح direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        auto& bitPulls = m_varAux(varp).bitPulls;
        auto it = bitPulls.find(bitIndex);
        إذا (it != bitPulls.end() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bitIndex << " of " << varp->prettyNameQ());
        }
        bitPulls[bitIndex] = direction;
    }

    AstConst* createPerBitPullConst(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ثابت auto& bitPulls = m_varAux(varp).bitPulls;
        V3_سان رەقەم{varp, varp->width(), 0};  // Start with all zeros
        لكل (ثابت auto& pair : bitPulls) {
            إذا (pair.second == 1) رەقەم.setBit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        أرجع جديد AstConst{varp->fileline(), رەقەم};
    }

    منطقي hasPerBitPulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        أرجع !m_varAux(varp).bitPulls.empty();
    }

    فراغ checkUnhandled(Ast_تۈگۈن* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        إذا (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        إذا ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    فراغ قىستۇرۇش_Tristates(Ast_تۈگۈن_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ثابت TristateGraph::VarVec vars = m_tgraph.tristateVars();
        لكل (auto varp : vars) {
            إذا (!m_tgraph.isTristate(varp)) استمر;
            ثابت auto it = m_lhsmap.find(varp);
            إذا (it != m_lhsmap.end()) استمر;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ثابت AstPull* ثابت pullp = m_varAux(varp).pullp;
            ثابت منطقي pull_قىممەت = pullp && pullp->direction() == 1;
            AstConst* ثابت constp = newAllZerosOrOnes(varp, pull_قىممەت);
            AstVarRef* ثابت varrefp = جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            AstAssignW* ثابت newp = جديد AstAssignW{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, خطأ));  // en=0 for floating/pullup vars
            nodep->addStmtsp(جديد AstAlways{newp});
            map_قىستۇرۇش_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        لكل (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ثابت std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            إذا (it == m_lhsmap.end()) استمر;
            AstVar* ثابت invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            إذا (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_تۈگۈن_Module* ثابت ownerModp = findParentModule(invarp);
                ثابت منطقي isCrossModule = ownerModp && ownerModp != nodep;
                ثابت منطقي isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                إذا (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    لكل (ثابت RefStrength& rs : *refsp) {
                        إذا (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            اكسر;
                        }
                    }
                } وإلا إذا (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = true;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    بنية PartitionInfo final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, PartitionInfo> partitions;
                    لكل (ثابت RefStrength& rs : *refsp) {
                        إذا (AstVarXRef* ثابت xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            PartitionInfo& pi = partitions[xrefp->dotted()];
                            pi.refs.ئىتتىرىش_back(rs);
                            إذا (pi.inlinedDots.empty()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } وإلا {
                            partitions[""].refs.ئىتتىرىش_back(rs);
                        }
                    }
                    لكل (auto& kv : partitions) {
                        قىستۇرۇش_Tristates_Signal(nodep, invarp, &kv.second.refs, true, kv.first,
                                              kv.second.inlinedDots,
                                              findModportForDotted(nodep, kv.first));
                    }
                } وإلا إذا (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = true;
                    قىستۇرۇش_Tristates_Signal(nodep, invarp, refsp, true, "", "", nullptr);
                } وإلا {
                    قىستۇرۇش_Tristates_Signal(nodep, invarp, refsp, خطأ, "", "", nullptr);
                }
            } وإلا {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(delete refsp, refsp);
        }
    }

    فراغ aggregateTriSameStrength(Ast_تۈگۈن_Module* nodep, AstVar* ثابت varp, AstVar* ثابت envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator endStrength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        بنية DriverGroup final {
            ثابت Ast_تۈگۈن_Procedure* m_procedurep;  // Process owning these assignments
            std::vector<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::vector<DriverGroup> drivers;
        std::map<ثابت Ast_تۈگۈن_Procedure*, size_t> procedureToDriver;
        لكل (auto it = beginStrength; it != endStrength; ++it) {
            إذا (it->m_procedurep) {
                ثابت auto emplaced = procedureToDriver.emplace(it->m_procedurep, drivers.size());
                إذا (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.ئىتتىرىش_back(&*it);
                    استمر;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ئىتتىرىش_back(DriverGroup{it->m_procedurep, {&*it}});
        }

        Ast_تۈگۈن_ئىپادە* orp = nullptr;
        Ast_تۈگۈن_ئىپادە* enp = nullptr;

        لكل (DriverGroup& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ثابت newLhsp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvtToStr(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->setDfgTriLowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->addStmtsp(newLhsp);
            AstVar* ثابت newEnLhsp
                = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvtToStr(m_unique++), envarp};  // 2-state ok
            newEnLhsp->setDfgTriLowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->addStmtsp(newEnLhsp);

            لكل (RefStrength* ثابت refStrengthp : driver.m_refsp) {
                Ast_تۈگۈن_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                إذا (VN_IS(refp, VarXRef)) {
                    AstVarRef* ثابت localRefp
                        = جديد AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(nullptr);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ئىتتىرىش_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } وإلا {
                    refp->varp(newLhsp);
                }

                إذا (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_تۈگۈن_ئىپادە* enLhsp = refStrengthp->m_enLhsp;
                    Ast_تۈگۈن_Var_Ref* ثابت enVarrefp = refStrengthp->m_enVarrefp;
                    إذا (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ثابت localRefp
                            = جديد AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        إذا (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_دەرەخ(), enVarrefp);
                        } وإلا {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ئىتتىرىش_Deletep(enVarrefp), enVarrefp);
                        }
                    } وإلا {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ئىتتىرىش_back(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = nullptr;
                    refStrengthp->m_enVarrefp = nullptr;
                    refStrengthp->m_enRhsp = nullptr;
                } وإلا {
                    AstAssignW* ثابت enLhspAssignp = جديد AstAssignW{
                        refp->fileline(),
                        جديد AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    إذا (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_دەرەخ(خطأ));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->addStmtsp(جديد AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_قۇر* ثابت fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_تۈگۈن_ئىپادە* ثابت ref1p = جديد AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_تۈگۈن_ئىپادە* ثابت ref2p = جديد AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_تۈگۈن_ئىپادە* ثابت andp = جديد AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : جديد AstOr{fl, orp, andp};

            Ast_تۈگۈن_ئىپادە* ثابت ref3p = جديد AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : جديد AstOr{ref3p->fileline(), enp, ref3p};
        }
        AstAssignW* ثابت assp = جديد AstAssignW{
            varp->fileline(), جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->addStmtsp(جديد AstAlways{assp});

        AstAssignW* ثابت enAssp = جديد AstAssignW{
            envarp->fileline(), جديد AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->addStmtsp(جديد AstAlways{enAssp});
    }

    فراغ قىستۇرۇش_Procedural_Enable_Assignments(Ast_تۈگۈن_Module* nodep) {
        لكل (auto& assignEnables : m_proceduralEnables) {
            Ast_تۈگۈن_Assign* ثابت assignp = assignEnables.first;
            Ast_تۈگۈن_ئىپادە* combinedLhsp = assignp->lhsp()->unlinkFrBack();
            Ast_تۈگۈن_ئىپادە* combinedRhsp = assignp->rhsp()->unlinkFrBack();
            لكل (ProceduralEnable& enable : assignEnables.second) {
                combinedLhsp = جديد AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = جديد AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            إذا (!VN_IS(assignp, AssignDly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ثابت sampledp
                    = جديد AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvtToStr(m_unique++), combinedRhsp->dtypep()};
                nodep->addStmtsp(sampledp);
                assignp->lhsp(جديد AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                AstAssign* ثابت applyp
                    = جديد AstAssign{assignp->fileline(), combinedLhsp,
                                    جديد AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->addNextHere(applyp);
            } وإلا {
                // Keeping data and enables in one assignment preserves NBA scheduling and makes
                // every partial LHS update affect the identical enable footprint.
                assignp->lhsp(combinedLhsp);
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                UINFO(9, "       augmented procedural tristate assignment " << assignp);
            }
        }
        m_proceduralEnables.clear();
    }

    // isIfaceTri: true when the var is a tristate in an interface module (local or external).
    // ifaceDottedPath/ifaceInlinedDots/ifaceModportp are non-empty only for external
    // (cross-module) drivers; empty for local drivers within the interface itself.
    فراغ قىستۇرۇش_Tristates_Signal(Ast_تۈگۈن_Module* nodep, AstVar* ثابت invarp, RefStrengthVec* refsp,
                               منطقي isIfaceTri, ثابت string& ifaceDottedPath,
                               ثابت string& ifaceInlinedDots, AstModport* ifaceModportp) {
        UINFO(8, "  TRISTATE EXPANDING:" << invarp);
        ++m_statTriSigs;
        m_tgraph.didProcess(invarp);

        // If the lhs var is a port, then we need to create ports for
        // the output (__out) and output enable (__en) signals. The
        // original port gets converted to an input. Don't tristate expand
        // if this is the top level so that we can force the final
        // tristate resolution at the top.
        // Top-level inouts are also expanded for pinsInoutEnables() or library generation.
        // Resolution is then performed by the caller or the library's SV wrapper.
        // Interface tristate vars skip port conversion (they use contribution vars instead).
        AstVar* envarp = nullptr;
        AstVar* outvarp = nullptr;  // __out
        AstVar* lhsp = invarp;  // Variable to assign drive-value to (<in> or __out)
        ثابت منطقي is_چوققا_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_چوققا();
        إذا (!isIfaceTri) {
            إذا (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty())
                 && is_چوققا_Inout)
                || (!nodep->is_چوققا() && invarp->isIO())) {
                // This var becomes an input
                invarp->varType2In();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = getCreateOutVarp(invarp, is_چوققا_Inout);
                outvarp->varType2Out();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_چوققا_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                إذا (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                إذا (!m_varAux(invarp).bitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } وإلا إذا (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_تۈگۈن_ئىپادە* orp = nullptr;
        Ast_تۈگۈن_ئىپادە* enp = nullptr;

        std::sort(refsp->begin(), refsp->end(),
                  [](RefStrength a, RefStrength b) { أرجع a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        طالما (beginStrength != refsp->end()) {
            RefStrengthVec::iterator endStrength = beginStrength + 1;
            طالما (endStrength != refsp->end()
                   && endStrength->m_strength == beginStrength->m_strength)
                endStrength++;

            File_قۇر* ثابت fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strengthPrefix;
            إذا (isIfaceTri && !ifaceDottedPath.empty()) {
                strengthPrefix = ifaceDottedPath;
                std::replace(strengthPrefix.begin(), strengthPrefix.end(), '.', '_');
                strengthPrefix += "__";
            }
            ثابت string strengthVarName
                = strengthPrefix + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ثابت varStrengthp = جديد AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->setDfgTriLowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->addStmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ثابت enVarStrengthp = جديد AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->setDfgTriLowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->addStmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     endStrength);

            Ast_تۈگۈن_ئىپادە* ئىپادە_Current_Strengthp;
            إذا (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                ئىپادە_Current_Strengthp
                    = جديد AstAnd{fl, جديد AstVarRef{fl, varStrengthp, VAccess::READ},
                                 جديد AstNot{fl, enp->clone_دەرەخ_Pure(خطأ)}};
            } وإلا {
                ئىپادە_Current_Strengthp = جديد AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? ئىپادە_Current_Strengthp : جديد AstOr{fl, orp, ئىپادە_Current_Strengthp};

            Ast_تۈگۈن_ئىپادە* ثابت enVarStrengthRefp
                = جديد AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : جديد AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = endStrength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        إذا (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->setDfgAllowMultidriveTri();
            Ast_تۈگۈن_Module* ثابت ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ثابت صحيح contribIdx = static_cast<صحيح>(m_ifaceContribs[invarp].size());
            File_قۇر* ثابت fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ثابت contribOutp = جديد AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvtToStr(contribIdx), invarp};
            contribOutp->setDfgTriLowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->addStmtsp(contribOutp);

            AstVar* ثابت contribEnp = جديد AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvtToStr(contribIdx), invarp};
            contribEnp->setDfgTriLowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->addStmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            إذا (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                addModportVarRefIfMissing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                addModportVarRefIfMissing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_تۈگۈن_Var_Ref* ثابت lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_تۈگۈن_Var_Ref*>(
                                                    جديد AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_تۈگۈن_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ثابت assp = جديد AstAssignW{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(جديد AstAlways{assp});
            }
            {
                Ast_تۈگۈن_Var_Ref* ثابت lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_تۈگۈن_Var_Ref*>(
                                                    جديد AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_تۈگۈن_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ثابت assp = جديد AstAssignW{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(جديد AstAlways{assp});
            }

            m_ifaceContribs[invarp].ئىتتىرىش_back({contribEnp, contribOutp});
            أرجع;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ثابت منطقي hasPull = m_varAux(lhsp).pullp || hasPerBitPulls(lhsp);
        إذا (!outvarp || hasPull) {
            Ast_تۈگۈن_ئىپادە* undrivenp;
            إذا (envarp) {
                undrivenp = جديد AstNot{envarp->fileline(),
                                       جديد AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } وإلا إذا (enp) {
                undrivenp = جديد AstNot{enp->fileline(), enp};
                enp = nullptr;  // moved into undrivenp
            } وإلا {
                undrivenp = newAllZerosOrOnes(invarp, true);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            إذا (hasPerBitPulls(lhsp)) {
                pullConstp = createPerBitPullConst(lhsp);
            } وإلا {
                ثابت AstPull* ثابت pullp = m_varAux(lhsp).pullp;
                ثابت منطقي pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = جديد AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? جديد AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        إذا (envarp && !enp) enp = newAllZerosOrOnes(invarp, خطأ);

        إذا (envarp) {
            AstAssignW* ثابت enAssp = جديد AstAssignW{
                enp->fileline(), جديد AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->addStmtsp(جديد AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        AstAssignW* ثابت assp = جديد AstAssignW{
            lhsp->fileline(), جديد AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->addStmtsp(جديد AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        إذا ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty()) && is_چوققا_Inout) {
            إذا (envarp) {
                envarp->primaryIO(true);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            إذا (outvarp) {
                outvarp->primaryIO(true);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            إذا (!v3Global.opt.libCreate().empty()) {
                ثابت صحيح id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(true);
            }
        }
    }

    منطقي isOnlyAssignmentIsToLhsVar(ثابت AstAssignW* ثابت nodep) {
        إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            auto foundIt = m_assigns.find(varRefp->varp());
            إذا (foundIt != m_assigns.end()) {
                auto ثابت& assignsToVar = foundIt->second;
                إذا (assignsToVar.size() == 1 && assignsToVar[0] == nodep) أرجع true;
            }
        }
        أرجع خطأ;
    }

    فراغ addToAssignmentList(AstAssignW* nodep) {
        إذا (AstVarRef* ثابت varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            إذا (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ئىتتىرىش_back(nodep);
            } وإلا إذا (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlinkFrBack()->delete_دەرەخ();
            }
        } وإلا إذا (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ثابت AstAssignW* ثابت nodep, منطقي قىممەت) {
        إذا (AstStrengthSpec* ثابت strengthSpec = nodep->strengthSpecp()) {
            أرجع قىممەت ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        أرجع VStrength::STRONG;  // default strength is strong
    }

    منطقي assignment_Of_قىممەت_On_All_Bits(ثابت AstAssignW* ثابت nodep, منطقي قىممەت) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const)) {
            ثابت V3_سان رەقەم = constp->رەقەم();
            أرجع قىممەت ? رەقەم.isEqAllOnes() : رەقەم.isEqZero();
        }
        أرجع خطأ;
    }

    AstAssignW* get_Strongest_Assignment_Of_قىممەت(ثابت Assigns& assigns, منطقي قىممەت) {
        auto maxIt = std::max_element(
            assigns.begin(), assigns.end(), [&](ثابت AstAssignW* ap, ثابت AstAssignW* bp) {
                ثابت منطقي قىممەتلەر_On_Rhs_A = assignment_Of_قىممەت_On_All_Bits(ap, قىممەت);
                ثابت منطقي قىممەتلەر_On_Rhs_B = assignment_Of_قىممەت_On_All_Bits(bp, قىممەت);
                إذا (!قىممەتلەر_On_Rhs_A) أرجع قىممەتلەر_On_Rhs_B;
                إذا (!قىممەتلەر_On_Rhs_B) أرجع خطأ;
                أرجع getStrength(ap, قىممەت) < getStrength(bp, قىممەت);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        أرجع assignment_Of_قىممەت_On_All_Bits(*maxIt, قىممەت) ? *maxIt : nullptr;
    }

    منطقي isAssignmentNotStrongerThanStrength(ثابت AstAssignW* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ثابت uint8_t strength0 = getStrength(assignp, 0);
        ثابت uint8_t strength1 = getStrength(assignp, 1);
        أرجع (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_Of_قىممەت_On_All_Bits(assignp, 0))
               || (strength1 <= strength && assignment_Of_قىممەت_On_All_Bits(assignp, 1));
    }

    Ast_تۈگۈن_ئىپادە* new_Merge_ئىپادە(Ast_تۈگۈن_ئىپادە* ثابت lhsp, Ast_تۈگۈن_ئىپادە* ثابت rhsp, File_قۇر* ثابت fl,
                              منطقي isWor) {
        Ast_تۈگۈن_ئىپادە* ئىپادە = nullptr;
        إذا (isWor)
            ئىپادە = جديد AstOr{fl, lhsp, rhsp};
        وإلا
            ئىپادە = جديد AstAnd{fl, lhsp, rhsp};
        أرجع ئىپادە;
    }

    فراغ mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        لكل (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            إذا (assigns.size() <= 1) استمر;
            AstVar* ثابت varp = varpAssigns.first;
            إذا (!varp->isWiredNet()) استمر;
            auto it = assigns.begin();
            AstAssignW* ثابت assignWp0 = *it;
            File_قۇر* ثابت fl = assignWp0->fileline();
            Ast_تۈگۈن_ئىپادە* wExp = nullptr;
            طالما (++it != assigns.end()) {
                AstAssignW* assignWpi = *it;
                إذا (!wExp) {
                    wExp
                        = new_Merge_ئىپادە(assignWp0->rhsp()->clone_دەرەخ_Pure(خطأ),
                                       assignWpi->rhsp()->clone_دەرەخ_Pure(خطأ), fl, varp->isWor());
                } وإلا {
                    wExp = new_Merge_ئىپادە(wExp, assignWpi->rhsp()->clone_دەرەخ_Pure(خطأ), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((assignWpi->unlinkFrBack()->delete_دەرەخ()), assignWpi);
            }
            AstVarRef* ثابت wVarRef = جديد AstVarRef{fl, varp, VAccess::WRITE};
            AstAssignW* ثابت wAssignp = جديد AstAssignW{fl, wVarRef, wExp};
            assignWp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ئىتتىرىش_Deletep(assignWp0), assignWp0);
            assigns.clear();
            assigns.ئىتتىرىش_back(wAssignp);
        }
    }

    فراغ removeNotStrongerAssignments(Assigns& assigns, ثابت AstAssignW* strongestp,
                                      uint8_t greatestKnownStrength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        auto removedIt = std::remove_if(assigns.begin(), assigns.end(), [&](AstAssignW* assignp) {
            إذا (assignp == strongestp) أرجع خطأ;
            إذا (isAssignmentNotStrongerThanStrength(assignp, greatestKnownStrength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ئىتتىرىش_Deletep(assignp->unlinkFrBack()), assignp);
                أرجع true;
            }
            أرجع خطأ;
        });
        assigns.erase(removedIt, assigns.end());
    }

    فراغ removeAssignmentsNotStrongerThanUniformConstant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        لكل (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            إذا (assigns.size() <= 1) استمر;
            ثابت AstAssignW* ثابت strongest0p = get_Strongest_Assignment_Of_قىممەت(assigns, 0);
            ثابت AstAssignW* ثابت strongest1p = get_Strongest_Assignment_Of_قىممەت(assigns, 1);
            ثابت AstAssignW* strongestp = nullptr;
            uint8_t greatestKnownStrength = 0;
            ثابت auto getIfStrongest
                = [&](ثابت AstAssignW* ثابت strongestCandidatep, منطقي قىممەت) {
                      إذا (!strongestCandidatep) أرجع;
                      ثابت uint8_t strength = getStrength(strongestCandidatep, قىممەت);
                      إذا (strength >= greatestKnownStrength) {
                          greatestKnownStrength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            إذا (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    فراغ removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        لكل (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            إذا (assigns.size() <= 1) استمر;
            auto maxIt = std::max_element(
                assigns.begin(), assigns.end(), [&](ثابت AstAssignW* ap, ثابت AstAssignW* bp) {
                    إذا (m_tgraph.isTristate(ap)) أرجع !m_tgraph.isTristate(bp);
                    إذا (m_tgraph.isTristate(bp)) أرجع خطأ;
                    ثابت uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ثابت uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    أرجع minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ثابت AstAssignW* ثابت strongestp = m_tgraph.isTristate(*maxIt) ? nullptr : *maxIt;
            إذا (strongestp) {
                ثابت uint8_t greatestKnownStrength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    // VISITORS
    فراغ visit(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        إذا (m_graphing) {
            إذا (!m_alhs && nodep->رەقەم().hasZ()) m_tgraph.setTristate(nodep);
        } وإلا {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            إذا (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ثابت varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_تۈگۈن* ثابت newp = جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
            } وإلا إذا (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_قۇر* ثابت fl = nodep->fileline();
                AstConst* ثابت enp = getNonZConstp(nodep);
                V3_سان num1{nodep, nodep->width()};
                num1.opAnd(nodep->رەقەم(), enp->رەقەم());  // 01X->01X, Z->0
                AstConst* ثابت newconstp = جديد AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    فراغ visit(AstCond* nodep) override {
        إذا (m_graphing) {
            iterateChildren(nodep);
            إذا (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } وإلا {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } وإلا {
            إذا (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                أرجع;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_تۈگۈن_ئىپادە* ثابت condp = nodep->condp();
            إذا (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_تۈگۈن_ئىپادە* ثابت thenp = nodep->thenp();
            Ast_تۈگۈن_ئىپادە* ثابت elsep = nodep->elsep();
            إذا (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_تۈگۈن_ئىپادە* ثابت en1p = getEnp(thenp);
                Ast_تۈگۈن_ئىپادە* ثابت en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_تۈگۈن_ئىپادە* ثابت enp
                    = جديد AstCond{nodep->fileline(), condp->clone_دەرەخ(خطأ), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                إذا (thenp->user1p() && !thenp->user1p()->backp()) ئىتتىرىش_Deletep(thenp->user1p());
                إذا (elsep->user1p() && !elsep->user1p()->backp()) ئىتتىرىش_Deletep(elsep->user1p());
                thenp->user1p(nullptr);
                elsep->user1p(nullptr);
            }
        }
    }

    فراغ visit(Ast_ئىپادە_Stmt* nodep) override {
        iterateChildren(nodep);
        إذا (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } وإلا إذا (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(nullptr);
            m_tgraph.didProcess(nodep);
        }
    }

    فراغ visit(AstSel* nodep) override {
        إذا (m_graphing) {
            iterateChildren(nodep);
            إذا (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } وإلا {
                associateLogic(nodep->fromp(), nodep);
            }
        } وإلا {
            إذا (m_alhs) {
                UINFO(9, dbgState() << nodep);
                إذا (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_تۈگۈن_ئىپادە* ثابت newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), تۈگۈن_ئىپادە));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } وإلا {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                إذا (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                إذا (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_تۈگۈن_ئىپادە* ثابت en1p = getEnp(nodep->fromp());
                    Ast_تۈگۈن_ئىپادە* ثابت enp
                        = جديد AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_دەرەخ_Pure(true),
                                     nodep->widthConst()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    فراغ visit(AstConcat* nodep) override {
        إذا (m_graphing) {
            iterateChildren(nodep);
            إذا (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } وإلا {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } وإلا {
            إذا (m_alhs) {
                UINFO(9, dbgState() << nodep);
                إذا (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_تۈگۈن_ئىپادە* ثابت enp = VN_AS(nodep->user1p(), تۈگۈن_ئىپادە);
                    nodep->user1p(nullptr);
                    nodep->lhsp()->user1p(جديد AstSel{nodep->fileline(), enp->clone_دەرەخ_Pure(true),
                                                     nodep->rhsp()->width(),
                                                     nodep->lhsp()->width()});
                    nodep->rhsp()->user1p(
                        جديد AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->width()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } وإلا {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_تۈگۈن_ئىپادە* ثابت expr1p = nodep->lhsp();
                Ast_تۈگۈن_ئىپادە* ثابت expr2p = nodep->rhsp();
                إذا (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_تۈگۈن_ئىپادە* ثابت en1p = getEnp(expr1p);
                    Ast_تۈگۈن_ئىپادە* ثابت en2p = getEnp(expr2p);
                    Ast_تۈگۈن_ئىپادە* ثابت enp = جديد AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(nullptr);
                    expr2p->user1p(nullptr);
                }
            }
        }
    }

    فراغ visit(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        إذا (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.setTristate(nodep);
        } وإلا {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            إذا (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                أرجع;
            }
            m_tgraph.didProcess(nodep);
            Ast_تۈگۈن_ئىپادە* ثابت expr1p = nodep->lhsp()->unlinkFrBack();
            Ast_تۈگۈن_ئىپادە* ثابت expr2p = nodep->rhsp()->unlinkFrBack();
            Ast_تۈگۈن_ئىپادە* enp;
            إذا (Ast_تۈگۈن_ئىپادە* ثابت en2p = VN_AS(expr2p->user1p(), تۈگۈن_ئىپادە)) {
                enp = جديد AstAnd{nodep->fileline(), expr1p, en2p};
            } وإلا {
                enp = expr1p;
            }
            expr1p->user1p(nullptr);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
        }
    }

    فراغ visitAndOr(Ast_تۈگۈن_Biop* nodep, منطقي isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        إذا (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } وإلا {
            إذا (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                أرجع;
            }
            // ANDs and Z's have issues. Earlier optimizations convert
            // expressions like "(COND) ? 1'bz : 1'b0" to "COND & 1'bz". So we
            // have to define what is means to AND 1'bz with other
            // expressions. I don't think this is spec, but here I take the
            // approach that when one expression is 1, that the Z passes. This
            // makes the COND's work. It is probably better to not perform the
            // conditional optimization if the bits are Z.
            //
            // ORs have the same issues as ANDs. Earlier optimizations convert
            // expressions like "(COND) ? 1'bz : 1'b1" to "COND | 1'bz". So we
            // have to define what is means to OR 1'bz with other
            // expressions. Here I take the approach that when one expression
            // is 0, that is passes the other.
            Ast_تۈگۈن_ئىپادە* ثابت expr1p = nodep->lhsp();
            Ast_تۈگۈن_ئىپادە* ثابت expr2p = nodep->rhsp();
            إذا (!expr1p->user1p() && !expr2p->user1p()) {
                أرجع;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_تۈگۈن_ئىپادە* en1p = getEnp(expr1p);
            Ast_تۈگۈن_ئىپادە* en2p = getEnp(expr2p);
            Ast_تۈگۈن_ئىپادە* subexpr1p = expr1p->clone_دەرەخ_Pure(خطأ);
            Ast_تۈگۈن_ئىپادە* subexpr2p = expr2p->clone_دەرەخ_Pure(خطأ);
            إذا (isAnd) {
                subexpr1p = جديد AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = جديد AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            إذا (!en1p->op1p() && en2p->op1p()) {
                std::swap(en1p, en2p);
                std::swap(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_تۈگۈن_ئىپادە* ثابت enp = جديد AstOr{
                nodep->fileline(),
                جديد AstAnd{nodep->fileline(), en1p, جديد AstOr{nodep->fileline(), en2p, subexpr1p}},
                جديد AstAnd{nodep->fileline(), en2p->clone_دەرەخ(خطأ), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(nullptr);
            expr2p->user1p(nullptr);
        }
    }
    فراغ visit(AstAnd* nodep) override { visitAndOr(nodep, true); }
    فراغ visit(AstOr* nodep) override { visitAndOr(nodep, خطأ); }

    فراغ visitAssign(Ast_تۈگۈن_Assign* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        إذا (VN_IS(nodep->rhsp(), CReset)) أرجع;
        إذا (AstAssignW* ثابت assignWp = VN_CAST(nodep, AssignW)) {
            m_currentDelayp = VN_CAST(assignWp->timingControlp(), Delay);
        }
        إذا (m_graphing) {
            إذا (AstAssignW* assignWp = VN_CAST(nodep, AssignW)) {
                إذا (assignWp->getLhsNetDelay()) أرجع;
                إذا (assignWp->timingControlp() && !m_currentDelayp) أرجع;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                إذا (m_currentDelayp && m_currentDelayp->fallDelay()) أرجع;
                إذا (!assignWp->timingControlp()) addToAssignmentList(assignWp);
            }

            إذا (nodep->user2() & U2_GRAPHING) أرجع;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterateAndNextNull(nodep->rhsp());
            m_alhs = true;
            iterateAndNextNull(nodep->lhsp());
            m_alhs = خطأ;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } وإلا {
            إذا (nodep->user2() & U2_NONGRAPH) {
                أرجع;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterateAndNextNull(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            إذا (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, AssignW), nodep,
                            "Continuous assignment inside procedural process");
                Ast_تۈگۈن_ئىپادە* ثابت enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), تۈگۈن_ئىپادە)->clone_دەرەخ_Pure(خطأ)
                          : newAllZerosOrOnes(nodep->lhsp(), true);
                ثابت منطقي inserted = m_assignEnps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            إذا (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(nullptr);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } وإلا {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                إذا (AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel)) {
                    إذا (Ast_تۈگۈن_Var_Ref* ثابت varrefp = VN_CAST(selp->fromp(), تۈگۈن_Var_Ref)) {
                        إذا (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), true));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } وإلا {
                            m_tgraph.didProcess(nodep, true);
                        }
                    } وإلا {
                        m_tgraph.didProcess(nodep, true);  // LCOV_EXCL_LINE
                    }
                } وإلا {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, true);
                }
            }
            m_alhs = true;  // And user1p() will indicate tristate equation, if any
            إذا (AstAssignW* ثابت assignWp = VN_CAST(nodep, AssignW)) {
                إذا (AstStrengthSpec* ثابت specp = assignWp->strengthSpecp()) {
                    إذا (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        إذا (!isOnlyAssignmentIsToLhsVar(assignWp)) {
                            assignWp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } وإلا {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterateAndNextNull(nodep->lhsp());
        }
    }
    فراغ visit(AstAssignW* nodep) override { visitAssign(nodep); }
    فراغ visit(AstAssign* nodep) override { visitAssign(nodep); }
    فراغ visit(AstAssignDly* nodep) override { visitAssign(nodep); }

    فراغ visit(Ast_تۈگۈن_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ثابت AstAlways* ثابت alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN ? nullptr : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    فراغ visit(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_inAlias);
        m_inAlias = true;
        إذا (m_graphing) {
            إذا (nodep->user2() & U2_GRAPHING) أرجع;
            m_alhs = true;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            لكل (Ast_تۈگۈن* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } وإلا {
            iterateChildren(nodep);
        }
    }

    فراغ visitCaseEq(Ast_تۈگۈن_Biop* nodep, منطقي neq) {
        إذا (m_graphing) {
            iterateChildren(nodep);
        } وإلا {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
            إذا (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_تۈگۈن_ئىپادە* ثابت rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                File_قۇر* ثابت fl = nodep->fileline();
                Ast_تۈگۈن_ئىپادە* enRhsp;
                إذا (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), تۈگۈن_ئىپادە);
                    rhsp->user1p(nullptr);
                } وإلا {
                    enRhsp = get_En_ئىپادە_Based_On_Originalp(rhsp);
                }
                Ast_تۈگۈن_ئىپادە* newp
                    = جديد AstLogAnd{fl, جديد AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    جديد AstEqCase{fl, جديد AstConst{fl, constp->رەقەم()}, rhsp}};
                constp->user1p(nullptr);
                إذا (neq) newp = جديد AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
            } وإلا إذا (constp && nodep->rhsp()->user1p()) {
                File_قۇر* ثابت fl = nodep->fileline();
                constp->unlinkFrBack();
                Ast_تۈگۈن_ئىپادە* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_تۈگۈن_ئىپادە* newp = جديد AstLogAnd{fl,
                                                  جديد AstEq{fl, newAllZerosOrOnes(constp, خطأ),
                                                            VN_AS(rhsp->user1p(), تۈگۈن_ئىپادە)},
                                                  // Keep the caseeq if there are X's present
                                                  جديد AstEqCase{fl, constp, rhsp}};
                إذا (neq) newp = جديد AstLogNot{fl, newp};
                rhsp->user1p(nullptr);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
            } وإلا {
                checkUnhandled(nodep);
            }
        }
    }
    فراغ visitEqNeqWild(Ast_تۈگۈن_Biop* nodep) {
        إذا (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterateAndNextNull(nodep->lhsp());
        إذا (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            أرجع;
        }
    }
    فراغ visit(AstEqCase* nodep) override { visitCaseEq(nodep, خطأ); }
    فراغ visit(AstNeqCase* nodep) override { visitCaseEq(nodep, true); }
    فراغ visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    فراغ visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }

    فراغ visit(Ast_سانى_Bits* nodep) override {
        std::array<منطقي, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->رەقەم().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->رەقەم().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->رەقەم().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        إذا (m_graphing) {
            iterateAndNextNull(nodep->lhsp());
            إذا (!dropop[0]) iterateAndNextNull(nodep->rhsp());
            إذا (!dropop[1]) iterateAndNextNull(nodep->thsp());
            إذا (!dropop[2]) iterateAndNextNull(nodep->fhsp());
        } وإلا {
            Ast_تۈگۈن_ئىپادە* nonXp = nullptr;
            إذا (!dropop[0]) {
                nonXp = nodep->rhsp();
            } وإلا إذا (!dropop[1]) {
                nonXp = nodep->thsp();
            } وإلا إذا (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            إذا (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                إذا (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    أرجع;
                }
                AstVar* ثابت envarp = getCreateEnVarp(varrefp->varp(), خطأ);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                Ast_تۈگۈن_ئىپادە* newp = جديد Ast_سانى_Ones{
                    nodep->fileline(), جديد AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                إذا (nonXp) {  // Need to still count '0 or '1 or 'x's
                    إذا (dropop[0]) {
                        nodep->rhsp()->unlinkFrBack()->delete_دەرەخ();
                        nodep->rhsp(nonXp->clone_دەرەخ_Pure(true));
                    }
                    إذا (dropop[1]) {
                        nodep->thsp()->unlinkFrBack()->delete_دەرەخ();
                        nodep->thsp(nonXp->clone_دەرەخ_Pure(true));
                    }
                    إذا (dropop[2]) {
                        nodep->fhsp()->unlinkFrBack()->delete_دەرەخ();
                        nodep->fhsp(nonXp->clone_دەرەخ_Pure(true));
                    }
                    newp = جديد AstAdd{nodep->fileline(), nodep, newp};
                } وإلا {
                    ئىتتىرىش_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = nullptr;
        إذا (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } وإلا إذا (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        إذا (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } وإلا {
            إذا (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.setTristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } وإلا {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ثابت AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel);
                ثابت AstConst* ثابت lsbp = selp ? VN_CAST(selp->lsbp(), Const) : nullptr;
                إذا (lsbp) {
                    ثابت صحيح lsb = lsbp->toSInt();
                    لكل (صحيح i = 0; i < selp->widthConst(); ++i) {
                        setBitPullDirection(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } وإلا {
                    setPullDirection(varrefp->varp(), nodep);
                }
            }
        }
        إذا (!m_graphing) {
            nodep->unlinkFrBack();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
        }
    }

    فراغ iteratePinGuts(AstPin* nodep) {
        إذا (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            إذا (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } وإلا {
            // All heavy lifting completed in graph visitor.
            إذا (nodep->exprp()) m_tgraph.didProcess(nodep);
            iterateChildren(nodep);
        }
    }

    // .tri(SEL(trisig,x)) becomes
    //   INPUT:   -> (VARREF(trisig__pinin)),
    //               trisig__pinin = SEL(trisig,x)       // via pinReconnectSimple
    //   OUTPUT:  -> (VARREF(trisig__pinout))
    //               SEL(trisig,x) = trisig__pinout
    //                                  ^-- ->user1p() == trisig__pinen
    //   ENABLE:  -> (VARREF(trisig__pinen)
    // Added complication is the signal may be an output/inout or just
    // input with tie off (or not) up top
    //     PIN  PORT    NEW PORTS AND CONNECTIONS
    //     N/C  input   in(from-resolver), __en(to-resolver-only), __out(to-resolver-only)
    //     N/C  inout   Spec says illegal
    //     N/C  output  Unsupported; Illegal?
    //     wire input   in(from-resolver-with-wire-value), __en(from-resolver-wire),
    //                     __out(to-resolver-only)
    //     wire inout   in, __en, __out
    //     wire output  in, __en, __out
    //     const input  in(from-resolver-with-const-value), __en(from-resolver-const),
    //                     __out(to-resolver-only)
    //     const inout  Spec says illegal
    //     const output Unsupported; Illegal?
    فراغ visit(AstPin* nodep) override {
        إذا (m_graphing) {
            إذا (nodep->user2() & U2_GRAPHING) أرجع;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ثابت enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            إذا (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, خطأ);
                iteratePinGuts(nodep);
                أرجع;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ثابت منطقي inDeclProcessing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::كىرگۈزۈش
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            إذا (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ثابت ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(جديد AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.setTristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } وإلا إذا (inDeclProcessing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ثابت AstAssignW* ثابت reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, true, true);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.setTristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ثابت enVarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvtToStr(m_unique++),
                                              VFlagBitPacked{}, enModVarp->width()};
            إذا (inDeclProcessing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->varType2In();
            } وإلا {
                enModVarp->varType2Out();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ثابت enpinp
                = جديد AstPin{nodep->fileline(), nodep->pin_رەقەم(),
                             enModVarp->name(),  // should be {var}"__en"
                             جديد AstVarRef{nodep->fileline(), enVarp,
                                           inDeclProcessing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->addNextHere(enpinp);
            m_modp->addStmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ثابت AstAssignW* outAssignp = nullptr;  // If reconnected, the related assignment
            AstPin* outpinp = nullptr;
            AstVar* ثابت outModVarp = m_varAux(nodep->modVarp()).outVarp;
            إذا (!outModVarp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                إذا (!m_modp->is_چوققا()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } وإلا {
                Ast_تۈگۈن_ئىپادە* ثابت outexprp = VN_AS(nodep->exprp(), تۈگۈن_ئىپادە)
                                                  ->clone_دەرەخ_Pure(خطأ);  // Note has lvalue() set
                outpinp = جديد AstPin{nodep->fileline(), nodep->pin_رەقەم(),
                                     outModVarp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(outModVarp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->addNextHere(outpinp);
                // Simplify
                إذا (inDeclProcessing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, true};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                outAssignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        true);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, outAssignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ثابت TristatePinVisitor visitor{nodep->exprp(), m_tgraph, خطأ};
            ثابت Ast_تۈگۈن* ثابت inAssignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, true);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, inAssignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            إذا (!outAssignp) {
                إذا (!outpinp) {
                    exprrefp = nullptr;  // Primary input only
                } وإلا {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    إذا (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } وإلا {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(outAssignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                إذا (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            إذا (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(جديد AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                إذا (!outAssignp) {
                    map_قىستۇرۇش_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            إذا (exprrefp) {
                AstPull* ثابت pullp = m_varAux(nodep->modVarp()).pullp;
                ثابت auto& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = nullptr;
                صحيح selLsb = 0;
                صحيح selWidth = 0;
                إذا (outAssignp) {
                    إذا (AstSel* ثابت selp = VN_CAST(outAssignp->lhsp(), Sel)) {
                        AstVarRef* ثابت vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
                        إذا (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->toSInt();
                            selWidth = selp->widthConst();
                        }
                    }
                }

                إذا (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    setPullDirection(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    إذا (selVarp) {
                        ثابت صحيح direction = pullp->direction();
                        لكل (صحيح i = 0; i < selWidth; ++i) {
                            setBitPullDirection(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                إذا (!srcBitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    لكل (ثابت auto& pair : srcBitPulls) {
                        setBitPullDirection(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    إذا (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        لكل (ثابت auto& pair : srcBitPulls) {
                            setBitPullDirection(selVarp, selLsb + pair.first, pair.second);
                        }  // LCOV_EXCL_LINE
                    }
                }
            }
            // Don't need to visit the created assigns, as it was added at
            // the end of the next links and normal iterateChild recursion
            // will come back to them eventually.
            // Mark the original signal as tristated
            iteratePinGuts(nodep);
        }
        // Not graph building
        وإلا {
            إذا (nodep->user2() & U2_NONGRAPH) أرجع;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    فراغ handle_تۈگۈن_Var_Ref(Ast_تۈگۈن_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        إذا (m_graphing) {
            إذا (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            إذا (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            إذا (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.setTristate(nodep->varp());
            }
        } وإلا {
            إذا (nodep->user2() & U2_NONGRAPH) أرجع;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            إذا (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                إذا (m_inAlias) {
                    إذا (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } وإلا {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    أرجع;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_قىستۇرۇش_Lhs_Var_Ref(nodep);
            } وإلا إذا (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_قۇر* ثابت fl = nodep->fileline();
                AstVar* ثابت enVarp = getCreateEnVarp(nodep->varp(), خطأ);
                إذا (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } وإلا {
                    nodep->user1p(جديد AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    فراغ visit(AstVarRef* nodep) override { handle_تۈگۈن_Var_Ref(nodep); }
    فراغ visit(AstVarXRef* nodep) override { handle_تۈگۈن_Var_Ref(nodep); }

    فراغ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        إذا (m_graphing) {
            // If tri0/1 force a pullup
            إذا (nodep->user2() & U2_GRAPHING) أرجع;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            إذا (nodep->isPulldown() || nodep->isPullup()) {
                Ast_تۈگۈن* ثابت newp = جديد AstPull{
                    nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->addNextHere(newp);
                // We'll iterate on the new AstPull later
            }
            إذا (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.setTristate(nodep);
            }
        } وإلا {  // !graphing
            إذا (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    فراغ visit(Ast_تۈگۈن_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        إذا (m_processedIfaces && VN_IS(nodep, Iface)) أرجع;
        UINFO(8, dbgState() << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_graphing);
        VL_RESTORER(m_unique);
        VL_RESTORER_CLEAR(m_lhsmap);
        VL_RESTORER_CLEAR(m_assigns);
        VL_RESTORER_CLEAR(m_assignEnps);
        VL_RESTORER_CLEAR(m_proceduralEnables);
        // Not preserved, needs pointer instead: TristateGraph origTgraph = m_tgraph;
        UASSERT_OBJ(m_tgraph.empty(), nodep, "Unsupported: NodeModule under NodeModule");

        // Clear state
        m_graphing = خطأ;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = nullptr;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = true;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = خطأ;
        UINFO(9, dbgState() << "processing mod " << nodep);

        // Merge the assignments for very Wired net LHS : wor, trior, wand and triand
        mergeWiredNetsAssignments();
        // Remove all assignments not stronger than the strongest uniform constant
        removeAssignmentsNotStrongerThanUniformConstant();
        // Use graph to find tristate signals
        m_tgraph.graphWalk(nodep);

        // Remove all assignments not stronger than the strongest non-tristate RHS
        removeAssignmentsNotStrongerThanNonTristate();

        // Build the LHS drivers map for this module
        iterateChildren(nodep);
        // Insert new logic for all tristates
        قىستۇرۇش_Tristates(nodep);
        قىستۇرۇش_Procedural_Enable_Assignments(nodep);

        لكل (auto& assignEnp : m_assignEnps) {
            VL_DO_DANGLING(assignEnp.second->delete_دەرەخ(), assignEnp.second);
        }
        m_assignEnps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    فراغ visit(AstClass* nodep) override {
        // don't deal with classes
    }
    فراغ visit(Ast_تۈگۈن_FTask* nodep) override {
        // don't deal with functions
    }

    فراغ visit(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterateAndNextNull(nodep->stmtsp());
    }

    فراغ visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = خطأ;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    فراغ visit(Ast_تۈگۈن* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    فراغ combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        لكل (ثابت std::pair<AstVar* ثابت, std::vector<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ثابت invarp = kv.first;
            ثابت std::vector<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_تۈگۈن_Module* ثابت ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_قۇر* ثابت fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.size() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_تۈگۈن_ئىپادە* orp = nullptr;
            Ast_تۈگۈن_ئىپادە* enp = nullptr;

            لكل (ثابت std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ثابت contribEnp = contrib.first;
                AstVar* ثابت contribOutp = contrib.second;

                Ast_تۈگۈن_ئىپادە* ثابت outRefp = جديد AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_تۈگۈن_ئىپادە* ثابت enRefp = جديد AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_تۈگۈن_ئىپادە* ثابت andp = جديد AstAnd{fl, outRefp, enRefp};

                orp = (!orp) ? andp : جديد AstOr{fl, orp, andp};

                Ast_تۈگۈن_ئىپادە* ثابت enRef2p = جديد AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : جديد AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            إذا (!envarp) {
                envarp = جديد AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->setDfgTriLowered();
                ifaceModp->addStmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            AstAssignW* ثابت enAssp
                = جديد AstAssignW{fl, جديد AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->addStmtsp(جديد AstAlways{enAssp});

            // Pull resolution
            ثابت AstPull* ثابت pullp = m_varAux(invarp).pullp;
            ثابت منطقي pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_تۈگۈن_ئىپادە* undrivenp = جديد AstNot{fl, جديد AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = جديد AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = جديد AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            AstAssignW* ثابت assp
                = جديد AstAssignW{fl, جديد AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->addStmtsp(جديد AstAlways{assp});
        }
    }

عام:
    // CONSTRUCTORS
    explicit TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::vector<Ast_تۈگۈن_Module*> ifacesp;
        لكل (Ast_تۈگۈن* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            إذا (VN_IS(modp, Iface)) ifacesp.ئىتتىرىش_back(VN_AS(modp, تۈگۈن_Module));
        }
        لكل (auto it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = true;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ifdef VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::vector<Ast_تۈگۈن*> unusedRootps;
        netlistp->foreach([&](Ast_تۈگۈن* nodep) {
            Ast_تۈگۈن* ثابت enp = nodep->user1p();
            إذا (!enp) أرجع;
            إذا (enp->backp()) أرجع;
            unusedRootps.emplace_back(enp);
        });
        لكل (Ast_تۈگۈن* ثابت nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
#endif
    }
    ~TristateVisitor() override {
        V3Stats::addStat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

فراغ V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_دەرەخ("tristate", 0, dump_دەرەخ_Either_Level() >= 3);
}
