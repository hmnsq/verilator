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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Tristate.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Inst.h"
#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

বর্গ TristateBaseVisitor VL_NOT_FINAL : সার্বজনিক VNVisitor {
সার্বজনিক:
    // METHODS
};

//######################################################################
// Graph support classes

বর্গ TristateVertex final : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_গ্রন্থি* ধ্রুব m_nodep;
    তার্কিক m_isTristate = অসত্য;  // Logic indicates a tristate
    তার্কিক m_feedsTri = অসত্য;  // Propagates to a tristate node (on RHS)
    তার্কিক m_processed = অসত্য;  // Tristating was cleaned up
সার্বজনিক:
    TristateVertex(V3Graph* graphp, Ast_গ্রন্থি* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = স্বভাব;
    // ACCESSORS
    Ast_গ্রন্থি* nodep() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_nodep; }
    ধ্রুব AstVar* varp() ধ্রুব { প্রত্যাগম VN_CAST(nodep(), Var); }
    string name() ধ্রুব override {
        প্রত্যাগম ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvtToHex(nodep())));
    }
    string dotColor() ধ্রুব override {
        প্রত্যাগম (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_সারি* fileline() ধ্রুব override { প্রত্যাগম nodep()->fileline(); }
    তার্কিক isTristate() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_isTristate; }
    শূন্য isTristate(তার্কিক flag) { m_isTristate = flag; }
    তার্কিক feedsTri() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_feedsTri; }
    শূন্য feedsTri(তার্কিক flag) { m_feedsTri = flag; }
    তার্কিক processed() ধ্রুব { প্রত্যাগম m_processed; }
    শূন্য processed(তার্কিক flag) { m_processed = flag; }
};

//######################################################################

বর্গ TristateGraph final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
সার্বজনিক:
    using VarVec = std::vector<AstVar*>;

গুপ্ত:
    // MEMBERS
    V3Graph m_graph;  // Logic graph

সার্বজনিক:
    // CONSTRUCTORS
    TristateGraph() { clearAndCheck(); }
    আভাস ~TristateGraph() { clearAndCheck(); }
    VL_UNCOPYABLE(TristateGraph);

গুপ্ত:
    // METHODS

    TristateVertex* makeVertex(Ast_গ্রন্থি* nodep) {
        TristateVertex* vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        যদি (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = নূতন TristateVertex{&m_graph, nodep};
            nodep->user4p(vertexp);
        }
        প্রত্যাগম vertexp;
    }

    // METHODS - Graph optimization
    শূন্য graphWalkRecurseFwd(TristateVertex* vtxp, পূর্ণাঙ্ক level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        যদি (!vtxp->isTristate()) প্রত্যাগম;  // tristate involved
        যদি (vtxp->user() == 1) প্রত্যাগম;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        যদি (!vtxp->varp()) {  // not a var where we stop the recursion
            প্রতি (V3GraphEdge& edge : vtxp->outEdges()) {
                TristateVertex* ধ্রুব vvertexp = static_cast<TristateVertex*>(edge.শীর্ষ());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                যদি (!vvertexp->isTristate()) {
                    vvertexp->isTristate(সত্য);
                    graphWalkRecurseFwd(vvertexp, level + 1);
                }
            }
        } অন্যথা {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            প্রতি (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ধ্রুব vvertexp = static_cast<TristateVertex*>(edge.fromp());
                যদি (ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    যদি (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(সত্য);
                        graphWalkRecurseFwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    শূন্য graphWalkRecurseBack(TristateVertex* vtxp, পূর্ণাঙ্ক level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        যদি (!(vtxp->isTristate() || vtxp->feedsTri())) প্রত্যাগম;  // tristate involved
        যদি (vtxp->user() == 3) প্রত্যাগম;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        যদি (!vtxp->varp()) {  // not a var where we stop the recursion
            প্রতি (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ধ্রুব vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                যদি (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(সত্য);
                    graphWalkRecurseBack(vvertexp, level + 1);
                }
            }
        }
    }

সার্বজনিক:
    // METHODS
    তার্কিক empty() ধ্রুব { প্রত্যাগম m_graph.empty(); }
    শূন্য clearAndCheck() {
        প্রতি (V3GraphVertex& vtx : m_graph.vertices()) {
            ধ্রুব TristateVertex& vvertex = static_cast<TristateVertex&>(vtx);
            যদি (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_graph.clear();
        Ast_গ্রন্থি::user4_Clear_বৃক্ষ();  // Wipe all node user4p's that point to vertexes
    }
    শূন্য graphWalk(Ast_গ্রন্থি_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        প্রতি (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        প্রতি (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseBack(static_cast<TristateVertex*>(&vtx), 0);
        }
        যদি (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    শূন্য associate(Ast_গ্রন্থি* fromp, Ast_গ্রন্থি* শীর্ষ) {
        নূতন V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(শীর্ষ), 1};
    }
    শূন্য deleteVerticesFromSubtreeRecurse(Ast_গ্রন্থি* nodep) {
        যদি (!nodep) প্রত্যাগম;
        // Skip vars, because they may be connected to more than one varref
        যদি (!VN_IS(nodep, Var)) {
            TristateVertex* ধ্রুব vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            যদি (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    শূন্য setTristate(Ast_গ্রন্থি* nodep) { makeVertex(nodep)->isTristate(সত্য); }
    তার্কিক isTristate(ধ্রুব Ast_গ্রন্থি* nodep) {
        ধ্রুব TristateVertex* ধ্রুব vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        প্রত্যাগম vertexp && vertexp->isTristate();
    }
    তার্কিক feedsTri(ধ্রুব Ast_গ্রন্থি* nodep) {
        ধ্রুব TristateVertex* ধ্রুব vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        প্রত্যাগম vertexp && vertexp->feedsTri();
    }
    শূন্য didProcess(Ast_গ্রন্থি* nodep, তার্কিক quiet = অসত্য) {
        যদি (TristateVertex* ধ্রুব vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(সত্য);
        } অন্যথা যদি (!quiet) {
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
        প্রতি (ধ্রুব V3GraphVertex& vtx : m_graph.vertices()) {
            ধ্রুব TristateVertex& vvertex = static_cast<ধ্রুব TristateVertex&>(vtx);
            যদি (vvertex.isTristate()) {
                যদি (AstVar* ধ্রুব nodep = VN_CAST(vvertex.nodep(), Var)) v.ঠেলো_back(nodep);
            }
        }
        প্রত্যাগম v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

বর্গ TristatePinVisitor final : সার্বজনিক TristateBaseVisitor {
    TristateGraph& m_tgraph;
    ধ্রুব তার্কিক m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        যদি (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } অন্যথা যদি (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.setTristate(nodep->varp());
        }
    }
    শূন্য visit(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    শূন্য visit(AstSliceSel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_গ্রন্থি* nodep, TristateGraph& tgraph, তার্কিক lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = স্বভাব;
};

//######################################################################

বর্গ TristateVisitor final : সার্বজনিক TristateBaseVisitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;
    ধ্রুব VNUser3InUse m_inuser3;
    ধ্রুব VNUser4InUse m_inuser4;

    সংরচনা AuxAstVar final {
        AstPull* pullp = nullptr;  // pullup/pulldown direction (whole variable)
        AstVar* outVarp = nullptr;  // output __out var
        তার্কিক ifaceTristate = অসত্য;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<পূর্ণাঙ্ক, পূর্ণাঙ্ক>
            bitPulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    সংরচনা RefStrength final {
        Ast_গ্রন্থি_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_গ্রন্থি_Assign* m_assignp;  // Assignment containing m_varrefp
        ধ্রুব Ast_গ্রন্থি_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_গ্রন্থি_রাশিমালা* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_গ্রন্থি_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_গ্রন্থি_রাশিমালা* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_গ্রন্থি_Var_Ref* varrefp, Ast_গ্রন্থি_Assign* assignp,
                    ধ্রুব Ast_গ্রন্থি_Procedure* procedurep, Ast_গ্রন্থি_রাশিমালা* enLhsp,
                    Ast_গ্রন্থি_Var_Ref* enVarrefp, Ast_গ্রন্থি_রাশিমালা* enRhsp, VStrength strength,
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
    সংরচনা LhsProjection final {
        Ast_গ্রন্থি_রাশিমালা* m_lhsp;  // LHS giving the enable assign
        Ast_গ্রন্থি_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_গ্রন্থি_রাশিমালা* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    সংরচনা ProceduralEnable final {
        Ast_গ্রন্থি_রাশিমালা* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_গ্রন্থি_রাশিমালা* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    using RefStrengthVec = std::vector<RefStrength>;
    using VarMap = std::map<AstVar*, RefStrengthVec*>;
    using Assigns = std::vector<AstAssignW*>;
    using VarToAssignsMap = std::map<AstVar*, Assigns>;
    using AssignEnableMap = std::map<Ast_গ্রন্থি_Assign*, Ast_গ্রন্থি_রাশিমালা*>;
    using ProceduralEnableMap = std::map<Ast_গ্রন্থি_Assign*, std::vector<ProceduralEnable>>;
    গণনা : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    তার্কিক m_graphing = অসত্য;  // Major mode - creating graph
    //
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    AstCell* m_cellp = nullptr;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    VarToAssignsMap m_assigns;  // Assigns in current module
    AssignEnableMap m_assignEnps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    পূর্ণাঙ্ক m_unique = 0;
    তার্কিক m_alhs = অসত্য;  // On LHS of assignment
    তার্কিক m_inAlias = অসত্য;  // Inside alias statement
    তার্কিক m_processedIfaces = অসত্য;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = nullptr;  // Delay of assignment currently collecting LHS drivers
    ধ্রুব Ast_গ্রন্থি* m_logicp = nullptr;  // Current logic being built
    Ast_গ্রন্থি_Assign* m_assignp = nullptr;  // Current assignment
    ধ্রুব Ast_গ্রন্থি_Procedure* m_procedurep = nullptr;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::vector<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ধ্রুব {
        string o = (m_graphing ? " gr " : " ng ");
        যদি (m_alhs) o += "alhs ";
        প্রত্যাগম o;
    }
    শূন্য modAddStmtp(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* newp) {
        যদি (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } অন্যথা {
            m_modp->addStmtsp(newp);
        }
    }
    শূন্য associateLogic(Ast_গ্রন্থি* fromp, Ast_গ্রন্থি* শীর্ষ) {
        যদি (m_logicp) m_tgraph.associate(fromp, শীর্ষ);
    }
    AstConst* newAllZerosOrOnes(Ast_গ্রন্থি* nodep, তার্কিক ones) {
        V3_সংখ্যা অঙ্ক{nodep, nodep->width()};
        যদি (ones) অঙ্ক.setAllBits1();
        AstConst* ধ্রুব newp = নূতন AstConst{nodep->fileline(), অঙ্ক};
        প্রত্যাগম newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_সারি* fl, AstVar* varp, ধ্রুব string& dotted, VAccess access,
                           ধ্রুব string& inlinedDots) {
        AstVarXRef* ধ্রুব xrefp = নূতন AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        প্রত্যাগম xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_গ্রন্থি_Module* modp, ধ্রুব string& dottedPath) {
        যদি (dottedPath.empty()) প্রত্যাগম nullptr;
        ধ্রুব string firstComp = dottedPath.substr(0, dottedPath.find('.'));
        প্রতি (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ধ্রুব ivarp = VN_CAST(stmtp, Var);
            যদি (!ivarp || ivarp->name() != firstComp) অনুবর্ত;
            ধ্রুব AstIfaceRefDType* ধ্রুব ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            যদি (ifaceDtp && ifaceDtp->modportp()) প্রত্যাগম ifaceDtp->modportp();
            ভঙ্গ;
        }
        প্রত্যাগম nullptr;
    }
    Ast_গ্রন্থি_রাশিমালা* getEnp(Ast_গ্রন্থি* nodep) {
        যদি (nodep->user1p()) {
            যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুব refp = VN_CAST(nodep, গ্রন্থি_Var_Ref)) {
                যদি (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    প্রত্যাগম newAllZerosOrOnes(nodep, সত্য);
                }
            }
        } অন্যথা {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, সত্য));
        }
        // Otherwise return the previous output enable
        প্রত্যাগম VN_AS(nodep->user1p(), গ্রন্থি_রাশিমালা);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    শূন্য addModportVarRefIfMissing(AstModport* modportp, AstVar* varp, VDirection dir) {
        প্রতি (Ast_গ্রন্থি* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ধ্রুব mvrp = VN_CAST(mrp, ModportVarRef);
            যদি (mvrp && mvrp->varp() == varp) প্রত্যাগম;  // Already present
        }
        AstModportVarRef* ধ্রুব mvrp = নূতন AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->addVarsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, তার্কিক is_শীর্ষ) {
        // Return the master __en for the specified input variable
        যদি (!invarp->user1p()) {
            AstVar* ধ্রুব newp
                = নূতন AstVar{invarp->fileline(), is_শীর্ষ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_গ্রন্থি_Module* ধ্রুব ownerModp = findParentModule(invarp);
            যদি (ownerModp && ownerModp != m_modp) {
                ownerModp->addStmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                প্রতি (Ast_গ্রন্থি* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ধ্রুব modportp = VN_CAST(stmtp, Modport);
                    যদি (!modportp) অনুবর্ত;
                    প্রতি (Ast_গ্রন্থি* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ধ্রুব mvrp = VN_CAST(mrp, ModportVarRef);
                        যদি (mvrp && mvrp->varp() == invarp) {
                            addModportVarRefIfMissing(modportp, newp, VDirection::প্রবেশ);
                            ভঙ্গ;
                        }
                    }
                }
            } অন্যথা {
                modAddStmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        প্রত্যাগম VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ধ্রুব constp) {
        File_সারি* ধ্রুব fl = constp->fileline();
        V3_সংখ্যা numz{constp, constp->width()};
        numz.opBitsZ(constp->অঙ্ক());  // Z->1, else 0
        V3_সংখ্যা numz0{constp, constp->width()};
        numz0.opNot(numz);  // Z->0, else 1
        প্রত্যাগম নূতন AstConst{fl, numz0};
    }
    Ast_গ্রন্থি_রাশিমালা* get_En_রাশিমালা_Based_On_Originalp(Ast_গ্রন্থি_রাশিমালা* ধ্রুব nodep) {
        যদি (AstVarRef* ধ্রুব varrefp = VN_CAST(nodep, VarRef)) {
            প্রত্যাগম নূতন AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), অসত্য),
                                 VAccess::READ};
        } অন্যথা যদি (AstVarXRef* ধ্রুব xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ধ্রুব enVarp = getCreateEnVarp(xrefp->varp(), অসত্য);
            প্রত্যাগম newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } অন্যথা যদি (AstConst* ধ্রুব constp = VN_CAST(nodep, Const)) {
            প্রত্যাগম getNonZConstp(constp);
        } অন্যথা যদি (AstExtend* ধ্রুব extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = extendp->lhsp()->unlinkFrBack();
            AstExtend* ধ্রুব enExtendp = extendp->clone_বৃক্ষ(অসত্য);
            extendp->lhsp(lhsp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব enLhsp = get_En_রাশিমালা_Based_On_Originalp(lhsp);
            enExtendp->lhsp(নূতন AstNot{enLhsp->fileline(), enLhsp});
            প্রত্যাগম নূতন AstNot{enExtendp->fileline(), enExtendp};
        } অন্যথা যদি (AstSel* ধ্রুব selp = VN_CAST(nodep, Sel)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব fromp = selp->fromp()->unlinkFrBack();
            AstSel* ধ্রুব enSelp = selp->clone_বৃক্ষ(অসত্য);
            selp->fromp(fromp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব enFromp = get_En_রাশিমালা_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            প্রত্যাগম enSelp;
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            প্রত্যাগম nullptr;
        }
    }
    AstVar* getCreateOutVarp(AstVar* invarp, তার্কিক is_শীর্ষ) {
        // Return the master __out for the specified input variable
        যদি (!m_varAux(invarp).outVarp) {
            AstVar* ধ্রুব newp
                = নূতন AstVar{invarp->fileline(), is_শীর্ষ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            modAddStmtp(invarp, newp);
            m_varAux(invarp).outVarp = newp;  // find outvar given invarp
        }
        প্রত্যাগম m_varAux(invarp).outVarp;
    }
    AstVar* getCreateUnconnVarp(Ast_গ্রন্থি* fromp, Ast_গ্রন্থি_DType* dtypep) {
        AstVar* ধ্রুব newp = নূতন AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvtToStr(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        modAddStmtp(newp, newp);
        প্রত্যাগম newp;
    }

    LhsProjection newLhsProjection(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_Var_Ref* targetp, Ast_গ্রন্থি_রাশিমালা* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব newLhsp = lhsp->clone_বৃক্ষ_Pure(অসত্য);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_গ্রন্থি* nodep) { nodep->user1p(nullptr); });
        Ast_গ্রন্থি_Var_Ref* ধ্রুব newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        প্রত্যাগম LhsProjection{newLhsp, newVarrefp, enp};
    }

    শূন্য map_ঢোকাও_Lhs_Var_Ref(Ast_গ্রন্থি_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ধ্রুব চাবি = nodep->varp();
        ধ্রুব auto pair = m_lhsmap.emplace(চাবি, nullptr);
        যদি (pair.second) pair.first->second = নূতন RefStrengthVec;
        Ast_গ্রন্থি_রাশিমালা* enLhsp = nullptr;
        Ast_গ্রন্থি_Var_Ref* enVarrefp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* enRhsp = nullptr;
        ধ্রুব auto enIt = m_assignEnps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ধ্রুব তার্কিক canCombineProcedural = !VN_IS(চাবি->dtypep()->skipRefp(), গ্রন্থি_UOr_Struct_DType);
        ধ্রুব Ast_গ্রন্থি_Procedure* ধ্রুব procedurep
            = enIt != m_assignEnps.end() && canCombineProcedural ? m_procedurep : nullptr;
        যদি (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_বৃক্ষ_Pure(অসত্য));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->ঠেলো_back(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_গ্রন্থি_রাশিমালা* newEnableDeposit(AstSel* selp, Ast_গ্রন্থি_রাশিমালা* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব newp = নূতন AstShiftL{
            selp->fileline(), নূতন AstExtend{selp->fileline(), enp, selp->fromp()->width()},
            selp->lsbp()->clone_বৃক্ষ_Pure(অসত্য), selp->fromp()->width()};
        প্রত্যাগম newp;
    }

    শূন্য setPullDirection(AstVar* varp, AstPull* pullp) {
        ধ্রুব AstPull* ধ্রুব oldpullp = m_varAux(varp).pullp;
        যদি (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } অন্যথা {
            যদি (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    শূন্য setBitPullDirection(AstVar* varp, পূর্ণাঙ্ক bitIndex, পূর্ণাঙ্ক direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        auto& bitPulls = m_varAux(varp).bitPulls;
        auto it = bitPulls.find(bitIndex);
        যদি (it != bitPulls.end() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bitIndex << " of " << varp->prettyNameQ());
        }
        bitPulls[bitIndex] = direction;
    }

    AstConst* createPerBitPullConst(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ধ্রুব auto& bitPulls = m_varAux(varp).bitPulls;
        V3_সংখ্যা অঙ্ক{varp, varp->width(), 0};  // Start with all zeros
        প্রতি (ধ্রুব auto& pair : bitPulls) {
            যদি (pair.second == 1) অঙ্ক.setBit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        প্রত্যাগম নূতন AstConst{varp->fileline(), অঙ্ক};
    }

    তার্কিক hasPerBitPulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        প্রত্যাগম !m_varAux(varp).bitPulls.empty();
    }

    শূন্য checkUnhandled(Ast_গ্রন্থি* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        যদি (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        যদি ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    শূন্য ঢোকাও_Tristates(Ast_গ্রন্থি_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ধ্রুব TristateGraph::VarVec vars = m_tgraph.tristateVars();
        প্রতি (auto varp : vars) {
            যদি (!m_tgraph.isTristate(varp)) অনুবর্ত;
            ধ্রুব auto it = m_lhsmap.find(varp);
            যদি (it != m_lhsmap.end()) অনুবর্ত;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ধ্রুব AstPull* ধ্রুব pullp = m_varAux(varp).pullp;
            ধ্রুব তার্কিক pull_মান = pullp && pullp->direction() == 1;
            AstConst* ধ্রুব constp = newAllZerosOrOnes(varp, pull_মান);
            AstVarRef* ধ্রুব varrefp = নূতন AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            AstAssignW* ধ্রুব newp = নূতন AstAssignW{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, অসত্য));  // en=0 for floating/pullup vars
            nodep->addStmtsp(নূতন AstAlways{newp});
            map_ঢোকাও_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        প্রতি (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ধ্রুব std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            যদি (it == m_lhsmap.end()) অনুবর্ত;
            AstVar* ধ্রুব invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            যদি (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_গ্রন্থি_Module* ধ্রুব ownerModp = findParentModule(invarp);
                ধ্রুব তার্কিক isCrossModule = ownerModp && ownerModp != nodep;
                ধ্রুব তার্কিক isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                যদি (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    প্রতি (ধ্রুব RefStrength& rs : *refsp) {
                        যদি (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            ভঙ্গ;
                        }
                    }
                } অন্যথা যদি (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = সত্য;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    সংরচনা PartitionInfo final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, PartitionInfo> partitions;
                    প্রতি (ধ্রুব RefStrength& rs : *refsp) {
                        যদি (AstVarXRef* ধ্রুব xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            PartitionInfo& pi = partitions[xrefp->dotted()];
                            pi.refs.ঠেলো_back(rs);
                            যদি (pi.inlinedDots.empty()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } অন্যথা {
                            partitions[""].refs.ঠেলো_back(rs);
                        }
                    }
                    প্রতি (auto& kv : partitions) {
                        ঢোকাও_Tristates_Signal(nodep, invarp, &kv.second.refs, সত্য, kv.first,
                                              kv.second.inlinedDots,
                                              findModportForDotted(nodep, kv.first));
                    }
                } অন্যথা যদি (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = সত্য;
                    ঢোকাও_Tristates_Signal(nodep, invarp, refsp, সত্য, "", "", nullptr);
                } অন্যথা {
                    ঢোকাও_Tristates_Signal(nodep, invarp, refsp, অসত্য, "", "", nullptr);
                }
            } অন্যথা {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(delete refsp, refsp);
        }
    }

    শূন্য aggregateTriSameStrength(Ast_গ্রন্থি_Module* nodep, AstVar* ধ্রুব varp, AstVar* ধ্রুব envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator endStrength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        সংরচনা DriverGroup final {
            ধ্রুব Ast_গ্রন্থি_Procedure* m_procedurep;  // Process owning these assignments
            std::vector<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::vector<DriverGroup> drivers;
        std::map<ধ্রুব Ast_গ্রন্থি_Procedure*, size_t> procedureToDriver;
        প্রতি (auto it = beginStrength; it != endStrength; ++it) {
            যদি (it->m_procedurep) {
                ধ্রুব auto emplaced = procedureToDriver.emplace(it->m_procedurep, drivers.size());
                যদি (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.ঠেলো_back(&*it);
                    অনুবর্ত;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ঠেলো_back(DriverGroup{it->m_procedurep, {&*it}});
        }

        Ast_গ্রন্থি_রাশিমালা* orp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* enp = nullptr;

        প্রতি (DriverGroup& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ধ্রুব newLhsp = নূতন AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvtToStr(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->setDfgTriLowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->addStmtsp(newLhsp);
            AstVar* ধ্রুব newEnLhsp
                = নূতন AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvtToStr(m_unique++), envarp};  // 2-state ok
            newEnLhsp->setDfgTriLowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->addStmtsp(newEnLhsp);

            প্রতি (RefStrength* ধ্রুব refStrengthp : driver.m_refsp) {
                Ast_গ্রন্থি_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                যদি (VN_IS(refp, VarXRef)) {
                    AstVarRef* ধ্রুব localRefp
                        = নূতন AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(nullptr);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } অন্যথা {
                    refp->varp(newLhsp);
                }

                যদি (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_গ্রন্থি_রাশিমালা* enLhsp = refStrengthp->m_enLhsp;
                    Ast_গ্রন্থি_Var_Ref* ধ্রুব enVarrefp = refStrengthp->m_enVarrefp;
                    যদি (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ধ্রুব localRefp
                            = নূতন AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        যদি (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_বৃক্ষ(), enVarrefp);
                        } অন্যথা {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ঠেলো_Deletep(enVarrefp), enVarrefp);
                        }
                    } অন্যথা {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ঠেলো_back(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = nullptr;
                    refStrengthp->m_enVarrefp = nullptr;
                    refStrengthp->m_enRhsp = nullptr;
                } অন্যথা {
                    AstAssignW* ধ্রুব enLhspAssignp = নূতন AstAssignW{
                        refp->fileline(),
                        নূতন AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    যদি (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_বৃক্ষ(অসত্য));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->addStmtsp(নূতন AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_সারি* ধ্রুব fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব ref1p = নূতন AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব ref2p = নূতন AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব andp = নূতন AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : নূতন AstOr{fl, orp, andp};

            Ast_গ্রন্থি_রাশিমালা* ধ্রুব ref3p = নূতন AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : নূতন AstOr{ref3p->fileline(), enp, ref3p};
        }
        AstAssignW* ধ্রুব assp = নূতন AstAssignW{
            varp->fileline(), নূতন AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->addStmtsp(নূতন AstAlways{assp});

        AstAssignW* ধ্রুব enAssp = নূতন AstAssignW{
            envarp->fileline(), নূতন AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->addStmtsp(নূতন AstAlways{enAssp});
    }

    শূন্য ঢোকাও_Procedural_Enable_Assignments(Ast_গ্রন্থি_Module* nodep) {
        প্রতি (auto& assignEnables : m_proceduralEnables) {
            Ast_গ্রন্থি_Assign* ধ্রুব assignp = assignEnables.first;
            Ast_গ্রন্থি_রাশিমালা* combinedLhsp = assignp->lhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* combinedRhsp = assignp->rhsp()->unlinkFrBack();
            প্রতি (ProceduralEnable& enable : assignEnables.second) {
                combinedLhsp = নূতন AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = নূতন AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            যদি (!VN_IS(assignp, AssignDly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ধ্রুব sampledp
                    = নূতন AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvtToStr(m_unique++), combinedRhsp->dtypep()};
                nodep->addStmtsp(sampledp);
                assignp->lhsp(নূতন AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                AstAssign* ধ্রুব applyp
                    = নূতন AstAssign{assignp->fileline(), combinedLhsp,
                                    নূতন AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->addNextHere(applyp);
            } অন্যথা {
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
    শূন্য ঢোকাও_Tristates_Signal(Ast_গ্রন্থি_Module* nodep, AstVar* ধ্রুব invarp, RefStrengthVec* refsp,
                               তার্কিক isIfaceTri, ধ্রুব string& ifaceDottedPath,
                               ধ্রুব string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        ধ্রুব তার্কিক is_শীর্ষ_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_শীর্ষ();
        যদি (!isIfaceTri) {
            যদি (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty())
                 && is_শীর্ষ_Inout)
                || (!nodep->is_শীর্ষ() && invarp->isIO())) {
                // This var becomes an input
                invarp->varType2In();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = getCreateOutVarp(invarp, is_শীর্ষ_Inout);
                outvarp->varType2Out();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_শীর্ষ_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                যদি (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                যদি (!m_varAux(invarp).bitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } অন্যথা যদি (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_গ্রন্থি_রাশিমালা* orp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* enp = nullptr;

        std::sort(refsp->begin(), refsp->end(),
                  [](RefStrength a, RefStrength b) { প্রত্যাগম a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        যাবত্ (beginStrength != refsp->end()) {
            RefStrengthVec::iterator endStrength = beginStrength + 1;
            যাবত্ (endStrength != refsp->end()
                   && endStrength->m_strength == beginStrength->m_strength)
                endStrength++;

            File_সারি* ধ্রুব fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strengthPrefix;
            যদি (isIfaceTri && !ifaceDottedPath.empty()) {
                strengthPrefix = ifaceDottedPath;
                std::replace(strengthPrefix.begin(), strengthPrefix.end(), '.', '_');
                strengthPrefix += "__";
            }
            ধ্রুব string strengthVarName
                = strengthPrefix + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ধ্রুব varStrengthp = নূতন AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->setDfgTriLowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->addStmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ধ্রুব enVarStrengthp = নূতন AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->setDfgTriLowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->addStmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     endStrength);

            Ast_গ্রন্থি_রাশিমালা* রাশিমালা_Current_Strengthp;
            যদি (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                রাশিমালা_Current_Strengthp
                    = নূতন AstAnd{fl, নূতন AstVarRef{fl, varStrengthp, VAccess::READ},
                                 নূতন AstNot{fl, enp->clone_বৃক্ষ_Pure(অসত্য)}};
            } অন্যথা {
                রাশিমালা_Current_Strengthp = নূতন AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? রাশিমালা_Current_Strengthp : নূতন AstOr{fl, orp, রাশিমালা_Current_Strengthp};

            Ast_গ্রন্থি_রাশিমালা* ধ্রুব enVarStrengthRefp
                = নূতন AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : নূতন AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = endStrength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        যদি (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->setDfgAllowMultidriveTri();
            Ast_গ্রন্থি_Module* ধ্রুব ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ধ্রুব পূর্ণাঙ্ক contribIdx = static_cast<পূর্ণাঙ্ক>(m_ifaceContribs[invarp].size());
            File_সারি* ধ্রুব fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ধ্রুব contribOutp = নূতন AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvtToStr(contribIdx), invarp};
            contribOutp->setDfgTriLowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->addStmtsp(contribOutp);

            AstVar* ধ্রুব contribEnp = নূতন AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvtToStr(contribIdx), invarp};
            contribEnp->setDfgTriLowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->addStmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            যদি (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                addModportVarRefIfMissing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                addModportVarRefIfMissing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_গ্রন্থি_Var_Ref* ধ্রুব lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_গ্রন্থি_Var_Ref*>(
                                                    নূতন AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_গ্রন্থি_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ধ্রুব assp = নূতন AstAssignW{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(নূতন AstAlways{assp});
            }
            {
                Ast_গ্রন্থি_Var_Ref* ধ্রুব lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_গ্রন্থি_Var_Ref*>(
                                                    নূতন AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_গ্রন্থি_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ধ্রুব assp = নূতন AstAssignW{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(নূতন AstAlways{assp});
            }

            m_ifaceContribs[invarp].ঠেলো_back({contribEnp, contribOutp});
            প্রত্যাগম;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ধ্রুব তার্কিক hasPull = m_varAux(lhsp).pullp || hasPerBitPulls(lhsp);
        যদি (!outvarp || hasPull) {
            Ast_গ্রন্থি_রাশিমালা* undrivenp;
            যদি (envarp) {
                undrivenp = নূতন AstNot{envarp->fileline(),
                                       নূতন AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } অন্যথা যদি (enp) {
                undrivenp = নূতন AstNot{enp->fileline(), enp};
                enp = nullptr;  // moved into undrivenp
            } অন্যথা {
                undrivenp = newAllZerosOrOnes(invarp, সত্য);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            যদি (hasPerBitPulls(lhsp)) {
                pullConstp = createPerBitPullConst(lhsp);
            } অন্যথা {
                ধ্রুব AstPull* ধ্রুব pullp = m_varAux(lhsp).pullp;
                ধ্রুব তার্কিক pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = নূতন AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? নূতন AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        যদি (envarp && !enp) enp = newAllZerosOrOnes(invarp, অসত্য);

        যদি (envarp) {
            AstAssignW* ধ্রুব enAssp = নূতন AstAssignW{
                enp->fileline(), নূতন AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->addStmtsp(নূতন AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        AstAssignW* ধ্রুব assp = নূতন AstAssignW{
            lhsp->fileline(), নূতন AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->addStmtsp(নূতন AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        যদি ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty()) && is_শীর্ষ_Inout) {
            যদি (envarp) {
                envarp->primaryIO(সত্য);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            যদি (outvarp) {
                outvarp->primaryIO(সত্য);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            যদি (!v3Global.opt.libCreate().empty()) {
                ধ্রুব পূর্ণাঙ্ক id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(সত্য);
            }
        }
    }

    তার্কিক isOnlyAssignmentIsToLhsVar(ধ্রুব AstAssignW* ধ্রুব nodep) {
        যদি (ধ্রুব AstVarRef* ধ্রুব varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            auto foundIt = m_assigns.find(varRefp->varp());
            যদি (foundIt != m_assigns.end()) {
                auto ধ্রুব& assignsToVar = foundIt->second;
                যদি (assignsToVar.size() == 1 && assignsToVar[0] == nodep) প্রত্যাগম সত্য;
            }
        }
        প্রত্যাগম অসত্য;
    }

    শূন্য addToAssignmentList(AstAssignW* nodep) {
        যদি (AstVarRef* ধ্রুব varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            যদি (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ঠেলো_back(nodep);
            } অন্যথা যদি (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlinkFrBack()->delete_বৃক্ষ();
            }
        } অন্যথা যদি (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ধ্রুব AstAssignW* ধ্রুব nodep, তার্কিক মান) {
        যদি (AstStrengthSpec* ধ্রুব strengthSpec = nodep->strengthSpecp()) {
            প্রত্যাগম মান ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        প্রত্যাগম VStrength::STRONG;  // default strength is strong
    }

    তার্কিক assignment_Of_মান_On_All_Bits(ধ্রুব AstAssignW* ধ্রুব nodep, তার্কিক মান) {
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep->rhsp(), Const)) {
            ধ্রুব V3_সংখ্যা অঙ্ক = constp->অঙ্ক();
            প্রত্যাগম মান ? অঙ্ক.isEqAllOnes() : অঙ্ক.isEqZero();
        }
        প্রত্যাগম অসত্য;
    }

    AstAssignW* get_Strongest_Assignment_Of_মান(ধ্রুব Assigns& assigns, তার্কিক মান) {
        auto maxIt = std::max_element(
            assigns.begin(), assigns.end(), [&](ধ্রুব AstAssignW* ap, ধ্রুব AstAssignW* bp) {
                ধ্রুব তার্কিক মানসমূহ_On_Rhs_A = assignment_Of_মান_On_All_Bits(ap, মান);
                ধ্রুব তার্কিক মানসমূহ_On_Rhs_B = assignment_Of_মান_On_All_Bits(bp, মান);
                যদি (!মানসমূহ_On_Rhs_A) প্রত্যাগম মানসমূহ_On_Rhs_B;
                যদি (!মানসমূহ_On_Rhs_B) প্রত্যাগম অসত্য;
                প্রত্যাগম getStrength(ap, মান) < getStrength(bp, মান);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        প্রত্যাগম assignment_Of_মান_On_All_Bits(*maxIt, মান) ? *maxIt : nullptr;
    }

    তার্কিক isAssignmentNotStrongerThanStrength(ধ্রুব AstAssignW* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ধ্রুব uint8_t strength0 = getStrength(assignp, 0);
        ধ্রুব uint8_t strength1 = getStrength(assignp, 1);
        প্রত্যাগম (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_Of_মান_On_All_Bits(assignp, 0))
               || (strength1 <= strength && assignment_Of_মান_On_All_Bits(assignp, 1));
    }

    Ast_গ্রন্থি_রাশিমালা* new_Merge_রাশিমালা(Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp, Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp, File_সারি* ধ্রুব fl,
                              তার্কিক isWor) {
        Ast_গ্রন্থি_রাশিমালা* রাশিমালা = nullptr;
        যদি (isWor)
            রাশিমালা = নূতন AstOr{fl, lhsp, rhsp};
        অন্যথা
            রাশিমালা = নূতন AstAnd{fl, lhsp, rhsp};
        প্রত্যাগম রাশিমালা;
    }

    শূন্য mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        প্রতি (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            যদি (assigns.size() <= 1) অনুবর্ত;
            AstVar* ধ্রুব varp = varpAssigns.first;
            যদি (!varp->isWiredNet()) অনুবর্ত;
            auto it = assigns.begin();
            AstAssignW* ধ্রুব assignWp0 = *it;
            File_সারি* ধ্রুব fl = assignWp0->fileline();
            Ast_গ্রন্থি_রাশিমালা* wExp = nullptr;
            যাবত্ (++it != assigns.end()) {
                AstAssignW* assignWpi = *it;
                যদি (!wExp) {
                    wExp
                        = new_Merge_রাশিমালা(assignWp0->rhsp()->clone_বৃক্ষ_Pure(অসত্য),
                                       assignWpi->rhsp()->clone_বৃক্ষ_Pure(অসত্য), fl, varp->isWor());
                } অন্যথা {
                    wExp = new_Merge_রাশিমালা(wExp, assignWpi->rhsp()->clone_বৃক্ষ_Pure(অসত্য), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((assignWpi->unlinkFrBack()->delete_বৃক্ষ()), assignWpi);
            }
            AstVarRef* ধ্রুব wVarRef = নূতন AstVarRef{fl, varp, VAccess::WRITE};
            AstAssignW* ধ্রুব wAssignp = নূতন AstAssignW{fl, wVarRef, wExp};
            assignWp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ঠেলো_Deletep(assignWp0), assignWp0);
            assigns.clear();
            assigns.ঠেলো_back(wAssignp);
        }
    }

    শূন্য removeNotStrongerAssignments(Assigns& assigns, ধ্রুব AstAssignW* strongestp,
                                      uint8_t greatestKnownStrength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        auto removedIt = std::remove_if(assigns.begin(), assigns.end(), [&](AstAssignW* assignp) {
            যদি (assignp == strongestp) প্রত্যাগম অসত্য;
            যদি (isAssignmentNotStrongerThanStrength(assignp, greatestKnownStrength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ঠেলো_Deletep(assignp->unlinkFrBack()), assignp);
                প্রত্যাগম সত্য;
            }
            প্রত্যাগম অসত্য;
        });
        assigns.erase(removedIt, assigns.end());
    }

    শূন্য removeAssignmentsNotStrongerThanUniformConstant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        প্রতি (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            যদি (assigns.size() <= 1) অনুবর্ত;
            ধ্রুব AstAssignW* ধ্রুব strongest0p = get_Strongest_Assignment_Of_মান(assigns, 0);
            ধ্রুব AstAssignW* ধ্রুব strongest1p = get_Strongest_Assignment_Of_মান(assigns, 1);
            ধ্রুব AstAssignW* strongestp = nullptr;
            uint8_t greatestKnownStrength = 0;
            ধ্রুব auto getIfStrongest
                = [&](ধ্রুব AstAssignW* ধ্রুব strongestCandidatep, তার্কিক মান) {
                      যদি (!strongestCandidatep) প্রত্যাগম;
                      ধ্রুব uint8_t strength = getStrength(strongestCandidatep, মান);
                      যদি (strength >= greatestKnownStrength) {
                          greatestKnownStrength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            যদি (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    শূন্য removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        প্রতি (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            যদি (assigns.size() <= 1) অনুবর্ত;
            auto maxIt = std::max_element(
                assigns.begin(), assigns.end(), [&](ধ্রুব AstAssignW* ap, ধ্রুব AstAssignW* bp) {
                    যদি (m_tgraph.isTristate(ap)) প্রত্যাগম !m_tgraph.isTristate(bp);
                    যদি (m_tgraph.isTristate(bp)) প্রত্যাগম অসত্য;
                    ধ্রুব uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ধ্রুব uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    প্রত্যাগম minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ধ্রুব AstAssignW* ধ্রুব strongestp = m_tgraph.isTristate(*maxIt) ? nullptr : *maxIt;
            যদি (strongestp) {
                ধ্রুব uint8_t greatestKnownStrength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    // VISITORS
    শূন্য visit(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        যদি (m_graphing) {
            যদি (!m_alhs && nodep->অঙ্ক().hasZ()) m_tgraph.setTristate(nodep);
        } অন্যথা {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            যদি (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ধ্রুব varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_গ্রন্থি* ধ্রুব newp = নূতন AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা যদি (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_সারি* ধ্রুব fl = nodep->fileline();
                AstConst* ধ্রুব enp = getNonZConstp(nodep);
                V3_সংখ্যা num1{nodep, nodep->width()};
                num1.opAnd(nodep->অঙ্ক(), enp->অঙ্ক());  // 01X->01X, Z->0
                AstConst* ধ্রুব newconstp = নূতন AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    শূন্য visit(AstCond* nodep) override {
        যদি (m_graphing) {
            iterateChildren(nodep);
            যদি (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } অন্যথা {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } অন্যথা {
            যদি (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                প্রত্যাগম;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = nodep->condp();
            যদি (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব thenp = nodep->thenp();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব elsep = nodep->elsep();
            যদি (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব en1p = getEnp(thenp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব enp
                    = নূতন AstCond{nodep->fileline(), condp->clone_বৃক্ষ(অসত্য), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                যদি (thenp->user1p() && !thenp->user1p()->backp()) ঠেলো_Deletep(thenp->user1p());
                যদি (elsep->user1p() && !elsep->user1p()->backp()) ঠেলো_Deletep(elsep->user1p());
                thenp->user1p(nullptr);
                elsep->user1p(nullptr);
            }
        }
    }

    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override {
        iterateChildren(nodep);
        যদি (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } অন্যথা যদি (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(nullptr);
            m_tgraph.didProcess(nodep);
        }
    }

    শূন্য visit(AstSel* nodep) override {
        যদি (m_graphing) {
            iterateChildren(nodep);
            যদি (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } অন্যথা {
                associateLogic(nodep->fromp(), nodep);
            }
        } অন্যথা {
            যদি (m_alhs) {
                UINFO(9, dbgState() << nodep);
                যদি (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), গ্রন্থি_রাশিমালা));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } অন্যথা {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                যদি (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                যদি (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব en1p = getEnp(nodep->fromp());
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব enp
                        = নূতন AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_বৃক্ষ_Pure(সত্য),
                                     nodep->widthConst()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    শূন্য visit(AstConcat* nodep) override {
        যদি (m_graphing) {
            iterateChildren(nodep);
            যদি (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } অন্যথা {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } অন্যথা {
            যদি (m_alhs) {
                UINFO(9, dbgState() << nodep);
                যদি (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব enp = VN_AS(nodep->user1p(), গ্রন্থি_রাশিমালা);
                    nodep->user1p(nullptr);
                    nodep->lhsp()->user1p(নূতন AstSel{nodep->fileline(), enp->clone_বৃক্ষ_Pure(সত্য),
                                                     nodep->rhsp()->width(),
                                                     nodep->lhsp()->width()});
                    nodep->rhsp()->user1p(
                        নূতন AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->width()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } অন্যথা {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব expr1p = nodep->lhsp();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব expr2p = nodep->rhsp();
                যদি (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব en1p = getEnp(expr1p);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব en2p = getEnp(expr2p);
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব enp = নূতন AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(nullptr);
                    expr2p->user1p(nullptr);
                }
            }
        }
    }

    শূন্য visit(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        যদি (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.setTristate(nodep);
        } অন্যথা {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            যদি (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                প্রত্যাগম;
            }
            m_tgraph.didProcess(nodep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব expr1p = nodep->lhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব expr2p = nodep->rhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* enp;
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব en2p = VN_AS(expr2p->user1p(), গ্রন্থি_রাশিমালা)) {
                enp = নূতন AstAnd{nodep->fileline(), expr1p, en2p};
            } অন্যথা {
                enp = expr1p;
            }
            expr1p->user1p(nullptr);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    শূন্য visitAndOr(Ast_গ্রন্থি_Biop* nodep, তার্কিক isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        যদি (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } অন্যথা {
            যদি (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                প্রত্যাগম;
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
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব expr1p = nodep->lhsp();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব expr2p = nodep->rhsp();
            যদি (!expr1p->user1p() && !expr2p->user1p()) {
                প্রত্যাগম;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_গ্রন্থি_রাশিমালা* en1p = getEnp(expr1p);
            Ast_গ্রন্থি_রাশিমালা* en2p = getEnp(expr2p);
            Ast_গ্রন্থি_রাশিমালা* subexpr1p = expr1p->clone_বৃক্ষ_Pure(অসত্য);
            Ast_গ্রন্থি_রাশিমালা* subexpr2p = expr2p->clone_বৃক্ষ_Pure(অসত্য);
            যদি (isAnd) {
                subexpr1p = নূতন AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = নূতন AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            যদি (!en1p->op1p() && en2p->op1p()) {
                std::swap(en1p, en2p);
                std::swap(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব enp = নূতন AstOr{
                nodep->fileline(),
                নূতন AstAnd{nodep->fileline(), en1p, নূতন AstOr{nodep->fileline(), en2p, subexpr1p}},
                নূতন AstAnd{nodep->fileline(), en2p->clone_বৃক্ষ(অসত্য), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(nullptr);
            expr2p->user1p(nullptr);
        }
    }
    শূন্য visit(AstAnd* nodep) override { visitAndOr(nodep, সত্য); }
    শূন্য visit(AstOr* nodep) override { visitAndOr(nodep, অসত্য); }

    শূন্য visitAssign(Ast_গ্রন্থি_Assign* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        যদি (VN_IS(nodep->rhsp(), CReset)) প্রত্যাগম;
        যদি (AstAssignW* ধ্রুব assignWp = VN_CAST(nodep, AssignW)) {
            m_currentDelayp = VN_CAST(assignWp->timingControlp(), Delay);
        }
        যদি (m_graphing) {
            যদি (AstAssignW* assignWp = VN_CAST(nodep, AssignW)) {
                যদি (assignWp->getLhsNetDelay()) প্রত্যাগম;
                যদি (assignWp->timingControlp() && !m_currentDelayp) প্রত্যাগম;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                যদি (m_currentDelayp && m_currentDelayp->fallDelay()) প্রত্যাগম;
                যদি (!assignWp->timingControlp()) addToAssignmentList(assignWp);
            }

            যদি (nodep->user2() & U2_GRAPHING) প্রত্যাগম;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterateAndNextNull(nodep->rhsp());
            m_alhs = সত্য;
            iterateAndNextNull(nodep->lhsp());
            m_alhs = অসত্য;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } অন্যথা {
            যদি (nodep->user2() & U2_NONGRAPH) {
                প্রত্যাগম;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterateAndNextNull(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            যদি (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, AssignW), nodep,
                            "Continuous assignment inside procedural process");
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), গ্রন্থি_রাশিমালা)->clone_বৃক্ষ_Pure(অসত্য)
                          : newAllZerosOrOnes(nodep->lhsp(), সত্য);
                ধ্রুব তার্কিক inserted = m_assignEnps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            যদি (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(nullptr);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } অন্যথা {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                যদি (AstSel* ধ্রুব selp = VN_CAST(nodep->lhsp(), Sel)) {
                    যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুব varrefp = VN_CAST(selp->fromp(), গ্রন্থি_Var_Ref)) {
                        যদি (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), সত্য));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } অন্যথা {
                            m_tgraph.didProcess(nodep, সত্য);
                        }
                    } অন্যথা {
                        m_tgraph.didProcess(nodep, সত্য);  // LCOV_EXCL_LINE
                    }
                } অন্যথা {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, সত্য);
                }
            }
            m_alhs = সত্য;  // And user1p() will indicate tristate equation, if any
            যদি (AstAssignW* ধ্রুব assignWp = VN_CAST(nodep, AssignW)) {
                যদি (AstStrengthSpec* ধ্রুব specp = assignWp->strengthSpecp()) {
                    যদি (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        যদি (!isOnlyAssignmentIsToLhsVar(assignWp)) {
                            assignWp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } অন্যথা {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterateAndNextNull(nodep->lhsp());
        }
    }
    শূন্য visit(AstAssignW* nodep) override { visitAssign(nodep); }
    শূন্য visit(AstAssign* nodep) override { visitAssign(nodep); }
    শূন্য visit(AstAssignDly* nodep) override { visitAssign(nodep); }

    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ধ্রুব AstAlways* ধ্রুব alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN ? nullptr : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    শূন্য visit(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_inAlias);
        m_inAlias = সত্য;
        যদি (m_graphing) {
            যদি (nodep->user2() & U2_GRAPHING) প্রত্যাগম;
            m_alhs = সত্য;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            প্রতি (Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } অন্যথা {
            iterateChildren(nodep);
        }
    }

    শূন্য visitCaseEq(Ast_গ্রন্থি_Biop* nodep, তার্কিক neq) {
        যদি (m_graphing) {
            iterateChildren(nodep);
        } অন্যথা {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ধ্রুব constp = VN_CAST(nodep->lhsp(), Const);
            যদি (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                File_সারি* ধ্রুব fl = nodep->fileline();
                Ast_গ্রন্থি_রাশিমালা* enRhsp;
                যদি (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), গ্রন্থি_রাশিমালা);
                    rhsp->user1p(nullptr);
                } অন্যথা {
                    enRhsp = get_En_রাশিমালা_Based_On_Originalp(rhsp);
                }
                Ast_গ্রন্থি_রাশিমালা* newp
                    = নূতন AstLogAnd{fl, নূতন AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    নূতন AstEqCase{fl, নূতন AstConst{fl, constp->অঙ্ক()}, rhsp}};
                constp->user1p(nullptr);
                যদি (neq) newp = নূতন AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা যদি (constp && nodep->rhsp()->user1p()) {
                File_সারি* ধ্রুব fl = nodep->fileline();
                constp->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* newp = নূতন AstLogAnd{fl,
                                                  নূতন AstEq{fl, newAllZerosOrOnes(constp, অসত্য),
                                                            VN_AS(rhsp->user1p(), গ্রন্থি_রাশিমালা)},
                                                  // Keep the caseeq if there are X's present
                                                  নূতন AstEqCase{fl, constp, rhsp}};
                যদি (neq) newp = নূতন AstLogNot{fl, newp};
                rhsp->user1p(nullptr);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা {
                checkUnhandled(nodep);
            }
        }
    }
    শূন্য visitEqNeqWild(Ast_গ্রন্থি_Biop* nodep) {
        যদি (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterateAndNextNull(nodep->lhsp());
        যদি (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            প্রত্যাগম;
        }
    }
    শূন্য visit(AstEqCase* nodep) override { visitCaseEq(nodep, অসত্য); }
    শূন্য visit(AstNeqCase* nodep) override { visitCaseEq(nodep, সত্য); }
    শূন্য visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    শূন্য visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }

    শূন্য visit(Ast_গুনতি_Bits* nodep) override {
        std::array<তার্কিক, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->অঙ্ক().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->অঙ্ক().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->অঙ্ক().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        যদি (m_graphing) {
            iterateAndNextNull(nodep->lhsp());
            যদি (!dropop[0]) iterateAndNextNull(nodep->rhsp());
            যদি (!dropop[1]) iterateAndNextNull(nodep->thsp());
            যদি (!dropop[2]) iterateAndNextNull(nodep->fhsp());
        } অন্যথা {
            Ast_গ্রন্থি_রাশিমালা* nonXp = nullptr;
            যদি (!dropop[0]) {
                nonXp = nodep->rhsp();
            } অন্যথা যদি (!dropop[1]) {
                nonXp = nodep->thsp();
            } অন্যথা যদি (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            যদি (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ধ্রুব AstVarRef* ধ্রুব varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                যদি (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    প্রত্যাগম;
                }
                AstVar* ধ্রুব envarp = getCreateEnVarp(varrefp->varp(), অসত্য);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                Ast_গ্রন্থি_রাশিমালা* newp = নূতন Ast_গুনতি_Ones{
                    nodep->fileline(), নূতন AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                যদি (nonXp) {  // Need to still count '0 or '1 or 'x's
                    যদি (dropop[0]) {
                        nodep->rhsp()->unlinkFrBack()->delete_বৃক্ষ();
                        nodep->rhsp(nonXp->clone_বৃক্ষ_Pure(সত্য));
                    }
                    যদি (dropop[1]) {
                        nodep->thsp()->unlinkFrBack()->delete_বৃক্ষ();
                        nodep->thsp(nonXp->clone_বৃক্ষ_Pure(সত্য));
                    }
                    যদি (dropop[2]) {
                        nodep->fhsp()->unlinkFrBack()->delete_বৃক্ষ();
                        nodep->fhsp(nonXp->clone_বৃক্ষ_Pure(সত্য));
                    }
                    newp = নূতন AstAdd{nodep->fileline(), nodep, newp};
                } অন্যথা {
                    ঠেলো_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = nullptr;
        যদি (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } অন্যথা যদি (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        যদি (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } অন্যথা {
            যদি (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.setTristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } অন্যথা {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ধ্রুব AstSel* ধ্রুব selp = VN_CAST(nodep->lhsp(), Sel);
                ধ্রুব AstConst* ধ্রুব lsbp = selp ? VN_CAST(selp->lsbp(), Const) : nullptr;
                যদি (lsbp) {
                    ধ্রুব পূর্ণাঙ্ক lsb = lsbp->toSInt();
                    প্রতি (পূর্ণাঙ্ক i = 0; i < selp->widthConst(); ++i) {
                        setBitPullDirection(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } অন্যথা {
                    setPullDirection(varrefp->varp(), nodep);
                }
            }
        }
        যদি (!m_graphing) {
            nodep->unlinkFrBack();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    শূন্য iteratePinGuts(AstPin* nodep) {
        যদি (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            যদি (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } অন্যথা {
            // All heavy lifting completed in graph visitor.
            যদি (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    শূন্য visit(AstPin* nodep) override {
        যদি (m_graphing) {
            যদি (nodep->user2() & U2_GRAPHING) প্রত্যাগম;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ধ্রুব enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            যদি (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, অসত্য);
                iteratePinGuts(nodep);
                প্রত্যাগম;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ধ্রুব তার্কিক inDeclProcessing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::প্রবেশ
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            যদি (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ধ্রুব ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(নূতন AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.setTristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } অন্যথা যদি (inDeclProcessing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ধ্রুব AstAssignW* ধ্রুব reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, সত্য, সত্য);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.setTristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ধ্রুব enVarp = নূতন AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvtToStr(m_unique++),
                                              VFlagBitPacked{}, enModVarp->width()};
            যদি (inDeclProcessing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->varType2In();
            } অন্যথা {
                enModVarp->varType2Out();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ধ্রুব enpinp
                = নূতন AstPin{nodep->fileline(), nodep->pin_অঙ্ক(),
                             enModVarp->name(),  // should be {var}"__en"
                             নূতন AstVarRef{nodep->fileline(), enVarp,
                                           inDeclProcessing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->addNextHere(enpinp);
            m_modp->addStmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ধ্রুব AstAssignW* outAssignp = nullptr;  // If reconnected, the related assignment
            AstPin* outpinp = nullptr;
            AstVar* ধ্রুব outModVarp = m_varAux(nodep->modVarp()).outVarp;
            যদি (!outModVarp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                যদি (!m_modp->is_শীর্ষ()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } অন্যথা {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব outexprp = VN_AS(nodep->exprp(), গ্রন্থি_রাশিমালা)
                                                  ->clone_বৃক্ষ_Pure(অসত্য);  // Note has lvalue() set
                outpinp = নূতন AstPin{nodep->fileline(), nodep->pin_অঙ্ক(),
                                     outModVarp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(outModVarp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->addNextHere(outpinp);
                // Simplify
                যদি (inDeclProcessing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, সত্য};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                outAssignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        সত্য);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, outAssignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ধ্রুব TristatePinVisitor visitor{nodep->exprp(), m_tgraph, অসত্য};
            ধ্রুব Ast_গ্রন্থি* ধ্রুব inAssignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, সত্য);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, inAssignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            যদি (!outAssignp) {
                যদি (!outpinp) {
                    exprrefp = nullptr;  // Primary input only
                } অন্যথা {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    যদি (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } অন্যথা {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(outAssignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                যদি (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            যদি (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(নূতন AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                যদি (!outAssignp) {
                    map_ঢোকাও_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            যদি (exprrefp) {
                AstPull* ধ্রুব pullp = m_varAux(nodep->modVarp()).pullp;
                ধ্রুব auto& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = nullptr;
                পূর্ণাঙ্ক selLsb = 0;
                পূর্ণাঙ্ক selWidth = 0;
                যদি (outAssignp) {
                    যদি (AstSel* ধ্রুব selp = VN_CAST(outAssignp->lhsp(), Sel)) {
                        AstVarRef* ধ্রুব vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ধ্রুব lsbp = VN_CAST(selp->lsbp(), Const);
                        যদি (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->toSInt();
                            selWidth = selp->widthConst();
                        }
                    }
                }

                যদি (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    setPullDirection(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    যদি (selVarp) {
                        ধ্রুব পূর্ণাঙ্ক direction = pullp->direction();
                        প্রতি (পূর্ণাঙ্ক i = 0; i < selWidth; ++i) {
                            setBitPullDirection(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                যদি (!srcBitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    প্রতি (ধ্রুব auto& pair : srcBitPulls) {
                        setBitPullDirection(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    যদি (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        প্রতি (ধ্রুব auto& pair : srcBitPulls) {
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
        অন্যথা {
            যদি (nodep->user2() & U2_NONGRAPH) প্রত্যাগম;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    শূন্য handle_গ্রন্থি_Var_Ref(Ast_গ্রন্থি_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        যদি (m_graphing) {
            যদি (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            যদি (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            যদি (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.setTristate(nodep->varp());
            }
        } অন্যথা {
            যদি (nodep->user2() & U2_NONGRAPH) প্রত্যাগম;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            যদি (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                যদি (m_inAlias) {
                    যদি (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } অন্যথা {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    প্রত্যাগম;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_ঢোকাও_Lhs_Var_Ref(nodep);
            } অন্যথা যদি (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_সারি* ধ্রুব fl = nodep->fileline();
                AstVar* ধ্রুব enVarp = getCreateEnVarp(nodep->varp(), অসত্য);
                যদি (AstVarXRef* ধ্রুব xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } অন্যথা {
                    nodep->user1p(নূতন AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    শূন্য visit(AstVarRef* nodep) override { handle_গ্রন্থি_Var_Ref(nodep); }
    শূন্য visit(AstVarXRef* nodep) override { handle_গ্রন্থি_Var_Ref(nodep); }

    শূন্য visit(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        যদি (m_graphing) {
            // If tri0/1 force a pullup
            যদি (nodep->user2() & U2_GRAPHING) প্রত্যাগম;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            যদি (nodep->isPulldown() || nodep->isPullup()) {
                Ast_গ্রন্থি* ধ্রুব newp = নূতন AstPull{
                    nodep->fileline(), নূতন AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->addNextHere(newp);
                // We'll iterate on the new AstPull later
            }
            যদি (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.setTristate(nodep);
            }
        } অন্যথা {  // !graphing
            যদি (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        যদি (m_processedIfaces && VN_IS(nodep, Iface)) প্রত্যাগম;
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
        m_graphing = অসত্য;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = nullptr;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = সত্য;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = অসত্য;
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
        ঢোকাও_Tristates(nodep);
        ঢোকাও_Procedural_Enable_Assignments(nodep);

        প্রতি (auto& assignEnp : m_assignEnps) {
            VL_DO_DANGLING(assignEnp.second->delete_বৃক্ষ(), assignEnp.second);
        }
        m_assignEnps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    শূন্য visit(AstClass* nodep) override {
        // don't deal with classes
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        // don't deal with functions
    }

    শূন্য visit(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterateAndNextNull(nodep->stmtsp());
    }

    শূন্য visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = অসত্য;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    শূন্য combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        প্রতি (ধ্রুব std::pair<AstVar* ধ্রুব, std::vector<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ধ্রুব invarp = kv.first;
            ধ্রুব std::vector<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_গ্রন্থি_Module* ধ্রুব ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_সারি* ধ্রুব fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.size() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_গ্রন্থি_রাশিমালা* orp = nullptr;
            Ast_গ্রন্থি_রাশিমালা* enp = nullptr;

            প্রতি (ধ্রুব std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ধ্রুব contribEnp = contrib.first;
                AstVar* ধ্রুব contribOutp = contrib.second;

                Ast_গ্রন্থি_রাশিমালা* ধ্রুব outRefp = নূতন AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব enRefp = নূতন AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব andp = নূতন AstAnd{fl, outRefp, enRefp};

                orp = (!orp) ? andp : নূতন AstOr{fl, orp, andp};

                Ast_গ্রন্থি_রাশিমালা* ধ্রুব enRef2p = নূতন AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : নূতন AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            যদি (!envarp) {
                envarp = নূতন AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->setDfgTriLowered();
                ifaceModp->addStmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            AstAssignW* ধ্রুব enAssp
                = নূতন AstAssignW{fl, নূতন AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->addStmtsp(নূতন AstAlways{enAssp});

            // Pull resolution
            ধ্রুব AstPull* ধ্রুব pullp = m_varAux(invarp).pullp;
            ধ্রুব তার্কিক pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_গ্রন্থি_রাশিমালা* undrivenp = নূতন AstNot{fl, নূতন AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = নূতন AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = নূতন AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            AstAssignW* ধ্রুব assp
                = নূতন AstAssignW{fl, নূতন AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->addStmtsp(নূতন AstAlways{assp});
        }
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::vector<Ast_গ্রন্থি_Module*> ifacesp;
        প্রতি (Ast_গ্রন্থি* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            যদি (VN_IS(modp, Iface)) ifacesp.ঠেলো_back(VN_AS(modp, গ্রন্থি_Module));
        }
        প্রতি (auto it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = সত্য;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ifdef VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::vector<Ast_গ্রন্থি*> unusedRootps;
        netlistp->foreach([&](Ast_গ্রন্থি* nodep) {
            Ast_গ্রন্থি* ধ্রুব enp = nodep->user1p();
            যদি (!enp) প্রত্যাগম;
            যদি (enp->backp()) প্রত্যাগম;
            unusedRootps.emplace_back(enp);
        });
        প্রতি (Ast_গ্রন্থি* ধ্রুব nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
#endif
    }
    ~TristateVisitor() override {
        V3Stats::addStat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

শূন্য V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("tristate", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
