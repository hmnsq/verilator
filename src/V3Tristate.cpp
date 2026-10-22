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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Tristate.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Inst.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ TristateBaseVisitor VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ VNVisitor {
ಸಾರ್ವಜನಿಕ:
    // METHODS
};

//######################################################################
// Graph support classes

ವರ್ಗ TristateVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_ಗಂಟು* ಧ್ರುವ m_nodep;
    ತಾರ್ಕಿಕ m_isTristate = ಅಸತ್ಯ;  // Logic indicates a tristate
    ತಾರ್ಕಿಕ m_feedsTri = ಅಸತ್ಯ;  // Propagates to a tristate node (on RHS)
    ತಾರ್ಕಿಕ m_processed = ಅಸತ್ಯ;  // Tristating was cleaned up
ಸಾರ್ವಜನಿಕ:
    TristateVertex(V3Graph* graphp, Ast_ಗಂಟು* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = ಸ್ವಭಾವ;
    // ACCESSORS
    Ast_ಗಂಟು* nodep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    ಧ್ರುವ AstVar* varp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_CAST(nodep(), Var); }
    string name() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvtToHex(nodep())));
    }
    string dotColor() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->fileline(); }
    ತಾರ್ಕಿಕ isTristate() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_isTristate; }
    ಶೂನ್ಯ isTristate(ತಾರ್ಕಿಕ flag) { m_isTristate = flag; }
    ತಾರ್ಕಿಕ feedsTri() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_feedsTri; }
    ಶೂನ್ಯ feedsTri(ತಾರ್ಕಿಕ flag) { m_feedsTri = flag; }
    ತಾರ್ಕಿಕ processed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_processed; }
    ಶೂನ್ಯ processed(ತಾರ್ಕಿಕ flag) { m_processed = flag; }
};

//######################################################################

ವರ್ಗ TristateGraph final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
ಸಾರ್ವಜನಿಕ:
    using VarVec = std::vector<AstVar*>;

ಗುಪ್ತ:
    // MEMBERS
    V3Graph m_graph;  // Logic graph

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    TristateGraph() { clearAndCheck(); }
    ಆಭಾಸ ~TristateGraph() { clearAndCheck(); }
    VL_UNCOPYABLE(TristateGraph);

ಗುಪ್ತ:
    // METHODS

    TristateVertex* makeVertex(Ast_ಗಂಟು* nodep) {
        TristateVertex* vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        ಯದಿ (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = ನೂತನ TristateVertex{&m_graph, nodep};
            nodep->user4p(vertexp);
        }
        ಪ್ರತ್ಯಾಗಮ vertexp;
    }

    // METHODS - Graph optimization
    ಶೂನ್ಯ graphWalkRecurseFwd(TristateVertex* vtxp, ಪೂರ್ಣಾಙ್ಕ level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        ಯದಿ (!vtxp->isTristate()) ಪ್ರತ್ಯಾಗಮ;  // tristate involved
        ಯದಿ (vtxp->user() == 1) ಪ್ರತ್ಯಾಗಮ;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        ಯದಿ (!vtxp->varp()) {  // not a var where we stop the recursion
            ಪ್ರತಿ (V3GraphEdge& edge : vtxp->outEdges()) {
                TristateVertex* ಧ್ರುವ vvertexp = static_cast<TristateVertex*>(edge.ತುದಿ());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ಯದಿ (!vvertexp->isTristate()) {
                    vvertexp->isTristate(ಸತ್ಯ);
                    graphWalkRecurseFwd(vvertexp, level + 1);
                }
            }
        } ಅನ್ಯಥಾ {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            ಪ್ರತಿ (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ಧ್ರುವ vvertexp = static_cast<TristateVertex*>(edge.fromp());
                ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    ಯದಿ (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(ಸತ್ಯ);
                        graphWalkRecurseFwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ graphWalkRecurseBack(TristateVertex* vtxp, ಪೂರ್ಣಾಙ್ಕ level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        ಯದಿ (!(vtxp->isTristate() || vtxp->feedsTri())) ಪ್ರತ್ಯಾಗಮ;  // tristate involved
        ಯದಿ (vtxp->user() == 3) ಪ್ರತ್ಯಾಗಮ;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        ಯದಿ (!vtxp->varp()) {  // not a var where we stop the recursion
            ಪ್ರತಿ (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ಧ್ರುವ vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ಯದಿ (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(ಸತ್ಯ);
                    graphWalkRecurseBack(vvertexp, level + 1);
                }
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ತಾರ್ಕಿಕ empty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_graph.empty(); }
    ಶೂನ್ಯ clearAndCheck() {
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            ಧ್ರುವ TristateVertex& vvertex = static_cast<TristateVertex&>(vtx);
            ಯದಿ (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_graph.clear();
        Ast_ಗಂಟು::user4_Clear_ವೃಕ್ಷ();  // Wipe all node user4p's that point to vertexes
    }
    ಶೂನ್ಯ graphWalk(Ast_ಗಂಟು_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseBack(static_cast<TristateVertex*>(&vtx), 0);
        }
        ಯದಿ (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    ಶೂನ್ಯ associate(Ast_ಗಂಟು* fromp, Ast_ಗಂಟು* ತುದಿ) {
        ನೂತನ V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(ತುದಿ), 1};
    }
    ಶೂನ್ಯ deleteVerticesFromSubtreeRecurse(Ast_ಗಂಟು* nodep) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
        // Skip vars, because they may be connected to more than one varref
        ಯದಿ (!VN_IS(nodep, Var)) {
            TristateVertex* ಧ್ರುವ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            ಯದಿ (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    ಶೂನ್ಯ setTristate(Ast_ಗಂಟು* nodep) { makeVertex(nodep)->isTristate(ಸತ್ಯ); }
    ತಾರ್ಕಿಕ isTristate(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ TristateVertex* ಧ್ರುವ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        ಪ್ರತ್ಯಾಗಮ vertexp && vertexp->isTristate();
    }
    ತಾರ್ಕಿಕ feedsTri(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ TristateVertex* ಧ್ರುವ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        ಪ್ರತ್ಯಾಗಮ vertexp && vertexp->feedsTri();
    }
    ಶೂನ್ಯ didProcess(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ quiet = ಅಸತ್ಯ) {
        ಯದಿ (TristateVertex* ಧ್ರುವ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(ಸತ್ಯ);
        } ಅನ್ಯಥಾ ಯದಿ (!quiet) {
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
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : m_graph.vertices()) {
            ಧ್ರುವ TristateVertex& vvertex = static_cast<ಧ್ರುವ TristateVertex&>(vtx);
            ಯದಿ (vvertex.isTristate()) {
                ಯದಿ (AstVar* ಧ್ರುವ nodep = VN_CAST(vvertex.nodep(), Var)) v.ತಳ್ಳು_back(nodep);
            }
        }
        ಪ್ರತ್ಯಾಗಮ v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

ವರ್ಗ TristatePinVisitor final : ಸಾರ್ವಜನಿಕ TristateBaseVisitor {
    TristateGraph& m_tgraph;
    ಧ್ರುವ ತಾರ್ಕಿಕ m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        ಯದಿ (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } ಅನ್ಯಥಾ ಯದಿ (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.setTristate(nodep->varp());
        }
    }
    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstSliceSel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_ಗಂಟು* nodep, TristateGraph& tgraph, ತಾರ್ಕಿಕ lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################

ವರ್ಗ TristateVisitor final : ಸಾರ್ವಜನಿಕ TristateBaseVisitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;
    ಧ್ರುವ VNUser3InUse m_inuser3;
    ಧ್ರುವ VNUser4InUse m_inuser4;

    ಸಂರಚನಾ AuxAstVar final {
        AstPull* pullp = nullptr;  // pullup/pulldown direction (whole variable)
        AstVar* outVarp = nullptr;  // output __out var
        ತಾರ್ಕಿಕ ifaceTristate = ಅಸತ್ಯ;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>
            bitPulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    ಸಂರಚನಾ RefStrength final {
        Ast_ಗಂಟು_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_ಗಂಟು_Assign* m_assignp;  // Assignment containing m_varrefp
        ಧ್ರುವ Ast_ಗಂಟು_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_ಗಂಟು_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_ಗಂಟು_Var_Ref* varrefp, Ast_ಗಂಟು_Assign* assignp,
                    ಧ್ರುವ Ast_ಗಂಟು_Procedure* procedurep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enLhsp,
                    Ast_ಗಂಟು_Var_Ref* enVarrefp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enRhsp, VStrength strength,
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
    ಸಂರಚನಾ LhsProjection final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_lhsp;  // LHS giving the enable assign
        Ast_ಗಂಟು_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    ಸಂರಚನಾ ProceduralEnable final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    using RefStrengthVec = std::vector<RefStrength>;
    using VarMap = std::map<AstVar*, RefStrengthVec*>;
    using Assigns = std::vector<AstAssignW*>;
    using VarToAssignsMap = std::map<AstVar*, Assigns>;
    using AssignEnableMap = std::map<Ast_ಗಂಟು_Assign*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>;
    using ProceduralEnableMap = std::map<Ast_ಗಂಟು_Assign*, std::vector<ProceduralEnable>>;
    ಗಣನಾ : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    ತಾರ್ಕಿಕ m_graphing = ಅಸತ್ಯ;  // Major mode - creating graph
    //
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    AstCell* m_cellp = nullptr;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    VarToAssignsMap m_assigns;  // Assigns in current module
    AssignEnableMap m_assignEnps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    ಪೂರ್ಣಾಙ್ಕ m_unique = 0;
    ತಾರ್ಕಿಕ m_alhs = ಅಸತ್ಯ;  // On LHS of assignment
    ತಾರ್ಕಿಕ m_inAlias = ಅಸತ್ಯ;  // Inside alias statement
    ತಾರ್ಕಿಕ m_processedIfaces = ಅಸತ್ಯ;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = nullptr;  // Delay of assignment currently collecting LHS drivers
    ಧ್ರುವ Ast_ಗಂಟು* m_logicp = nullptr;  // Current logic being built
    Ast_ಗಂಟು_Assign* m_assignp = nullptr;  // Current assignment
    ಧ್ರುವ Ast_ಗಂಟು_Procedure* m_procedurep = nullptr;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::vector<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ಧ್ರುವ {
        string o = (m_graphing ? " gr " : " ng ");
        ಯದಿ (m_alhs) o += "alhs ";
        ಪ್ರತ್ಯಾಗಮ o;
    }
    ಶೂನ್ಯ modAddStmtp(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        ಯದಿ (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ {
            m_modp->addStmtsp(newp);
        }
    }
    ಶೂನ್ಯ associateLogic(Ast_ಗಂಟು* fromp, Ast_ಗಂಟು* ತುದಿ) {
        ಯದಿ (m_logicp) m_tgraph.associate(fromp, ತುದಿ);
    }
    AstConst* newAllZerosOrOnes(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ ones) {
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, nodep->width()};
        ಯದಿ (ones) ಅಂಕೆ.setAllBits1();
        AstConst* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), ಅಂಕೆ};
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_ಸಾಲು* fl, AstVar* varp, ಧ್ರುವ string& dotted, VAccess access,
                           ಧ್ರುವ string& inlinedDots) {
        AstVarXRef* ಧ್ರುವ xrefp = ನೂತನ AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        ಪ್ರತ್ಯಾಗಮ xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_ಗಂಟು_Module* modp, ಧ್ರುವ string& dottedPath) {
        ಯದಿ (dottedPath.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಧ್ರುವ string firstComp = dottedPath.substr(0, dottedPath.find('.'));
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ಧ್ರುವ ivarp = VN_CAST(stmtp, Var);
            ಯದಿ (!ivarp || ivarp->name() != firstComp) ಅನುವರ್ತ;
            ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            ಯದಿ (ifaceDtp && ifaceDtp->modportp()) ಪ್ರತ್ಯಾಗಮ ifaceDtp->modportp();
            ಭಙ್ಗ;
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getEnp(Ast_ಗಂಟು* nodep) {
        ಯದಿ (nodep->user1p()) {
            ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ refp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
                ಯದಿ (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    ಪ್ರತ್ಯಾಗಮ newAllZerosOrOnes(nodep, ಸತ್ಯ);
                }
            }
        } ಅನ್ಯಥಾ {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, ಸತ್ಯ));
        }
        // Otherwise return the previous output enable
        ಪ್ರತ್ಯಾಗಮ VN_AS(nodep->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    ಶೂನ್ಯ addModportVarRefIfMissing(AstModport* modportp, AstVar* varp, VDirection dir) {
        ಪ್ರತಿ (Ast_ಗಂಟು* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ಧ್ರುವ mvrp = VN_CAST(mrp, ModportVarRef);
            ಯದಿ (mvrp && mvrp->varp() == varp) ಪ್ರತ್ಯಾಗಮ;  // Already present
        }
        AstModportVarRef* ಧ್ರುವ mvrp = ನೂತನ AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->addVarsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, ತಾರ್ಕಿಕ is_ತುದಿ) {
        // Return the master __en for the specified input variable
        ಯದಿ (!invarp->user1p()) {
            AstVar* ಧ್ರುವ newp
                = ನೂತನ AstVar{invarp->fileline(), is_ತುದಿ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp = findParentModule(invarp);
            ಯದಿ (ownerModp && ownerModp != m_modp) {
                ownerModp->addStmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ಧ್ರುವ modportp = VN_CAST(stmtp, Modport);
                    ಯದಿ (!modportp) ಅನುವರ್ತ;
                    ಪ್ರತಿ (Ast_ಗಂಟು* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ಧ್ರುವ mvrp = VN_CAST(mrp, ModportVarRef);
                        ಯದಿ (mvrp && mvrp->varp() == invarp) {
                            addModportVarRefIfMissing(modportp, newp, VDirection::ಒಳಹರಿವು);
                            ಭಙ್ಗ;
                        }
                    }
                }
            } ಅನ್ಯಥಾ {
                modAddStmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        ಪ್ರತ್ಯಾಗಮ VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ಧ್ರುವ constp) {
        File_ಸಾಲು* ಧ್ರುವ fl = constp->fileline();
        V3_ಸಂಖ್ಯೆ numz{constp, constp->width()};
        numz.opBitsZ(constp->ಅಂಕೆ());  // Z->1, else 0
        V3_ಸಂಖ್ಯೆ numz0{constp, constp->width()};
        numz0.opNot(numz);  // Z->0, else 1
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, numz0};
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* get_En_ಅಭಿವ್ಯಕ್ತಿ_Based_On_Originalp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep) {
        ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep, VarRef)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), ಅಸತ್ಯ),
                                 VAccess::READ};
        } ಅನ್ಯಥಾ ಯದಿ (AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ಧ್ರುವ enVarp = getCreateEnVarp(xrefp->varp(), ಅಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } ಅನ್ಯಥಾ ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
            ಪ್ರತ್ಯಾಗಮ getNonZConstp(constp);
        } ಅನ್ಯಥಾ ಯದಿ (AstExtend* ಧ್ರುವ extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = extendp->lhsp()->unlinkFrBack();
            AstExtend* ಧ್ರುವ enExtendp = extendp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            extendp->lhsp(lhsp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enLhsp = get_En_ಅಭಿವ್ಯಕ್ತಿ_Based_On_Originalp(lhsp);
            enExtendp->lhsp(ನೂತನ AstNot{enLhsp->fileline(), enLhsp});
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstNot{enExtendp->fileline(), enExtendp};
        } ಅನ್ಯಥಾ ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = selp->fromp()->unlinkFrBack();
            AstSel* ಧ್ರುವ enSelp = selp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            selp->fromp(fromp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enFromp = get_En_ಅಭಿವ್ಯಕ್ತಿ_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            ಪ್ರತ್ಯಾಗಮ enSelp;
        } ಅನ್ಯಥಾ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
    }
    AstVar* getCreateOutVarp(AstVar* invarp, ತಾರ್ಕಿಕ is_ತುದಿ) {
        // Return the master __out for the specified input variable
        ಯದಿ (!m_varAux(invarp).outVarp) {
            AstVar* ಧ್ರುವ newp
                = ನೂತನ AstVar{invarp->fileline(), is_ತುದಿ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            modAddStmtp(invarp, newp);
            m_varAux(invarp).outVarp = newp;  // find outvar given invarp
        }
        ಪ್ರತ್ಯಾಗಮ m_varAux(invarp).outVarp;
    }
    AstVar* getCreateUnconnVarp(Ast_ಗಂಟು* fromp, Ast_ಗಂಟು_DType* dtypep) {
        AstVar* ಧ್ರುವ newp = ನೂತನ AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvtToStr(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        modAddStmtp(newp, newp);
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    LhsProjection newLhsProjection(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_Var_Ref* targetp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newLhsp = lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_ಗಂಟು* nodep) { nodep->user1p(nullptr); });
        Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        ಪ್ರತ್ಯಾಗಮ LhsProjection{newLhsp, newVarrefp, enp};
    }

    ಶೂನ್ಯ map_ಸೇರಿಸು_Lhs_Var_Ref(Ast_ಗಂಟು_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ಧ್ರುವ ಕೀಲಿ = nodep->varp();
        ಧ್ರುವ auto pair = m_lhsmap.emplace(ಕೀಲಿ, nullptr);
        ಯದಿ (pair.second) pair.first->second = ನೂತನ RefStrengthVec;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enLhsp = nullptr;
        Ast_ಗಂಟು_Var_Ref* enVarrefp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enRhsp = nullptr;
        ಧ್ರುವ auto enIt = m_assignEnps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ಧ್ರುವ ತಾರ್ಕಿಕ canCombineProcedural = !VN_IS(ಕೀಲಿ->dtypep()->skipRefp(), ಗಂಟು_UOr_Struct_DType);
        ಧ್ರುವ Ast_ಗಂಟು_Procedure* ಧ್ರುವ procedurep
            = enIt != m_assignEnps.end() && canCombineProcedural ? m_procedurep : nullptr;
        ಯದಿ (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->ತಳ್ಳು_back(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newEnableDeposit(AstSel* selp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstShiftL{
            selp->fileline(), ನೂತನ AstExtend{selp->fileline(), enp, selp->fromp()->width()},
            selp->lsbp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), selp->fromp()->width()};
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    ಶೂನ್ಯ setPullDirection(AstVar* varp, AstPull* pullp) {
        ಧ್ರುವ AstPull* ಧ್ರುವ oldpullp = m_varAux(varp).pullp;
        ಯದಿ (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } ಅನ್ಯಥಾ {
            ಯದಿ (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    ಶೂನ್ಯ setBitPullDirection(AstVar* varp, ಪೂರ್ಣಾಙ್ಕ bitIndex, ಪೂರ್ಣಾಙ್ಕ direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        auto& bitPulls = m_varAux(varp).bitPulls;
        auto it = bitPulls.find(bitIndex);
        ಯದಿ (it != bitPulls.end() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bitIndex << " of " << varp->prettyNameQ());
        }
        bitPulls[bitIndex] = direction;
    }

    AstConst* createPerBitPullConst(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ಧ್ರುವ auto& bitPulls = m_varAux(varp).bitPulls;
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{varp, varp->width(), 0};  // Start with all zeros
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : bitPulls) {
            ಯದಿ (pair.second == 1) ಅಂಕೆ.setBit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{varp->fileline(), ಅಂಕೆ};
    }

    ತಾರ್ಕಿಕ hasPerBitPulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        ಪ್ರತ್ಯಾಗಮ !m_varAux(varp).bitPulls.empty();
    }

    ಶೂನ್ಯ checkUnhandled(Ast_ಗಂಟು* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        ಯದಿ (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        ಯದಿ ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Tristates(Ast_ಗಂಟು_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ಧ್ರುವ TristateGraph::VarVec vars = m_tgraph.tristateVars();
        ಪ್ರತಿ (auto varp : vars) {
            ಯದಿ (!m_tgraph.isTristate(varp)) ಅನುವರ್ತ;
            ಧ್ರುವ auto it = m_lhsmap.find(varp);
            ಯದಿ (it != m_lhsmap.end()) ಅನುವರ್ತ;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ಧ್ರುವ AstPull* ಧ್ರುವ pullp = m_varAux(varp).pullp;
            ಧ್ರುವ ತಾರ್ಕಿಕ pull_ಮೌಲ್ಯ = pullp && pullp->direction() == 1;
            AstConst* ಧ್ರುವ constp = newAllZerosOrOnes(varp, pull_ಮೌಲ್ಯ);
            AstVarRef* ಧ್ರುವ varrefp = ನೂತನ AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            AstAssignW* ಧ್ರುವ newp = ನೂತನ AstAssignW{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, ಅಸತ್ಯ));  // en=0 for floating/pullup vars
            nodep->addStmtsp(ನೂತನ AstAlways{newp});
            map_ಸೇರಿಸು_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        ಪ್ರತಿ (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ಧ್ರುವ std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            ಯದಿ (it == m_lhsmap.end()) ಅನುವರ್ತ;
            AstVar* ಧ್ರುವ invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            ಯದಿ (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp = findParentModule(invarp);
                ಧ್ರುವ ತಾರ್ಕಿಕ isCrossModule = ownerModp && ownerModp != nodep;
                ಧ್ರುವ ತಾರ್ಕಿಕ isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                ಯದಿ (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    ಪ್ರತಿ (ಧ್ರುವ RefStrength& rs : *refsp) {
                        ಯದಿ (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            ಭಙ್ಗ;
                        }
                    }
                } ಅನ್ಯಥಾ ಯದಿ (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = ಸತ್ಯ;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    ಸಂರಚನಾ PartitionInfo final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, PartitionInfo> partitions;
                    ಪ್ರತಿ (ಧ್ರುವ RefStrength& rs : *refsp) {
                        ಯದಿ (AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            PartitionInfo& pi = partitions[xrefp->dotted()];
                            pi.refs.ತಳ್ಳು_back(rs);
                            ಯದಿ (pi.inlinedDots.empty()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } ಅನ್ಯಥಾ {
                            partitions[""].refs.ತಳ್ಳು_back(rs);
                        }
                    }
                    ಪ್ರತಿ (auto& kv : partitions) {
                        ಸೇರಿಸು_Tristates_Signal(nodep, invarp, &kv.second.refs, ಸತ್ಯ, kv.first,
                                              kv.second.inlinedDots,
                                              findModportForDotted(nodep, kv.first));
                    }
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = ಸತ್ಯ;
                    ಸೇರಿಸು_Tristates_Signal(nodep, invarp, refsp, ಸತ್ಯ, "", "", nullptr);
                } ಅನ್ಯಥಾ {
                    ಸೇರಿಸು_Tristates_Signal(nodep, invarp, refsp, ಅಸತ್ಯ, "", "", nullptr);
                }
            } ಅನ್ಯಥಾ {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(delete refsp, refsp);
        }
    }

    ಶೂನ್ಯ aggregateTriSameStrength(Ast_ಗಂಟು_Module* nodep, AstVar* ಧ್ರುವ varp, AstVar* ಧ್ರುವ envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator endStrength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        ಸಂರಚನಾ DriverGroup final {
            ಧ್ರುವ Ast_ಗಂಟು_Procedure* m_procedurep;  // Process owning these assignments
            std::vector<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::vector<DriverGroup> drivers;
        std::map<ಧ್ರುವ Ast_ಗಂಟು_Procedure*, size_t> procedureToDriver;
        ಪ್ರತಿ (auto it = beginStrength; it != endStrength; ++it) {
            ಯದಿ (it->m_procedurep) {
                ಧ್ರುವ auto emplaced = procedureToDriver.emplace(it->m_procedurep, drivers.size());
                ಯದಿ (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.ತಳ್ಳು_back(&*it);
                    ಅನುವರ್ತ;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ತಳ್ಳು_back(DriverGroup{it->m_procedurep, {&*it}});
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* orp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp = nullptr;

        ಪ್ರತಿ (DriverGroup& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ಧ್ರುವ newLhsp = ನೂತನ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvtToStr(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->setDfgTriLowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->addStmtsp(newLhsp);
            AstVar* ಧ್ರುವ newEnLhsp
                = ನೂತನ AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvtToStr(m_unique++), envarp};  // 2-state ok
            newEnLhsp->setDfgTriLowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->addStmtsp(newEnLhsp);

            ಪ್ರತಿ (RefStrength* ಧ್ರುವ refStrengthp : driver.m_refsp) {
                Ast_ಗಂಟು_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                ಯದಿ (VN_IS(refp, VarXRef)) {
                    AstVarRef* ಧ್ರುವ localRefp
                        = ನೂತನ AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(nullptr);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } ಅನ್ಯಥಾ {
                    refp->varp(newLhsp);
                }

                ಯದಿ (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enLhsp = refStrengthp->m_enLhsp;
                    Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ enVarrefp = refStrengthp->m_enVarrefp;
                    ಯದಿ (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ಧ್ರುವ localRefp
                            = ನೂತನ AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        ಯದಿ (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_ವೃಕ್ಷ(), enVarrefp);
                        } ಅನ್ಯಥಾ {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(enVarrefp), enVarrefp);
                        }
                    } ಅನ್ಯಥಾ {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ತಳ್ಳು_back(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = nullptr;
                    refStrengthp->m_enVarrefp = nullptr;
                    refStrengthp->m_enRhsp = nullptr;
                } ಅನ್ಯಥಾ {
                    AstAssignW* ಧ್ರುವ enLhspAssignp = ನೂತನ AstAssignW{
                        refp->fileline(),
                        ನೂತನ AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    ಯದಿ (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->addStmtsp(ನೂತನ AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_ಸಾಲು* ಧ್ರುವ fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ref1p = ನೂತನ AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ref2p = ನೂತನ AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ andp = ನೂತನ AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : ನೂತನ AstOr{fl, orp, andp};

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ref3p = ನೂತನ AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : ನೂತನ AstOr{ref3p->fileline(), enp, ref3p};
        }
        AstAssignW* ಧ್ರುವ assp = ನೂತನ AstAssignW{
            varp->fileline(), ನೂತನ AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->addStmtsp(ನೂತನ AstAlways{assp});

        AstAssignW* ಧ್ರುವ enAssp = ನೂತನ AstAssignW{
            envarp->fileline(), ನೂತನ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->addStmtsp(ನೂತನ AstAlways{enAssp});
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Procedural_Enable_Assignments(Ast_ಗಂಟು_Module* nodep) {
        ಪ್ರತಿ (auto& assignEnables : m_proceduralEnables) {
            Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = assignEnables.first;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* combinedLhsp = assignp->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* combinedRhsp = assignp->rhsp()->unlinkFrBack();
            ಪ್ರತಿ (ProceduralEnable& enable : assignEnables.second) {
                combinedLhsp = ನೂತನ AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = ನೂತನ AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            ಯದಿ (!VN_IS(assignp, AssignDly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ಧ್ರುವ sampledp
                    = ನೂತನ AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvtToStr(m_unique++), combinedRhsp->dtypep()};
                nodep->addStmtsp(sampledp);
                assignp->lhsp(ನೂತನ AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                AstAssign* ಧ್ರುವ applyp
                    = ನೂತನ AstAssign{assignp->fileline(), combinedLhsp,
                                    ನೂತನ AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->addNextHere(applyp);
            } ಅನ್ಯಥಾ {
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
    ಶೂನ್ಯ ಸೇರಿಸು_Tristates_Signal(Ast_ಗಂಟು_Module* nodep, AstVar* ಧ್ರುವ invarp, RefStrengthVec* refsp,
                               ತಾರ್ಕಿಕ isIfaceTri, ಧ್ರುವ string& ifaceDottedPath,
                               ಧ್ರುವ string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        ಧ್ರುವ ತಾರ್ಕಿಕ is_ತುದಿ_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_ತುದಿ();
        ಯದಿ (!isIfaceTri) {
            ಯದಿ (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty())
                 && is_ತುದಿ_Inout)
                || (!nodep->is_ತುದಿ() && invarp->isIO())) {
                // This var becomes an input
                invarp->varType2In();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = getCreateOutVarp(invarp, is_ತುದಿ_Inout);
                outvarp->varType2Out();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_ತುದಿ_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                ಯದಿ (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                ಯದಿ (!m_varAux(invarp).bitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } ಅನ್ಯಥಾ ಯದಿ (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* orp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp = nullptr;

        std::sort(refsp->begin(), refsp->end(),
                  [](RefStrength a, RefStrength b) { ಪ್ರತ್ಯಾಗಮ a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        ಯಾವತ್ (beginStrength != refsp->end()) {
            RefStrengthVec::iterator endStrength = beginStrength + 1;
            ಯಾವತ್ (endStrength != refsp->end()
                   && endStrength->m_strength == beginStrength->m_strength)
                endStrength++;

            File_ಸಾಲು* ಧ್ರುವ fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strengthPrefix;
            ಯದಿ (isIfaceTri && !ifaceDottedPath.empty()) {
                strengthPrefix = ifaceDottedPath;
                std::replace(strengthPrefix.begin(), strengthPrefix.end(), '.', '_');
                strengthPrefix += "__";
            }
            ಧ್ರುವ string strengthVarName
                = strengthPrefix + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ಧ್ರುವ varStrengthp = ನೂತನ AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->setDfgTriLowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->addStmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ಧ್ರುವ enVarStrengthp = ನೂತನ AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->setDfgTriLowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->addStmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     endStrength);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp;
            ಯದಿ (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp
                    = ನೂತನ AstAnd{fl, ನೂತನ AstVarRef{fl, varStrengthp, VAccess::READ},
                                 ನೂತನ AstNot{fl, enp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}};
            } ಅನ್ಯಥಾ {
                ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp = ನೂತನ AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp : ನೂತನ AstOr{fl, orp, ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp};

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enVarStrengthRefp
                = ನೂತನ AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : ನೂತನ AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = endStrength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        ಯದಿ (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->setDfgAllowMultidriveTri();
            Ast_ಗಂಟು_Module* ಧ್ರುವ ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ contribIdx = static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_ifaceContribs[invarp].size());
            File_ಸಾಲು* ಧ್ರುವ fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ಧ್ರುವ contribOutp = ನೂತನ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvtToStr(contribIdx), invarp};
            contribOutp->setDfgTriLowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->addStmtsp(contribOutp);

            AstVar* ಧ್ರುವ contribEnp = ನೂತನ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvtToStr(contribIdx), invarp};
            contribEnp->setDfgTriLowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->addStmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            ಯದಿ (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                addModportVarRefIfMissing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                addModportVarRefIfMissing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_ಗಂಟು_Var_Ref*>(
                                                    ನೂತನ AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_ಗಂಟು_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ಧ್ರುವ assp = ನೂತನ AstAssignW{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(ನೂತನ AstAlways{assp});
            }
            {
                Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_ಗಂಟು_Var_Ref*>(
                                                    ನೂತನ AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_ಗಂಟು_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ಧ್ರುವ assp = ನೂತನ AstAssignW{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(ನೂತನ AstAlways{assp});
            }

            m_ifaceContribs[invarp].ತಳ್ಳು_back({contribEnp, contribOutp});
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ಧ್ರುವ ತಾರ್ಕಿಕ hasPull = m_varAux(lhsp).pullp || hasPerBitPulls(lhsp);
        ಯದಿ (!outvarp || hasPull) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* undrivenp;
            ಯದಿ (envarp) {
                undrivenp = ನೂತನ AstNot{envarp->fileline(),
                                       ನೂತನ AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } ಅನ್ಯಥಾ ಯದಿ (enp) {
                undrivenp = ನೂತನ AstNot{enp->fileline(), enp};
                enp = nullptr;  // moved into undrivenp
            } ಅನ್ಯಥಾ {
                undrivenp = newAllZerosOrOnes(invarp, ಸತ್ಯ);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            ಯದಿ (hasPerBitPulls(lhsp)) {
                pullConstp = createPerBitPullConst(lhsp);
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ AstPull* ಧ್ರುವ pullp = m_varAux(lhsp).pullp;
                ಧ್ರುವ ತಾರ್ಕಿಕ pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = ನೂತನ AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? ನೂತನ AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        ಯದಿ (envarp && !enp) enp = newAllZerosOrOnes(invarp, ಅಸತ್ಯ);

        ಯದಿ (envarp) {
            AstAssignW* ಧ್ರುವ enAssp = ನೂತನ AstAssignW{
                enp->fileline(), ನೂತನ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->addStmtsp(ನೂತನ AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        AstAssignW* ಧ್ರುವ assp = ನೂತನ AstAssignW{
            lhsp->fileline(), ನೂತನ AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->addStmtsp(ನೂತನ AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        ಯದಿ ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty()) && is_ತುದಿ_Inout) {
            ಯದಿ (envarp) {
                envarp->primaryIO(ಸತ್ಯ);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            ಯದಿ (outvarp) {
                outvarp->primaryIO(ಸತ್ಯ);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            ಯದಿ (!v3Global.opt.libCreate().empty()) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(ಸತ್ಯ);
            }
        }
    }

    ತಾರ್ಕಿಕ isOnlyAssignmentIsToLhsVar(ಧ್ರುವ AstAssignW* ಧ್ರುವ nodep) {
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            auto foundIt = m_assigns.find(varRefp->varp());
            ಯದಿ (foundIt != m_assigns.end()) {
                auto ಧ್ರುವ& assignsToVar = foundIt->second;
                ಯದಿ (assignsToVar.size() == 1 && assignsToVar[0] == nodep) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ addToAssignmentList(AstAssignW* nodep) {
        ಯದಿ (AstVarRef* ಧ್ರುವ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            ಯದಿ (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ತಳ್ಳು_back(nodep);
            } ಅನ್ಯಥಾ ಯದಿ (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ಧ್ರುವ AstAssignW* ಧ್ರುವ nodep, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
        ಯದಿ (AstStrengthSpec* ಧ್ರುವ strengthSpec = nodep->strengthSpecp()) {
            ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        ಪ್ರತ್ಯಾಗಮ VStrength::STRONG;  // default strength is strong
    }

    ತಾರ್ಕಿಕ assignment_Of_ಮೌಲ್ಯ_On_All_Bits(ಧ್ರುವ AstAssignW* ಧ್ರುವ nodep, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->rhsp(), Const)) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ = constp->ಅಂಕೆ();
            ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ ? ಅಂಕೆ.isEqAllOnes() : ಅಂಕೆ.isEqZero();
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    AstAssignW* get_Strongest_Assignment_Of_ಮೌಲ್ಯ(ಧ್ರುವ Assigns& assigns, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
        auto maxIt = std::max_element(
            assigns.begin(), assigns.end(), [&](ಧ್ರುವ AstAssignW* ap, ಧ್ರುವ AstAssignW* bp) {
                ಧ್ರುವ ತಾರ್ಕಿಕ ಮೌಲ್ಯಗಳು_On_Rhs_A = assignment_Of_ಮೌಲ್ಯ_On_All_Bits(ap, ಮೌಲ್ಯ);
                ಧ್ರುವ ತಾರ್ಕಿಕ ಮೌಲ್ಯಗಳು_On_Rhs_B = assignment_Of_ಮೌಲ್ಯ_On_All_Bits(bp, ಮೌಲ್ಯ);
                ಯದಿ (!ಮೌಲ್ಯಗಳು_On_Rhs_A) ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು_On_Rhs_B;
                ಯದಿ (!ಮೌಲ್ಯಗಳು_On_Rhs_B) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ getStrength(ap, ಮೌಲ್ಯ) < getStrength(bp, ಮೌಲ್ಯ);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        ಪ್ರತ್ಯಾಗಮ assignment_Of_ಮೌಲ್ಯ_On_All_Bits(*maxIt, ಮೌಲ್ಯ) ? *maxIt : nullptr;
    }

    ತಾರ್ಕಿಕ isAssignmentNotStrongerThanStrength(ಧ್ರುವ AstAssignW* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ಧ್ರುವ uint8_t strength0 = getStrength(assignp, 0);
        ಧ್ರುವ uint8_t strength1 = getStrength(assignp, 1);
        ಪ್ರತ್ಯಾಗಮ (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_Of_ಮೌಲ್ಯ_On_All_Bits(assignp, 0))
               || (strength1 <= strength && assignment_Of_ಮೌಲ್ಯ_On_All_Bits(assignp, 1));
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Merge_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp, File_ಸಾಲು* ಧ್ರುವ fl,
                              ತಾರ್ಕಿಕ isWor) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಅಭಿವ್ಯಕ್ತಿ = nullptr;
        ಯದಿ (isWor)
            ಅಭಿವ್ಯಕ್ತಿ = ನೂತನ AstOr{fl, lhsp, rhsp};
        ಅನ್ಯಥಾ
            ಅಭಿವ್ಯಕ್ತಿ = ನೂತನ AstAnd{fl, lhsp, rhsp};
        ಪ್ರತ್ಯಾಗಮ ಅಭಿವ್ಯಕ್ತಿ;
    }

    ಶೂನ್ಯ mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        ಪ್ರತಿ (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            ಯದಿ (assigns.size() <= 1) ಅನುವರ್ತ;
            AstVar* ಧ್ರುವ varp = varpAssigns.first;
            ಯದಿ (!varp->isWiredNet()) ಅನುವರ್ತ;
            auto it = assigns.begin();
            AstAssignW* ಧ್ರುವ assignWp0 = *it;
            File_ಸಾಲು* ಧ್ರುವ fl = assignWp0->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wExp = nullptr;
            ಯಾವತ್ (++it != assigns.end()) {
                AstAssignW* assignWpi = *it;
                ಯದಿ (!wExp) {
                    wExp
                        = new_Merge_ಅಭಿವ್ಯಕ್ತಿ(assignWp0->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                       assignWpi->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), fl, varp->isWor());
                } ಅನ್ಯಥಾ {
                    wExp = new_Merge_ಅಭಿವ್ಯಕ್ತಿ(wExp, assignWpi->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((assignWpi->unlinkFrBack()->delete_ವೃಕ್ಷ()), assignWpi);
            }
            AstVarRef* ಧ್ರುವ wVarRef = ನೂತನ AstVarRef{fl, varp, VAccess::WRITE};
            AstAssignW* ಧ್ರುವ wAssignp = ನೂತನ AstAssignW{fl, wVarRef, wExp};
            assignWp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(assignWp0), assignWp0);
            assigns.clear();
            assigns.ತಳ್ಳು_back(wAssignp);
        }
    }

    ಶೂನ್ಯ removeNotStrongerAssignments(Assigns& assigns, ಧ್ರುವ AstAssignW* strongestp,
                                      uint8_t greatestKnownStrength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        auto removedIt = std::remove_if(assigns.begin(), assigns.end(), [&](AstAssignW* assignp) {
            ಯದಿ (assignp == strongestp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (isAssignmentNotStrongerThanStrength(assignp, greatestKnownStrength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(assignp->unlinkFrBack()), assignp);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        assigns.erase(removedIt, assigns.end());
    }

    ಶೂನ್ಯ removeAssignmentsNotStrongerThanUniformConstant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        ಪ್ರತಿ (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            ಯದಿ (assigns.size() <= 1) ಅನುವರ್ತ;
            ಧ್ರುವ AstAssignW* ಧ್ರುವ strongest0p = get_Strongest_Assignment_Of_ಮೌಲ್ಯ(assigns, 0);
            ಧ್ರುವ AstAssignW* ಧ್ರುವ strongest1p = get_Strongest_Assignment_Of_ಮೌಲ್ಯ(assigns, 1);
            ಧ್ರುವ AstAssignW* strongestp = nullptr;
            uint8_t greatestKnownStrength = 0;
            ಧ್ರುವ auto getIfStrongest
                = [&](ಧ್ರುವ AstAssignW* ಧ್ರುವ strongestCandidatep, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
                      ಯದಿ (!strongestCandidatep) ಪ್ರತ್ಯಾಗಮ;
                      ಧ್ರುವ uint8_t strength = getStrength(strongestCandidatep, ಮೌಲ್ಯ);
                      ಯದಿ (strength >= greatestKnownStrength) {
                          greatestKnownStrength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            ಯದಿ (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    ಶೂನ್ಯ removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        ಪ್ರತಿ (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            ಯದಿ (assigns.size() <= 1) ಅನುವರ್ತ;
            auto maxIt = std::max_element(
                assigns.begin(), assigns.end(), [&](ಧ್ರುವ AstAssignW* ap, ಧ್ರುವ AstAssignW* bp) {
                    ಯದಿ (m_tgraph.isTristate(ap)) ಪ್ರತ್ಯಾಗಮ !m_tgraph.isTristate(bp);
                    ಯದಿ (m_tgraph.isTristate(bp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                    ಧ್ರುವ uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ಧ್ರುವ uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    ಪ್ರತ್ಯಾಗಮ minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ಧ್ರುವ AstAssignW* ಧ್ರುವ strongestp = m_tgraph.isTristate(*maxIt) ? nullptr : *maxIt;
            ಯದಿ (strongestp) {
                ಧ್ರುವ uint8_t greatestKnownStrength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        ಯದಿ (m_graphing) {
            ಯದಿ (!m_alhs && nodep->ಅಂಕೆ().hasZ()) m_tgraph.setTristate(nodep);
        } ಅನ್ಯಥಾ {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            ಯದಿ (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ಧ್ರುವ varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                AstConst* ಧ್ರುವ enp = getNonZConstp(nodep);
                V3_ಸಂಖ್ಯೆ num1{nodep, nodep->width()};
                num1.opAnd(nodep->ಅಂಕೆ(), enp->ಅಂಕೆ());  // 01X->01X, Z->0
                AstConst* ಧ್ರುವ newconstp = ನೂತನ AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    ಶೂನ್ಯ visit(AstCond* nodep) override {
        ಯದಿ (m_graphing) {
            iterateChildren(nodep);
            ಯದಿ (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } ಅನ್ಯಥಾ {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ಪ್ರತ್ಯಾಗಮ;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = nodep->condp();
            ಯದಿ (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thenp = nodep->thenp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elsep = nodep->elsep();
            ಯದಿ (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ en1p = getEnp(thenp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enp
                    = ನೂತನ AstCond{nodep->fileline(), condp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                ಯದಿ (thenp->user1p() && !thenp->user1p()->backp()) ತಳ್ಳು_Deletep(thenp->user1p());
                ಯದಿ (elsep->user1p() && !elsep->user1p()->backp()) ತಳ್ಳು_Deletep(elsep->user1p());
                thenp->user1p(nullptr);
                elsep->user1p(nullptr);
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(nullptr);
            m_tgraph.didProcess(nodep);
        }
    }

    ಶೂನ್ಯ visit(AstSel* nodep) override {
        ಯದಿ (m_graphing) {
            iterateChildren(nodep);
            ಯದಿ (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } ಅನ್ಯಥಾ {
                associateLogic(nodep->fromp(), nodep);
            }
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ಯದಿ (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ಅನ್ಯಥಾ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                ಯದಿ (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                ಯದಿ (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ en1p = getEnp(nodep->fromp());
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enp
                        = ನೂತನ AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                     nodep->widthConst()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    ಶೂನ್ಯ visit(AstConcat* nodep) override {
        ಯದಿ (m_graphing) {
            iterateChildren(nodep);
            ಯದಿ (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } ಅನ್ಯಥಾ {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ಯದಿ (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enp = VN_AS(nodep->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    nodep->user1p(nullptr);
                    nodep->lhsp()->user1p(ನೂತನ AstSel{nodep->fileline(), enp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                                     nodep->rhsp()->width(),
                                                     nodep->lhsp()->width()});
                    nodep->rhsp()->user1p(
                        ನೂತನ AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->width()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ಅನ್ಯಥಾ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p = nodep->lhsp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr2p = nodep->rhsp();
                ಯದಿ (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ en1p = getEnp(expr1p);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ en2p = getEnp(expr2p);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enp = ನೂತನ AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(nullptr);
                    expr2p->user1p(nullptr);
                }
            }
        }
    }

    ಶೂನ್ಯ visit(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ಯದಿ (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.setTristate(nodep);
        } ಅನ್ಯಥಾ {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            ಯದಿ (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ಪ್ರತ್ಯಾಗಮ;
            }
            m_tgraph.didProcess(nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr2p = nodep->rhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp;
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ en2p = VN_AS(expr2p->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                enp = ನೂತನ AstAnd{nodep->fileline(), expr1p, en2p};
            } ಅನ್ಯಥಾ {
                enp = expr1p;
            }
            expr1p->user1p(nullptr);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ visitAndOr(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ಯದಿ (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ಪ್ರತ್ಯಾಗಮ;
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
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p = nodep->lhsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr2p = nodep->rhsp();
            ಯದಿ (!expr1p->user1p() && !expr2p->user1p()) {
                ಪ್ರತ್ಯಾಗಮ;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* en1p = getEnp(expr1p);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* en2p = getEnp(expr2p);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subexpr1p = expr1p->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subexpr2p = expr2p->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            ಯದಿ (isAnd) {
                subexpr1p = ನೂತನ AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = ನೂತನ AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            ಯದಿ (!en1p->op1p() && en2p->op1p()) {
                std::swap(en1p, en2p);
                std::swap(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enp = ನೂತನ AstOr{
                nodep->fileline(),
                ನೂತನ AstAnd{nodep->fileline(), en1p, ನೂತನ AstOr{nodep->fileline(), en2p, subexpr1p}},
                ನೂತನ AstAnd{nodep->fileline(), en2p->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(nullptr);
            expr2p->user1p(nullptr);
        }
    }
    ಶೂನ್ಯ visit(AstAnd* nodep) override { visitAndOr(nodep, ಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstOr* nodep) override { visitAndOr(nodep, ಅಸತ್ಯ); }

    ಶೂನ್ಯ visitAssign(Ast_ಗಂಟು_Assign* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        ಯದಿ (VN_IS(nodep->rhsp(), CReset)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (AstAssignW* ಧ್ರುವ assignWp = VN_CAST(nodep, AssignW)) {
            m_currentDelayp = VN_CAST(assignWp->timingControlp(), Delay);
        }
        ಯದಿ (m_graphing) {
            ಯದಿ (AstAssignW* assignWp = VN_CAST(nodep, AssignW)) {
                ಯದಿ (assignWp->getLhsNetDelay()) ಪ್ರತ್ಯಾಗಮ;
                ಯದಿ (assignWp->timingControlp() && !m_currentDelayp) ಪ್ರತ್ಯಾಗಮ;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                ಯದಿ (m_currentDelayp && m_currentDelayp->fallDelay()) ಪ್ರತ್ಯಾಗಮ;
                ಯದಿ (!assignWp->timingControlp()) addToAssignmentList(assignWp);
            }

            ಯದಿ (nodep->user2() & U2_GRAPHING) ಪ್ರತ್ಯಾಗಮ;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterateAndNextNull(nodep->rhsp());
            m_alhs = ಸತ್ಯ;
            iterateAndNextNull(nodep->lhsp());
            m_alhs = ಅಸತ್ಯ;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->user2() & U2_NONGRAPH) {
                ಪ್ರತ್ಯಾಗಮ;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterateAndNextNull(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            ಯದಿ (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, AssignW), nodep,
                            "Continuous assignment inside procedural process");
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)
                          : newAllZerosOrOnes(nodep->lhsp(), ಸತ್ಯ);
                ಧ್ರುವ ತಾರ್ಕಿಕ inserted = m_assignEnps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            ಯದಿ (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(nullptr);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } ಅನ್ಯಥಾ {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(nodep->lhsp(), Sel)) {
                    ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(selp->fromp(), ಗಂಟು_Var_Ref)) {
                        ಯದಿ (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), ಸತ್ಯ));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } ಅನ್ಯಥಾ {
                            m_tgraph.didProcess(nodep, ಸತ್ಯ);
                        }
                    } ಅನ್ಯಥಾ {
                        m_tgraph.didProcess(nodep, ಸತ್ಯ);  // LCOV_EXCL_LINE
                    }
                } ಅನ್ಯಥಾ {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, ಸತ್ಯ);
                }
            }
            m_alhs = ಸತ್ಯ;  // And user1p() will indicate tristate equation, if any
            ಯದಿ (AstAssignW* ಧ್ರುವ assignWp = VN_CAST(nodep, AssignW)) {
                ಯದಿ (AstStrengthSpec* ಧ್ರುವ specp = assignWp->strengthSpecp()) {
                    ಯದಿ (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        ಯದಿ (!isOnlyAssignmentIsToLhsVar(assignWp)) {
                            assignWp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } ಅನ್ಯಥಾ {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterateAndNextNull(nodep->lhsp());
        }
    }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override { visitAssign(nodep); }
    ಶೂನ್ಯ visit(AstAssign* nodep) override { visitAssign(nodep); }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override { visitAssign(nodep); }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ಧ್ರುವ AstAlways* ಧ್ರುವ alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN ? nullptr : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    ಶೂನ್ಯ visit(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_inAlias);
        m_inAlias = ಸತ್ಯ;
        ಯದಿ (m_graphing) {
            ಯದಿ (nodep->user2() & U2_GRAPHING) ಪ್ರತ್ಯಾಗಮ;
            m_alhs = ಸತ್ಯ;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }

    ಶೂನ್ಯ visitCaseEq(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ neq) {
        ಯದಿ (m_graphing) {
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ಧ್ರುವ constp = VN_CAST(nodep->lhsp(), Const);
            ಯದಿ (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enRhsp;
                ಯದಿ (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    rhsp->user1p(nullptr);
                } ಅನ್ಯಥಾ {
                    enRhsp = get_En_ಅಭಿವ್ಯಕ್ತಿ_Based_On_Originalp(rhsp);
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp
                    = ನೂತನ AstLogAnd{fl, ನೂತನ AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    ನೂತನ AstEqCase{fl, ನೂತನ AstConst{fl, constp->ಅಂಕೆ()}, rhsp}};
                constp->user1p(nullptr);
                ಯದಿ (neq) newp = ನೂತನ AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (constp && nodep->rhsp()->user1p()) {
                File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                constp->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ನೂತನ AstLogAnd{fl,
                                                  ನೂತನ AstEq{fl, newAllZerosOrOnes(constp, ಅಸತ್ಯ),
                                                            VN_AS(rhsp->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)},
                                                  // Keep the caseeq if there are X's present
                                                  ನೂತನ AstEqCase{fl, constp, rhsp}};
                ಯದಿ (neq) newp = ನೂತನ AstLogNot{fl, newp};
                rhsp->user1p(nullptr);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ {
                checkUnhandled(nodep);
            }
        }
    }
    ಶೂನ್ಯ visitEqNeqWild(Ast_ಗಂಟು_Biop* nodep) {
        ಯದಿ (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterateAndNextNull(nodep->lhsp());
        ಯದಿ (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            ಪ್ರತ್ಯಾಗಮ;
        }
    }
    ಶೂನ್ಯ visit(AstEqCase* nodep) override { visitCaseEq(nodep, ಅಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstNeqCase* nodep) override { visitCaseEq(nodep, ಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    ಶೂನ್ಯ visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }

    ಶೂನ್ಯ visit(Ast_ಎಣಿಕೆ_Bits* nodep) override {
        std::array<ತಾರ್ಕಿಕ, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->ಅಂಕೆ().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->ಅಂಕೆ().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        ಯದಿ (m_graphing) {
            iterateAndNextNull(nodep->lhsp());
            ಯದಿ (!dropop[0]) iterateAndNextNull(nodep->rhsp());
            ಯದಿ (!dropop[1]) iterateAndNextNull(nodep->thsp());
            ಯದಿ (!dropop[2]) iterateAndNextNull(nodep->fhsp());
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nonXp = nullptr;
            ಯದಿ (!dropop[0]) {
                nonXp = nodep->rhsp();
            } ಅನ್ಯಥಾ ಯದಿ (!dropop[1]) {
                nonXp = nodep->thsp();
            } ಅನ್ಯಥಾ ಯದಿ (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            ಯದಿ (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                ಯದಿ (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    ಪ್ರತ್ಯಾಗಮ;
                }
                AstVar* ಧ್ರುವ envarp = getCreateEnVarp(varrefp->varp(), ಅಸತ್ಯ);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ನೂತನ Ast_ಎಣಿಕೆ_Ones{
                    nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                ಯದಿ (nonXp) {  // Need to still count '0 or '1 or 'x's
                    ಯದಿ (dropop[0]) {
                        nodep->rhsp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                        nodep->rhsp(nonXp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
                    }
                    ಯದಿ (dropop[1]) {
                        nodep->thsp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                        nodep->thsp(nonXp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
                    }
                    ಯದಿ (dropop[2]) {
                        nodep->fhsp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                        nodep->fhsp(nonXp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
                    }
                    newp = ನೂತನ AstAdd{nodep->fileline(), nodep, newp};
                } ಅನ್ಯಥಾ {
                    ತಳ್ಳು_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = nullptr;
        ಯದಿ (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        ಯದಿ (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.setTristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } ಅನ್ಯಥಾ {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(nodep->lhsp(), Sel);
                ಧ್ರುವ AstConst* ಧ್ರುವ lsbp = selp ? VN_CAST(selp->lsbp(), Const) : nullptr;
                ಯದಿ (lsbp) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = lsbp->toSInt();
                    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < selp->widthConst(); ++i) {
                        setBitPullDirection(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } ಅನ್ಯಥಾ {
                    setPullDirection(varrefp->varp(), nodep);
                }
            }
        }
        ಯದಿ (!m_graphing) {
            nodep->unlinkFrBack();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ iteratePinGuts(AstPin* nodep) {
        ಯದಿ (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            ಯದಿ (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ {
            // All heavy lifting completed in graph visitor.
            ಯದಿ (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    ಶೂನ್ಯ visit(AstPin* nodep) override {
        ಯದಿ (m_graphing) {
            ಯದಿ (nodep->user2() & U2_GRAPHING) ಪ್ರತ್ಯಾಗಮ;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ಧ್ರುವ enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            ಯದಿ (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, ಅಸತ್ಯ);
                iteratePinGuts(nodep);
                ಪ್ರತ್ಯಾಗಮ;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ಧ್ರುವ ತಾರ್ಕಿಕ inDeclProcessing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::ಒಳಹರಿವು
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            ಯದಿ (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ಧ್ರುವ ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(ನೂತನ AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.setTristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } ಅನ್ಯಥಾ ಯದಿ (inDeclProcessing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ಧ್ರುವ AstAssignW* ಧ್ರುವ reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, ಸತ್ಯ, ಸತ್ಯ);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.setTristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ಧ್ರುವ enVarp = ನೂತನ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvtToStr(m_unique++),
                                              VFlagBitPacked{}, enModVarp->width()};
            ಯದಿ (inDeclProcessing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->varType2In();
            } ಅನ್ಯಥಾ {
                enModVarp->varType2Out();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ಧ್ರುವ enpinp
                = ನೂತನ AstPin{nodep->fileline(), nodep->pin_ಅಂಕೆ(),
                             enModVarp->name(),  // should be {var}"__en"
                             ನೂತನ AstVarRef{nodep->fileline(), enVarp,
                                           inDeclProcessing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->addNextHere(enpinp);
            m_modp->addStmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ಧ್ರುವ AstAssignW* outAssignp = nullptr;  // If reconnected, the related assignment
            AstPin* outpinp = nullptr;
            AstVar* ಧ್ರುವ outModVarp = m_varAux(nodep->modVarp()).outVarp;
            ಯದಿ (!outModVarp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                ಯದಿ (!m_modp->is_ತುದಿ()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } ಅನ್ಯಥಾ {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ outexprp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)
                                                  ->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);  // Note has lvalue() set
                outpinp = ನೂತನ AstPin{nodep->fileline(), nodep->pin_ಅಂಕೆ(),
                                     outModVarp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(outModVarp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->addNextHere(outpinp);
                // Simplify
                ಯದಿ (inDeclProcessing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, ಸತ್ಯ};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                outAssignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        ಸತ್ಯ);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, outAssignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ಧ್ರುವ TristatePinVisitor visitor{nodep->exprp(), m_tgraph, ಅಸತ್ಯ};
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ inAssignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, ಸತ್ಯ);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, inAssignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            ಯದಿ (!outAssignp) {
                ಯದಿ (!outpinp) {
                    exprrefp = nullptr;  // Primary input only
                } ಅನ್ಯಥಾ {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    ಯದಿ (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } ಅನ್ಯಥಾ {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(outAssignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                ಯದಿ (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            ಯದಿ (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(ನೂತನ AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                ಯದಿ (!outAssignp) {
                    map_ಸೇರಿಸು_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            ಯದಿ (exprrefp) {
                AstPull* ಧ್ರುವ pullp = m_varAux(nodep->modVarp()).pullp;
                ಧ್ರುವ auto& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = nullptr;
                ಪೂರ್ಣಾಙ್ಕ selLsb = 0;
                ಪೂರ್ಣಾಙ್ಕ selWidth = 0;
                ಯದಿ (outAssignp) {
                    ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(outAssignp->lhsp(), Sel)) {
                        AstVarRef* ಧ್ರುವ vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ಧ್ರುವ lsbp = VN_CAST(selp->lsbp(), Const);
                        ಯದಿ (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->toSInt();
                            selWidth = selp->widthConst();
                        }
                    }
                }

                ಯದಿ (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    setPullDirection(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    ಯದಿ (selVarp) {
                        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ direction = pullp->direction();
                        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < selWidth; ++i) {
                            setBitPullDirection(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                ಯದಿ (!srcBitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    ಪ್ರತಿ (ಧ್ರುವ auto& pair : srcBitPulls) {
                        setBitPullDirection(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    ಯದಿ (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        ಪ್ರತಿ (ಧ್ರುವ auto& pair : srcBitPulls) {
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
        ಅನ್ಯಥಾ {
            ಯದಿ (nodep->user2() & U2_NONGRAPH) ಪ್ರತ್ಯಾಗಮ;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    ಶೂನ್ಯ handle_ಗಂಟು_Var_Ref(Ast_ಗಂಟು_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        ಯದಿ (m_graphing) {
            ಯದಿ (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            ಯದಿ (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            ಯದಿ (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.setTristate(nodep->varp());
            }
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->user2() & U2_NONGRAPH) ಪ್ರತ್ಯಾಗಮ;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            ಯದಿ (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                ಯದಿ (m_inAlias) {
                    ಯದಿ (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } ಅನ್ಯಥಾ {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    ಪ್ರತ್ಯಾಗಮ;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_ಸೇರಿಸು_Lhs_Var_Ref(nodep);
            } ಅನ್ಯಥಾ ಯದಿ (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                AstVar* ಧ್ರುವ enVarp = getCreateEnVarp(nodep->varp(), ಅಸತ್ಯ);
                ಯದಿ (AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } ಅನ್ಯಥಾ {
                    nodep->user1p(ನೂತನ AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    ಶೂನ್ಯ visit(AstVarRef* nodep) override { handle_ಗಂಟು_Var_Ref(nodep); }
    ಶೂನ್ಯ visit(AstVarXRef* nodep) override { handle_ಗಂಟು_Var_Ref(nodep); }

    ಶೂನ್ಯ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ಯದಿ (m_graphing) {
            // If tri0/1 force a pullup
            ಯದಿ (nodep->user2() & U2_GRAPHING) ಪ್ರತ್ಯಾಗಮ;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            ಯದಿ (nodep->isPulldown() || nodep->isPullup()) {
                Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstPull{
                    nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->addNextHere(newp);
                // We'll iterate on the new AstPull later
            }
            ಯದಿ (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.setTristate(nodep);
            }
        } ಅನ್ಯಥಾ {  // !graphing
            ಯದಿ (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        ಯದಿ (m_processedIfaces && VN_IS(nodep, Iface)) ಪ್ರತ್ಯಾಗಮ;
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
        m_graphing = ಅಸತ್ಯ;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = nullptr;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = ಸತ್ಯ;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = ಅಸತ್ಯ;
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
        ಸೇರಿಸು_Tristates(nodep);
        ಸೇರಿಸು_Procedural_Enable_Assignments(nodep);

        ಪ್ರತಿ (auto& assignEnp : m_assignEnps) {
            VL_DO_DANGLING(assignEnp.second->delete_ವೃಕ್ಷ(), assignEnp.second);
        }
        m_assignEnps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    ಶೂನ್ಯ visit(AstClass* nodep) override {
        // don't deal with classes
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        // don't deal with functions
    }

    ಶೂನ್ಯ visit(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterateAndNextNull(nodep->stmtsp());
    }

    ಶೂನ್ಯ visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = ಅಸತ್ಯ;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    ಶೂನ್ಯ combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        ಪ್ರತಿ (ಧ್ರುವ std::pair<AstVar* ಧ್ರುವ, std::vector<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ಧ್ರುವ invarp = kv.first;
            ಧ್ರುವ std::vector<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_ಗಂಟು_Module* ಧ್ರುವ ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_ಸಾಲು* ಧ್ರುವ fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.size() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* orp = nullptr;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp = nullptr;

            ಪ್ರತಿ (ಧ್ರುವ std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ಧ್ರುವ contribEnp = contrib.first;
                AstVar* ಧ್ರುವ contribOutp = contrib.second;

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ outRefp = ನೂತನ AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enRefp = ನೂತನ AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ andp = ನೂತನ AstAnd{fl, outRefp, enRefp};

                orp = (!orp) ? andp : ನೂತನ AstOr{fl, orp, andp};

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enRef2p = ನೂತನ AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : ನೂತನ AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            ಯದಿ (!envarp) {
                envarp = ನೂತನ AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->setDfgTriLowered();
                ifaceModp->addStmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            AstAssignW* ಧ್ರುವ enAssp
                = ನೂತನ AstAssignW{fl, ನೂತನ AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->addStmtsp(ನೂತನ AstAlways{enAssp});

            // Pull resolution
            ಧ್ರುವ AstPull* ಧ್ರುವ pullp = m_varAux(invarp).pullp;
            ಧ್ರುವ ತಾರ್ಕಿಕ pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* undrivenp = ನೂತನ AstNot{fl, ನೂತನ AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = ನೂತನ AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = ನೂತನ AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            AstAssignW* ಧ್ರುವ assp
                = ನೂತನ AstAssignW{fl, ನೂತನ AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->addStmtsp(ನೂತನ AstAlways{assp});
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::vector<Ast_ಗಂಟು_Module*> ifacesp;
        ಪ್ರತಿ (Ast_ಗಂಟು* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            ಯದಿ (VN_IS(modp, Iface)) ifacesp.ತಳ್ಳು_back(VN_AS(modp, ಗಂಟು_Module));
        }
        ಪ್ರತಿ (auto it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = ಸತ್ಯ;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ifdef VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::vector<Ast_ಗಂಟು*> unusedRootps;
        netlistp->foreach([&](Ast_ಗಂಟು* nodep) {
            Ast_ಗಂಟು* ಧ್ರುವ enp = nodep->user1p();
            ಯದಿ (!enp) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (enp->backp()) ಪ್ರತ್ಯಾಗಮ;
            unusedRootps.emplace_back(enp);
        });
        ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
#endif
    }
    ~TristateVisitor() override {
        V3Stats::addStat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

ಶೂನ್ಯ V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("tristate", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
