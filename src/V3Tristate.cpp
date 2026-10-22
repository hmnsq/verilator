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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Tristate.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Inst.h"
#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग Tristate_आधारः_Visitor VL_NOT_FINAL : सार्वजनिक VNVisitor {
सार्वजनिक:
    // METHODS
};

//######################################################################
// Graph support classes

वर्ग TristateVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_ग्रन्थिः* ध्रुव m_nodep;
    तार्किक m_isTristate = असत्य;  // Logic indicates a tristate
    तार्किक m_feedsTri = असत्य;  // Propagates to a tristate node (on RHS)
    तार्किक m_processed = असत्य;  // Tristating was cleaned up
सार्वजनिक:
    TristateVertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = स्वभाव;
    // ACCESSORS
    Ast_ग्रन्थिः* nodep() ध्रुव VL_MT_STABLE { प्रत्यागम m_nodep; }
    ध्रुव AstVar* varp() ध्रुव { प्रत्यागम VN_CAST(nodep(), Var); }
    string name() ध्रुव override {
        प्रत्यागम ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvt_गन्तव्यम्_Hex(nodep())));
    }
    string dotColor() ध्रुव override {
        प्रत्यागम (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }
    तार्किक isTristate() ध्रुव VL_MT_SAFE { प्रत्यागम m_isTristate; }
    शून्य isTristate(तार्किक flag) { m_isTristate = flag; }
    तार्किक feedsTri() ध्रुव VL_MT_SAFE { प्रत्यागम m_feedsTri; }
    शून्य feedsTri(तार्किक flag) { m_feedsTri = flag; }
    तार्किक processed() ध्रुव { प्रत्यागम m_processed; }
    शून्य processed(तार्किक flag) { m_processed = flag; }
};

//######################################################################

वर्ग TristateGraph final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
सार्वजनिक:
    उपयुज्य VarVec = std::सदिशम्<AstVar*>;

गुप्त:
    // MEMBERS
    V3Graph m_graph;  // Logic graph

सार्वजनिक:
    // CONSTRUCTORS
    TristateGraph() { clearAndCheck(); }
    आभास ~TristateGraph() { clearAndCheck(); }
    VL_UNCOPYABLE(TristateGraph);

गुप्त:
    // METHODS

    TristateVertex* makeVertex(Ast_ग्रन्थिः* nodep) {
        TristateVertex* vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        यदि (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = नूतन TristateVertex{&m_graph, nodep};
            nodep->user4p(vertexp);
        }
        प्रत्यागम vertexp;
    }

    // METHODS - Graph optimization
    शून्य graphWalkRecurseFwd(TristateVertex* vtxp, पूर्णाङ्क level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        यदि (!vtxp->isTristate()) प्रत्यागम;  // tristate involved
        यदि (vtxp->user() == 1) प्रत्यागम;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        यदि (!vtxp->varp()) {  // not a var where we stop the recursion
            प्रति (V3GraphEdge& edge : vtxp->बहिर्गमनम्_धाराः()) {
                TristateVertex* ध्रुव vvertexp = static_cast<TristateVertex*>(edge.शिखरम्());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                यदि (!vvertexp->isTristate()) {
                    vvertexp->isTristate(सत्य);
                    graphWalkRecurseFwd(vvertexp, level + 1);
                }
            }
        } अन्यथा {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            प्रति (V3GraphEdge& edge : vtxp->अन्तर्_धाराः()) {
                TristateVertex* ध्रुव vvertexp = static_cast<TristateVertex*>(edge.fromp());
                यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    यदि (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(सत्य);
                        graphWalkRecurseFwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    शून्य graph_Walk_Recurse_पृष्ठभागः(TristateVertex* vtxp, पूर्णाङ्क level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        यदि (!(vtxp->isTristate() || vtxp->feedsTri())) प्रत्यागम;  // tristate involved
        यदि (vtxp->user() == 3) प्रत्यागम;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        यदि (!vtxp->varp()) {  // not a var where we stop the recursion
            प्रति (V3GraphEdge& edge : vtxp->अन्तर्_धाराः()) {
                TristateVertex* ध्रुव vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                यदि (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(सत्य);
                    graph_Walk_Recurse_पृष्ठभागः(vvertexp, level + 1);
                }
            }
        }
    }

सार्वजनिक:
    // METHODS
    तार्किक रिक्तानि() ध्रुव { प्रत्यागम m_graph.रिक्तानि(); }
    शून्य clearAndCheck() {
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            ध्रुव TristateVertex& vvertex = static_cast<TristateVertex&>(vtx);
            यदि (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_graph.clear();
        Ast_ग्रन्थिः::user4_Clear_वृक्षः();  // Wipe all node user4p's that point to vertexes
    }
    शून्य graphWalk(Ast_ग्रन्थिः_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            graph_Walk_Recurse_पृष्ठभागः(static_cast<TristateVertex*>(&vtx), 0);
        }
        यदि (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    शून्य associate(Ast_ग्रन्थिः* fromp, Ast_ग्रन्थिः* शिखरम्) {
        नूतन V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(शिखरम्), 1};
    }
    शून्य deleteVerticesFromSubtreeRecurse(Ast_ग्रन्थिः* nodep) {
        यदि (!nodep) प्रत्यागम;
        // Skip vars, because they may be connected to more than one varref
        यदि (!VN_IS(nodep, Var)) {
            TristateVertex* ध्रुव vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            यदि (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    शून्य समुच्चयः_Tristate(Ast_ग्रन्थिः* nodep) { makeVertex(nodep)->isTristate(सत्य); }
    तार्किक isTristate(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव TristateVertex* ध्रुव vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        प्रत्यागम vertexp && vertexp->isTristate();
    }
    तार्किक feedsTri(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव TristateVertex* ध्रुव vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        प्रत्यागम vertexp && vertexp->feedsTri();
    }
    शून्य didProcess(Ast_ग्रन्थिः* nodep, तार्किक quiet = असत्य) {
        यदि (TristateVertex* ध्रुव vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(सत्य);
        } अन्यथा यदि (!quiet) {
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
        प्रति (ध्रुव V3GraphVertex& vtx : m_graph.vertices()) {
            ध्रुव TristateVertex& vvertex = static_cast<ध्रुव TristateVertex&>(vtx);
            यदि (vvertex.isTristate()) {
                यदि (AstVar* ध्रुव nodep = VN_CAST(vvertex.nodep(), Var)) v.प्रक्षिप_पृष्ठभागः(nodep);
            }
        }
        प्रत्यागम v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

वर्ग TristatePinVisitor final : सार्वजनिक Tristate_आधारः_Visitor {
    TristateGraph& m_tgraph;
    ध्रुव तार्किक m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        यदि (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } अन्यथा यदि (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.समुच्चयः_Tristate(nodep->varp());
        }
    }
    शून्य दर्शय(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_खण्डिका_Sel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_ग्रन्थिः* nodep, TristateGraph& tgraph, तार्किक lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = स्वभाव;
};

//######################################################################

वर्ग TristateVisitor final : सार्वजनिक Tristate_आधारः_Visitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;
    ध्रुव VNUser3_अन्तर्_Use m_inuser3;
    ध्रुव VNUser4_अन्तर्_Use m_inuser4;

    संरचना AuxAstVar final {
        AstPull* pullp = शून्यसूचकः;  // pullup/pulldown direction (whole variable)
        AstVar* बहिर्गमनम्_Varp = शून्यसूचकः;  // output __out var
        तार्किक ifaceTristate = असत्य;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<पूर्णाङ्क, पूर्णाङ्क>
            bitPulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    संरचना RefStrength final {
        Ast_ग्रन्थिः_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_ग्रन्थिः_नियोजय* m_assignp;  // Assignment containing m_varrefp
        ध्रुव Ast_ग्रन्थिः_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_ग्रन्थिः_व्यञ्जकम्* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_ग्रन्थिः_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_ग्रन्थिः_व्यञ्जकम्* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_ग्रन्थिः_Var_Ref* varrefp, Ast_ग्रन्थिः_नियोजय* assignp,
                    ध्रुव Ast_ग्रन्थिः_Procedure* procedurep, Ast_ग्रन्थिः_व्यञ्जकम्* enLhsp,
                    Ast_ग्रन्थिः_Var_Ref* enVarrefp, Ast_ग्रन्थिः_व्यञ्जकम्* enRhsp, VStrength strength,
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
    संरचना LhsProjection final {
        Ast_ग्रन्थिः_व्यञ्जकम्* m_lhsp;  // LHS giving the enable assign
        Ast_ग्रन्थिः_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_ग्रन्थिः_व्यञ्जकम्* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    संरचना ProceduralEnable final {
        Ast_ग्रन्थिः_व्यञ्जकम्* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_ग्रन्थिः_व्यञ्जकम्* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    उपयुज्य RefStrengthVec = std::सदिशम्<RefStrength>;
    उपयुज्य VarMap = std::map<AstVar*, RefStrengthVec*>;
    उपयुज्य Assigns = std::सदिशम्<Ast_नियोजय_W*>;
    उपयुज्य Var_गन्तव्यम्_Assigns_Map = std::map<AstVar*, Assigns>;
    उपयुज्य नियोजय_Enable_Map = std::map<Ast_ग्रन्थिः_नियोजय*, Ast_ग्रन्थिः_व्यञ्जकम्*>;
    उपयुज्य ProceduralEnableMap = std::map<Ast_ग्रन्थिः_नियोजय*, std::सदिशम्<ProceduralEnable>>;
    गणना : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    तार्किक m_graphing = असत्य;  // Major mode - creating graph
    //
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    AstCell* m_cellp = शून्यसूचकः;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    Var_गन्तव्यम्_Assigns_Map m_assigns;  // Assigns in current module
    नियोजय_Enable_Map m_नियोजय_Enps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    पूर्णाङ्क m_unique = 0;
    तार्किक m_alhs = असत्य;  // On LHS of assignment
    तार्किक m_अन्तर्_Alias = असत्य;  // Inside alias statement
    तार्किक m_processedIfaces = असत्य;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = शून्यसूचकः;  // Delay of assignment currently collecting LHS drivers
    ध्रुव Ast_ग्रन्थिः* m_logicp = शून्यसूचकः;  // Current logic being built
    Ast_ग्रन्थिः_नियोजय* m_assignp = शून्यसूचकः;  // Current assignment
    ध्रुव Ast_ग्रन्थिः_Procedure* m_procedurep = शून्यसूचकः;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::सदिशम्<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ध्रुव {
        string o = (m_graphing ? " gr " : " ng ");
        यदि (m_alhs) o += "alhs ";
        प्रत्यागम o;
    }
    शून्य mod_सङ्कलय_Stmtp(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* newp) {
        यदि (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } अन्यथा {
            m_modp->सङ्कलय_Stmtsp(newp);
        }
    }
    शून्य associateLogic(Ast_ग्रन्थिः* fromp, Ast_ग्रन्थिः* शिखरम्) {
        यदि (m_logicp) m_tgraph.associate(fromp, शिखरम्);
    }
    AstConst* newAllZerosOrOnes(Ast_ग्रन्थिः* nodep, तार्किक ones) {
        V3_सङ्ख्या अङ्कः{nodep, nodep->विस्तारः()};
        यदि (ones) अङ्कः.समुच्चयः_All_Bits1();
        AstConst* ध्रुव newp = नूतन AstConst{nodep->fileline(), अङ्कः};
        प्रत्यागम newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_पङ्क्तिः* fl, AstVar* varp, ध्रुव string& dotted, VAccess access,
                           ध्रुव string& inlinedDots) {
        AstVarXRef* ध्रुव xrefp = नूतन AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        प्रत्यागम xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_ग्रन्थिः_Module* modp, ध्रुव string& dotted_मार्गः) {
        यदि (dotted_मार्गः.रिक्तानि()) प्रत्यागम शून्यसूचकः;
        ध्रुव string firstComp = dotted_मार्गः.substr(0, dotted_मार्गः.find('.'));
        प्रति (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ध्रुव ivarp = VN_CAST(stmtp, Var);
            यदि (!ivarp || ivarp->name() != firstComp) अनुवर्त;
            ध्रुव AstIfaceRefDType* ध्रुव ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            यदि (ifaceDtp && ifaceDtp->modportp()) प्रत्यागम ifaceDtp->modportp();
            भङ्ग;
        }
        प्रत्यागम शून्यसूचकः;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* getEnp(Ast_ग्रन्थिः* nodep) {
        यदि (nodep->user1p()) {
            यदि (Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
                यदि (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    प्रत्यागम newAllZerosOrOnes(nodep, सत्य);
                }
            }
        } अन्यथा {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, सत्य));
        }
        // Otherwise return the previous output enable
        प्रत्यागम VN_AS(nodep->user1p(), ग्रन्थिः_व्यञ्जकम्);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    शून्य सङ्कलय_Modport_Var_Ref_If_Missing(AstModport* modportp, AstVar* varp, VDirection dir) {
        प्रति (Ast_ग्रन्थिः* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ध्रुव mvrp = VN_CAST(mrp, ModportVarRef);
            यदि (mvrp && mvrp->varp() == varp) प्रत्यागम;  // Already present
        }
        AstModportVarRef* ध्रुव mvrp = नूतन AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->सङ्कलय_Varsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, तार्किक is_शिखरम्) {
        // Return the master __en for the specified input variable
        यदि (!invarp->user1p()) {
            AstVar* ध्रुव newp
                = नूतन AstVar{invarp->fileline(), is_शिखरम् ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_ग्रन्थिः_Module* ध्रुव ownerModp = findParentModule(invarp);
            यदि (ownerModp && ownerModp != m_modp) {
                ownerModp->सङ्कलय_Stmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                प्रति (Ast_ग्रन्थिः* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ध्रुव modportp = VN_CAST(stmtp, Modport);
                    यदि (!modportp) अनुवर्त;
                    प्रति (Ast_ग्रन्थिः* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ध्रुव mvrp = VN_CAST(mrp, ModportVarRef);
                        यदि (mvrp && mvrp->varp() == invarp) {
                            सङ्कलय_Modport_Var_Ref_If_Missing(modportp, newp, VDirection::निवेशः);
                            भङ्ग;
                        }
                    }
                }
            } अन्यथा {
                mod_सङ्कलय_Stmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        प्रत्यागम VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ध्रुव constp) {
        File_पङ्क्तिः* ध्रुव fl = constp->fileline();
        V3_सङ्ख्या numz{constp, constp->विस्तारः()};
        numz.op_द्वयङ्काः_Z(constp->अङ्कः());  // Z->1, else 0
        V3_सङ्ख्या numz0{constp, constp->विस्तारः()};
        numz0.opNot(numz);  // Z->0, else 1
        प्रत्यागम नूतन AstConst{fl, numz0};
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* get_En_व्यञ्जकम्_Based_On_Originalp(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
        यदि (AstVarRef* ध्रुव varrefp = VN_CAST(nodep, VarRef)) {
            प्रत्यागम नूतन AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), असत्य),
                                 VAccess::READ};
        } अन्यथा यदि (AstVarXRef* ध्रुव xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ध्रुव enVarp = getCreateEnVarp(xrefp->varp(), असत्य);
            प्रत्यागम newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } अन्यथा यदि (AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            प्रत्यागम getNonZConstp(constp);
        } अन्यथा यदि (AstExtend* ध्रुव extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = extendp->lhsp()->unlink_Fr_पृष्ठभागः();
            AstExtend* ध्रुव enExtendp = extendp->clone_वृक्षः(असत्य);
            extendp->lhsp(lhsp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enLhsp = get_En_व्यञ्जकम्_Based_On_Originalp(lhsp);
            enExtendp->lhsp(नूतन AstNot{enLhsp->fileline(), enLhsp});
            प्रत्यागम नूतन AstNot{enExtendp->fileline(), enExtendp};
        } अन्यथा यदि (AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = selp->fromp()->unlink_Fr_पृष्ठभागः();
            AstSel* ध्रुव enSelp = selp->clone_वृक्षः(असत्य);
            selp->fromp(fromp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enFromp = get_En_व्यञ्जकम्_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            प्रत्यागम enSelp;
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            प्रत्यागम शून्यसूचकः;
        }
    }
    AstVar* get_Create_बहिर्गमनम्_Varp(AstVar* invarp, तार्किक is_शिखरम्) {
        // Return the master __out for the specified input variable
        यदि (!m_varAux(invarp).बहिर्गमनम्_Varp) {
            AstVar* ध्रुव newp
                = नूतन AstVar{invarp->fileline(), is_शिखरम् ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            mod_सङ्कलय_Stmtp(invarp, newp);
            m_varAux(invarp).बहिर्गमनम्_Varp = newp;  // find outvar given invarp
        }
        प्रत्यागम m_varAux(invarp).बहिर्गमनम्_Varp;
    }
    AstVar* getCreateUnconnVarp(Ast_ग्रन्थिः* fromp, Ast_ग्रन्थिः_DType* dtypep) {
        AstVar* ध्रुव newp = नूतन AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvt_गन्तव्यम्_Str(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        mod_सङ्कलय_Stmtp(newp, newp);
        प्रत्यागम newp;
    }

    LhsProjection newLhsProjection(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_Var_Ref* targetp, Ast_ग्रन्थिः_व्यञ्जकम्* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newLhsp = lhsp->clone_वृक्षः_विशुद्धम्(असत्य);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_ग्रन्थिः* nodep) { nodep->user1p(शून्यसूचकः); });
        Ast_ग्रन्थिः_Var_Ref* ध्रुव newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        प्रत्यागम LhsProjection{newLhsp, newVarrefp, enp};
    }

    शून्य map_निवेशय_Lhs_Var_Ref(Ast_ग्रन्थिः_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ध्रुव कुञ्चिका = nodep->varp();
        ध्रुव स्वचालित pair = m_lhsmap.emplace(कुञ्चिका, शून्यसूचकः);
        यदि (pair.second) pair.first->second = नूतन RefStrengthVec;
        Ast_ग्रन्थिः_व्यञ्जकम्* enLhsp = शून्यसूचकः;
        Ast_ग्रन्थिः_Var_Ref* enVarrefp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* enRhsp = शून्यसूचकः;
        ध्रुव स्वचालित enIt = m_नियोजय_Enps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ध्रुव तार्किक canCombineProcedural = !VN_IS(कुञ्चिका->dtypep()->skipRefp(), ग्रन्थिः_UOr_Struct_DType);
        ध्रुव Ast_ग्रन्थिः_Procedure* ध्रुव procedurep
            = enIt != m_नियोजय_Enps.अन्तः_2() && canCombineProcedural ? m_procedurep : शून्यसूचकः;
        यदि (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_वृक्षः_विशुद्धम्(असत्य));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->प्रक्षिप_पृष्ठभागः(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newEnableDeposit(AstSel* selp, Ast_ग्रन्थिः_व्यञ्जकम्* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन Ast_सरणम्_L{
            selp->fileline(), नूतन AstExtend{selp->fileline(), enp, selp->fromp()->विस्तारः()},
            selp->lsbp()->clone_वृक्षः_विशुद्धम्(असत्य), selp->fromp()->विस्तारः()};
        प्रत्यागम newp;
    }

    शून्य समुच्चयः_Pull_Direction(AstVar* varp, AstPull* pullp) {
        ध्रुव AstPull* ध्रुव oldpullp = m_varAux(varp).pullp;
        यदि (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } अन्यथा {
            यदि (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    शून्य समुच्चयः_Bit_Pull_Direction(AstVar* varp, पूर्णाङ्क bit_सूचकांक, पूर्णाङ्क direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        स्वचालित& bitPulls = m_varAux(varp).bitPulls;
        स्वचालित it = bitPulls.find(bit_सूचकांक);
        यदि (it != bitPulls.अन्तः_2() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bit_सूचकांक << " of " << varp->prettyNameQ());
        }
        bitPulls[bit_सूचकांक] = direction;
    }

    AstConst* create_प्रत्येकम्_Bit_Pull_Const(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ध्रुव स्वचालित& bitPulls = m_varAux(varp).bitPulls;
        V3_सङ्ख्या अङ्कः{varp, varp->विस्तारः(), 0};  // Start with all zeros
        प्रति (ध्रुव स्वचालित& pair : bitPulls) {
            यदि (pair.second == 1) अङ्कः.समुच्चयः_Bit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        प्रत्यागम नूतन AstConst{varp->fileline(), अङ्कः};
    }

    तार्किक has_प्रत्येकम्_Bit_Pulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        प्रत्यागम !m_varAux(varp).bitPulls.रिक्तानि();
    }

    शून्य checkUnhandled(Ast_ग्रन्थिः* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        यदि (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        यदि ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    शून्य निवेशय_Tristates(Ast_ग्रन्थिः_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ध्रुव TristateGraph::VarVec vars = m_tgraph.tristateVars();
        प्रति (स्वचालित varp : vars) {
            यदि (!m_tgraph.isTristate(varp)) अनुवर्त;
            ध्रुव स्वचालित it = m_lhsmap.find(varp);
            यदि (it != m_lhsmap.अन्तः_2()) अनुवर्त;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ध्रुव AstPull* ध्रुव pullp = m_varAux(varp).pullp;
            ध्रुव तार्किक pull_मानम् = pullp && pullp->direction() == 1;
            AstConst* ध्रुव constp = newAllZerosOrOnes(varp, pull_मानम्);
            AstVarRef* ध्रुव varrefp = नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            Ast_नियोजय_W* ध्रुव newp = नूतन Ast_नियोजय_W{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, असत्य));  // en=0 for floating/pullup vars
            nodep->सङ्कलय_Stmtsp(नूतन AstAlways{newp});
            map_निवेशय_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        प्रति (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ध्रुव std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            यदि (it == m_lhsmap.अन्तः_2()) अनुवर्त;
            AstVar* ध्रुव invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            यदि (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_ग्रन्थिः_Module* ध्रुव ownerModp = findParentModule(invarp);
                ध्रुव तार्किक isCrossModule = ownerModp && ownerModp != nodep;
                ध्रुव तार्किक isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                यदि (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    प्रति (ध्रुव RefStrength& rs : *refsp) {
                        यदि (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            भङ्ग;
                        }
                    }
                } अन्यथा यदि (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = सत्य;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    संरचना PartitionInfo final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, PartitionInfo> partitions;
                    प्रति (ध्रुव RefStrength& rs : *refsp) {
                        यदि (AstVarXRef* ध्रुव xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            PartitionInfo& pi = partitions[xrefp->dotted()];
                            pi.refs.प्रक्षिप_पृष्ठभागः(rs);
                            यदि (pi.inlinedDots.रिक्तानि()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } अन्यथा {
                            partitions[""].refs.प्रक्षिप_पृष्ठभागः(rs);
                        }
                    }
                    प्रति (स्वचालित& kv : partitions) {
                        निवेशय_Tristates_Signal(nodep, invarp, &kv.second.refs, सत्य, kv.first,
                                              kv.second.inlinedDots,
                                              findModportForDotted(nodep, kv.first));
                    }
                } अन्यथा यदि (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = सत्य;
                    निवेशय_Tristates_Signal(nodep, invarp, refsp, सत्य, "", "", शून्यसूचकः);
                } अन्यथा {
                    निवेशय_Tristates_Signal(nodep, invarp, refsp, असत्य, "", "", शून्यसूचकः);
                }
            } अन्यथा {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(विलोपय refsp, refsp);
        }
    }

    शून्य aggregateTriSameStrength(Ast_ग्रन्थिः_Module* nodep, AstVar* ध्रुव varp, AstVar* ध्रुव envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator अन्तः_Strength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        संरचना Driver_गणः final {
            ध्रुव Ast_ग्रन्थिः_Procedure* m_procedurep;  // Process owning these assignments
            std::सदिशम्<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::सदिशम्<Driver_गणः> drivers;
        std::map<ध्रुव Ast_ग्रन्थिः_Procedure*, आकार_t> procedure_गन्तव्यम्_Driver;
        प्रति (स्वचालित it = beginStrength; it != अन्तः_Strength; ++it) {
            यदि (it->m_procedurep) {
                ध्रुव स्वचालित emplaced = procedure_गन्तव्यम्_Driver.emplace(it->m_procedurep, drivers.आकार());
                यदि (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.प्रक्षिप_पृष्ठभागः(&*it);
                    अनुवर्त;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.प्रक्षिप_पृष्ठभागः(Driver_गणः{it->m_procedurep, {&*it}});
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* orp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* enp = शून्यसूचकः;

        प्रति (Driver_गणः& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ध्रुव newLhsp = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvt_गन्तव्यम्_Str(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->समुच्चयः_Dfg_Tri_Lowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->सङ्कलय_Stmtsp(newLhsp);
            AstVar* ध्रुव newEnLhsp
                = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvt_गन्तव्यम्_Str(m_unique++), envarp};  // 2-state ok
            newEnLhsp->समुच्चयः_Dfg_Tri_Lowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->सङ्कलय_Stmtsp(newEnLhsp);

            प्रति (RefStrength* ध्रुव refStrengthp : driver.m_refsp) {
                Ast_ग्रन्थिः_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                यदि (VN_IS(refp, VarXRef)) {
                    AstVarRef* ध्रुव localRefp
                        = नूतन AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(शून्यसूचकः);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } अन्यथा {
                    refp->varp(newLhsp);
                }

                यदि (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_ग्रन्थिः_व्यञ्जकम्* enLhsp = refStrengthp->m_enLhsp;
                    Ast_ग्रन्थिः_Var_Ref* ध्रुव enVarrefp = refStrengthp->m_enVarrefp;
                    यदि (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ध्रुव localRefp
                            = नूतन AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        यदि (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_वृक्षः(), enVarrefp);
                        } अन्यथा {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(प्रक्षिप_Deletep(enVarrefp), enVarrefp);
                        }
                    } अन्यथा {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].प्रक्षिप_पृष्ठभागः(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = शून्यसूचकः;
                    refStrengthp->m_enVarrefp = शून्यसूचकः;
                    refStrengthp->m_enRhsp = शून्यसूचकः;
                } अन्यथा {
                    Ast_नियोजय_W* ध्रुव enLhspAssignp = नूतन Ast_नियोजय_W{
                        refp->fileline(),
                        नूतन AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    यदि (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_वृक्षः(असत्य));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->सङ्कलय_Stmtsp(नूतन AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_पङ्क्तिः* ध्रुव fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ref1p = नूतन AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ref2p = नूतन AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव andp = नूतन AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : नूतन AstOr{fl, orp, andp};

            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ref3p = नूतन AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : नूतन AstOr{ref3p->fileline(), enp, ref3p};
        }
        Ast_नियोजय_W* ध्रुव assp = नूतन Ast_नियोजय_W{
            varp->fileline(), नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->सङ्कलय_Stmtsp(नूतन AstAlways{assp});

        Ast_नियोजय_W* ध्रुव enAssp = नूतन Ast_नियोजय_W{
            envarp->fileline(), नूतन AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->सङ्कलय_Stmtsp(नूतन AstAlways{enAssp});
    }

    शून्य निवेशय_Procedural_Enable_Assignments(Ast_ग्रन्थिः_Module* nodep) {
        प्रति (स्वचालित& नियोजय_Enables : m_proceduralEnables) {
            Ast_ग्रन्थिः_नियोजय* ध्रुव assignp = नियोजय_Enables.first;
            Ast_ग्रन्थिः_व्यञ्जकम्* combinedLhsp = assignp->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* combinedRhsp = assignp->rhsp()->unlink_Fr_पृष्ठभागः();
            प्रति (ProceduralEnable& enable : नियोजय_Enables.second) {
                combinedLhsp = नूतन AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = नूतन AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            यदि (!VN_IS(assignp, नियोजय_Dly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ध्रुव sampledp
                    = नूतन AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvt_गन्तव्यम्_Str(m_unique++), combinedRhsp->dtypep()};
                nodep->सङ्कलय_Stmtsp(sampledp);
                assignp->lhsp(नूतन AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                Ast_नियोजय* ध्रुव applyp
                    = नूतन Ast_नियोजय{assignp->fileline(), combinedLhsp,
                                    नूतन AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->सङ्कलय_अग्र_अत्र(applyp);
            } अन्यथा {
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
    शून्य निवेशय_Tristates_Signal(Ast_ग्रन्थिः_Module* nodep, AstVar* ध्रुव invarp, RefStrengthVec* refsp,
                               तार्किक isIfaceTri, ध्रुव string& iface_Dotted_मार्गः,
                               ध्रुव string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        AstVar* envarp = शून्यसूचकः;
        AstVar* outvarp = शून्यसूचकः;  // __out
        AstVar* lhsp = invarp;  // Variable to assign drive-value to (<in> or __out)
        ध्रुव तार्किक is_शिखरम्_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_शिखरम्();
        यदि (!isIfaceTri) {
            यदि (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().रिक्तानि())
                 && is_शिखरम्_Inout)
                || (!nodep->is_शिखरम्() && invarp->isIO())) {
                // This var becomes an input
                invarp->var_Type2_अन्तर्();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = get_Create_बहिर्गमनम्_Varp(invarp, is_शिखरम्_Inout);
                outvarp->var_Type2_बहिर्गमनम्();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_शिखरम्_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                यदि (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                यदि (!m_varAux(invarp).bitPulls.रिक्तानि()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } अन्यथा यदि (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* orp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* enp = शून्यसूचकः;

        std::क्रमय(refsp->begin(), refsp->अन्तः_2(),
                  [](RefStrength a, RefStrength b) { प्रत्यागम a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        यावत् (beginStrength != refsp->अन्तः_2()) {
            RefStrengthVec::iterator अन्तः_Strength = beginStrength + 1;
            यावत् (अन्तः_Strength != refsp->अन्तः_2()
                   && अन्तः_Strength->m_strength == beginStrength->m_strength)
                अन्तः_Strength++;

            File_पङ्क्तिः* ध्रुव fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strength_उपसर्गः;
            यदि (isIfaceTri && !iface_Dotted_मार्गः.रिक्तानि()) {
                strength_उपसर्गः = iface_Dotted_मार्गः;
                std::replace(strength_उपसर्गः.begin(), strength_उपसर्गः.अन्तः_2(), '.', '_');
                strength_उपसर्गः += "__";
            }
            ध्रुव string strengthVarName
                = strength_उपसर्गः + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ध्रुव varStrengthp = नूतन AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->समुच्चयः_Dfg_Tri_Lowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->सङ्कलय_Stmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ध्रुव enVarStrengthp = नूतन AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->समुच्चयः_Dfg_Tri_Lowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->सङ्कलय_Stmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     अन्तः_Strength);

            Ast_ग्रन्थिः_व्यञ्जकम्* व्यञ्जकम्_Current_Strengthp;
            यदि (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                व्यञ्जकम्_Current_Strengthp
                    = नूतन AstAnd{fl, नूतन AstVarRef{fl, varStrengthp, VAccess::READ},
                                 नूतन AstNot{fl, enp->clone_वृक्षः_विशुद्धम्(असत्य)}};
            } अन्यथा {
                व्यञ्जकम्_Current_Strengthp = नूतन AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? व्यञ्जकम्_Current_Strengthp : नूतन AstOr{fl, orp, व्यञ्जकम्_Current_Strengthp};

            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enVarStrengthRefp
                = नूतन AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : नूतन AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = अन्तः_Strength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        यदि (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->समुच्चयः_Dfg_Allow_Multidrive_Tri();
            Ast_ग्रन्थिः_Module* ध्रुव ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ध्रुव पूर्णाङ्क contribIdx = static_cast<पूर्णाङ्क>(m_ifaceContribs[invarp].आकार());
            File_पङ्क्तिः* ध्रुव fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ध्रुव contribOutp = नूतन AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvt_गन्तव्यम्_Str(contribIdx), invarp};
            contribOutp->समुच्चयः_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->सङ्कलय_Stmtsp(contribOutp);

            AstVar* ध्रुव contribEnp = नूतन AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvt_गन्तव्यम्_Str(contribIdx), invarp};
            contribEnp->समुच्चयः_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->सङ्कलय_Stmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            यदि (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                सङ्कलय_Modport_Var_Ref_If_Missing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                सङ्कलय_Modport_Var_Ref_If_Missing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_ग्रन्थिः_Var_Ref* ध्रुव lhsp
                    = iface_Dotted_मार्गः.रिक्तानि() ? static_cast<Ast_ग्रन्थिः_Var_Ref*>(
                                                    नूतन AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_ग्रन्थिः_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, iface_Dotted_मार्गः,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_नियोजय_W* ध्रुव assp = नूतन Ast_नियोजय_W{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->सङ्कलय_Stmtsp(नूतन AstAlways{assp});
            }
            {
                Ast_ग्रन्थिः_Var_Ref* ध्रुव lhsp
                    = iface_Dotted_मार्गः.रिक्तानि() ? static_cast<Ast_ग्रन्थिः_Var_Ref*>(
                                                    नूतन AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_ग्रन्थिः_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, iface_Dotted_मार्गः,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_नियोजय_W* ध्रुव assp = नूतन Ast_नियोजय_W{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->सङ्कलय_Stmtsp(नूतन AstAlways{assp});
            }

            m_ifaceContribs[invarp].प्रक्षिप_पृष्ठभागः({contribEnp, contribOutp});
            प्रत्यागम;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ध्रुव तार्किक hasPull = m_varAux(lhsp).pullp || has_प्रत्येकम्_Bit_Pulls(lhsp);
        यदि (!outvarp || hasPull) {
            Ast_ग्रन्थिः_व्यञ्जकम्* undrivenp;
            यदि (envarp) {
                undrivenp = नूतन AstNot{envarp->fileline(),
                                       नूतन AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } अन्यथा यदि (enp) {
                undrivenp = नूतन AstNot{enp->fileline(), enp};
                enp = शून्यसूचकः;  // moved into undrivenp
            } अन्यथा {
                undrivenp = newAllZerosOrOnes(invarp, सत्य);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            यदि (has_प्रत्येकम्_Bit_Pulls(lhsp)) {
                pullConstp = create_प्रत्येकम्_Bit_Pull_Const(lhsp);
            } अन्यथा {
                ध्रुव AstPull* ध्रुव pullp = m_varAux(lhsp).pullp;
                ध्रुव तार्किक pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = नूतन AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? नूतन AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        यदि (envarp && !enp) enp = newAllZerosOrOnes(invarp, असत्य);

        यदि (envarp) {
            Ast_नियोजय_W* ध्रुव enAssp = नूतन Ast_नियोजय_W{
                enp->fileline(), नूतन AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->सङ्कलय_Stmtsp(नूतन AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        Ast_नियोजय_W* ध्रुव assp = नूतन Ast_नियोजय_W{
            lhsp->fileline(), नूतन AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->सङ्कलय_Stmtsp(नूतन AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        यदि ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().रिक्तानि()) && is_शिखरम्_Inout) {
            यदि (envarp) {
                envarp->primaryIO(सत्य);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            यदि (outvarp) {
                outvarp->primaryIO(सत्य);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            यदि (!v3Global.opt.libCreate().रिक्तानि()) {
                ध्रुव पूर्णाङ्क id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(सत्य);
            }
        }
    }

    तार्किक is_Only_Assignment_Is_गन्तव्यम्_Lhs_Var(ध्रुव Ast_नियोजय_W* ध्रुव nodep) {
        यदि (ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            स्वचालित प्राप्तम्_It = m_assigns.find(varRefp->varp());
            यदि (प्राप्तम्_It != m_assigns.अन्तः_2()) {
                स्वचालित ध्रुव& assigns_गन्तव्यम्_Var = प्राप्तम्_It->second;
                यदि (assigns_गन्तव्यम्_Var.आकार() == 1 && assigns_गन्तव्यम्_Var[0] == nodep) प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }

    शून्य सङ्कलय_गन्तव्यम्_Assignment_List(Ast_नियोजय_W* nodep) {
        यदि (AstVarRef* ध्रुव varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            यदि (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].प्रक्षिप_पृष्ठभागः(nodep);
            } अन्यथा यदि (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
            }
        } अन्यथा यदि (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ध्रुव Ast_नियोजय_W* ध्रुव nodep, तार्किक मानम्) {
        यदि (AstStrengthSpec* ध्रुव strengthSpec = nodep->strengthSpecp()) {
            प्रत्यागम मानम् ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        प्रत्यागम VStrength::STRONG;  // default strength is strong
    }

    तार्किक assignment_सम्बन्धि_मानम्_On_All_द्वयङ्काः(ध्रुव Ast_नियोजय_W* ध्रुव nodep, तार्किक मानम्) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->rhsp(), Const)) {
            ध्रुव V3_सङ्ख्या अङ्कः = constp->अङ्कः();
            प्रत्यागम मानम् ? अङ्कः.isEqAllOnes() : अङ्कः.isEqZero();
        }
        प्रत्यागम असत्य;
    }

    Ast_नियोजय_W* get_Strongest_Assignment_सम्बन्धि_मानम्(ध्रुव Assigns& assigns, तार्किक मानम्) {
        स्वचालित maxIt = std::max_element(
            assigns.begin(), assigns.अन्तः_2(), [&](ध्रुव Ast_नियोजय_W* ap, ध्रुव Ast_नियोजय_W* bp) {
                ध्रुव तार्किक मानानि_On_Rhs_A = assignment_सम्बन्धि_मानम्_On_All_द्वयङ्काः(ap, मानम्);
                ध्रुव तार्किक मानानि_On_Rhs_B = assignment_सम्बन्धि_मानम्_On_All_द्वयङ्काः(bp, मानम्);
                यदि (!मानानि_On_Rhs_A) प्रत्यागम मानानि_On_Rhs_B;
                यदि (!मानानि_On_Rhs_B) प्रत्यागम असत्य;
                प्रत्यागम getStrength(ap, मानम्) < getStrength(bp, मानम्);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        प्रत्यागम assignment_सम्बन्धि_मानम्_On_All_द्वयङ्काः(*maxIt, मानम्) ? *maxIt : शून्यसूचकः;
    }

    तार्किक isAssignmentNotStrongerThanStrength(ध्रुव Ast_नियोजय_W* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ध्रुव uint8_t strength0 = getStrength(assignp, 0);
        ध्रुव uint8_t strength1 = getStrength(assignp, 1);
        प्रत्यागम (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_सम्बन्धि_मानम्_On_All_द्वयङ्काः(assignp, 0))
               || (strength1 <= strength && assignment_सम्बन्धि_मानम्_On_All_द्वयङ्काः(assignp, 1));
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* new_Merge_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp, File_पङ्क्तिः* ध्रुव fl,
                              तार्किक isWor) {
        Ast_ग्रन्थिः_व्यञ्जकम्* व्यञ्जकम् = शून्यसूचकः;
        यदि (isWor)
            व्यञ्जकम् = नूतन AstOr{fl, lhsp, rhsp};
        अन्यथा
            व्यञ्जकम् = नूतन AstAnd{fl, lhsp, rhsp};
        प्रत्यागम व्यञ्जकम्;
    }

    शून्य mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        प्रति (स्वचालित& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            यदि (assigns.आकार() <= 1) अनुवर्त;
            AstVar* ध्रुव varp = varpAssigns.first;
            यदि (!varp->isWiredNet()) अनुवर्त;
            स्वचालित it = assigns.begin();
            Ast_नियोजय_W* ध्रुव नियोजय_Wp0 = *it;
            File_पङ्क्तिः* ध्रुव fl = नियोजय_Wp0->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* wExp = शून्यसूचकः;
            यावत् (++it != assigns.अन्तः_2()) {
                Ast_नियोजय_W* नियोजय_Wpi = *it;
                यदि (!wExp) {
                    wExp
                        = new_Merge_व्यञ्जकम्(नियोजय_Wp0->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य),
                                       नियोजय_Wpi->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य), fl, varp->isWor());
                } अन्यथा {
                    wExp = new_Merge_व्यञ्जकम्(wExp, नियोजय_Wpi->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((नियोजय_Wpi->unlink_Fr_पृष्ठभागः()->delete_वृक्षः()), नियोजय_Wpi);
            }
            AstVarRef* ध्रुव wVarRef = नूतन AstVarRef{fl, varp, VAccess::WRITE};
            Ast_नियोजय_W* ध्रुव wAssignp = नूतन Ast_नियोजय_W{fl, wVarRef, wExp};
            नियोजय_Wp0->replaceWith(wAssignp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(नियोजय_Wp0), नियोजय_Wp0);
            assigns.clear();
            assigns.प्रक्षिप_पृष्ठभागः(wAssignp);
        }
    }

    शून्य removeNotStrongerAssignments(Assigns& assigns, ध्रुव Ast_नियोजय_W* strongestp,
                                      uint8_t greatest_ज्ञातम्_Strength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        स्वचालित removedIt = std::remove_if(assigns.begin(), assigns.अन्तः_2(), [&](Ast_नियोजय_W* assignp) {
            यदि (assignp == strongestp) प्रत्यागम असत्य;
            यदि (isAssignmentNotStrongerThanStrength(assignp, greatest_ज्ञातम्_Strength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(assignp->unlink_Fr_पृष्ठभागः()), assignp);
                प्रत्यागम सत्य;
            }
            प्रत्यागम असत्य;
        });
        assigns.erase(removedIt, assigns.अन्तः_2());
    }

    शून्य remove_Assignments_Not_Stronger_Than_एकरूपम्_Constant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        प्रति (स्वचालित& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            यदि (assigns.आकार() <= 1) अनुवर्त;
            ध्रुव Ast_नियोजय_W* ध्रुव strongest0p = get_Strongest_Assignment_सम्बन्धि_मानम्(assigns, 0);
            ध्रुव Ast_नियोजय_W* ध्रुव strongest1p = get_Strongest_Assignment_सम्बन्धि_मानम्(assigns, 1);
            ध्रुव Ast_नियोजय_W* strongestp = शून्यसूचकः;
            uint8_t greatest_ज्ञातम्_Strength = 0;
            ध्रुव स्वचालित getIfStrongest
                = [&](ध्रुव Ast_नियोजय_W* ध्रुव strongestCandidatep, तार्किक मानम्) {
                      यदि (!strongestCandidatep) प्रत्यागम;
                      ध्रुव uint8_t strength = getStrength(strongestCandidatep, मानम्);
                      यदि (strength >= greatest_ज्ञातम्_Strength) {
                          greatest_ज्ञातम्_Strength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            यदि (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatest_ज्ञातम्_Strength);
            }
        }
    }

    शून्य removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        प्रति (स्वचालित& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            यदि (assigns.आकार() <= 1) अनुवर्त;
            स्वचालित maxIt = std::max_element(
                assigns.begin(), assigns.अन्तः_2(), [&](ध्रुव Ast_नियोजय_W* ap, ध्रुव Ast_नियोजय_W* bp) {
                    यदि (m_tgraph.isTristate(ap)) प्रत्यागम !m_tgraph.isTristate(bp);
                    यदि (m_tgraph.isTristate(bp)) प्रत्यागम असत्य;
                    ध्रुव uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ध्रुव uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    प्रत्यागम minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ध्रुव Ast_नियोजय_W* ध्रुव strongestp = m_tgraph.isTristate(*maxIt) ? शून्यसूचकः : *maxIt;
            यदि (strongestp) {
                ध्रुव uint8_t greatest_ज्ञातम्_Strength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatest_ज्ञातम्_Strength);
            }
        }
    }

    // VISITORS
    शून्य दर्शय(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        यदि (m_graphing) {
            यदि (!m_alhs && nodep->अङ्कः().hasZ()) m_tgraph.समुच्चयः_Tristate(nodep);
        } अन्यथा {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            यदि (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ध्रुव varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_ग्रन्थिः* ध्रुव newp = नूतन AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            } अन्यथा यदि (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                AstConst* ध्रुव enp = getNonZConstp(nodep);
                V3_सङ्ख्या num1{nodep, nodep->विस्तारः()};
                num1.opAnd(nodep->अङ्कः(), enp->अङ्कः());  // 01X->01X, Z->0
                AstConst* ध्रुव newconstp = नूतन AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    शून्य दर्शय(AstCond* nodep) override {
        यदि (m_graphing) {
            iterateChildren(nodep);
            यदि (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } अन्यथा {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } अन्यथा {
            यदि (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                प्रत्यागम;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = nodep->condp();
            यदि (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenp = nodep->thenp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elsep = nodep->elsep();
            यदि (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव en1p = getEnp(thenp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enp
                    = नूतन AstCond{nodep->fileline(), condp->clone_वृक्षः(असत्य), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                यदि (thenp->user1p() && !thenp->user1p()->backp()) प्रक्षिप_Deletep(thenp->user1p());
                यदि (elsep->user1p() && !elsep->user1p()->backp()) प्रक्षिप_Deletep(elsep->user1p());
                thenp->user1p(शून्यसूचकः);
                elsep->user1p(शून्यसूचकः);
            }
        }
    }

    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        iterateChildren(nodep);
        यदि (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } अन्यथा यदि (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(शून्यसूचकः);
            m_tgraph.didProcess(nodep);
        }
    }

    शून्य दर्शय(AstSel* nodep) override {
        यदि (m_graphing) {
            iterateChildren(nodep);
            यदि (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } अन्यथा {
                associateLogic(nodep->fromp(), nodep);
            }
        } अन्यथा {
            यदि (m_alhs) {
                UINFO(9, dbgState() << nodep);
                यदि (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), ग्रन्थिः_व्यञ्जकम्));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } अन्यथा {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                यदि (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                यदि (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव en1p = getEnp(nodep->fromp());
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enp
                        = नूतन AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_वृक्षः_विशुद्धम्(सत्य),
                                     nodep->विस्तारः_Const()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    शून्य दर्शय(AstConcat* nodep) override {
        यदि (m_graphing) {
            iterateChildren(nodep);
            यदि (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } अन्यथा {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } अन्यथा {
            यदि (m_alhs) {
                UINFO(9, dbgState() << nodep);
                यदि (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enp = VN_AS(nodep->user1p(), ग्रन्थिः_व्यञ्जकम्);
                    nodep->user1p(शून्यसूचकः);
                    nodep->lhsp()->user1p(नूतन AstSel{nodep->fileline(), enp->clone_वृक्षः_विशुद्धम्(सत्य),
                                                     nodep->rhsp()->विस्तारः(),
                                                     nodep->lhsp()->विस्तारः()});
                    nodep->rhsp()->user1p(
                        नूतन AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->विस्तारः()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } अन्यथा {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p = nodep->lhsp();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr2p = nodep->rhsp();
                यदि (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव en1p = getEnp(expr1p);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव en2p = getEnp(expr2p);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enp = नूतन AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(शून्यसूचकः);
                    expr2p->user1p(शून्यसूचकः);
                }
            }
        }
    }

    शून्य दर्शय(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        यदि (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.समुच्चयः_Tristate(nodep);
        } अन्यथा {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            यदि (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                प्रत्यागम;
            }
            m_tgraph.didProcess(nodep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr2p = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* enp;
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव en2p = VN_AS(expr2p->user1p(), ग्रन्थिः_व्यञ्जकम्)) {
                enp = नूतन AstAnd{nodep->fileline(), expr1p, en2p};
            } अन्यथा {
                enp = expr1p;
            }
            expr1p->user1p(शून्यसूचकः);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    शून्य दर्शय_And_Or(Ast_ग्रन्थिः_Biop* nodep, तार्किक isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        यदि (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } अन्यथा {
            यदि (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                प्रत्यागम;
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
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p = nodep->lhsp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr2p = nodep->rhsp();
            यदि (!expr1p->user1p() && !expr2p->user1p()) {
                प्रत्यागम;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_ग्रन्थिः_व्यञ्जकम्* en1p = getEnp(expr1p);
            Ast_ग्रन्थिः_व्यञ्जकम्* en2p = getEnp(expr2p);
            Ast_ग्रन्थिः_व्यञ्जकम्* subexpr1p = expr1p->clone_वृक्षः_विशुद्धम्(असत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* subexpr2p = expr2p->clone_वृक्षः_विशुद्धम्(असत्य);
            यदि (isAnd) {
                subexpr1p = नूतन AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = नूतन AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            यदि (!en1p->op1p() && en2p->op1p()) {
                std::swap(en1p, en2p);
                std::swap(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enp = नूतन AstOr{
                nodep->fileline(),
                नूतन AstAnd{nodep->fileline(), en1p, नूतन AstOr{nodep->fileline(), en2p, subexpr1p}},
                नूतन AstAnd{nodep->fileline(), en2p->clone_वृक्षः(असत्य), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(शून्यसूचकः);
            expr2p->user1p(शून्यसूचकः);
        }
    }
    शून्य दर्शय(AstAnd* nodep) override { दर्शय_And_Or(nodep, सत्य); }
    शून्य दर्शय(AstOr* nodep) override { दर्शय_And_Or(nodep, असत्य); }

    शून्य दर्शय_नियोजय(Ast_ग्रन्थिः_नियोजय* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        यदि (VN_IS(nodep->rhsp(), CReset)) प्रत्यागम;
        यदि (Ast_नियोजय_W* ध्रुव नियोजय_Wp = VN_CAST(nodep, नियोजय_W)) {
            m_currentDelayp = VN_CAST(नियोजय_Wp->timingControlp(), Delay);
        }
        यदि (m_graphing) {
            यदि (Ast_नियोजय_W* नियोजय_Wp = VN_CAST(nodep, नियोजय_W)) {
                यदि (नियोजय_Wp->getLhsNetDelay()) प्रत्यागम;
                यदि (नियोजय_Wp->timingControlp() && !m_currentDelayp) प्रत्यागम;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                यदि (m_currentDelayp && m_currentDelayp->fallDelay()) प्रत्यागम;
                यदि (!नियोजय_Wp->timingControlp()) सङ्कलय_गन्तव्यम्_Assignment_List(नियोजय_Wp);
            }

            यदि (nodep->user2() & U2_GRAPHING) प्रत्यागम;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterate_And_अग्र_Null(nodep->rhsp());
            m_alhs = सत्य;
            iterate_And_अग्र_Null(nodep->lhsp());
            m_alhs = असत्य;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } अन्यथा {
            यदि (nodep->user2() & U2_NONGRAPH) {
                प्रत्यागम;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterate_And_अग्र_Null(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            यदि (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, नियोजय_W), nodep,
                            "Continuous assignment inside procedural process");
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), ग्रन्थिः_व्यञ्जकम्)->clone_वृक्षः_विशुद्धम्(असत्य)
                          : newAllZerosOrOnes(nodep->lhsp(), सत्य);
                ध्रुव तार्किक inserted = m_नियोजय_Enps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            यदि (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(शून्यसूचकः);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } अन्यथा {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                यदि (AstSel* ध्रुव selp = VN_CAST(nodep->lhsp(), Sel)) {
                    यदि (Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(selp->fromp(), ग्रन्थिः_Var_Ref)) {
                        यदि (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), सत्य));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } अन्यथा {
                            m_tgraph.didProcess(nodep, सत्य);
                        }
                    } अन्यथा {
                        m_tgraph.didProcess(nodep, सत्य);  // LCOV_EXCL_LINE
                    }
                } अन्यथा {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, सत्य);
                }
            }
            m_alhs = सत्य;  // And user1p() will indicate tristate equation, if any
            यदि (Ast_नियोजय_W* ध्रुव नियोजय_Wp = VN_CAST(nodep, नियोजय_W)) {
                यदि (AstStrengthSpec* ध्रुव specp = नियोजय_Wp->strengthSpecp()) {
                    यदि (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        यदि (!is_Only_Assignment_Is_गन्तव्यम्_Lhs_Var(नियोजय_Wp)) {
                            नियोजय_Wp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } अन्यथा {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterate_And_अग्र_Null(nodep->lhsp());
        }
    }
    शून्य दर्शय(Ast_नियोजय_W* nodep) override { दर्शय_नियोजय(nodep); }
    शून्य दर्शय(Ast_नियोजय* nodep) override { दर्शय_नियोजय(nodep); }
    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override { दर्शय_नियोजय(nodep); }

    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ध्रुव AstAlways* ध्रुव alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_नियोजय ? शून्यसूचकः : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    शून्य दर्शय(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_अन्तर्_Alias);
        m_अन्तर्_Alias = सत्य;
        यदि (m_graphing) {
            यदि (nodep->user2() & U2_GRAPHING) प्रत्यागम;
            m_alhs = सत्य;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            प्रति (Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

    शून्य दर्शय_Case_Eq(Ast_ग्रन्थिः_Biop* nodep, तार्किक neq) {
        यदि (m_graphing) {
            iterateChildren(nodep);
        } अन्यथा {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ध्रुव constp = VN_CAST(nodep->lhsp(), Const);
            यदि (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
                rhsp->unlink_Fr_पृष्ठभागः();
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                Ast_ग्रन्थिः_व्यञ्जकम्* enRhsp;
                यदि (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), ग्रन्थिः_व्यञ्जकम्);
                    rhsp->user1p(शून्यसूचकः);
                } अन्यथा {
                    enRhsp = get_En_व्यञ्जकम्_Based_On_Originalp(rhsp);
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* newp
                    = नूतन AstLogAnd{fl, नूतन AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    नूतन AstEqCase{fl, नूतन AstConst{fl, constp->अङ्कः()}, rhsp}};
                constp->user1p(शून्यसूचकः);
                यदि (neq) newp = नूतन AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            } अन्यथा यदि (constp && nodep->rhsp()->user1p()) {
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                constp->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* newp = नूतन AstLogAnd{fl,
                                                  नूतन AstEq{fl, newAllZerosOrOnes(constp, असत्य),
                                                            VN_AS(rhsp->user1p(), ग्रन्थिः_व्यञ्जकम्)},
                                                  // Keep the caseeq if there are X's present
                                                  नूतन AstEqCase{fl, constp, rhsp}};
                यदि (neq) newp = नूतन AstLogNot{fl, newp};
                rhsp->user1p(शून्यसूचकः);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            } अन्यथा {
                checkUnhandled(nodep);
            }
        }
    }
    शून्य दर्शय_Eq_Neq_Wild(Ast_ग्रन्थिः_Biop* nodep) {
        यदि (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterate_And_अग्र_Null(nodep->lhsp());
        यदि (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            प्रत्यागम;
        }
    }
    शून्य दर्शय(AstEqCase* nodep) override { दर्शय_Case_Eq(nodep, असत्य); }
    शून्य दर्शय(AstNeqCase* nodep) override { दर्शय_Case_Eq(nodep, सत्य); }
    शून्य दर्शय(AstEqWild* nodep) override { दर्शय_Eq_Neq_Wild(nodep); }
    शून्य दर्शय(AstNeqWild* nodep) override { दर्शय_Eq_Neq_Wild(nodep); }

    शून्य दर्शय(Ast_गणनम्_द्वयङ्काः* nodep) override {
        std::array<तार्किक, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->अङ्कः().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->अङ्कः().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->अङ्कः().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        यदि (m_graphing) {
            iterate_And_अग्र_Null(nodep->lhsp());
            यदि (!dropop[0]) iterate_And_अग्र_Null(nodep->rhsp());
            यदि (!dropop[1]) iterate_And_अग्र_Null(nodep->thsp());
            यदि (!dropop[2]) iterate_And_अग्र_Null(nodep->fhsp());
        } अन्यथा {
            Ast_ग्रन्थिः_व्यञ्जकम्* nonXp = शून्यसूचकः;
            यदि (!dropop[0]) {
                nonXp = nodep->rhsp();
            } अन्यथा यदि (!dropop[1]) {
                nonXp = nodep->thsp();
            } अन्यथा यदि (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            यदि (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                यदि (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    प्रत्यागम;
                }
                AstVar* ध्रुव envarp = getCreateEnVarp(varrefp->varp(), असत्य);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlink_Fr_पृष्ठभागः(&relinkHandle);
                Ast_ग्रन्थिः_व्यञ्जकम्* newp = नूतन Ast_गणनम्_Ones{
                    nodep->fileline(), नूतन AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                यदि (nonXp) {  // Need to still count '0 or '1 or 'x's
                    यदि (dropop[0]) {
                        nodep->rhsp()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                        nodep->rhsp(nonXp->clone_वृक्षः_विशुद्धम्(सत्य));
                    }
                    यदि (dropop[1]) {
                        nodep->thsp()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                        nodep->thsp(nonXp->clone_वृक्षः_विशुद्धम्(सत्य));
                    }
                    यदि (dropop[2]) {
                        nodep->fhsp()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                        nodep->fhsp(nonXp->clone_वृक्षः_विशुद्धम्(सत्य));
                    }
                    newp = नूतन Ast_सङ्कलय{nodep->fileline(), nodep, newp};
                } अन्यथा {
                    प्रक्षिप_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = शून्यसूचकः;
        यदि (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } अन्यथा यदि (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        यदि (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } अन्यथा {
            यदि (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.समुच्चयः_Tristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } अन्यथा {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep->lhsp(), Sel);
                ध्रुव AstConst* ध्रुव lsbp = selp ? VN_CAST(selp->lsbp(), Const) : शून्यसूचकः;
                यदि (lsbp) {
                    ध्रुव पूर्णाङ्क lsb = lsbp->गन्तव्यम्_SInt();
                    प्रति (पूर्णाङ्क i = 0; i < selp->विस्तारः_Const(); ++i) {
                        समुच्चयः_Bit_Pull_Direction(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } अन्यथा {
                    समुच्चयः_Pull_Direction(varrefp->varp(), nodep);
                }
            }
        }
        यदि (!m_graphing) {
            nodep->unlink_Fr_पृष्ठभागः();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    शून्य iteratePinGuts(AstPin* nodep) {
        यदि (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            यदि (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } अन्यथा {
            // All heavy lifting completed in graph visitor.
            यदि (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    शून्य दर्शय(AstPin* nodep) override {
        यदि (m_graphing) {
            यदि (nodep->user2() & U2_GRAPHING) प्रत्यागम;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ध्रुव enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            यदि (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, असत्य);
                iteratePinGuts(nodep);
                प्रत्यागम;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ध्रुव तार्किक अन्तर्_Decl_Processing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::निवेशः
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            यदि (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ध्रुव ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(नूतन AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.समुच्चयः_Tristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } अन्यथा यदि (अन्तर्_Decl_Processing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ध्रुव Ast_नियोजय_W* ध्रुव reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, सत्य, सत्य);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.समुच्चयः_Tristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ध्रुव enVarp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvt_गन्तव्यम्_Str(m_unique++),
                                              VFlagBitPacked{}, enModVarp->विस्तारः()};
            यदि (अन्तर्_Decl_Processing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->var_Type2_अन्तर्();
            } अन्यथा {
                enModVarp->var_Type2_बहिर्गमनम्();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ध्रुव enpinp
                = नूतन AstPin{nodep->fileline(), nodep->pin_अङ्कः(),
                             enModVarp->name(),  // should be {var}"__en"
                             नूतन AstVarRef{nodep->fileline(), enVarp,
                                           अन्तर्_Decl_Processing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->सङ्कलय_अग्र_अत्र(enpinp);
            m_modp->सङ्कलय_Stmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ध्रुव Ast_नियोजय_W* बहिर्गमनम्_Assignp = शून्यसूचकः;  // If reconnected, the related assignment
            AstPin* outpinp = शून्यसूचकः;
            AstVar* ध्रुव बहिर्गमनम्_Mod_Varp = m_varAux(nodep->modVarp()).बहिर्गमनम्_Varp;
            यदि (!बहिर्गमनम्_Mod_Varp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                यदि (!m_modp->is_शिखरम्()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } अन्यथा {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव outexprp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)
                                                  ->clone_वृक्षः_विशुद्धम्(असत्य);  // Note has lvalue() set
                outpinp = नूतन AstPin{nodep->fileline(), nodep->pin_अङ्कः(),
                                     बहिर्गमनम्_Mod_Varp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(बहिर्गमनम्_Mod_Varp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->सङ्कलय_अग्र_अत्र(outpinp);
                // Simplify
                यदि (अन्तर्_Decl_Processing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, सत्य};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                बहिर्गमनम्_Assignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        सत्य);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, बहिर्गमनम्_Assignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ध्रुव TristatePinVisitor visitor{nodep->exprp(), m_tgraph, असत्य};
            ध्रुव Ast_ग्रन्थिः* ध्रुव अन्तर्_Assignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, सत्य);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, अन्तर्_Assignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            यदि (!बहिर्गमनम्_Assignp) {
                यदि (!outpinp) {
                    exprrefp = शून्यसूचकः;  // Primary input only
                } अन्यथा {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    यदि (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } अन्यथा {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(बहिर्गमनम्_Assignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                यदि (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            यदि (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(नूतन AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                यदि (!बहिर्गमनम्_Assignp) {
                    map_निवेशय_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            यदि (exprrefp) {
                AstPull* ध्रुव pullp = m_varAux(nodep->modVarp()).pullp;
                ध्रुव स्वचालित& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = शून्यसूचकः;
                पूर्णाङ्क selLsb = 0;
                पूर्णाङ्क sel_विस्तारः = 0;
                यदि (बहिर्गमनम्_Assignp) {
                    यदि (AstSel* ध्रुव selp = VN_CAST(बहिर्गमनम्_Assignp->lhsp(), Sel)) {
                        AstVarRef* ध्रुव vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const);
                        यदि (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->गन्तव्यम्_SInt();
                            sel_विस्तारः = selp->विस्तारः_Const();
                        }
                    }
                }

                यदि (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    समुच्चयः_Pull_Direction(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    यदि (selVarp) {
                        ध्रुव पूर्णाङ्क direction = pullp->direction();
                        प्रति (पूर्णाङ्क i = 0; i < sel_विस्तारः; ++i) {
                            समुच्चयः_Bit_Pull_Direction(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                यदि (!srcBitPulls.रिक्तानि()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    प्रति (ध्रुव स्वचालित& pair : srcBitPulls) {
                        समुच्चयः_Bit_Pull_Direction(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    यदि (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        प्रति (ध्रुव स्वचालित& pair : srcBitPulls) {
                            समुच्चयः_Bit_Pull_Direction(selVarp, selLsb + pair.first, pair.second);
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
        अन्यथा {
            यदि (nodep->user2() & U2_NONGRAPH) प्रत्यागम;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    शून्य handle_ग्रन्थिः_Var_Ref(Ast_ग्रन्थिः_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        यदि (m_graphing) {
            यदि (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            यदि (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            यदि (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.समुच्चयः_Tristate(nodep->varp());
            }
        } अन्यथा {
            यदि (nodep->user2() & U2_NONGRAPH) प्रत्यागम;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            यदि (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                यदि (m_अन्तर्_Alias) {
                    यदि (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } अन्यथा {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    प्रत्यागम;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_निवेशय_Lhs_Var_Ref(nodep);
            } अन्यथा यदि (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                AstVar* ध्रुव enVarp = getCreateEnVarp(nodep->varp(), असत्य);
                यदि (AstVarXRef* ध्रुव xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } अन्यथा {
                    nodep->user1p(नूतन AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    शून्य दर्शय(AstVarRef* nodep) override { handle_ग्रन्थिः_Var_Ref(nodep); }
    शून्य दर्शय(AstVarXRef* nodep) override { handle_ग्रन्थिः_Var_Ref(nodep); }

    शून्य दर्शय(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        यदि (m_graphing) {
            // If tri0/1 force a pullup
            यदि (nodep->user2() & U2_GRAPHING) प्रत्यागम;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            यदि (nodep->isPulldown() || nodep->isPullup()) {
                Ast_ग्रन्थिः* ध्रुव newp = नूतन AstPull{
                    nodep->fileline(), नूतन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->सङ्कलय_अग्र_अत्र(newp);
                // We'll iterate on the new AstPull later
            }
            यदि (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.समुच्चयः_Tristate(nodep);
            }
        } अन्यथा {  // !graphing
            यदि (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        यदि (m_processedIfaces && VN_IS(nodep, Iface)) प्रत्यागम;
        UINFO(8, dbgState() << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_graphing);
        VL_RESTORER(m_unique);
        VL_RESTORER_CLEAR(m_lhsmap);
        VL_RESTORER_CLEAR(m_assigns);
        VL_RESTORER_CLEAR(m_नियोजय_Enps);
        VL_RESTORER_CLEAR(m_proceduralEnables);
        // Not preserved, needs pointer instead: TristateGraph origTgraph = m_tgraph;
        UASSERT_OBJ(m_tgraph.रिक्तानि(), nodep, "Unsupported: NodeModule under NodeModule");

        // Clear state
        m_graphing = असत्य;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = शून्यसूचकः;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = सत्य;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = असत्य;
        UINFO(9, dbgState() << "processing mod " << nodep);

        // Merge the assignments for very Wired net LHS : wor, trior, wand and triand
        mergeWiredNetsAssignments();
        // Remove all assignments not stronger than the strongest uniform constant
        remove_Assignments_Not_Stronger_Than_एकरूपम्_Constant();
        // Use graph to find tristate signals
        m_tgraph.graphWalk(nodep);

        // Remove all assignments not stronger than the strongest non-tristate RHS
        removeAssignmentsNotStrongerThanNonTristate();

        // Build the LHS drivers map for this module
        iterateChildren(nodep);
        // Insert new logic for all tristates
        निवेशय_Tristates(nodep);
        निवेशय_Procedural_Enable_Assignments(nodep);

        प्रति (स्वचालित& नियोजय_Enp : m_नियोजय_Enps) {
            VL_DO_DANGLING(नियोजय_Enp.second->delete_वृक्षः(), नियोजय_Enp.second);
        }
        m_नियोजय_Enps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    शून्य दर्शय(AstClass* nodep) override {
        // don't deal with classes
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        // don't deal with functions
    }

    शून्य दर्शय(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterate_And_अग्र_Null(nodep->stmtsp());
    }

    शून्य दर्शय(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = असत्य;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    शून्य combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        प्रति (ध्रुव std::pair<AstVar* ध्रुव, std::सदिशम्<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ध्रुव invarp = kv.first;
            ध्रुव std::सदिशम्<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_ग्रन्थिः_Module* ध्रुव ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_पङ्क्तिः* ध्रुव fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.आकार() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_ग्रन्थिः_व्यञ्जकम्* orp = शून्यसूचकः;
            Ast_ग्रन्थिः_व्यञ्जकम्* enp = शून्यसूचकः;

            प्रति (ध्रुव std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ध्रुव contribEnp = contrib.first;
                AstVar* ध्रुव contribOutp = contrib.second;

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव बहिर्गमनम्_Refp = नूतन AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enRefp = नूतन AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव andp = नूतन AstAnd{fl, बहिर्गमनम्_Refp, enRefp};

                orp = (!orp) ? andp : नूतन AstOr{fl, orp, andp};

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enRef2p = नूतन AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : नूतन AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            यदि (!envarp) {
                envarp = नूतन AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->समुच्चयः_Dfg_Tri_Lowered();
                ifaceModp->सङ्कलय_Stmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            Ast_नियोजय_W* ध्रुव enAssp
                = नूतन Ast_नियोजय_W{fl, नूतन AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->सङ्कलय_Stmtsp(नूतन AstAlways{enAssp});

            // Pull resolution
            ध्रुव AstPull* ध्रुव pullp = m_varAux(invarp).pullp;
            ध्रुव तार्किक pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_ग्रन्थिः_व्यञ्जकम्* undrivenp = नूतन AstNot{fl, नूतन AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = नूतन AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = नूतन AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            Ast_नियोजय_W* ध्रुव assp
                = नूतन Ast_नियोजय_W{fl, नूतन AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->सङ्कलय_Stmtsp(नूतन AstAlways{assp});
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::सदिशम्<Ast_ग्रन्थिः_Module*> ifacesp;
        प्रति (Ast_ग्रन्थिः* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            यदि (VN_IS(modp, Iface)) ifacesp.प्रक्षिप_पृष्ठभागः(VN_AS(modp, ग्रन्थिः_Module));
        }
        प्रति (स्वचालित it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = सत्य;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#यदि_परिभाषितम् VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::सदिशम्<Ast_ग्रन्थिः*> अप्रयुक्तम्_Rootps;
        netlistp->foreach([&](Ast_ग्रन्थिः* nodep) {
            Ast_ग्रन्थिः* ध्रुव enp = nodep->user1p();
            यदि (!enp) प्रत्यागम;
            यदि (enp->backp()) प्रत्यागम;
            अप्रयुक्तम्_Rootps.emplace_पृष्ठभागः(enp);
        });
        प्रति (Ast_ग्रन्थिः* ध्रुव nodep : अप्रयुक्तम्_Rootps) VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
#यदि_अन्तः
    }
    ~TristateVisitor() override {
        V3Stats::सङ्कलय_Stat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

शून्य V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("tristate", 0, dump_वृक्षः_Either_Level() >= 3);
}
