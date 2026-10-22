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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Tristate.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Inst.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ TristateBaseVisitor VL_NOT_FINAL : ସର୍ବସାଧାରଣ VNVisitor {
ସର୍ବସାଧାରଣ:
    // METHODS
};

//######################################################################
// Graph support classes

ଶ୍ରେଣୀ TristateVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;
    ତାର୍କିକ m_isTristate = ମିଥ୍ୟା;  // Logic indicates a tristate
    ତାର୍କିକ m_feedsTri = ମିଥ୍ୟା;  // Propagates to a tristate node (on RHS)
    ତାର୍କିକ m_processed = ମିଥ୍ୟା;  // Tristating was cleaned up
ସର୍ବସାଧାରଣ:
    TristateVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    Ast_ଗଣ୍ଠି* nodep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_nodep; }
    ଅଚଳ AstVar* varp() ଅଚଳ { ଫେରନ୍ତୁ VN_CAST(nodep(), Var); }
    string name() ଅଚଳ override {
        ଫେରନ୍ତୁ ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvtToHex(nodep())));
    }
    string dotColor() ଅଚଳ override {
        ଫେରନ୍ତୁ (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->fileline(); }
    ତାର୍କିକ isTristate() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_isTristate; }
    ଶୂନ୍ୟ isTristate(ତାର୍କିକ flag) { m_isTristate = flag; }
    ତାର୍କିକ feedsTri() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_feedsTri; }
    ଶୂନ୍ୟ feedsTri(ତାର୍କିକ flag) { m_feedsTri = flag; }
    ତାର୍କିକ processed() ଅଚଳ { ଫେରନ୍ତୁ m_processed; }
    ଶୂନ୍ୟ processed(ତାର୍କିକ flag) { m_processed = flag; }
};

//######################################################################

ଶ୍ରେଣୀ TristateGraph final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
ସର୍ବସାଧାରଣ:
    using VarVec = std::vector<AstVar*>;

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    V3Graph m_graph;  // Logic graph

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    TristateGraph() { clearAndCheck(); }
    ଆଭାସୀ ~TristateGraph() { clearAndCheck(); }
    VL_UNCOPYABLE(TristateGraph);

ବ୍ୟକ୍ତିଗତ:
    // METHODS

    TristateVertex* makeVertex(Ast_ଗଣ୍ଠି* nodep) {
        TristateVertex* vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        ଯଦି (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = ନୂତନ TristateVertex{&m_graph, nodep};
            nodep->user4p(vertexp);
        }
        ଫେରନ୍ତୁ vertexp;
    }

    // METHODS - Graph optimization
    ଶୂନ୍ୟ graphWalkRecurseFwd(TristateVertex* vtxp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        ଯଦି (!vtxp->isTristate()) ଫେରନ୍ତୁ;  // tristate involved
        ଯଦି (vtxp->user() == 1) ଫେରନ୍ତୁ;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        ଯଦି (!vtxp->varp()) {  // not a var where we stop the recursion
            ପାଇଁ (V3GraphEdge& edge : vtxp->outEdges()) {
                TristateVertex* ଅଚଳ vvertexp = static_cast<TristateVertex*>(edge.ଶୀର୍ଷ());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ଯଦି (!vvertexp->isTristate()) {
                    vvertexp->isTristate(ସତ୍ୟ);
                    graphWalkRecurseFwd(vvertexp, level + 1);
                }
            }
        } ଅନ୍ୟଥା {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            ପାଇଁ (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ଅଚଳ vvertexp = static_cast<TristateVertex*>(edge.fromp());
                ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    ଯଦି (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(ସତ୍ୟ);
                        graphWalkRecurseFwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    ଶୂନ୍ୟ graphWalkRecurseBack(TristateVertex* vtxp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        ଯଦି (!(vtxp->isTristate() || vtxp->feedsTri())) ଫେରନ୍ତୁ;  // tristate involved
        ଯଦି (vtxp->user() == 3) ଫେରନ୍ତୁ;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        ଯଦି (!vtxp->varp()) {  // not a var where we stop the recursion
            ପାଇଁ (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ଅଚଳ vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ଯଦି (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(ସତ୍ୟ);
                    graphWalkRecurseBack(vvertexp, level + 1);
                }
            }
        }
    }

ସର୍ବସାଧାରଣ:
    // METHODS
    ତାର୍କିକ empty() ଅଚଳ { ଫେରନ୍ତୁ m_graph.empty(); }
    ଶୂନ୍ୟ clearAndCheck() {
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            ଅଚଳ TristateVertex& vvertex = static_cast<TristateVertex&>(vtx);
            ଯଦି (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_graph.clear();
        Ast_ଗଣ୍ଠି::user4_Clear_ବୃକ୍ଷ();  // Wipe all node user4p's that point to vertexes
    }
    ଶୂନ୍ୟ graphWalk(Ast_ଗଣ୍ଠି_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseBack(static_cast<TristateVertex*>(&vtx), 0);
        }
        ଯଦି (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    ଶୂନ୍ୟ associate(Ast_ଗଣ୍ଠି* fromp, Ast_ଗଣ୍ଠି* ଶୀର୍ଷ) {
        ନୂତନ V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(ଶୀର୍ଷ), 1};
    }
    ଶୂନ୍ୟ deleteVerticesFromSubtreeRecurse(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        // Skip vars, because they may be connected to more than one varref
        ଯଦି (!VN_IS(nodep, Var)) {
            TristateVertex* ଅଚଳ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            ଯଦି (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    ଶୂନ୍ୟ setTristate(Ast_ଗଣ୍ଠି* nodep) { makeVertex(nodep)->isTristate(ସତ୍ୟ); }
    ତାର୍କିକ isTristate(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ TristateVertex* ଅଚଳ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        ଫେରନ୍ତୁ vertexp && vertexp->isTristate();
    }
    ତାର୍କିକ feedsTri(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ TristateVertex* ଅଚଳ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        ଫେରନ୍ତୁ vertexp && vertexp->feedsTri();
    }
    ଶୂନ୍ୟ didProcess(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ quiet = ମିଥ୍ୟା) {
        ଯଦି (TristateVertex* ଅଚଳ vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(ସତ୍ୟ);
        } ଅନ୍ୟଥା ଯଦି (!quiet) {
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
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : m_graph.vertices()) {
            ଅଚଳ TristateVertex& vvertex = static_cast<ଅଚଳ TristateVertex&>(vtx);
            ଯଦି (vvertex.isTristate()) {
                ଯଦି (AstVar* ଅଚଳ nodep = VN_CAST(vvertex.nodep(), Var)) v.ଠେଲ_back(nodep);
            }
        }
        ଫେରନ୍ତୁ v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

ଶ୍ରେଣୀ TristatePinVisitor final : ସର୍ବସାଧାରଣ TristateBaseVisitor {
    TristateGraph& m_tgraph;
    ଅଚଳ ତାର୍କିକ m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        ଯଦି (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } ଅନ୍ୟଥା ଯଦି (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.setTristate(nodep->varp());
        }
    }
    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstSliceSel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_ଗଣ୍ଠି* nodep, TristateGraph& tgraph, ତାର୍କିକ lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################

ଶ୍ରେଣୀ TristateVisitor final : ସର୍ବସାଧାରଣ TristateBaseVisitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    ଅଚଳ VNUser3InUse m_inuser3;
    ଅଚଳ VNUser4InUse m_inuser4;

    ସଂରଚନା AuxAstVar final {
        AstPull* pullp = nullptr;  // pullup/pulldown direction (whole variable)
        AstVar* outVarp = nullptr;  // output __out var
        ତାର୍କିକ ifaceTristate = ମିଥ୍ୟା;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>
            bitPulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    ସଂରଚନା RefStrength final {
        Ast_ଗଣ୍ଠି_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_ଗଣ୍ଠି_Assign* m_assignp;  // Assignment containing m_varrefp
        ଅଚଳ Ast_ଗଣ୍ଠି_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_ଗଣ୍ଠି_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_ଗଣ୍ଠି_Var_Ref* varrefp, Ast_ଗଣ୍ଠି_Assign* assignp,
                    ଅଚଳ Ast_ଗଣ୍ଠି_Procedure* procedurep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enLhsp,
                    Ast_ଗଣ୍ଠି_Var_Ref* enVarrefp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enRhsp, VStrength strength,
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
    ସଂରଚନା LhsProjection final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_lhsp;  // LHS giving the enable assign
        Ast_ଗଣ୍ଠି_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    ସଂରଚନା ProceduralEnable final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    using RefStrengthVec = std::vector<RefStrength>;
    using VarMap = std::map<AstVar*, RefStrengthVec*>;
    using Assigns = std::vector<AstAssignW*>;
    using VarToAssignsMap = std::map<AstVar*, Assigns>;
    using AssignEnableMap = std::map<Ast_ଗଣ୍ଠି_Assign*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>;
    using ProceduralEnableMap = std::map<Ast_ଗଣ୍ଠି_Assign*, std::vector<ProceduralEnable>>;
    ଗଣନା : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    ତାର୍କିକ m_graphing = ମିଥ୍ୟା;  // Major mode - creating graph
    //
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    AstCell* m_cellp = nullptr;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    VarToAssignsMap m_assigns;  // Assigns in current module
    AssignEnableMap m_assignEnps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_unique = 0;
    ତାର୍କିକ m_alhs = ମିଥ୍ୟା;  // On LHS of assignment
    ତାର୍କିକ m_inAlias = ମିଥ୍ୟା;  // Inside alias statement
    ତାର୍କିକ m_processedIfaces = ମିଥ୍ୟା;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = nullptr;  // Delay of assignment currently collecting LHS drivers
    ଅଚଳ Ast_ଗଣ୍ଠି* m_logicp = nullptr;  // Current logic being built
    Ast_ଗଣ୍ଠି_Assign* m_assignp = nullptr;  // Current assignment
    ଅଚଳ Ast_ଗଣ୍ଠି_Procedure* m_procedurep = nullptr;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::vector<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ଅଚଳ {
        string o = (m_graphing ? " gr " : " ng ");
        ଯଦି (m_alhs) o += "alhs ";
        ଫେରନ୍ତୁ o;
    }
    ଶୂନ୍ୟ modAddStmtp(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* newp) {
        ଯଦି (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } ଅନ୍ୟଥା {
            m_modp->addStmtsp(newp);
        }
    }
    ଶୂନ୍ୟ associateLogic(Ast_ଗଣ୍ଠି* fromp, Ast_ଗଣ୍ଠି* ଶୀର୍ଷ) {
        ଯଦି (m_logicp) m_tgraph.associate(fromp, ଶୀର୍ଷ);
    }
    AstConst* newAllZerosOrOnes(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ ones) {
        V3_ସଂଖ୍ୟା ଅଙ୍କ{nodep, nodep->width()};
        ଯଦି (ones) ଅଙ୍କ.setAllBits1();
        AstConst* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), ଅଙ୍କ};
        ଫେରନ୍ତୁ newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_ଧାଡ଼ି* fl, AstVar* varp, ଅଚଳ string& dotted, VAccess access,
                           ଅଚଳ string& inlinedDots) {
        AstVarXRef* ଅଚଳ xrefp = ନୂତନ AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        ଫେରନ୍ତୁ xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ string& dottedPath) {
        ଯଦି (dottedPath.empty()) ଫେରନ୍ତୁ nullptr;
        ଅଚଳ string firstComp = dottedPath.substr(0, dottedPath.find('.'));
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ଅଚଳ ivarp = VN_CAST(stmtp, Var);
            ଯଦି (!ivarp || ivarp->name() != firstComp) ଜାରି;
            ଅଚଳ AstIfaceRefDType* ଅଚଳ ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            ଯଦି (ifaceDtp && ifaceDtp->modportp()) ଫେରନ୍ତୁ ifaceDtp->modportp();
            ଭାଙ୍ଗ;
        }
        ଫେରନ୍ତୁ nullptr;
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getEnp(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (nodep->user1p()) {
            ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp = VN_CAST(nodep, ଗଣ୍ଠି_Var_Ref)) {
                ଯଦି (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    ଫେରନ୍ତୁ newAllZerosOrOnes(nodep, ସତ୍ୟ);
                }
            }
        } ଅନ୍ୟଥା {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, ସତ୍ୟ));
        }
        // Otherwise return the previous output enable
        ଫେରନ୍ତୁ VN_AS(nodep->user1p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    ଶୂନ୍ୟ addModportVarRefIfMissing(AstModport* modportp, AstVar* varp, VDirection dir) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ଅଚଳ mvrp = VN_CAST(mrp, ModportVarRef);
            ଯଦି (mvrp && mvrp->varp() == varp) ଫେରନ୍ତୁ;  // Already present
        }
        AstModportVarRef* ଅଚଳ mvrp = ନୂତନ AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->addVarsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, ତାର୍କିକ is_ଶୀର୍ଷ) {
        // Return the master __en for the specified input variable
        ଯଦି (!invarp->user1p()) {
            AstVar* ଅଚଳ newp
                = ନୂତନ AstVar{invarp->fileline(), is_ଶୀର୍ଷ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp = findParentModule(invarp);
            ଯଦି (ownerModp && ownerModp != m_modp) {
                ownerModp->addStmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ଅଚଳ modportp = VN_CAST(stmtp, Modport);
                    ଯଦି (!modportp) ଜାରି;
                    ପାଇଁ (Ast_ଗଣ୍ଠି* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ଅଚଳ mvrp = VN_CAST(mrp, ModportVarRef);
                        ଯଦି (mvrp && mvrp->varp() == invarp) {
                            addModportVarRefIfMissing(modportp, newp, VDirection::ନିବେଶ);
                            ଭାଙ୍ଗ;
                        }
                    }
                }
            } ଅନ୍ୟଥା {
                modAddStmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        ଫେରନ୍ତୁ VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ଅଚଳ constp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = constp->fileline();
        V3_ସଂଖ୍ୟା numz{constp, constp->width()};
        numz.opBitsZ(constp->ଅଙ୍କ());  // Z->1, else 0
        V3_ସଂଖ୍ୟା numz0{constp, constp->width()};
        numz0.opNot(numz);  // Z->0, else 1
        ଫେରନ୍ତୁ ନୂତନ AstConst{fl, numz0};
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* get_En_ପଦସମଷ୍ଟି_Based_On_Originalp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nodep) {
        ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep, VarRef)) {
            ଫେରନ୍ତୁ ନୂତନ AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), ମିଥ୍ୟା),
                                 VAccess::READ};
        } ଅନ୍ୟଥା ଯଦି (AstVarXRef* ଅଚଳ xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ଅଚଳ enVarp = getCreateEnVarp(xrefp->varp(), ମିଥ୍ୟା);
            ଫେରନ୍ତୁ newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } ଅନ୍ୟଥା ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
            ଫେରନ୍ତୁ getNonZConstp(constp);
        } ଅନ୍ୟଥା ଯଦି (AstExtend* ଅଚଳ extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = extendp->lhsp()->unlinkFrBack();
            AstExtend* ଅଚଳ enExtendp = extendp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            extendp->lhsp(lhsp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enLhsp = get_En_ପଦସମଷ୍ଟି_Based_On_Originalp(lhsp);
            enExtendp->lhsp(ନୂତନ AstNot{enLhsp->fileline(), enLhsp});
            ଫେରନ୍ତୁ ନୂତନ AstNot{enExtendp->fileline(), enExtendp};
        } ଅନ୍ୟଥା ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(nodep, Sel)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = selp->fromp()->unlinkFrBack();
            AstSel* ଅଚଳ enSelp = selp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            selp->fromp(fromp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enFromp = get_En_ପଦସମଷ୍ଟି_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            ଫେରନ୍ତୁ enSelp;
        } ଅନ୍ୟଥା {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            ଫେରନ୍ତୁ nullptr;
        }
    }
    AstVar* getCreateOutVarp(AstVar* invarp, ତାର୍କିକ is_ଶୀର୍ଷ) {
        // Return the master __out for the specified input variable
        ଯଦି (!m_varAux(invarp).outVarp) {
            AstVar* ଅଚଳ newp
                = ନୂତନ AstVar{invarp->fileline(), is_ଶୀର୍ଷ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            modAddStmtp(invarp, newp);
            m_varAux(invarp).outVarp = newp;  // find outvar given invarp
        }
        ଫେରନ୍ତୁ m_varAux(invarp).outVarp;
    }
    AstVar* getCreateUnconnVarp(Ast_ଗଣ୍ଠି* fromp, Ast_ଗଣ୍ଠି_DType* dtypep) {
        AstVar* ଅଚଳ newp = ନୂତନ AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvtToStr(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        modAddStmtp(newp, newp);
        ଫେରନ୍ତୁ newp;
    }

    LhsProjection newLhsProjection(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_Var_Ref* targetp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newLhsp = lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_ଗଣ୍ଠି* nodep) { nodep->user1p(nullptr); });
        Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        ଫେରନ୍ତୁ LhsProjection{newLhsp, newVarrefp, enp};
    }

    ଶୂନ୍ୟ map_ଭର୍ତ୍ତି_Lhs_Var_Ref(Ast_ଗଣ୍ଠି_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ଅଚଳ ଚାବି = nodep->varp();
        ଅଚଳ auto pair = m_lhsmap.emplace(ଚାବି, nullptr);
        ଯଦି (pair.second) pair.first->second = ନୂତନ RefStrengthVec;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enLhsp = nullptr;
        Ast_ଗଣ୍ଠି_Var_Ref* enVarrefp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enRhsp = nullptr;
        ଅଚଳ auto enIt = m_assignEnps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ଅଚଳ ତାର୍କିକ canCombineProcedural = !VN_IS(ଚାବି->dtypep()->skipRefp(), ଗଣ୍ଠି_UOr_Struct_DType);
        ଅଚଳ Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ procedurep
            = enIt != m_assignEnps.end() && canCombineProcedural ? m_procedurep : nullptr;
        ଯଦି (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->ଠେଲ_back(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newEnableDeposit(AstSel* selp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstShiftL{
            selp->fileline(), ନୂତନ AstExtend{selp->fileline(), enp, selp->fromp()->width()},
            selp->lsbp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), selp->fromp()->width()};
        ଫେରନ୍ତୁ newp;
    }

    ଶୂନ୍ୟ setPullDirection(AstVar* varp, AstPull* pullp) {
        ଅଚଳ AstPull* ଅଚଳ oldpullp = m_varAux(varp).pullp;
        ଯଦି (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } ଅନ୍ୟଥା {
            ଯଦି (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    ଶୂନ୍ୟ setBitPullDirection(AstVar* varp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitIndex, ପୂର୍ଣ୍ଣସଂଖ୍ୟା direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        auto& bitPulls = m_varAux(varp).bitPulls;
        auto it = bitPulls.find(bitIndex);
        ଯଦି (it != bitPulls.end() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bitIndex << " of " << varp->prettyNameQ());
        }
        bitPulls[bitIndex] = direction;
    }

    AstConst* createPerBitPullConst(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ଅଚଳ auto& bitPulls = m_varAux(varp).bitPulls;
        V3_ସଂଖ୍ୟା ଅଙ୍କ{varp, varp->width(), 0};  // Start with all zeros
        ପାଇଁ (ଅଚଳ auto& pair : bitPulls) {
            ଯଦି (pair.second == 1) ଅଙ୍କ.setBit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        ଫେରନ୍ତୁ ନୂତନ AstConst{varp->fileline(), ଅଙ୍କ};
    }

    ତାର୍କିକ hasPerBitPulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        ଫେରନ୍ତୁ !m_varAux(varp).bitPulls.empty();
    }

    ଶୂନ୍ୟ checkUnhandled(Ast_ଗଣ୍ଠି* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        ଯଦି (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        ଯଦି ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Tristates(Ast_ଗଣ୍ଠି_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ଅଚଳ TristateGraph::VarVec vars = m_tgraph.tristateVars();
        ପାଇଁ (auto varp : vars) {
            ଯଦି (!m_tgraph.isTristate(varp)) ଜାରି;
            ଅଚଳ auto it = m_lhsmap.find(varp);
            ଯଦି (it != m_lhsmap.end()) ଜାରି;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ଅଚଳ AstPull* ଅଚଳ pullp = m_varAux(varp).pullp;
            ଅଚଳ ତାର୍କିକ pull_ମୂଲ୍ୟ = pullp && pullp->direction() == 1;
            AstConst* ଅଚଳ constp = newAllZerosOrOnes(varp, pull_ମୂଲ୍ୟ);
            AstVarRef* ଅଚଳ varrefp = ନୂତନ AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            AstAssignW* ଅଚଳ newp = ନୂତନ AstAssignW{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, ମିଥ୍ୟା));  // en=0 for floating/pullup vars
            nodep->addStmtsp(ନୂତନ AstAlways{newp});
            map_ଭର୍ତ୍ତି_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        ପାଇଁ (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ଅଚଳ std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            ଯଦି (it == m_lhsmap.end()) ଜାରି;
            AstVar* ଅଚଳ invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            ଯଦି (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp = findParentModule(invarp);
                ଅଚଳ ତାର୍କିକ isCrossModule = ownerModp && ownerModp != nodep;
                ଅଚଳ ତାର୍କିକ isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                ଯଦି (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    ପାଇଁ (ଅଚଳ RefStrength& rs : *refsp) {
                        ଯଦି (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            ଭାଙ୍ଗ;
                        }
                    }
                } ଅନ୍ୟଥା ଯଦି (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = ସତ୍ୟ;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    ସଂରଚନା PartitionInfo final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, PartitionInfo> partitions;
                    ପାଇଁ (ଅଚଳ RefStrength& rs : *refsp) {
                        ଯଦି (AstVarXRef* ଅଚଳ xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            PartitionInfo& pi = partitions[xrefp->dotted()];
                            pi.refs.ଠେଲ_back(rs);
                            ଯଦି (pi.inlinedDots.empty()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } ଅନ୍ୟଥା {
                            partitions[""].refs.ଠେଲ_back(rs);
                        }
                    }
                    ପାଇଁ (auto& kv : partitions) {
                        ଭର୍ତ୍ତି_Tristates_Signal(nodep, invarp, &kv.second.refs, ସତ୍ୟ, kv.first,
                                              kv.second.inlinedDots,
                                              findModportForDotted(nodep, kv.first));
                    }
                } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = ସତ୍ୟ;
                    ଭର୍ତ୍ତି_Tristates_Signal(nodep, invarp, refsp, ସତ୍ୟ, "", "", nullptr);
                } ଅନ୍ୟଥା {
                    ଭର୍ତ୍ତି_Tristates_Signal(nodep, invarp, refsp, ମିଥ୍ୟା, "", "", nullptr);
                }
            } ଅନ୍ୟଥା {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(delete refsp, refsp);
        }
    }

    ଶୂନ୍ୟ aggregateTriSameStrength(Ast_ଗଣ୍ଠି_Module* nodep, AstVar* ଅଚଳ varp, AstVar* ଅଚଳ envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator endStrength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        ସଂରଚନା DriverGroup final {
            ଅଚଳ Ast_ଗଣ୍ଠି_Procedure* m_procedurep;  // Process owning these assignments
            std::vector<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::vector<DriverGroup> drivers;
        std::map<ଅଚଳ Ast_ଗଣ୍ଠି_Procedure*, size_t> procedureToDriver;
        ପାଇଁ (auto it = beginStrength; it != endStrength; ++it) {
            ଯଦି (it->m_procedurep) {
                ଅଚଳ auto emplaced = procedureToDriver.emplace(it->m_procedurep, drivers.size());
                ଯଦି (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.ଠେଲ_back(&*it);
                    ଜାରି;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ଠେଲ_back(DriverGroup{it->m_procedurep, {&*it}});
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* orp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enp = nullptr;

        ପାଇଁ (DriverGroup& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ଅଚଳ newLhsp = ନୂତନ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvtToStr(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->setDfgTriLowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->addStmtsp(newLhsp);
            AstVar* ଅଚଳ newEnLhsp
                = ନୂତନ AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvtToStr(m_unique++), envarp};  // 2-state ok
            newEnLhsp->setDfgTriLowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->addStmtsp(newEnLhsp);

            ପାଇଁ (RefStrength* ଅଚଳ refStrengthp : driver.m_refsp) {
                Ast_ଗଣ୍ଠି_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                ଯଦି (VN_IS(refp, VarXRef)) {
                    AstVarRef* ଅଚଳ localRefp
                        = ନୂତନ AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(nullptr);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } ଅନ୍ୟଥା {
                    refp->varp(newLhsp);
                }

                ଯଦି (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enLhsp = refStrengthp->m_enLhsp;
                    Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ enVarrefp = refStrengthp->m_enVarrefp;
                    ଯଦି (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ଅଚଳ localRefp
                            = ନୂତନ AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        ଯଦି (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_ବୃକ୍ଷ(), enVarrefp);
                        } ଅନ୍ୟଥା {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ଠେଲ_Deletep(enVarrefp), enVarrefp);
                        }
                    } ଅନ୍ୟଥା {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ଠେଲ_back(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = nullptr;
                    refStrengthp->m_enVarrefp = nullptr;
                    refStrengthp->m_enRhsp = nullptr;
                } ଅନ୍ୟଥା {
                    AstAssignW* ଅଚଳ enLhspAssignp = ନୂତନ AstAssignW{
                        refp->fileline(),
                        ନୂତନ AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    ଯଦି (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->addStmtsp(ନୂତନ AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_ଧାଡ଼ି* ଅଚଳ fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ref1p = ନୂତନ AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ref2p = ନୂତନ AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ andp = ନୂତନ AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : ନୂତନ AstOr{fl, orp, andp};

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ref3p = ନୂତନ AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : ନୂତନ AstOr{ref3p->fileline(), enp, ref3p};
        }
        AstAssignW* ଅଚଳ assp = ନୂତନ AstAssignW{
            varp->fileline(), ନୂତନ AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->addStmtsp(ନୂତନ AstAlways{assp});

        AstAssignW* ଅଚଳ enAssp = ନୂତନ AstAssignW{
            envarp->fileline(), ନୂତନ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->addStmtsp(ନୂତନ AstAlways{enAssp});
    }

    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Procedural_Enable_Assignments(Ast_ଗଣ୍ଠି_Module* nodep) {
        ପାଇଁ (auto& assignEnables : m_proceduralEnables) {
            Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = assignEnables.first;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* combinedLhsp = assignp->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* combinedRhsp = assignp->rhsp()->unlinkFrBack();
            ପାଇଁ (ProceduralEnable& enable : assignEnables.second) {
                combinedLhsp = ନୂତନ AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = ନୂତନ AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            ଯଦି (!VN_IS(assignp, AssignDly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ଅଚଳ sampledp
                    = ନୂତନ AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvtToStr(m_unique++), combinedRhsp->dtypep()};
                nodep->addStmtsp(sampledp);
                assignp->lhsp(ନୂତନ AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                AstAssign* ଅଚଳ applyp
                    = ନୂତନ AstAssign{assignp->fileline(), combinedLhsp,
                                    ନୂତନ AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->addNextHere(applyp);
            } ଅନ୍ୟଥା {
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
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Tristates_Signal(Ast_ଗଣ୍ଠି_Module* nodep, AstVar* ଅଚଳ invarp, RefStrengthVec* refsp,
                               ତାର୍କିକ isIfaceTri, ଅଚଳ string& ifaceDottedPath,
                               ଅଚଳ string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        ଅଚଳ ତାର୍କିକ is_ଶୀର୍ଷ_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_ଶୀର୍ଷ();
        ଯଦି (!isIfaceTri) {
            ଯଦି (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty())
                 && is_ଶୀର୍ଷ_Inout)
                || (!nodep->is_ଶୀର୍ଷ() && invarp->isIO())) {
                // This var becomes an input
                invarp->varType2In();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = getCreateOutVarp(invarp, is_ଶୀର୍ଷ_Inout);
                outvarp->varType2Out();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_ଶୀର୍ଷ_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                ଯଦି (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                ଯଦି (!m_varAux(invarp).bitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } ଅନ୍ୟଥା ଯଦି (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* orp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enp = nullptr;

        std::sort(refsp->begin(), refsp->end(),
                  [](RefStrength a, RefStrength b) { ଫେରନ୍ତୁ a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        ଯେତେବେଳେ (beginStrength != refsp->end()) {
            RefStrengthVec::iterator endStrength = beginStrength + 1;
            ଯେତେବେଳେ (endStrength != refsp->end()
                   && endStrength->m_strength == beginStrength->m_strength)
                endStrength++;

            File_ଧାଡ଼ି* ଅଚଳ fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strengthPrefix;
            ଯଦି (isIfaceTri && !ifaceDottedPath.empty()) {
                strengthPrefix = ifaceDottedPath;
                std::replace(strengthPrefix.begin(), strengthPrefix.end(), '.', '_');
                strengthPrefix += "__";
            }
            ଅଚଳ string strengthVarName
                = strengthPrefix + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ଅଚଳ varStrengthp = ନୂତନ AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->setDfgTriLowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->addStmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ଅଚଳ enVarStrengthp = ନୂତନ AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->setDfgTriLowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->addStmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     endStrength);

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ପଦସମଷ୍ଟି_Current_Strengthp;
            ଯଦି (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                ପଦସମଷ୍ଟି_Current_Strengthp
                    = ନୂତନ AstAnd{fl, ନୂତନ AstVarRef{fl, varStrengthp, VAccess::READ},
                                 ନୂତନ AstNot{fl, enp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}};
            } ଅନ୍ୟଥା {
                ପଦସମଷ୍ଟି_Current_Strengthp = ନୂତନ AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? ପଦସମଷ୍ଟି_Current_Strengthp : ନୂତନ AstOr{fl, orp, ପଦସମଷ୍ଟି_Current_Strengthp};

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enVarStrengthRefp
                = ନୂତନ AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : ନୂତନ AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = endStrength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        ଯଦି (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->setDfgAllowMultidriveTri();
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା contribIdx = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_ifaceContribs[invarp].size());
            File_ଧାଡ଼ି* ଅଚଳ fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ଅଚଳ contribOutp = ନୂତନ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvtToStr(contribIdx), invarp};
            contribOutp->setDfgTriLowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->addStmtsp(contribOutp);

            AstVar* ଅଚଳ contribEnp = ନୂତନ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvtToStr(contribIdx), invarp};
            contribEnp->setDfgTriLowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->addStmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            ଯଦି (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                addModportVarRefIfMissing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                addModportVarRefIfMissing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_ଗଣ୍ଠି_Var_Ref*>(
                                                    ନୂତନ AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_ଗଣ୍ଠି_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ଅଚଳ assp = ନୂତନ AstAssignW{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(ନୂତନ AstAlways{assp});
            }
            {
                Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_ଗଣ୍ଠି_Var_Ref*>(
                                                    ନୂତନ AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_ଗଣ୍ଠି_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ଅଚଳ assp = ନୂତନ AstAssignW{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(ନୂତନ AstAlways{assp});
            }

            m_ifaceContribs[invarp].ଠେଲ_back({contribEnp, contribOutp});
            ଫେରନ୍ତୁ;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ଅଚଳ ତାର୍କିକ hasPull = m_varAux(lhsp).pullp || hasPerBitPulls(lhsp);
        ଯଦି (!outvarp || hasPull) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* undrivenp;
            ଯଦି (envarp) {
                undrivenp = ନୂତନ AstNot{envarp->fileline(),
                                       ନୂତନ AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } ଅନ୍ୟଥା ଯଦି (enp) {
                undrivenp = ନୂତନ AstNot{enp->fileline(), enp};
                enp = nullptr;  // moved into undrivenp
            } ଅନ୍ୟଥା {
                undrivenp = newAllZerosOrOnes(invarp, ସତ୍ୟ);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            ଯଦି (hasPerBitPulls(lhsp)) {
                pullConstp = createPerBitPullConst(lhsp);
            } ଅନ୍ୟଥା {
                ଅଚଳ AstPull* ଅଚଳ pullp = m_varAux(lhsp).pullp;
                ଅଚଳ ତାର୍କିକ pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = ନୂତନ AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? ନୂତନ AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        ଯଦି (envarp && !enp) enp = newAllZerosOrOnes(invarp, ମିଥ୍ୟା);

        ଯଦି (envarp) {
            AstAssignW* ଅଚଳ enAssp = ନୂତନ AstAssignW{
                enp->fileline(), ନୂତନ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->addStmtsp(ନୂତନ AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        AstAssignW* ଅଚଳ assp = ନୂତନ AstAssignW{
            lhsp->fileline(), ନୂତନ AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->addStmtsp(ନୂତନ AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        ଯଦି ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty()) && is_ଶୀର୍ଷ_Inout) {
            ଯଦି (envarp) {
                envarp->primaryIO(ସତ୍ୟ);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            ଯଦି (outvarp) {
                outvarp->primaryIO(ସତ୍ୟ);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            ଯଦି (!v3Global.opt.libCreate().empty()) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(ସତ୍ୟ);
            }
        }
    }

    ତାର୍କିକ isOnlyAssignmentIsToLhsVar(ଅଚଳ AstAssignW* ଅଚଳ nodep) {
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            auto foundIt = m_assigns.find(varRefp->varp());
            ଯଦି (foundIt != m_assigns.end()) {
                auto ଅଚଳ& assignsToVar = foundIt->second;
                ଯଦି (assignsToVar.size() == 1 && assignsToVar[0] == nodep) ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ addToAssignmentList(AstAssignW* nodep) {
        ଯଦି (AstVarRef* ଅଚଳ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            ଯଦି (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ଠେଲ_back(nodep);
            } ଅନ୍ୟଥା ଯଦି (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ଅଚଳ AstAssignW* ଅଚଳ nodep, ତାର୍କିକ ମୂଲ୍ୟ) {
        ଯଦି (AstStrengthSpec* ଅଚଳ strengthSpec = nodep->strengthSpecp()) {
            ଫେରନ୍ତୁ ମୂଲ୍ୟ ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        ଫେରନ୍ତୁ VStrength::STRONG;  // default strength is strong
    }

    ତାର୍କିକ assignment_Of_ମୂଲ୍ୟ_On_All_Bits(ଅଚଳ AstAssignW* ଅଚଳ nodep, ତାର୍କିକ ମୂଲ୍ୟ) {
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->rhsp(), Const)) {
            ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ = constp->ଅଙ୍କ();
            ଫେରନ୍ତୁ ମୂଲ୍ୟ ? ଅଙ୍କ.isEqAllOnes() : ଅଙ୍କ.isEqZero();
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    AstAssignW* get_Strongest_Assignment_Of_ମୂଲ୍ୟ(ଅଚଳ Assigns& assigns, ତାର୍କିକ ମୂଲ୍ୟ) {
        auto maxIt = std::max_element(
            assigns.begin(), assigns.end(), [&](ଅଚଳ AstAssignW* ap, ଅଚଳ AstAssignW* bp) {
                ଅଚଳ ତାର୍କିକ ମୂଲ୍ୟଗୁଡ଼ିକ_On_Rhs_A = assignment_Of_ମୂଲ୍ୟ_On_All_Bits(ap, ମୂଲ୍ୟ);
                ଅଚଳ ତାର୍କିକ ମୂଲ୍ୟଗୁଡ଼ିକ_On_Rhs_B = assignment_Of_ମୂଲ୍ୟ_On_All_Bits(bp, ମୂଲ୍ୟ);
                ଯଦି (!ମୂଲ୍ୟଗୁଡ଼ିକ_On_Rhs_A) ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ_On_Rhs_B;
                ଯଦି (!ମୂଲ୍ୟଗୁଡ଼ିକ_On_Rhs_B) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                ଫେରନ୍ତୁ getStrength(ap, ମୂଲ୍ୟ) < getStrength(bp, ମୂଲ୍ୟ);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        ଫେରନ୍ତୁ assignment_Of_ମୂଲ୍ୟ_On_All_Bits(*maxIt, ମୂଲ୍ୟ) ? *maxIt : nullptr;
    }

    ତାର୍କିକ isAssignmentNotStrongerThanStrength(ଅଚଳ AstAssignW* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ଅଚଳ uint8_t strength0 = getStrength(assignp, 0);
        ଅଚଳ uint8_t strength1 = getStrength(assignp, 1);
        ଫେରନ୍ତୁ (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_Of_ମୂଲ୍ୟ_On_All_Bits(assignp, 0))
               || (strength1 <= strength && assignment_Of_ମୂଲ୍ୟ_On_All_Bits(assignp, 1));
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* new_Merge_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp, File_ଧାଡ଼ି* ଅଚଳ fl,
                              ତାର୍କିକ isWor) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ପଦସମଷ୍ଟି = nullptr;
        ଯଦି (isWor)
            ପଦସମଷ୍ଟି = ନୂତନ AstOr{fl, lhsp, rhsp};
        ଅନ୍ୟଥା
            ପଦସମଷ୍ଟି = ନୂତନ AstAnd{fl, lhsp, rhsp};
        ଫେରନ୍ତୁ ପଦସମଷ୍ଟି;
    }

    ଶୂନ୍ୟ mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        ପାଇଁ (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            ଯଦି (assigns.size() <= 1) ଜାରି;
            AstVar* ଅଚଳ varp = varpAssigns.first;
            ଯଦି (!varp->isWiredNet()) ଜାରି;
            auto it = assigns.begin();
            AstAssignW* ଅଚଳ assignWp0 = *it;
            File_ଧାଡ଼ି* ଅଚଳ fl = assignWp0->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* wExp = nullptr;
            ଯେତେବେଳେ (++it != assigns.end()) {
                AstAssignW* assignWpi = *it;
                ଯଦି (!wExp) {
                    wExp
                        = new_Merge_ପଦସମଷ୍ଟି(assignWp0->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                       assignWpi->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), fl, varp->isWor());
                } ଅନ୍ୟଥା {
                    wExp = new_Merge_ପଦସମଷ୍ଟି(wExp, assignWpi->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((assignWpi->unlinkFrBack()->delete_ବୃକ୍ଷ()), assignWpi);
            }
            AstVarRef* ଅଚଳ wVarRef = ନୂତନ AstVarRef{fl, varp, VAccess::WRITE};
            AstAssignW* ଅଚଳ wAssignp = ନୂତନ AstAssignW{fl, wVarRef, wExp};
            assignWp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ଠେଲ_Deletep(assignWp0), assignWp0);
            assigns.clear();
            assigns.ଠେଲ_back(wAssignp);
        }
    }

    ଶୂନ୍ୟ removeNotStrongerAssignments(Assigns& assigns, ଅଚଳ AstAssignW* strongestp,
                                      uint8_t greatestKnownStrength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        auto removedIt = std::remove_if(assigns.begin(), assigns.end(), [&](AstAssignW* assignp) {
            ଯଦି (assignp == strongestp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (isAssignmentNotStrongerThanStrength(assignp, greatestKnownStrength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ଠେଲ_Deletep(assignp->unlinkFrBack()), assignp);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        assigns.erase(removedIt, assigns.end());
    }

    ଶୂନ୍ୟ removeAssignmentsNotStrongerThanUniformConstant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        ପାଇଁ (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            ଯଦି (assigns.size() <= 1) ଜାରି;
            ଅଚଳ AstAssignW* ଅଚଳ strongest0p = get_Strongest_Assignment_Of_ମୂଲ୍ୟ(assigns, 0);
            ଅଚଳ AstAssignW* ଅଚଳ strongest1p = get_Strongest_Assignment_Of_ମୂଲ୍ୟ(assigns, 1);
            ଅଚଳ AstAssignW* strongestp = nullptr;
            uint8_t greatestKnownStrength = 0;
            ଅଚଳ auto getIfStrongest
                = [&](ଅଚଳ AstAssignW* ଅଚଳ strongestCandidatep, ତାର୍କିକ ମୂଲ୍ୟ) {
                      ଯଦି (!strongestCandidatep) ଫେରନ୍ତୁ;
                      ଅଚଳ uint8_t strength = getStrength(strongestCandidatep, ମୂଲ୍ୟ);
                      ଯଦି (strength >= greatestKnownStrength) {
                          greatestKnownStrength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            ଯଦି (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    ଶୂନ୍ୟ removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        ପାଇଁ (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            ଯଦି (assigns.size() <= 1) ଜାରି;
            auto maxIt = std::max_element(
                assigns.begin(), assigns.end(), [&](ଅଚଳ AstAssignW* ap, ଅଚଳ AstAssignW* bp) {
                    ଯଦି (m_tgraph.isTristate(ap)) ଫେରନ୍ତୁ !m_tgraph.isTristate(bp);
                    ଯଦି (m_tgraph.isTristate(bp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                    ଅଚଳ uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ଅଚଳ uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    ଫେରନ୍ତୁ minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ଅଚଳ AstAssignW* ଅଚଳ strongestp = m_tgraph.isTristate(*maxIt) ? nullptr : *maxIt;
            ଯଦି (strongestp) {
                ଅଚଳ uint8_t greatestKnownStrength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        ଯଦି (m_graphing) {
            ଯଦି (!m_alhs && nodep->ଅଙ୍କ().hasZ()) m_tgraph.setTristate(nodep);
        } ଅନ୍ୟଥା {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            ଯଦି (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ଅଚଳ varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା ଯଦି (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                AstConst* ଅଚଳ enp = getNonZConstp(nodep);
                V3_ସଂଖ୍ୟା num1{nodep, nodep->width()};
                num1.opAnd(nodep->ଅଙ୍କ(), enp->ଅଙ୍କ());  // 01X->01X, Z->0
                AstConst* ଅଚଳ newconstp = ନୂତନ AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        ଯଦି (m_graphing) {
            iterateChildren(nodep);
            ଯଦି (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } ଅନ୍ୟଥା {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } ଅନ୍ୟଥା {
            ଯଦି (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ଫେରନ୍ତୁ;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = nodep->condp();
            ଯଦି (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thenp = nodep->thenp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elsep = nodep->elsep();
            ଯଦି (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ en1p = getEnp(thenp);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enp
                    = ନୂତନ AstCond{nodep->fileline(), condp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                ଯଦି (thenp->user1p() && !thenp->user1p()->backp()) ଠେଲ_Deletep(thenp->user1p());
                ଯଦି (elsep->user1p() && !elsep->user1p()->backp()) ଠେଲ_Deletep(elsep->user1p());
                thenp->user1p(nullptr);
                elsep->user1p(nullptr);
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(nullptr);
            m_tgraph.didProcess(nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        ଯଦି (m_graphing) {
            iterateChildren(nodep);
            ଯଦି (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } ଅନ୍ୟଥା {
                associateLogic(nodep->fromp(), nodep);
            }
        } ଅନ୍ୟଥା {
            ଯଦି (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ଯଦି (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ଅନ୍ୟଥା {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                ଯଦି (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                ଯଦି (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ en1p = getEnp(nodep->fromp());
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enp
                        = ନୂତନ AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                     nodep->widthConst()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(AstConcat* nodep) override {
        ଯଦି (m_graphing) {
            iterateChildren(nodep);
            ଯଦି (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } ଅନ୍ୟଥା {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } ଅନ୍ୟଥା {
            ଯଦି (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ଯଦି (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enp = VN_AS(nodep->user1p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                    nodep->user1p(nullptr);
                    nodep->lhsp()->user1p(ନୂତନ AstSel{nodep->fileline(), enp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                                     nodep->rhsp()->width(),
                                                     nodep->lhsp()->width()});
                    nodep->rhsp()->user1p(
                        ନୂତନ AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->width()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ଅନ୍ୟଥା {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p = nodep->lhsp();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr2p = nodep->rhsp();
                ଯଦି (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ en1p = getEnp(expr1p);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ en2p = getEnp(expr2p);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enp = ନୂତନ AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(nullptr);
                    expr2p->user1p(nullptr);
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ଯଦି (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.setTristate(nodep);
        } ଅନ୍ୟଥା {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            ଯଦି (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ଫେରନ୍ତୁ;
            }
            m_tgraph.didProcess(nodep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr2p = nodep->rhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enp;
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ en2p = VN_AS(expr2p->user1p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                enp = ନୂତନ AstAnd{nodep->fileline(), expr1p, en2p};
            } ଅନ୍ୟଥା {
                enp = expr1p;
            }
            expr1p->user1p(nullptr);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    ଶୂନ୍ୟ visitAndOr(Ast_ଗଣ୍ଠି_Biop* nodep, ତାର୍କିକ isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ଯଦି (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } ଅନ୍ୟଥା {
            ଯଦି (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ଫେରନ୍ତୁ;
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
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p = nodep->lhsp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr2p = nodep->rhsp();
            ଯଦି (!expr1p->user1p() && !expr2p->user1p()) {
                ଫେରନ୍ତୁ;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* en1p = getEnp(expr1p);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* en2p = getEnp(expr2p);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subexpr1p = expr1p->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subexpr2p = expr2p->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            ଯଦି (isAnd) {
                subexpr1p = ନୂତନ AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = ନୂତନ AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            ଯଦି (!en1p->op1p() && en2p->op1p()) {
                std::swap(en1p, en2p);
                std::swap(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enp = ନୂତନ AstOr{
                nodep->fileline(),
                ନୂତନ AstAnd{nodep->fileline(), en1p, ନୂତନ AstOr{nodep->fileline(), en2p, subexpr1p}},
                ନୂତନ AstAnd{nodep->fileline(), en2p->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(nullptr);
            expr2p->user1p(nullptr);
        }
    }
    ଶୂନ୍ୟ visit(AstAnd* nodep) override { visitAndOr(nodep, ସତ୍ୟ); }
    ଶୂନ୍ୟ visit(AstOr* nodep) override { visitAndOr(nodep, ମିଥ୍ୟା); }

    ଶୂନ୍ୟ visitAssign(Ast_ଗଣ୍ଠି_Assign* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        ଯଦି (VN_IS(nodep->rhsp(), CReset)) ଫେରନ୍ତୁ;
        ଯଦି (AstAssignW* ଅଚଳ assignWp = VN_CAST(nodep, AssignW)) {
            m_currentDelayp = VN_CAST(assignWp->timingControlp(), Delay);
        }
        ଯଦି (m_graphing) {
            ଯଦି (AstAssignW* assignWp = VN_CAST(nodep, AssignW)) {
                ଯଦି (assignWp->getLhsNetDelay()) ଫେରନ୍ତୁ;
                ଯଦି (assignWp->timingControlp() && !m_currentDelayp) ଫେରନ୍ତୁ;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                ଯଦି (m_currentDelayp && m_currentDelayp->fallDelay()) ଫେରନ୍ତୁ;
                ଯଦି (!assignWp->timingControlp()) addToAssignmentList(assignWp);
            }

            ଯଦି (nodep->user2() & U2_GRAPHING) ଫେରନ୍ତୁ;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterateAndNextNull(nodep->rhsp());
            m_alhs = ସତ୍ୟ;
            iterateAndNextNull(nodep->lhsp());
            m_alhs = ମିଥ୍ୟା;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->user2() & U2_NONGRAPH) {
                ଫେରନ୍ତୁ;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterateAndNextNull(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            ଯଦି (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, AssignW), nodep,
                            "Continuous assignment inside procedural process");
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)
                          : newAllZerosOrOnes(nodep->lhsp(), ସତ୍ୟ);
                ଅଚଳ ତାର୍କିକ inserted = m_assignEnps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            ଯଦି (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(nullptr);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } ଅନ୍ୟଥା {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(nodep->lhsp(), Sel)) {
                    ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(selp->fromp(), ଗଣ୍ଠି_Var_Ref)) {
                        ଯଦି (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), ସତ୍ୟ));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } ଅନ୍ୟଥା {
                            m_tgraph.didProcess(nodep, ସତ୍ୟ);
                        }
                    } ଅନ୍ୟଥା {
                        m_tgraph.didProcess(nodep, ସତ୍ୟ);  // LCOV_EXCL_LINE
                    }
                } ଅନ୍ୟଥା {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, ସତ୍ୟ);
                }
            }
            m_alhs = ସତ୍ୟ;  // And user1p() will indicate tristate equation, if any
            ଯଦି (AstAssignW* ଅଚଳ assignWp = VN_CAST(nodep, AssignW)) {
                ଯଦି (AstStrengthSpec* ଅଚଳ specp = assignWp->strengthSpecp()) {
                    ଯଦି (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        ଯଦି (!isOnlyAssignmentIsToLhsVar(assignWp)) {
                            assignWp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } ଅନ୍ୟଥା {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterateAndNextNull(nodep->lhsp());
        }
    }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override { visitAssign(nodep); }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override { visitAssign(nodep); }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override { visitAssign(nodep); }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ଅଚଳ AstAlways* ଅଚଳ alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN ? nullptr : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_inAlias);
        m_inAlias = ସତ୍ୟ;
        ଯଦି (m_graphing) {
            ଯଦି (nodep->user2() & U2_GRAPHING) ଫେରନ୍ତୁ;
            m_alhs = ସତ୍ୟ;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }

    ଶୂନ୍ୟ visitCaseEq(Ast_ଗଣ୍ଠି_Biop* nodep, ତାର୍କିକ neq) {
        ଯଦି (m_graphing) {
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ଅଚଳ constp = VN_CAST(nodep->lhsp(), Const);
            ଯଦି (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enRhsp;
                ଯଦି (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                    rhsp->user1p(nullptr);
                } ଅନ୍ୟଥା {
                    enRhsp = get_En_ପଦସମଷ୍ଟି_Based_On_Originalp(rhsp);
                }
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp
                    = ନୂତନ AstLogAnd{fl, ନୂତନ AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    ନୂତନ AstEqCase{fl, ନୂତନ AstConst{fl, constp->ଅଙ୍କ()}, rhsp}};
                constp->user1p(nullptr);
                ଯଦି (neq) newp = ନୂତନ AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା ଯଦି (constp && nodep->rhsp()->user1p()) {
                File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                constp->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = ନୂତନ AstLogAnd{fl,
                                                  ନୂତନ AstEq{fl, newAllZerosOrOnes(constp, ମିଥ୍ୟା),
                                                            VN_AS(rhsp->user1p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)},
                                                  // Keep the caseeq if there are X's present
                                                  ନୂତନ AstEqCase{fl, constp, rhsp}};
                ଯଦି (neq) newp = ନୂତନ AstLogNot{fl, newp};
                rhsp->user1p(nullptr);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା {
                checkUnhandled(nodep);
            }
        }
    }
    ଶୂନ୍ୟ visitEqNeqWild(Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଯଦି (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterateAndNextNull(nodep->lhsp());
        ଯଦି (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            ଫେରନ୍ତୁ;
        }
    }
    ଶୂନ୍ୟ visit(AstEqCase* nodep) override { visitCaseEq(nodep, ମିଥ୍ୟା); }
    ଶୂନ୍ୟ visit(AstNeqCase* nodep) override { visitCaseEq(nodep, ସତ୍ୟ); }
    ଶୂନ୍ୟ visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    ଶୂନ୍ୟ visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }

    ଶୂନ୍ୟ visit(Ast_ଗଣନା_Bits* nodep) override {
        std::array<ତାର୍କିକ, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->ଅଙ୍କ().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->ଅଙ୍କ().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        ଯଦି (m_graphing) {
            iterateAndNextNull(nodep->lhsp());
            ଯଦି (!dropop[0]) iterateAndNextNull(nodep->rhsp());
            ଯଦି (!dropop[1]) iterateAndNextNull(nodep->thsp());
            ଯଦି (!dropop[2]) iterateAndNextNull(nodep->fhsp());
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nonXp = nullptr;
            ଯଦି (!dropop[0]) {
                nonXp = nodep->rhsp();
            } ଅନ୍ୟଥା ଯଦି (!dropop[1]) {
                nonXp = nodep->thsp();
            } ଅନ୍ୟଥା ଯଦି (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            ଯଦି (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                ଯଦି (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    ଫେରନ୍ତୁ;
                }
                AstVar* ଅଚଳ envarp = getCreateEnVarp(varrefp->varp(), ମିଥ୍ୟା);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = ନୂତନ Ast_ଗଣନା_Ones{
                    nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                ଯଦି (nonXp) {  // Need to still count '0 or '1 or 'x's
                    ଯଦି (dropop[0]) {
                        nodep->rhsp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                        nodep->rhsp(nonXp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
                    }
                    ଯଦି (dropop[1]) {
                        nodep->thsp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                        nodep->thsp(nonXp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
                    }
                    ଯଦି (dropop[2]) {
                        nodep->fhsp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                        nodep->fhsp(nonXp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
                    }
                    newp = ନୂତନ AstAdd{nodep->fileline(), nodep, newp};
                } ଅନ୍ୟଥା {
                    ଠେଲ_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = nullptr;
        ଯଦି (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        ଯଦି (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } ଅନ୍ୟଥା {
            ଯଦି (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.setTristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } ଅନ୍ୟଥା {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(nodep->lhsp(), Sel);
                ଅଚଳ AstConst* ଅଚଳ lsbp = selp ? VN_CAST(selp->lsbp(), Const) : nullptr;
                ଯଦି (lsbp) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = lsbp->toSInt();
                    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < selp->widthConst(); ++i) {
                        setBitPullDirection(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } ଅନ୍ୟଥା {
                    setPullDirection(varrefp->varp(), nodep);
                }
            }
        }
        ଯଦି (!m_graphing) {
            nodep->unlinkFrBack();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    ଶୂନ୍ୟ iteratePinGuts(AstPin* nodep) {
        ଯଦି (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            ଯଦି (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            // All heavy lifting completed in graph visitor.
            ଯଦି (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    ଶୂନ୍ୟ visit(AstPin* nodep) override {
        ଯଦି (m_graphing) {
            ଯଦି (nodep->user2() & U2_GRAPHING) ଫେରନ୍ତୁ;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ଅଚଳ enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            ଯଦି (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, ମିଥ୍ୟା);
                iteratePinGuts(nodep);
                ଫେରନ୍ତୁ;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ଅଚଳ ତାର୍କିକ inDeclProcessing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::ନିବେଶ
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            ଯଦି (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ଅଚଳ ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(ନୂତନ AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.setTristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } ଅନ୍ୟଥା ଯଦି (inDeclProcessing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ଅଚଳ AstAssignW* ଅଚଳ reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, ସତ୍ୟ, ସତ୍ୟ);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.setTristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ଅଚଳ enVarp = ନୂତନ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvtToStr(m_unique++),
                                              VFlagBitPacked{}, enModVarp->width()};
            ଯଦି (inDeclProcessing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->varType2In();
            } ଅନ୍ୟଥା {
                enModVarp->varType2Out();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ଅଚଳ enpinp
                = ନୂତନ AstPin{nodep->fileline(), nodep->pin_ଅଙ୍କ(),
                             enModVarp->name(),  // should be {var}"__en"
                             ନୂତନ AstVarRef{nodep->fileline(), enVarp,
                                           inDeclProcessing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->addNextHere(enpinp);
            m_modp->addStmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ଅଚଳ AstAssignW* outAssignp = nullptr;  // If reconnected, the related assignment
            AstPin* outpinp = nullptr;
            AstVar* ଅଚଳ outModVarp = m_varAux(nodep->modVarp()).outVarp;
            ଯଦି (!outModVarp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                ଯଦି (!m_modp->is_ଶୀର୍ଷ()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } ଅନ୍ୟଥା {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ outexprp = VN_AS(nodep->exprp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)
                                                  ->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);  // Note has lvalue() set
                outpinp = ନୂତନ AstPin{nodep->fileline(), nodep->pin_ଅଙ୍କ(),
                                     outModVarp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(outModVarp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->addNextHere(outpinp);
                // Simplify
                ଯଦି (inDeclProcessing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, ସତ୍ୟ};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                outAssignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        ସତ୍ୟ);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, outAssignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ଅଚଳ TristatePinVisitor visitor{nodep->exprp(), m_tgraph, ମିଥ୍ୟା};
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ inAssignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, ସତ୍ୟ);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, inAssignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            ଯଦି (!outAssignp) {
                ଯଦି (!outpinp) {
                    exprrefp = nullptr;  // Primary input only
                } ଅନ୍ୟଥା {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    ଯଦି (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } ଅନ୍ୟଥା {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(outAssignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                ଯଦି (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            ଯଦି (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(ନୂତନ AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                ଯଦି (!outAssignp) {
                    map_ଭର୍ତ୍ତି_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            ଯଦି (exprrefp) {
                AstPull* ଅଚଳ pullp = m_varAux(nodep->modVarp()).pullp;
                ଅଚଳ auto& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = nullptr;
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା selLsb = 0;
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା selWidth = 0;
                ଯଦି (outAssignp) {
                    ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(outAssignp->lhsp(), Sel)) {
                        AstVarRef* ଅଚଳ vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ଅଚଳ lsbp = VN_CAST(selp->lsbp(), Const);
                        ଯଦି (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->toSInt();
                            selWidth = selp->widthConst();
                        }
                    }
                }

                ଯଦି (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    setPullDirection(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    ଯଦି (selVarp) {
                        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା direction = pullp->direction();
                        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < selWidth; ++i) {
                            setBitPullDirection(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                ଯଦି (!srcBitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    ପାଇଁ (ଅଚଳ auto& pair : srcBitPulls) {
                        setBitPullDirection(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    ଯଦି (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        ପାଇଁ (ଅଚଳ auto& pair : srcBitPulls) {
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
        ଅନ୍ୟଥା {
            ଯଦି (nodep->user2() & U2_NONGRAPH) ଫେରନ୍ତୁ;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    ଶୂନ୍ୟ handle_ଗଣ୍ଠି_Var_Ref(Ast_ଗଣ୍ଠି_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        ଯଦି (m_graphing) {
            ଯଦି (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            ଯଦି (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            ଯଦି (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.setTristate(nodep->varp());
            }
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->user2() & U2_NONGRAPH) ଫେରନ୍ତୁ;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            ଯଦି (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                ଯଦି (m_inAlias) {
                    ଯଦି (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } ଅନ୍ୟଥା {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    ଫେରନ୍ତୁ;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_ଭର୍ତ୍ତି_Lhs_Var_Ref(nodep);
            } ଅନ୍ୟଥା ଯଦି (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                AstVar* ଅଚଳ enVarp = getCreateEnVarp(nodep->varp(), ମିଥ୍ୟା);
                ଯଦି (AstVarXRef* ଅଚଳ xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } ଅନ୍ୟଥା {
                    nodep->user1p(ନୂତନ AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override { handle_ଗଣ୍ଠି_Var_Ref(nodep); }
    ଶୂନ୍ୟ visit(AstVarXRef* nodep) override { handle_ଗଣ୍ଠି_Var_Ref(nodep); }

    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ଯଦି (m_graphing) {
            // If tri0/1 force a pullup
            ଯଦି (nodep->user2() & U2_GRAPHING) ଫେରନ୍ତୁ;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            ଯଦି (nodep->isPulldown() || nodep->isPullup()) {
                Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstPull{
                    nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->addNextHere(newp);
                // We'll iterate on the new AstPull later
            }
            ଯଦି (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.setTristate(nodep);
            }
        } ଅନ୍ୟଥା {  // !graphing
            ଯଦି (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        ଯଦି (m_processedIfaces && VN_IS(nodep, Iface)) ଫେରନ୍ତୁ;
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
        m_graphing = ମିଥ୍ୟା;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = nullptr;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = ସତ୍ୟ;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = ମିଥ୍ୟା;
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
        ଭର୍ତ୍ତି_Tristates(nodep);
        ଭର୍ତ୍ତି_Procedural_Enable_Assignments(nodep);

        ପାଇଁ (auto& assignEnp : m_assignEnps) {
            VL_DO_DANGLING(assignEnp.second->delete_ବୃକ୍ଷ(), assignEnp.second);
        }
        m_assignEnps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        // don't deal with classes
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        // don't deal with functions
    }

    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterateAndNextNull(nodep->stmtsp());
    }

    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    ଶୂନ୍ୟ combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        ପାଇଁ (ଅଚଳ std::pair<AstVar* ଅଚଳ, std::vector<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ଅଚଳ invarp = kv.first;
            ଅଚଳ std::vector<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_ଧାଡ଼ି* ଅଚଳ fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.size() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* orp = nullptr;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enp = nullptr;

            ପାଇଁ (ଅଚଳ std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ଅଚଳ contribEnp = contrib.first;
                AstVar* ଅଚଳ contribOutp = contrib.second;

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ outRefp = ନୂତନ AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enRefp = ନୂତନ AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ andp = ନୂତନ AstAnd{fl, outRefp, enRefp};

                orp = (!orp) ? andp : ନୂତନ AstOr{fl, orp, andp};

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enRef2p = ନୂତନ AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : ନୂତନ AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            ଯଦି (!envarp) {
                envarp = ନୂତନ AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->setDfgTriLowered();
                ifaceModp->addStmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            AstAssignW* ଅଚଳ enAssp
                = ନୂତନ AstAssignW{fl, ନୂତନ AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->addStmtsp(ନୂତନ AstAlways{enAssp});

            // Pull resolution
            ଅଚଳ AstPull* ଅଚଳ pullp = m_varAux(invarp).pullp;
            ଅଚଳ ତାର୍କିକ pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* undrivenp = ନୂତନ AstNot{fl, ନୂତନ AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = ନୂତନ AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = ନୂତନ AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            AstAssignW* ଅଚଳ assp
                = ନୂତନ AstAssignW{fl, ନୂତନ AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->addStmtsp(ନୂତନ AstAlways{assp});
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::vector<Ast_ଗଣ୍ଠି_Module*> ifacesp;
        ପାଇଁ (Ast_ଗଣ୍ଠି* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            ଯଦି (VN_IS(modp, Iface)) ifacesp.ଠେଲ_back(VN_AS(modp, ଗଣ୍ଠି_Module));
        }
        ପାଇଁ (auto it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = ସତ୍ୟ;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ifdef VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::vector<Ast_ଗଣ୍ଠି*> unusedRootps;
        netlistp->foreach([&](Ast_ଗଣ୍ଠି* nodep) {
            Ast_ଗଣ୍ଠି* ଅଚଳ enp = nodep->user1p();
            ଯଦି (!enp) ଫେରନ୍ତୁ;
            ଯଦି (enp->backp()) ଫେରନ୍ତୁ;
            unusedRootps.emplace_back(enp);
        });
        ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
#endif
    }
    ~TristateVisitor() override {
        V3Stats::addStat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

ଶୂନ୍ୟ V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("tristate", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
