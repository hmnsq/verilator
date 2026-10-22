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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Tristate.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Inst.h"
#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ TristateBaseVisitor VL_NOT_FINAL : જાહેર VNVisitor {
જાહેર:
    // METHODS
};

//######################################################################
// Graph support classes

વર્ગ TristateVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_ગાંઠ* અચલ m_nodep;
    તાર્કિક m_isTristate = ખોટું;  // Logic indicates a tristate
    તાર્કિક m_feedsTri = ખોટું;  // Propagates to a tristate node (on RHS)
    તાર્કિક m_processed = ખોટું;  // Tristating was cleaned up
જાહેર:
    TristateVertex(V3Graph* graphp, Ast_ગાંઠ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = મૂળભૂત;
    // ACCESSORS
    Ast_ગાંઠ* nodep() અચલ VL_MT_STABLE { પાછા m_nodep; }
    અચલ AstVar* varp() અચલ { પાછા VN_CAST(nodep(), Var); }
    string name() અચલ override {
        પાછા ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvtToHex(nodep())));
    }
    string dotColor() અચલ override {
        પાછા (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_લીટી* fileline() અચલ override { પાછા nodep()->fileline(); }
    તાર્કિક isTristate() અચલ VL_MT_SAFE { પાછા m_isTristate; }
    શૂન્ય isTristate(તાર્કિક flag) { m_isTristate = flag; }
    તાર્કિક feedsTri() અચલ VL_MT_SAFE { પાછા m_feedsTri; }
    શૂન્ય feedsTri(તાર્કિક flag) { m_feedsTri = flag; }
    તાર્કિક processed() અચલ { પાછા m_processed; }
    શૂન્ય processed(તાર્કિક flag) { m_processed = flag; }
};

//######################################################################

વર્ગ TristateGraph final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
જાહેર:
    using VarVec = std::vector<AstVar*>;

ખાનગી:
    // MEMBERS
    V3Graph m_graph;  // Logic graph

જાહેર:
    // CONSTRUCTORS
    TristateGraph() { clearAndCheck(); }
    આભાસી ~TristateGraph() { clearAndCheck(); }
    VL_UNCOPYABLE(TristateGraph);

ખાનગી:
    // METHODS

    TristateVertex* makeVertex(Ast_ગાંઠ* nodep) {
        TristateVertex* vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        જો (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = નવું TristateVertex{&m_graph, nodep};
            nodep->user4p(vertexp);
        }
        પાછા vertexp;
    }

    // METHODS - Graph optimization
    શૂન્ય graphWalkRecurseFwd(TristateVertex* vtxp, પૂર્ણાંક level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        જો (!vtxp->isTristate()) પાછા;  // tristate involved
        જો (vtxp->user() == 1) પાછા;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        જો (!vtxp->varp()) {  // not a var where we stop the recursion
            માટે (V3GraphEdge& edge : vtxp->outEdges()) {
                TristateVertex* અચલ vvertexp = static_cast<TristateVertex*>(edge.ટોચ());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                જો (!vvertexp->isTristate()) {
                    vvertexp->isTristate(સાચું);
                    graphWalkRecurseFwd(vvertexp, level + 1);
                }
            }
        } નહીંતર {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            માટે (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* અચલ vvertexp = static_cast<TristateVertex*>(edge.fromp());
                જો (અચલ AstVarRef* અચલ refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    જો (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(સાચું);
                        graphWalkRecurseFwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    શૂન્ય graphWalkRecurseBack(TristateVertex* vtxp, પૂર્ણાંક level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        જો (!(vtxp->isTristate() || vtxp->feedsTri())) પાછા;  // tristate involved
        જો (vtxp->user() == 3) પાછા;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        જો (!vtxp->varp()) {  // not a var where we stop the recursion
            માટે (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* અચલ vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                જો (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(સાચું);
                    graphWalkRecurseBack(vvertexp, level + 1);
                }
            }
        }
    }

જાહેર:
    // METHODS
    તાર્કિક empty() અચલ { પાછા m_graph.empty(); }
    શૂન્ય clearAndCheck() {
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            અચલ TristateVertex& vvertex = static_cast<TristateVertex&>(vtx);
            જો (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_graph.clear();
        Ast_ગાંઠ::user4_Clear_વૃક્ષ();  // Wipe all node user4p's that point to vertexes
    }
    શૂન્ય graphWalk(Ast_ગાંઠ_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseBack(static_cast<TristateVertex*>(&vtx), 0);
        }
        જો (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    શૂન્ય associate(Ast_ગાંઠ* fromp, Ast_ગાંઠ* ટોચ) {
        નવું V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(ટોચ), 1};
    }
    શૂન્ય deleteVerticesFromSubtreeRecurse(Ast_ગાંઠ* nodep) {
        જો (!nodep) પાછા;
        // Skip vars, because they may be connected to more than one varref
        જો (!VN_IS(nodep, Var)) {
            TristateVertex* અચલ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            જો (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        જો (Ast_ગાંઠ* અચલ refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    શૂન્ય setTristate(Ast_ગાંઠ* nodep) { makeVertex(nodep)->isTristate(સાચું); }
    તાર્કિક isTristate(અચલ Ast_ગાંઠ* nodep) {
        અચલ TristateVertex* અચલ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        પાછા vertexp && vertexp->isTristate();
    }
    તાર્કિક feedsTri(અચલ Ast_ગાંઠ* nodep) {
        અચલ TristateVertex* અચલ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        પાછા vertexp && vertexp->feedsTri();
    }
    શૂન્ય didProcess(Ast_ગાંઠ* nodep, તાર્કિક quiet = ખોટું) {
        જો (TristateVertex* અચલ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(સાચું);
        } નહીંતર જો (!quiet) {
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
        માટે (અચલ V3GraphVertex& vtx : m_graph.vertices()) {
            અચલ TristateVertex& vvertex = static_cast<અચલ TristateVertex&>(vtx);
            જો (vvertex.isTristate()) {
                જો (AstVar* અચલ nodep = VN_CAST(vvertex.nodep(), Var)) v.ધકેલો_back(nodep);
            }
        }
        પાછા v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

વર્ગ TristatePinVisitor final : જાહેર TristateBaseVisitor {
    TristateGraph& m_tgraph;
    અચલ તાર્કિક m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        જો (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } નહીંતર જો (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.setTristate(nodep->varp());
        }
    }
    શૂન્ય visit(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstSliceSel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_ગાંઠ* nodep, TristateGraph& tgraph, તાર્કિક lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = મૂળભૂત;
};

//######################################################################

વર્ગ TristateVisitor final : જાહેર TristateBaseVisitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;
    અચલ VNUser3InUse m_inuser3;
    અચલ VNUser4InUse m_inuser4;

    સંરચના AuxAstVar final {
        AstPull* pullp = nullptr;  // pullup/pulldown direction (whole variable)
        AstVar* outVarp = nullptr;  // output __out var
        તાર્કિક ifaceTristate = ખોટું;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<પૂર્ણાંક, પૂર્ણાંક>
            bitPulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    સંરચના RefStrength final {
        Ast_ગાંઠ_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_ગાંઠ_Assign* m_assignp;  // Assignment containing m_varrefp
        અચલ Ast_ગાંઠ_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_ગાંઠ_પદાવલિ* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_ગાંઠ_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_ગાંઠ_પદાવલિ* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_ગાંઠ_Var_Ref* varrefp, Ast_ગાંઠ_Assign* assignp,
                    અચલ Ast_ગાંઠ_Procedure* procedurep, Ast_ગાંઠ_પદાવલિ* enLhsp,
                    Ast_ગાંઠ_Var_Ref* enVarrefp, Ast_ગાંઠ_પદાવલિ* enRhsp, VStrength strength,
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
    સંરચના LhsProjection final {
        Ast_ગાંઠ_પદાવલિ* m_lhsp;  // LHS giving the enable assign
        Ast_ગાંઠ_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_ગાંઠ_પદાવલિ* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    સંરચના ProceduralEnable final {
        Ast_ગાંઠ_પદાવલિ* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_ગાંઠ_પદાવલિ* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    using RefStrengthVec = std::vector<RefStrength>;
    using VarMap = std::map<AstVar*, RefStrengthVec*>;
    using Assigns = std::vector<AstAssignW*>;
    using VarToAssignsMap = std::map<AstVar*, Assigns>;
    using AssignEnableMap = std::map<Ast_ગાંઠ_Assign*, Ast_ગાંઠ_પદાવલિ*>;
    using ProceduralEnableMap = std::map<Ast_ગાંઠ_Assign*, std::vector<ProceduralEnable>>;
    ગણના : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    તાર્કિક m_graphing = ખોટું;  // Major mode - creating graph
    //
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstCell* m_cellp = nullptr;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    VarToAssignsMap m_assigns;  // Assigns in current module
    AssignEnableMap m_assignEnps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    પૂર્ણાંક m_unique = 0;
    તાર્કિક m_alhs = ખોટું;  // On LHS of assignment
    તાર્કિક m_inAlias = ખોટું;  // Inside alias statement
    તાર્કિક m_processedIfaces = ખોટું;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = nullptr;  // Delay of assignment currently collecting LHS drivers
    અચલ Ast_ગાંઠ* m_logicp = nullptr;  // Current logic being built
    Ast_ગાંઠ_Assign* m_assignp = nullptr;  // Current assignment
    અચલ Ast_ગાંઠ_Procedure* m_procedurep = nullptr;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::vector<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() અચલ {
        string o = (m_graphing ? " gr " : " ng ");
        જો (m_alhs) o += "alhs ";
        પાછા o;
    }
    શૂન્ય modAddStmtp(Ast_ગાંઠ* nodep, Ast_ગાંઠ* newp) {
        જો (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } નહીંતર {
            m_modp->addStmtsp(newp);
        }
    }
    શૂન્ય associateLogic(Ast_ગાંઠ* fromp, Ast_ગાંઠ* ટોચ) {
        જો (m_logicp) m_tgraph.associate(fromp, ટોચ);
    }
    AstConst* newAllZerosOrOnes(Ast_ગાંઠ* nodep, તાર્કિક ones) {
        V3_સંખ્યા અંક{nodep, nodep->width()};
        જો (ones) અંક.setAllBits1();
        AstConst* અચલ newp = નવું AstConst{nodep->fileline(), અંક};
        પાછા newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_લીટી* fl, AstVar* varp, અચલ string& dotted, VAccess access,
                           અચલ string& inlinedDots) {
        AstVarXRef* અચલ xrefp = નવું AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        પાછા xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_ગાંઠ_Module* modp, અચલ string& dottedPath) {
        જો (dottedPath.empty()) પાછા nullptr;
        અચલ string firstComp = dottedPath.substr(0, dottedPath.find('.'));
        માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* અચલ ivarp = VN_CAST(stmtp, Var);
            જો (!ivarp || ivarp->name() != firstComp) ચાલુ;
            અચલ AstIfaceRefDType* અચલ ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            જો (ifaceDtp && ifaceDtp->modportp()) પાછા ifaceDtp->modportp();
            તોડો;
        }
        પાછા nullptr;
    }
    Ast_ગાંઠ_પદાવલિ* getEnp(Ast_ગાંઠ* nodep) {
        જો (nodep->user1p()) {
            જો (Ast_ગાંઠ_Var_Ref* અચલ refp = VN_CAST(nodep, ગાંઠ_Var_Ref)) {
                જો (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    પાછા newAllZerosOrOnes(nodep, સાચું);
                }
            }
        } નહીંતર {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, સાચું));
        }
        // Otherwise return the previous output enable
        પાછા VN_AS(nodep->user1p(), ગાંઠ_પદાવલિ);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    શૂન્ય addModportVarRefIfMissing(AstModport* modportp, AstVar* varp, VDirection dir) {
        માટે (Ast_ગાંઠ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* અચલ mvrp = VN_CAST(mrp, ModportVarRef);
            જો (mvrp && mvrp->varp() == varp) પાછા;  // Already present
        }
        AstModportVarRef* અચલ mvrp = નવું AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->addVarsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, તાર્કિક is_ટોચ) {
        // Return the master __en for the specified input variable
        જો (!invarp->user1p()) {
            AstVar* અચલ newp
                = નવું AstVar{invarp->fileline(), is_ટોચ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_ગાંઠ_Module* અચલ ownerModp = findParentModule(invarp);
            જો (ownerModp && ownerModp != m_modp) {
                ownerModp->addStmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                માટે (Ast_ગાંઠ* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* અચલ modportp = VN_CAST(stmtp, Modport);
                    જો (!modportp) ચાલુ;
                    માટે (Ast_ગાંઠ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* અચલ mvrp = VN_CAST(mrp, ModportVarRef);
                        જો (mvrp && mvrp->varp() == invarp) {
                            addModportVarRefIfMissing(modportp, newp, VDirection::આગત);
                            તોડો;
                        }
                    }
                }
            } નહીંતર {
                modAddStmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        પાછા VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* અચલ constp) {
        File_લીટી* અચલ fl = constp->fileline();
        V3_સંખ્યા numz{constp, constp->width()};
        numz.opBitsZ(constp->અંક());  // Z->1, else 0
        V3_સંખ્યા numz0{constp, constp->width()};
        numz0.opNot(numz);  // Z->0, else 1
        પાછા નવું AstConst{fl, numz0};
    }
    Ast_ગાંઠ_પદાવલિ* get_En_પદાવલિ_Based_On_Originalp(Ast_ગાંઠ_પદાવલિ* અચલ nodep) {
        જો (AstVarRef* અચલ varrefp = VN_CAST(nodep, VarRef)) {
            પાછા નવું AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), ખોટું),
                                 VAccess::READ};
        } નહીંતર જો (AstVarXRef* અચલ xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* અચલ enVarp = getCreateEnVarp(xrefp->varp(), ખોટું);
            પાછા newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } નહીંતર જો (AstConst* અચલ constp = VN_CAST(nodep, Const)) {
            પાછા getNonZConstp(constp);
        } નહીંતર જો (AstExtend* અચલ extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = extendp->lhsp()->unlinkFrBack();
            AstExtend* અચલ enExtendp = extendp->clone_વૃક્ષ(ખોટું);
            extendp->lhsp(lhsp);
            Ast_ગાંઠ_પદાવલિ* અચલ enLhsp = get_En_પદાવલિ_Based_On_Originalp(lhsp);
            enExtendp->lhsp(નવું AstNot{enLhsp->fileline(), enLhsp});
            પાછા નવું AstNot{enExtendp->fileline(), enExtendp};
        } નહીંતર જો (AstSel* અચલ selp = VN_CAST(nodep, Sel)) {
            Ast_ગાંઠ_પદાવલિ* અચલ fromp = selp->fromp()->unlinkFrBack();
            AstSel* અચલ enSelp = selp->clone_વૃક્ષ(ખોટું);
            selp->fromp(fromp);
            Ast_ગાંઠ_પદાવલિ* અચલ enFromp = get_En_પદાવલિ_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            પાછા enSelp;
        } નહીંતર {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            પાછા nullptr;
        }
    }
    AstVar* getCreateOutVarp(AstVar* invarp, તાર્કિક is_ટોચ) {
        // Return the master __out for the specified input variable
        જો (!m_varAux(invarp).outVarp) {
            AstVar* અચલ newp
                = નવું AstVar{invarp->fileline(), is_ટોચ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            modAddStmtp(invarp, newp);
            m_varAux(invarp).outVarp = newp;  // find outvar given invarp
        }
        પાછા m_varAux(invarp).outVarp;
    }
    AstVar* getCreateUnconnVarp(Ast_ગાંઠ* fromp, Ast_ગાંઠ_DType* dtypep) {
        AstVar* અચલ newp = નવું AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvtToStr(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        modAddStmtp(newp, newp);
        પાછા newp;
    }

    LhsProjection newLhsProjection(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_Var_Ref* targetp, Ast_ગાંઠ_પદાવલિ* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_ગાંઠ_પદાવલિ* અચલ newLhsp = lhsp->clone_વૃક્ષ_Pure(ખોટું);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_ગાંઠ* nodep) { nodep->user1p(nullptr); });
        Ast_ગાંઠ_Var_Ref* અચલ newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        પાછા LhsProjection{newLhsp, newVarrefp, enp};
    }

    શૂન્ય map_ઉમેરો_Lhs_Var_Ref(Ast_ગાંઠ_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* અચલ ચાવી = nodep->varp();
        અચલ auto pair = m_lhsmap.emplace(ચાવી, nullptr);
        જો (pair.second) pair.first->second = નવું RefStrengthVec;
        Ast_ગાંઠ_પદાવલિ* enLhsp = nullptr;
        Ast_ગાંઠ_Var_Ref* enVarrefp = nullptr;
        Ast_ગાંઠ_પદાવલિ* enRhsp = nullptr;
        અચલ auto enIt = m_assignEnps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        અચલ તાર્કિક canCombineProcedural = !VN_IS(ચાવી->dtypep()->skipRefp(), ગાંઠ_UOr_Struct_DType);
        અચલ Ast_ગાંઠ_Procedure* અચલ procedurep
            = enIt != m_assignEnps.end() && canCombineProcedural ? m_procedurep : nullptr;
        જો (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_વૃક્ષ_Pure(ખોટું));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->ધકેલો_back(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_ગાંઠ_પદાવલિ* newEnableDeposit(AstSel* selp, Ast_ગાંઠ_પદાવલિ* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstShiftL{
            selp->fileline(), નવું AstExtend{selp->fileline(), enp, selp->fromp()->width()},
            selp->lsbp()->clone_વૃક્ષ_Pure(ખોટું), selp->fromp()->width()};
        પાછા newp;
    }

    શૂન્ય setPullDirection(AstVar* varp, AstPull* pullp) {
        અચલ AstPull* અચલ oldpullp = m_varAux(varp).pullp;
        જો (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } નહીંતર {
            જો (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    શૂન્ય setBitPullDirection(AstVar* varp, પૂર્ણાંક bitIndex, પૂર્ણાંક direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        auto& bitPulls = m_varAux(varp).bitPulls;
        auto it = bitPulls.find(bitIndex);
        જો (it != bitPulls.end() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bitIndex << " of " << varp->prettyNameQ());
        }
        bitPulls[bitIndex] = direction;
    }

    AstConst* createPerBitPullConst(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        અચલ auto& bitPulls = m_varAux(varp).bitPulls;
        V3_સંખ્યા અંક{varp, varp->width(), 0};  // Start with all zeros
        માટે (અચલ auto& pair : bitPulls) {
            જો (pair.second == 1) અંક.setBit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        પાછા નવું AstConst{varp->fileline(), અંક};
    }

    તાર્કિક hasPerBitPulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        પાછા !m_varAux(varp).bitPulls.empty();
    }

    શૂન્ય checkUnhandled(Ast_ગાંઠ* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        જો (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        જો ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    શૂન્ય ઉમેરો_Tristates(Ast_ગાંઠ_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        અચલ TristateGraph::VarVec vars = m_tgraph.tristateVars();
        માટે (auto varp : vars) {
            જો (!m_tgraph.isTristate(varp)) ચાલુ;
            અચલ auto it = m_lhsmap.find(varp);
            જો (it != m_lhsmap.end()) ચાલુ;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            અચલ AstPull* અચલ pullp = m_varAux(varp).pullp;
            અચલ તાર્કિક pull_કિંમત = pullp && pullp->direction() == 1;
            AstConst* અચલ constp = newAllZerosOrOnes(varp, pull_કિંમત);
            AstVarRef* અચલ varrefp = નવું AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            AstAssignW* અચલ newp = નવું AstAssignW{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, ખોટું));  // en=0 for floating/pullup vars
            nodep->addStmtsp(નવું AstAlways{newp});
            map_ઉમેરો_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        માટે (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            અચલ std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            જો (it == m_lhsmap.end()) ચાલુ;
            AstVar* અચલ invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            જો (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_ગાંઠ_Module* અચલ ownerModp = findParentModule(invarp);
                અચલ તાર્કિક isCrossModule = ownerModp && ownerModp != nodep;
                અચલ તાર્કિક isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                જો (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    માટે (અચલ RefStrength& rs : *refsp) {
                        જો (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            તોડો;
                        }
                    }
                } નહીંતર જો (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = સાચું;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    સંરચના PartitionInfo final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, PartitionInfo> partitions;
                    માટે (અચલ RefStrength& rs : *refsp) {
                        જો (AstVarXRef* અચલ xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            PartitionInfo& pi = partitions[xrefp->dotted()];
                            pi.refs.ધકેલો_back(rs);
                            જો (pi.inlinedDots.empty()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } નહીંતર {
                            partitions[""].refs.ધકેલો_back(rs);
                        }
                    }
                    માટે (auto& kv : partitions) {
                        ઉમેરો_Tristates_Signal(nodep, invarp, &kv.second.refs, સાચું, kv.first,
                                              kv.second.inlinedDots,
                                              findModportForDotted(nodep, kv.first));
                    }
                } નહીંતર જો (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = સાચું;
                    ઉમેરો_Tristates_Signal(nodep, invarp, refsp, સાચું, "", "", nullptr);
                } નહીંતર {
                    ઉમેરો_Tristates_Signal(nodep, invarp, refsp, ખોટું, "", "", nullptr);
                }
            } નહીંતર {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(delete refsp, refsp);
        }
    }

    શૂન્ય aggregateTriSameStrength(Ast_ગાંઠ_Module* nodep, AstVar* અચલ varp, AstVar* અચલ envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator endStrength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        સંરચના DriverGroup final {
            અચલ Ast_ગાંઠ_Procedure* m_procedurep;  // Process owning these assignments
            std::vector<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::vector<DriverGroup> drivers;
        std::map<અચલ Ast_ગાંઠ_Procedure*, size_t> procedureToDriver;
        માટે (auto it = beginStrength; it != endStrength; ++it) {
            જો (it->m_procedurep) {
                અચલ auto emplaced = procedureToDriver.emplace(it->m_procedurep, drivers.size());
                જો (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.ધકેલો_back(&*it);
                    ચાલુ;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ધકેલો_back(DriverGroup{it->m_procedurep, {&*it}});
        }

        Ast_ગાંઠ_પદાવલિ* orp = nullptr;
        Ast_ગાંઠ_પદાવલિ* enp = nullptr;

        માટે (DriverGroup& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* અચલ newLhsp = નવું AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvtToStr(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->setDfgTriLowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->addStmtsp(newLhsp);
            AstVar* અચલ newEnLhsp
                = નવું AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvtToStr(m_unique++), envarp};  // 2-state ok
            newEnLhsp->setDfgTriLowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->addStmtsp(newEnLhsp);

            માટે (RefStrength* અચલ refStrengthp : driver.m_refsp) {
                Ast_ગાંઠ_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                જો (VN_IS(refp, VarXRef)) {
                    AstVarRef* અચલ localRefp
                        = નવું AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(nullptr);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } નહીંતર {
                    refp->varp(newLhsp);
                }

                જો (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_ગાંઠ_પદાવલિ* enLhsp = refStrengthp->m_enLhsp;
                    Ast_ગાંઠ_Var_Ref* અચલ enVarrefp = refStrengthp->m_enVarrefp;
                    જો (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* અચલ localRefp
                            = નવું AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        જો (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_વૃક્ષ(), enVarrefp);
                        } નહીંતર {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ધકેલો_Deletep(enVarrefp), enVarrefp);
                        }
                    } નહીંતર {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ધકેલો_back(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = nullptr;
                    refStrengthp->m_enVarrefp = nullptr;
                    refStrengthp->m_enRhsp = nullptr;
                } નહીંતર {
                    AstAssignW* અચલ enLhspAssignp = નવું AstAssignW{
                        refp->fileline(),
                        નવું AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    જો (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_વૃક્ષ(ખોટું));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->addStmtsp(નવું AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_લીટી* અચલ fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_ગાંઠ_પદાવલિ* અચલ ref1p = નવું AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_ગાંઠ_પદાવલિ* અચલ ref2p = નવું AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_ગાંઠ_પદાવલિ* અચલ andp = નવું AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : નવું AstOr{fl, orp, andp};

            Ast_ગાંઠ_પદાવલિ* અચલ ref3p = નવું AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : નવું AstOr{ref3p->fileline(), enp, ref3p};
        }
        AstAssignW* અચલ assp = નવું AstAssignW{
            varp->fileline(), નવું AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->addStmtsp(નવું AstAlways{assp});

        AstAssignW* અચલ enAssp = નવું AstAssignW{
            envarp->fileline(), નવું AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->addStmtsp(નવું AstAlways{enAssp});
    }

    શૂન્ય ઉમેરો_Procedural_Enable_Assignments(Ast_ગાંઠ_Module* nodep) {
        માટે (auto& assignEnables : m_proceduralEnables) {
            Ast_ગાંઠ_Assign* અચલ assignp = assignEnables.first;
            Ast_ગાંઠ_પદાવલિ* combinedLhsp = assignp->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* combinedRhsp = assignp->rhsp()->unlinkFrBack();
            માટે (ProceduralEnable& enable : assignEnables.second) {
                combinedLhsp = નવું AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = નવું AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            જો (!VN_IS(assignp, AssignDly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* અચલ sampledp
                    = નવું AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvtToStr(m_unique++), combinedRhsp->dtypep()};
                nodep->addStmtsp(sampledp);
                assignp->lhsp(નવું AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                AstAssign* અચલ applyp
                    = નવું AstAssign{assignp->fileline(), combinedLhsp,
                                    નવું AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->addNextHere(applyp);
            } નહીંતર {
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
    શૂન્ય ઉમેરો_Tristates_Signal(Ast_ગાંઠ_Module* nodep, AstVar* અચલ invarp, RefStrengthVec* refsp,
                               તાર્કિક isIfaceTri, અચલ string& ifaceDottedPath,
                               અચલ string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        અચલ તાર્કિક is_ટોચ_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_ટોચ();
        જો (!isIfaceTri) {
            જો (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty())
                 && is_ટોચ_Inout)
                || (!nodep->is_ટોચ() && invarp->isIO())) {
                // This var becomes an input
                invarp->varType2In();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = getCreateOutVarp(invarp, is_ટોચ_Inout);
                outvarp->varType2Out();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_ટોચ_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                જો (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                જો (!m_varAux(invarp).bitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } નહીંતર જો (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_ગાંઠ_પદાવલિ* orp = nullptr;
        Ast_ગાંઠ_પદાવલિ* enp = nullptr;

        std::sort(refsp->begin(), refsp->end(),
                  [](RefStrength a, RefStrength b) { પાછા a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        જ્યારે (beginStrength != refsp->end()) {
            RefStrengthVec::iterator endStrength = beginStrength + 1;
            જ્યારે (endStrength != refsp->end()
                   && endStrength->m_strength == beginStrength->m_strength)
                endStrength++;

            File_લીટી* અચલ fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strengthPrefix;
            જો (isIfaceTri && !ifaceDottedPath.empty()) {
                strengthPrefix = ifaceDottedPath;
                std::replace(strengthPrefix.begin(), strengthPrefix.end(), '.', '_');
                strengthPrefix += "__";
            }
            અચલ string strengthVarName
                = strengthPrefix + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* અચલ varStrengthp = નવું AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->setDfgTriLowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->addStmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* અચલ enVarStrengthp = નવું AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->setDfgTriLowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->addStmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     endStrength);

            Ast_ગાંઠ_પદાવલિ* પદાવલિ_Current_Strengthp;
            જો (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                પદાવલિ_Current_Strengthp
                    = નવું AstAnd{fl, નવું AstVarRef{fl, varStrengthp, VAccess::READ},
                                 નવું AstNot{fl, enp->clone_વૃક્ષ_Pure(ખોટું)}};
            } નહીંતર {
                પદાવલિ_Current_Strengthp = નવું AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? પદાવલિ_Current_Strengthp : નવું AstOr{fl, orp, પદાવલિ_Current_Strengthp};

            Ast_ગાંઠ_પદાવલિ* અચલ enVarStrengthRefp
                = નવું AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : નવું AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = endStrength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        જો (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->setDfgAllowMultidriveTri();
            Ast_ગાંઠ_Module* અચલ ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            અચલ પૂર્ણાંક contribIdx = static_cast<પૂર્ણાંક>(m_ifaceContribs[invarp].size());
            File_લીટી* અચલ fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* અચલ contribOutp = નવું AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvtToStr(contribIdx), invarp};
            contribOutp->setDfgTriLowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->addStmtsp(contribOutp);

            AstVar* અચલ contribEnp = નવું AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvtToStr(contribIdx), invarp};
            contribEnp->setDfgTriLowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->addStmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            જો (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                addModportVarRefIfMissing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                addModportVarRefIfMissing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_ગાંઠ_Var_Ref* અચલ lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_ગાંઠ_Var_Ref*>(
                                                    નવું AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_ગાંઠ_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* અચલ assp = નવું AstAssignW{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(નવું AstAlways{assp});
            }
            {
                Ast_ગાંઠ_Var_Ref* અચલ lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_ગાંઠ_Var_Ref*>(
                                                    નવું AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_ગાંઠ_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* અચલ assp = નવું AstAssignW{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(નવું AstAlways{assp});
            }

            m_ifaceContribs[invarp].ધકેલો_back({contribEnp, contribOutp});
            પાછા;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        અચલ તાર્કિક hasPull = m_varAux(lhsp).pullp || hasPerBitPulls(lhsp);
        જો (!outvarp || hasPull) {
            Ast_ગાંઠ_પદાવલિ* undrivenp;
            જો (envarp) {
                undrivenp = નવું AstNot{envarp->fileline(),
                                       નવું AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } નહીંતર જો (enp) {
                undrivenp = નવું AstNot{enp->fileline(), enp};
                enp = nullptr;  // moved into undrivenp
            } નહીંતર {
                undrivenp = newAllZerosOrOnes(invarp, સાચું);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            જો (hasPerBitPulls(lhsp)) {
                pullConstp = createPerBitPullConst(lhsp);
            } નહીંતર {
                અચલ AstPull* અચલ pullp = m_varAux(lhsp).pullp;
                અચલ તાર્કિક pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = નવું AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? નવું AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        જો (envarp && !enp) enp = newAllZerosOrOnes(invarp, ખોટું);

        જો (envarp) {
            AstAssignW* અચલ enAssp = નવું AstAssignW{
                enp->fileline(), નવું AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->addStmtsp(નવું AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        AstAssignW* અચલ assp = નવું AstAssignW{
            lhsp->fileline(), નવું AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->addStmtsp(નવું AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        જો ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty()) && is_ટોચ_Inout) {
            જો (envarp) {
                envarp->primaryIO(સાચું);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            જો (outvarp) {
                outvarp->primaryIO(સાચું);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            જો (!v3Global.opt.libCreate().empty()) {
                અચલ પૂર્ણાંક id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(સાચું);
            }
        }
    }

    તાર્કિક isOnlyAssignmentIsToLhsVar(અચલ AstAssignW* અચલ nodep) {
        જો (અચલ AstVarRef* અચલ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            auto foundIt = m_assigns.find(varRefp->varp());
            જો (foundIt != m_assigns.end()) {
                auto અચલ& assignsToVar = foundIt->second;
                જો (assignsToVar.size() == 1 && assignsToVar[0] == nodep) પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }

    શૂન્ય addToAssignmentList(AstAssignW* nodep) {
        જો (AstVarRef* અચલ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            જો (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ધકેલો_back(nodep);
            } નહીંતર જો (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlinkFrBack()->delete_વૃક્ષ();
            }
        } નહીંતર જો (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(અચલ AstAssignW* અચલ nodep, તાર્કિક કિંમત) {
        જો (AstStrengthSpec* અચલ strengthSpec = nodep->strengthSpecp()) {
            પાછા કિંમત ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        પાછા VStrength::STRONG;  // default strength is strong
    }

    તાર્કિક assignment_Of_કિંમત_On_All_Bits(અચલ AstAssignW* અચલ nodep, તાર્કિક કિંમત) {
        જો (અચલ AstConst* અચલ constp = VN_CAST(nodep->rhsp(), Const)) {
            અચલ V3_સંખ્યા અંક = constp->અંક();
            પાછા કિંમત ? અંક.isEqAllOnes() : અંક.isEqZero();
        }
        પાછા ખોટું;
    }

    AstAssignW* get_Strongest_Assignment_Of_કિંમત(અચલ Assigns& assigns, તાર્કિક કિંમત) {
        auto maxIt = std::max_element(
            assigns.begin(), assigns.end(), [&](અચલ AstAssignW* ap, અચલ AstAssignW* bp) {
                અચલ તાર્કિક કિંમતો_On_Rhs_A = assignment_Of_કિંમત_On_All_Bits(ap, કિંમત);
                અચલ તાર્કિક કિંમતો_On_Rhs_B = assignment_Of_કિંમત_On_All_Bits(bp, કિંમત);
                જો (!કિંમતો_On_Rhs_A) પાછા કિંમતો_On_Rhs_B;
                જો (!કિંમતો_On_Rhs_B) પાછા ખોટું;
                પાછા getStrength(ap, કિંમત) < getStrength(bp, કિંમત);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        પાછા assignment_Of_કિંમત_On_All_Bits(*maxIt, કિંમત) ? *maxIt : nullptr;
    }

    તાર્કિક isAssignmentNotStrongerThanStrength(અચલ AstAssignW* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        અચલ uint8_t strength0 = getStrength(assignp, 0);
        અચલ uint8_t strength1 = getStrength(assignp, 1);
        પાછા (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_Of_કિંમત_On_All_Bits(assignp, 0))
               || (strength1 <= strength && assignment_Of_કિંમત_On_All_Bits(assignp, 1));
    }

    Ast_ગાંઠ_પદાવલિ* new_Merge_પદાવલિ(Ast_ગાંઠ_પદાવલિ* અચલ lhsp, Ast_ગાંઠ_પદાવલિ* અચલ rhsp, File_લીટી* અચલ fl,
                              તાર્કિક isWor) {
        Ast_ગાંઠ_પદાવલિ* પદાવલિ = nullptr;
        જો (isWor)
            પદાવલિ = નવું AstOr{fl, lhsp, rhsp};
        નહીંતર
            પદાવલિ = નવું AstAnd{fl, lhsp, rhsp};
        પાછા પદાવલિ;
    }

    શૂન્ય mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        માટે (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            જો (assigns.size() <= 1) ચાલુ;
            AstVar* અચલ varp = varpAssigns.first;
            જો (!varp->isWiredNet()) ચાલુ;
            auto it = assigns.begin();
            AstAssignW* અચલ assignWp0 = *it;
            File_લીટી* અચલ fl = assignWp0->fileline();
            Ast_ગાંઠ_પદાવલિ* wExp = nullptr;
            જ્યારે (++it != assigns.end()) {
                AstAssignW* assignWpi = *it;
                જો (!wExp) {
                    wExp
                        = new_Merge_પદાવલિ(assignWp0->rhsp()->clone_વૃક્ષ_Pure(ખોટું),
                                       assignWpi->rhsp()->clone_વૃક્ષ_Pure(ખોટું), fl, varp->isWor());
                } નહીંતર {
                    wExp = new_Merge_પદાવલિ(wExp, assignWpi->rhsp()->clone_વૃક્ષ_Pure(ખોટું), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((assignWpi->unlinkFrBack()->delete_વૃક્ષ()), assignWpi);
            }
            AstVarRef* અચલ wVarRef = નવું AstVarRef{fl, varp, VAccess::WRITE};
            AstAssignW* અચલ wAssignp = નવું AstAssignW{fl, wVarRef, wExp};
            assignWp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ધકેલો_Deletep(assignWp0), assignWp0);
            assigns.clear();
            assigns.ધકેલો_back(wAssignp);
        }
    }

    શૂન્ય removeNotStrongerAssignments(Assigns& assigns, અચલ AstAssignW* strongestp,
                                      uint8_t greatestKnownStrength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        auto removedIt = std::remove_if(assigns.begin(), assigns.end(), [&](AstAssignW* assignp) {
            જો (assignp == strongestp) પાછા ખોટું;
            જો (isAssignmentNotStrongerThanStrength(assignp, greatestKnownStrength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ધકેલો_Deletep(assignp->unlinkFrBack()), assignp);
                પાછા સાચું;
            }
            પાછા ખોટું;
        });
        assigns.erase(removedIt, assigns.end());
    }

    શૂન્ય removeAssignmentsNotStrongerThanUniformConstant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        માટે (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            જો (assigns.size() <= 1) ચાલુ;
            અચલ AstAssignW* અચલ strongest0p = get_Strongest_Assignment_Of_કિંમત(assigns, 0);
            અચલ AstAssignW* અચલ strongest1p = get_Strongest_Assignment_Of_કિંમત(assigns, 1);
            અચલ AstAssignW* strongestp = nullptr;
            uint8_t greatestKnownStrength = 0;
            અચલ auto getIfStrongest
                = [&](અચલ AstAssignW* અચલ strongestCandidatep, તાર્કિક કિંમત) {
                      જો (!strongestCandidatep) પાછા;
                      અચલ uint8_t strength = getStrength(strongestCandidatep, કિંમત);
                      જો (strength >= greatestKnownStrength) {
                          greatestKnownStrength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            જો (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    શૂન્ય removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        માટે (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            જો (assigns.size() <= 1) ચાલુ;
            auto maxIt = std::max_element(
                assigns.begin(), assigns.end(), [&](અચલ AstAssignW* ap, અચલ AstAssignW* bp) {
                    જો (m_tgraph.isTristate(ap)) પાછા !m_tgraph.isTristate(bp);
                    જો (m_tgraph.isTristate(bp)) પાછા ખોટું;
                    અચલ uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    અચલ uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    પાછા minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            અચલ AstAssignW* અચલ strongestp = m_tgraph.isTristate(*maxIt) ? nullptr : *maxIt;
            જો (strongestp) {
                અચલ uint8_t greatestKnownStrength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    // VISITORS
    શૂન્ય visit(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        જો (m_graphing) {
            જો (!m_alhs && nodep->અંક().hasZ()) m_tgraph.setTristate(nodep);
        } નહીંતર {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            જો (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* અચલ varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_ગાંઠ* અચલ newp = નવું AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            } નહીંતર જો (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_લીટી* અચલ fl = nodep->fileline();
                AstConst* અચલ enp = getNonZConstp(nodep);
                V3_સંખ્યા num1{nodep, nodep->width()};
                num1.opAnd(nodep->અંક(), enp->અંક());  // 01X->01X, Z->0
                AstConst* અચલ newconstp = નવું AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    શૂન્ય visit(AstCond* nodep) override {
        જો (m_graphing) {
            iterateChildren(nodep);
            જો (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } નહીંતર {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } નહીંતર {
            જો (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                પાછા;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_ગાંઠ_પદાવલિ* અચલ condp = nodep->condp();
            જો (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_ગાંઠ_પદાવલિ* અચલ thenp = nodep->thenp();
            Ast_ગાંઠ_પદાવલિ* અચલ elsep = nodep->elsep();
            જો (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_ગાંઠ_પદાવલિ* અચલ en1p = getEnp(thenp);
                Ast_ગાંઠ_પદાવલિ* અચલ en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_ગાંઠ_પદાવલિ* અચલ enp
                    = નવું AstCond{nodep->fileline(), condp->clone_વૃક્ષ(ખોટું), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                જો (thenp->user1p() && !thenp->user1p()->backp()) ધકેલો_Deletep(thenp->user1p());
                જો (elsep->user1p() && !elsep->user1p()->backp()) ધકેલો_Deletep(elsep->user1p());
                thenp->user1p(nullptr);
                elsep->user1p(nullptr);
            }
        }
    }

    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        iterateChildren(nodep);
        જો (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } નહીંતર જો (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(nullptr);
            m_tgraph.didProcess(nodep);
        }
    }

    શૂન્ય visit(AstSel* nodep) override {
        જો (m_graphing) {
            iterateChildren(nodep);
            જો (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } નહીંતર {
                associateLogic(nodep->fromp(), nodep);
            }
        } નહીંતર {
            જો (m_alhs) {
                UINFO(9, dbgState() << nodep);
                જો (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_ગાંઠ_પદાવલિ* અચલ newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), ગાંઠ_પદાવલિ));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } નહીંતર {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                જો (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                જો (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_ગાંઠ_પદાવલિ* અચલ en1p = getEnp(nodep->fromp());
                    Ast_ગાંઠ_પદાવલિ* અચલ enp
                        = નવું AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_વૃક્ષ_Pure(સાચું),
                                     nodep->widthConst()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    શૂન્ય visit(AstConcat* nodep) override {
        જો (m_graphing) {
            iterateChildren(nodep);
            જો (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } નહીંતર {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } નહીંતર {
            જો (m_alhs) {
                UINFO(9, dbgState() << nodep);
                જો (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_ગાંઠ_પદાવલિ* અચલ enp = VN_AS(nodep->user1p(), ગાંઠ_પદાવલિ);
                    nodep->user1p(nullptr);
                    nodep->lhsp()->user1p(નવું AstSel{nodep->fileline(), enp->clone_વૃક્ષ_Pure(સાચું),
                                                     nodep->rhsp()->width(),
                                                     nodep->lhsp()->width()});
                    nodep->rhsp()->user1p(
                        નવું AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->width()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } નહીંતર {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_ગાંઠ_પદાવલિ* અચલ expr1p = nodep->lhsp();
                Ast_ગાંઠ_પદાવલિ* અચલ expr2p = nodep->rhsp();
                જો (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_ગાંઠ_પદાવલિ* અચલ en1p = getEnp(expr1p);
                    Ast_ગાંઠ_પદાવલિ* અચલ en2p = getEnp(expr2p);
                    Ast_ગાંઠ_પદાવલિ* અચલ enp = નવું AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(nullptr);
                    expr2p->user1p(nullptr);
                }
            }
        }
    }

    શૂન્ય visit(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        જો (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.setTristate(nodep);
        } નહીંતર {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            જો (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                પાછા;
            }
            m_tgraph.didProcess(nodep);
            Ast_ગાંઠ_પદાવલિ* અચલ expr1p = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ expr2p = nodep->rhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* enp;
            જો (Ast_ગાંઠ_પદાવલિ* અચલ en2p = VN_AS(expr2p->user1p(), ગાંઠ_પદાવલિ)) {
                enp = નવું AstAnd{nodep->fileline(), expr1p, en2p};
            } નહીંતર {
                enp = expr1p;
            }
            expr1p->user1p(nullptr);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }

    શૂન્ય visitAndOr(Ast_ગાંઠ_Biop* nodep, તાર્કિક isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        જો (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } નહીંતર {
            જો (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                પાછા;
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
            Ast_ગાંઠ_પદાવલિ* અચલ expr1p = nodep->lhsp();
            Ast_ગાંઠ_પદાવલિ* અચલ expr2p = nodep->rhsp();
            જો (!expr1p->user1p() && !expr2p->user1p()) {
                પાછા;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_ગાંઠ_પદાવલિ* en1p = getEnp(expr1p);
            Ast_ગાંઠ_પદાવલિ* en2p = getEnp(expr2p);
            Ast_ગાંઠ_પદાવલિ* subexpr1p = expr1p->clone_વૃક્ષ_Pure(ખોટું);
            Ast_ગાંઠ_પદાવલિ* subexpr2p = expr2p->clone_વૃક્ષ_Pure(ખોટું);
            જો (isAnd) {
                subexpr1p = નવું AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = નવું AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            જો (!en1p->op1p() && en2p->op1p()) {
                std::swap(en1p, en2p);
                std::swap(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_ગાંઠ_પદાવલિ* અચલ enp = નવું AstOr{
                nodep->fileline(),
                નવું AstAnd{nodep->fileline(), en1p, નવું AstOr{nodep->fileline(), en2p, subexpr1p}},
                નવું AstAnd{nodep->fileline(), en2p->clone_વૃક્ષ(ખોટું), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(nullptr);
            expr2p->user1p(nullptr);
        }
    }
    શૂન્ય visit(AstAnd* nodep) override { visitAndOr(nodep, સાચું); }
    શૂન્ય visit(AstOr* nodep) override { visitAndOr(nodep, ખોટું); }

    શૂન્ય visitAssign(Ast_ગાંઠ_Assign* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        જો (VN_IS(nodep->rhsp(), CReset)) પાછા;
        જો (AstAssignW* અચલ assignWp = VN_CAST(nodep, AssignW)) {
            m_currentDelayp = VN_CAST(assignWp->timingControlp(), Delay);
        }
        જો (m_graphing) {
            જો (AstAssignW* assignWp = VN_CAST(nodep, AssignW)) {
                જો (assignWp->getLhsNetDelay()) પાછા;
                જો (assignWp->timingControlp() && !m_currentDelayp) પાછા;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                જો (m_currentDelayp && m_currentDelayp->fallDelay()) પાછા;
                જો (!assignWp->timingControlp()) addToAssignmentList(assignWp);
            }

            જો (nodep->user2() & U2_GRAPHING) પાછા;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterateAndNextNull(nodep->rhsp());
            m_alhs = સાચું;
            iterateAndNextNull(nodep->lhsp());
            m_alhs = ખોટું;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } નહીંતર {
            જો (nodep->user2() & U2_NONGRAPH) {
                પાછા;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterateAndNextNull(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            જો (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, AssignW), nodep,
                            "Continuous assignment inside procedural process");
                Ast_ગાંઠ_પદાવલિ* અચલ enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), ગાંઠ_પદાવલિ)->clone_વૃક્ષ_Pure(ખોટું)
                          : newAllZerosOrOnes(nodep->lhsp(), સાચું);
                અચલ તાર્કિક inserted = m_assignEnps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            જો (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(nullptr);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } નહીંતર {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                જો (AstSel* અચલ selp = VN_CAST(nodep->lhsp(), Sel)) {
                    જો (Ast_ગાંઠ_Var_Ref* અચલ varrefp = VN_CAST(selp->fromp(), ગાંઠ_Var_Ref)) {
                        જો (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), સાચું));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } નહીંતર {
                            m_tgraph.didProcess(nodep, સાચું);
                        }
                    } નહીંતર {
                        m_tgraph.didProcess(nodep, સાચું);  // LCOV_EXCL_LINE
                    }
                } નહીંતર {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, સાચું);
                }
            }
            m_alhs = સાચું;  // And user1p() will indicate tristate equation, if any
            જો (AstAssignW* અચલ assignWp = VN_CAST(nodep, AssignW)) {
                જો (AstStrengthSpec* અચલ specp = assignWp->strengthSpecp()) {
                    જો (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        જો (!isOnlyAssignmentIsToLhsVar(assignWp)) {
                            assignWp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } નહીંતર {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterateAndNextNull(nodep->lhsp());
        }
    }
    શૂન્ય visit(AstAssignW* nodep) override { visitAssign(nodep); }
    શૂન્ય visit(AstAssign* nodep) override { visitAssign(nodep); }
    શૂન્ય visit(AstAssignDly* nodep) override { visitAssign(nodep); }

    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        અચલ AstAlways* અચલ alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN ? nullptr : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    શૂન્ય visit(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_inAlias);
        m_inAlias = સાચું;
        જો (m_graphing) {
            જો (nodep->user2() & U2_GRAPHING) પાછા;
            m_alhs = સાચું;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            માટે (Ast_ગાંઠ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } નહીંતર {
            iterateChildren(nodep);
        }
    }

    શૂન્ય visitCaseEq(Ast_ગાંઠ_Biop* nodep, તાર્કિક neq) {
        જો (m_graphing) {
            iterateChildren(nodep);
        } નહીંતર {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* અચલ constp = VN_CAST(nodep->lhsp(), Const);
            જો (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                File_લીટી* અચલ fl = nodep->fileline();
                Ast_ગાંઠ_પદાવલિ* enRhsp;
                જો (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), ગાંઠ_પદાવલિ);
                    rhsp->user1p(nullptr);
                } નહીંતર {
                    enRhsp = get_En_પદાવલિ_Based_On_Originalp(rhsp);
                }
                Ast_ગાંઠ_પદાવલિ* newp
                    = નવું AstLogAnd{fl, નવું AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    નવું AstEqCase{fl, નવું AstConst{fl, constp->અંક()}, rhsp}};
                constp->user1p(nullptr);
                જો (neq) newp = નવું AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            } નહીંતર જો (constp && nodep->rhsp()->user1p()) {
                File_લીટી* અચલ fl = nodep->fileline();
                constp->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* newp = નવું AstLogAnd{fl,
                                                  નવું AstEq{fl, newAllZerosOrOnes(constp, ખોટું),
                                                            VN_AS(rhsp->user1p(), ગાંઠ_પદાવલિ)},
                                                  // Keep the caseeq if there are X's present
                                                  નવું AstEqCase{fl, constp, rhsp}};
                જો (neq) newp = નવું AstLogNot{fl, newp};
                rhsp->user1p(nullptr);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            } નહીંતર {
                checkUnhandled(nodep);
            }
        }
    }
    શૂન્ય visitEqNeqWild(Ast_ગાંઠ_Biop* nodep) {
        જો (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterateAndNextNull(nodep->lhsp());
        જો (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            પાછા;
        }
    }
    શૂન્ય visit(AstEqCase* nodep) override { visitCaseEq(nodep, ખોટું); }
    શૂન્ય visit(AstNeqCase* nodep) override { visitCaseEq(nodep, સાચું); }
    શૂન્ય visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    શૂન્ય visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }

    શૂન્ય visit(Ast_ગણતરી_Bits* nodep) override {
        std::array<તાર્કિક, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->અંક().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->અંક().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->અંક().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        જો (m_graphing) {
            iterateAndNextNull(nodep->lhsp());
            જો (!dropop[0]) iterateAndNextNull(nodep->rhsp());
            જો (!dropop[1]) iterateAndNextNull(nodep->thsp());
            જો (!dropop[2]) iterateAndNextNull(nodep->fhsp());
        } નહીંતર {
            Ast_ગાંઠ_પદાવલિ* nonXp = nullptr;
            જો (!dropop[0]) {
                nonXp = nodep->rhsp();
            } નહીંતર જો (!dropop[1]) {
                nonXp = nodep->thsp();
            } નહીંતર જો (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            જો (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                અચલ AstVarRef* અચલ varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                જો (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    પાછા;
                }
                AstVar* અચલ envarp = getCreateEnVarp(varrefp->varp(), ખોટું);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                Ast_ગાંઠ_પદાવલિ* newp = નવું Ast_ગણતરી_Ones{
                    nodep->fileline(), નવું AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                જો (nonXp) {  // Need to still count '0 or '1 or 'x's
                    જો (dropop[0]) {
                        nodep->rhsp()->unlinkFrBack()->delete_વૃક્ષ();
                        nodep->rhsp(nonXp->clone_વૃક્ષ_Pure(સાચું));
                    }
                    જો (dropop[1]) {
                        nodep->thsp()->unlinkFrBack()->delete_વૃક્ષ();
                        nodep->thsp(nonXp->clone_વૃક્ષ_Pure(સાચું));
                    }
                    જો (dropop[2]) {
                        nodep->fhsp()->unlinkFrBack()->delete_વૃક્ષ();
                        nodep->fhsp(nonXp->clone_વૃક્ષ_Pure(સાચું));
                    }
                    newp = નવું AstAdd{nodep->fileline(), nodep, newp};
                } નહીંતર {
                    ધકેલો_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = nullptr;
        જો (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } નહીંતર જો (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        જો (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } નહીંતર {
            જો (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.setTristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } નહીંતર {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                અચલ AstSel* અચલ selp = VN_CAST(nodep->lhsp(), Sel);
                અચલ AstConst* અચલ lsbp = selp ? VN_CAST(selp->lsbp(), Const) : nullptr;
                જો (lsbp) {
                    અચલ પૂર્ણાંક lsb = lsbp->toSInt();
                    માટે (પૂર્ણાંક i = 0; i < selp->widthConst(); ++i) {
                        setBitPullDirection(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } નહીંતર {
                    setPullDirection(varrefp->varp(), nodep);
                }
            }
        }
        જો (!m_graphing) {
            nodep->unlinkFrBack();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }

    શૂન્ય iteratePinGuts(AstPin* nodep) {
        જો (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            જો (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } નહીંતર {
            // All heavy lifting completed in graph visitor.
            જો (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    શૂન્ય visit(AstPin* nodep) override {
        જો (m_graphing) {
            જો (nodep->user2() & U2_GRAPHING) પાછા;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* અચલ enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            જો (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, ખોટું);
                iteratePinGuts(nodep);
                પાછા;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            અચલ તાર્કિક inDeclProcessing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::આગત
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            જો (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* અચલ ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(નવું AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.setTristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } નહીંતર જો (inDeclProcessing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                અચલ AstAssignW* અચલ reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, સાચું, સાચું);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.setTristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* અચલ enVarp = નવું AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvtToStr(m_unique++),
                                              VFlagBitPacked{}, enModVarp->width()};
            જો (inDeclProcessing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->varType2In();
            } નહીંતર {
                enModVarp->varType2Out();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* અચલ enpinp
                = નવું AstPin{nodep->fileline(), nodep->pin_અંક(),
                             enModVarp->name(),  // should be {var}"__en"
                             નવું AstVarRef{nodep->fileline(), enVarp,
                                           inDeclProcessing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->addNextHere(enpinp);
            m_modp->addStmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            અચલ AstAssignW* outAssignp = nullptr;  // If reconnected, the related assignment
            AstPin* outpinp = nullptr;
            AstVar* અચલ outModVarp = m_varAux(nodep->modVarp()).outVarp;
            જો (!outModVarp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                જો (!m_modp->is_ટોચ()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } નહીંતર {
                Ast_ગાંઠ_પદાવલિ* અચલ outexprp = VN_AS(nodep->exprp(), ગાંઠ_પદાવલિ)
                                                  ->clone_વૃક્ષ_Pure(ખોટું);  // Note has lvalue() set
                outpinp = નવું AstPin{nodep->fileline(), nodep->pin_અંક(),
                                     outModVarp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(outModVarp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->addNextHere(outpinp);
                // Simplify
                જો (inDeclProcessing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, સાચું};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                outAssignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        સાચું);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, outAssignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            અચલ TristatePinVisitor visitor{nodep->exprp(), m_tgraph, ખોટું};
            અચલ Ast_ગાંઠ* અચલ inAssignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, સાચું);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, inAssignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            જો (!outAssignp) {
                જો (!outpinp) {
                    exprrefp = nullptr;  // Primary input only
                } નહીંતર {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    જો (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } નહીંતર {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(outAssignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                જો (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            જો (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(નવું AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                જો (!outAssignp) {
                    map_ઉમેરો_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            જો (exprrefp) {
                AstPull* અચલ pullp = m_varAux(nodep->modVarp()).pullp;
                અચલ auto& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = nullptr;
                પૂર્ણાંક selLsb = 0;
                પૂર્ણાંક selWidth = 0;
                જો (outAssignp) {
                    જો (AstSel* અચલ selp = VN_CAST(outAssignp->lhsp(), Sel)) {
                        AstVarRef* અચલ vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* અચલ lsbp = VN_CAST(selp->lsbp(), Const);
                        જો (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->toSInt();
                            selWidth = selp->widthConst();
                        }
                    }
                }

                જો (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    setPullDirection(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    જો (selVarp) {
                        અચલ પૂર્ણાંક direction = pullp->direction();
                        માટે (પૂર્ણાંક i = 0; i < selWidth; ++i) {
                            setBitPullDirection(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                જો (!srcBitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    માટે (અચલ auto& pair : srcBitPulls) {
                        setBitPullDirection(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    જો (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        માટે (અચલ auto& pair : srcBitPulls) {
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
        નહીંતર {
            જો (nodep->user2() & U2_NONGRAPH) પાછા;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    શૂન્ય handle_ગાંઠ_Var_Ref(Ast_ગાંઠ_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        જો (m_graphing) {
            જો (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            જો (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            જો (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.setTristate(nodep->varp());
            }
        } નહીંતર {
            જો (nodep->user2() & U2_NONGRAPH) પાછા;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            જો (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                જો (m_inAlias) {
                    જો (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } નહીંતર {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    પાછા;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_ઉમેરો_Lhs_Var_Ref(nodep);
            } નહીંતર જો (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_લીટી* અચલ fl = nodep->fileline();
                AstVar* અચલ enVarp = getCreateEnVarp(nodep->varp(), ખોટું);
                જો (AstVarXRef* અચલ xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } નહીંતર {
                    nodep->user1p(નવું AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    શૂન્ય visit(AstVarRef* nodep) override { handle_ગાંઠ_Var_Ref(nodep); }
    શૂન્ય visit(AstVarXRef* nodep) override { handle_ગાંઠ_Var_Ref(nodep); }

    શૂન્ય visit(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        જો (m_graphing) {
            // If tri0/1 force a pullup
            જો (nodep->user2() & U2_GRAPHING) પાછા;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            જો (nodep->isPulldown() || nodep->isPullup()) {
                Ast_ગાંઠ* અચલ newp = નવું AstPull{
                    nodep->fileline(), નવું AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->addNextHere(newp);
                // We'll iterate on the new AstPull later
            }
            જો (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.setTristate(nodep);
            }
        } નહીંતર {  // !graphing
            જો (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        જો (m_processedIfaces && VN_IS(nodep, Iface)) પાછા;
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
        m_graphing = ખોટું;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = nullptr;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = સાચું;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = ખોટું;
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
        ઉમેરો_Tristates(nodep);
        ઉમેરો_Procedural_Enable_Assignments(nodep);

        માટે (auto& assignEnp : m_assignEnps) {
            VL_DO_DANGLING(assignEnp.second->delete_વૃક્ષ(), assignEnp.second);
        }
        m_assignEnps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    શૂન્ય visit(AstClass* nodep) override {
        // don't deal with classes
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        // don't deal with functions
    }

    શૂન્ય visit(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterateAndNextNull(nodep->stmtsp());
    }

    શૂન્ય visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = ખોટું;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    શૂન્ય combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        માટે (અચલ std::pair<AstVar* અચલ, std::vector<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* અચલ invarp = kv.first;
            અચલ std::vector<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_ગાંઠ_Module* અચલ ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_લીટી* અચલ fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.size() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_ગાંઠ_પદાવલિ* orp = nullptr;
            Ast_ગાંઠ_પદાવલિ* enp = nullptr;

            માટે (અચલ std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* અચલ contribEnp = contrib.first;
                AstVar* અચલ contribOutp = contrib.second;

                Ast_ગાંઠ_પદાવલિ* અચલ outRefp = નવું AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_ગાંઠ_પદાવલિ* અચલ enRefp = નવું AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_ગાંઠ_પદાવલિ* અચલ andp = નવું AstAnd{fl, outRefp, enRefp};

                orp = (!orp) ? andp : નવું AstOr{fl, orp, andp};

                Ast_ગાંઠ_પદાવલિ* અચલ enRef2p = નવું AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : નવું AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            જો (!envarp) {
                envarp = નવું AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->setDfgTriLowered();
                ifaceModp->addStmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            AstAssignW* અચલ enAssp
                = નવું AstAssignW{fl, નવું AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->addStmtsp(નવું AstAlways{enAssp});

            // Pull resolution
            અચલ AstPull* અચલ pullp = m_varAux(invarp).pullp;
            અચલ તાર્કિક pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_ગાંઠ_પદાવલિ* undrivenp = નવું AstNot{fl, નવું AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = નવું AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = નવું AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            AstAssignW* અચલ assp
                = નવું AstAssignW{fl, નવું AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->addStmtsp(નવું AstAlways{assp});
        }
    }

જાહેર:
    // CONSTRUCTORS
    explicit TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::vector<Ast_ગાંઠ_Module*> ifacesp;
        માટે (Ast_ગાંઠ* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            જો (VN_IS(modp, Iface)) ifacesp.ધકેલો_back(VN_AS(modp, ગાંઠ_Module));
        }
        માટે (auto it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = સાચું;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ifdef VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::vector<Ast_ગાંઠ*> unusedRootps;
        netlistp->foreach([&](Ast_ગાંઠ* nodep) {
            Ast_ગાંઠ* અચલ enp = nodep->user1p();
            જો (!enp) પાછા;
            જો (enp->backp()) પાછા;
            unusedRootps.emplace_back(enp);
        });
        માટે (Ast_ગાંઠ* અચલ nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
#endif
    }
    ~TristateVisitor() override {
        V3Stats::addStat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

શૂન્ય V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("tristate", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
