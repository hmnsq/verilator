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

वर्ग TristateBaseVisitor VL_NOT_FINAL : सार्वजनिक VNVisitor {
सार्वजनिक:
    // METHODS
};

//######################################################################
// Graph support classes

वर्ग TristateVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_गाँठ* ध्रुव m_nodep;
    तार्किक m_isTristate = असत्य;  // Logic indicates a tristate
    तार्किक m_feedsTri = असत्य;  // Propagates to a tristate node (on RHS)
    तार्किक m_processed = असत्य;  // Tristating was cleaned up
सार्वजनिक:
    TristateVertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = स्वभाव;
    // ACCESSORS
    Ast_गाँठ* nodep() ध्रुव VL_MT_STABLE { प्रत्यागम m_nodep; }
    ध्रुव AstVar* varp() ध्रुव { प्रत्यागम VN_CAST(nodep(), Var); }
    string name() ध्रुव override {
        प्रत्यागम ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvtToHex(nodep())));
    }
    string dotColor() ध्रुव override {
        प्रत्यागम (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_पंक्ति* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }
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
    using VarVec = std::vector<AstVar*>;

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

    TristateVertex* makeVertex(Ast_गाँठ* nodep) {
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
            प्रति (V3GraphEdge& edge : vtxp->outEdges()) {
                TristateVertex* ध्रुव vvertexp = static_cast<TristateVertex*>(edge.शीर्ष());
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
            प्रति (V3GraphEdge& edge : vtxp->inEdges()) {
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
    शून्य graphWalkRecurseBack(TristateVertex* vtxp, पूर्णाङ्क level) {
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
            प्रति (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* ध्रुव vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                यदि (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(सत्य);
                    graphWalkRecurseBack(vvertexp, level + 1);
                }
            }
        }
    }

सार्वजनिक:
    // METHODS
    तार्किक empty() ध्रुव { प्रत्यागम m_graph.empty(); }
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
        Ast_गाँठ::user4_Clear_वृक्ष();  // Wipe all node user4p's that point to vertexes
    }
    शून्य graphWalk(Ast_गाँठ_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseBack(static_cast<TristateVertex*>(&vtx), 0);
        }
        यदि (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    शून्य associate(Ast_गाँठ* fromp, Ast_गाँठ* शीर्ष) {
        नूतन V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(शीर्ष), 1};
    }
    शून्य deleteVerticesFromSubtreeRecurse(Ast_गाँठ* nodep) {
        यदि (!nodep) प्रत्यागम;
        // Skip vars, because they may be connected to more than one varref
        यदि (!VN_IS(nodep, Var)) {
            TristateVertex* ध्रुव vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            यदि (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        यदि (Ast_गाँठ* ध्रुव refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        यदि (Ast_गाँठ* ध्रुव refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        यदि (Ast_गाँठ* ध्रुव refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        यदि (Ast_गाँठ* ध्रुव refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    शून्य setTristate(Ast_गाँठ* nodep) { makeVertex(nodep)->isTristate(सत्य); }
    तार्किक isTristate(ध्रुव Ast_गाँठ* nodep) {
        ध्रुव TristateVertex* ध्रुव vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        प्रत्यागम vertexp && vertexp->isTristate();
    }
    तार्किक feedsTri(ध्रुव Ast_गाँठ* nodep) {
        ध्रुव TristateVertex* ध्रुव vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        प्रत्यागम vertexp && vertexp->feedsTri();
    }
    शून्य didProcess(Ast_गाँठ* nodep, तार्किक quiet = असत्य) {
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
                यदि (AstVar* ध्रुव nodep = VN_CAST(vvertex.nodep(), Var)) v.धकेलो_back(nodep);
            }
        }
        प्रत्यागम v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

वर्ग TristatePinVisitor final : सार्वजनिक TristateBaseVisitor {
    TristateGraph& m_tgraph;
    ध्रुव तार्किक m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    शून्य visit(Ast_गाँठ_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        यदि (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } अन्यथा यदि (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.setTristate(nodep->varp());
        }
    }
    शून्य visit(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    शून्य visit(AstSliceSel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_गाँठ* nodep, TristateGraph& tgraph, तार्किक lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = स्वभाव;
};

//######################################################################

वर्ग TristateVisitor final : सार्वजनिक TristateBaseVisitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;
    ध्रुव VNUser3InUse m_inuser3;
    ध्रुव VNUser4InUse m_inuser4;

    संरचना AuxAstVar final {
        AstPull* pullp = nullptr;  // pullup/pulldown direction (whole variable)
        AstVar* outVarp = nullptr;  // output __out var
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
        Ast_गाँठ_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_गाँठ_Assign* m_assignp;  // Assignment containing m_varrefp
        ध्रुव Ast_गाँठ_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_गाँठ_व्यंजक* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_गाँठ_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_गाँठ_व्यंजक* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_गाँठ_Var_Ref* varrefp, Ast_गाँठ_Assign* assignp,
                    ध्रुव Ast_गाँठ_Procedure* procedurep, Ast_गाँठ_व्यंजक* enLhsp,
                    Ast_गाँठ_Var_Ref* enVarrefp, Ast_गाँठ_व्यंजक* enRhsp, VStrength strength,
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
        Ast_गाँठ_व्यंजक* m_lhsp;  // LHS giving the enable assign
        Ast_गाँठ_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_गाँठ_व्यंजक* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    संरचना ProceduralEnable final {
        Ast_गाँठ_व्यंजक* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_गाँठ_व्यंजक* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    using RefStrengthVec = std::vector<RefStrength>;
    using VarMap = std::map<AstVar*, RefStrengthVec*>;
    using Assigns = std::vector<AstAssignW*>;
    using VarToAssignsMap = std::map<AstVar*, Assigns>;
    using AssignEnableMap = std::map<Ast_गाँठ_Assign*, Ast_गाँठ_व्यंजक*>;
    using ProceduralEnableMap = std::map<Ast_गाँठ_Assign*, std::vector<ProceduralEnable>>;
    गणना : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    तार्किक m_graphing = असत्य;  // Major mode - creating graph
    //
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    AstCell* m_cellp = nullptr;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    VarToAssignsMap m_assigns;  // Assigns in current module
    AssignEnableMap m_assignEnps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    पूर्णाङ्क m_unique = 0;
    तार्किक m_alhs = असत्य;  // On LHS of assignment
    तार्किक m_inAlias = असत्य;  // Inside alias statement
    तार्किक m_processedIfaces = असत्य;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = nullptr;  // Delay of assignment currently collecting LHS drivers
    ध्रुव Ast_गाँठ* m_logicp = nullptr;  // Current logic being built
    Ast_गाँठ_Assign* m_assignp = nullptr;  // Current assignment
    ध्रुव Ast_गाँठ_Procedure* m_procedurep = nullptr;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::vector<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ध्रुव {
        string o = (m_graphing ? " gr " : " ng ");
        यदि (m_alhs) o += "alhs ";
        प्रत्यागम o;
    }
    शून्य modAddStmtp(Ast_गाँठ* nodep, Ast_गाँठ* newp) {
        यदि (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } अन्यथा {
            m_modp->addStmtsp(newp);
        }
    }
    शून्य associateLogic(Ast_गाँठ* fromp, Ast_गाँठ* शीर्ष) {
        यदि (m_logicp) m_tgraph.associate(fromp, शीर्ष);
    }
    AstConst* newAllZerosOrOnes(Ast_गाँठ* nodep, तार्किक ones) {
        V3_संख्या अंक{nodep, nodep->width()};
        यदि (ones) अंक.setAllBits1();
        AstConst* ध्रुव newp = नूतन AstConst{nodep->fileline(), अंक};
        प्रत्यागम newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_पंक्ति* fl, AstVar* varp, ध्रुव string& dotted, VAccess access,
                           ध्रुव string& inlinedDots) {
        AstVarXRef* ध्रुव xrefp = नूतन AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        प्रत्यागम xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_गाँठ_Module* modp, ध्रुव string& dottedPath) {
        यदि (dottedPath.empty()) प्रत्यागम nullptr;
        ध्रुव string firstComp = dottedPath.substr(0, dottedPath.find('.'));
        प्रति (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ध्रुव ivarp = VN_CAST(stmtp, Var);
            यदि (!ivarp || ivarp->name() != firstComp) अनुवर्त;
            ध्रुव AstIfaceRefDType* ध्रुव ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            यदि (ifaceDtp && ifaceDtp->modportp()) प्रत्यागम ifaceDtp->modportp();
            भङ्ग;
        }
        प्रत्यागम nullptr;
    }
    Ast_गाँठ_व्यंजक* getEnp(Ast_गाँठ* nodep) {
        यदि (nodep->user1p()) {
            यदि (Ast_गाँठ_Var_Ref* ध्रुव refp = VN_CAST(nodep, गाँठ_Var_Ref)) {
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
        प्रत्यागम VN_AS(nodep->user1p(), गाँठ_व्यंजक);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    शून्य addModportVarRefIfMissing(AstModport* modportp, AstVar* varp, VDirection dir) {
        प्रति (Ast_गाँठ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ध्रुव mvrp = VN_CAST(mrp, ModportVarRef);
            यदि (mvrp && mvrp->varp() == varp) प्रत्यागम;  // Already present
        }
        AstModportVarRef* ध्रुव mvrp = नूतन AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->addVarsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, तार्किक is_शीर्ष) {
        // Return the master __en for the specified input variable
        यदि (!invarp->user1p()) {
            AstVar* ध्रुव newp
                = नूतन AstVar{invarp->fileline(), is_शीर्ष ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_गाँठ_Module* ध्रुव ownerModp = findParentModule(invarp);
            यदि (ownerModp && ownerModp != m_modp) {
                ownerModp->addStmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                प्रति (Ast_गाँठ* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ध्रुव modportp = VN_CAST(stmtp, Modport);
                    यदि (!modportp) अनुवर्त;
                    प्रति (Ast_गाँठ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ध्रुव mvrp = VN_CAST(mrp, ModportVarRef);
                        यदि (mvrp && mvrp->varp() == invarp) {
                            addModportVarRefIfMissing(modportp, newp, VDirection::निवेश);
                            भङ्ग;
                        }
                    }
                }
            } अन्यथा {
                modAddStmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        प्रत्यागम VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ध्रुव constp) {
        File_पंक्ति* ध्रुव fl = constp->fileline();
        V3_संख्या numz{constp, constp->width()};
        numz.opBitsZ(constp->अंक());  // Z->1, else 0
        V3_संख्या numz0{constp, constp->width()};
        numz0.opNot(numz);  // Z->0, else 1
        प्रत्यागम नूतन AstConst{fl, numz0};
    }
    Ast_गाँठ_व्यंजक* get_En_व्यंजक_Based_On_Originalp(Ast_गाँठ_व्यंजक* ध्रुव nodep) {
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
            Ast_गाँठ_व्यंजक* ध्रुव lhsp = extendp->lhsp()->unlinkFrBack();
            AstExtend* ध्रुव enExtendp = extendp->clone_वृक्ष(असत्य);
            extendp->lhsp(lhsp);
            Ast_गाँठ_व्यंजक* ध्रुव enLhsp = get_En_व्यंजक_Based_On_Originalp(lhsp);
            enExtendp->lhsp(नूतन AstNot{enLhsp->fileline(), enLhsp});
            प्रत्यागम नूतन AstNot{enExtendp->fileline(), enExtendp};
        } अन्यथा यदि (AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            Ast_गाँठ_व्यंजक* ध्रुव fromp = selp->fromp()->unlinkFrBack();
            AstSel* ध्रुव enSelp = selp->clone_वृक्ष(असत्य);
            selp->fromp(fromp);
            Ast_गाँठ_व्यंजक* ध्रुव enFromp = get_En_व्यंजक_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            प्रत्यागम enSelp;
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            प्रत्यागम nullptr;
        }
    }
    AstVar* getCreateOutVarp(AstVar* invarp, तार्किक is_शीर्ष) {
        // Return the master __out for the specified input variable
        यदि (!m_varAux(invarp).outVarp) {
            AstVar* ध्रुव newp
                = नूतन AstVar{invarp->fileline(), is_शीर्ष ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            modAddStmtp(invarp, newp);
            m_varAux(invarp).outVarp = newp;  // find outvar given invarp
        }
        प्रत्यागम m_varAux(invarp).outVarp;
    }
    AstVar* getCreateUnconnVarp(Ast_गाँठ* fromp, Ast_गाँठ_DType* dtypep) {
        AstVar* ध्रुव newp = नूतन AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvtToStr(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        modAddStmtp(newp, newp);
        प्रत्यागम newp;
    }

    LhsProjection newLhsProjection(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_Var_Ref* targetp, Ast_गाँठ_व्यंजक* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_गाँठ_व्यंजक* ध्रुव newLhsp = lhsp->clone_वृक्ष_Pure(असत्य);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_गाँठ* nodep) { nodep->user1p(nullptr); });
        Ast_गाँठ_Var_Ref* ध्रुव newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        प्रत्यागम LhsProjection{newLhsp, newVarrefp, enp};
    }

    शून्य map_जोड़ो_Lhs_Var_Ref(Ast_गाँठ_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ध्रुव कुंजी = nodep->varp();
        ध्रुव स्वतः pair = m_lhsmap.emplace(कुंजी, nullptr);
        यदि (pair.second) pair.first->second = नूतन RefStrengthVec;
        Ast_गाँठ_व्यंजक* enLhsp = nullptr;
        Ast_गाँठ_Var_Ref* enVarrefp = nullptr;
        Ast_गाँठ_व्यंजक* enRhsp = nullptr;
        ध्रुव स्वतः enIt = m_assignEnps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ध्रुव तार्किक canCombineProcedural = !VN_IS(कुंजी->dtypep()->skipRefp(), गाँठ_UOr_Struct_DType);
        ध्रुव Ast_गाँठ_Procedure* ध्रुव procedurep
            = enIt != m_assignEnps.end() && canCombineProcedural ? m_procedurep : nullptr;
        यदि (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_वृक्ष_Pure(असत्य));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->धकेलो_back(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_गाँठ_व्यंजक* newEnableDeposit(AstSel* selp, Ast_गाँठ_व्यंजक* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_गाँठ_व्यंजक* ध्रुव newp = नूतन AstShiftL{
            selp->fileline(), नूतन AstExtend{selp->fileline(), enp, selp->fromp()->width()},
            selp->lsbp()->clone_वृक्ष_Pure(असत्य), selp->fromp()->width()};
        प्रत्यागम newp;
    }

    शून्य setPullDirection(AstVar* varp, AstPull* pullp) {
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

    शून्य setBitPullDirection(AstVar* varp, पूर्णाङ्क bitIndex, पूर्णाङ्क direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        स्वतः& bitPulls = m_varAux(varp).bitPulls;
        स्वतः it = bitPulls.find(bitIndex);
        यदि (it != bitPulls.end() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bitIndex << " of " << varp->prettyNameQ());
        }
        bitPulls[bitIndex] = direction;
    }

    AstConst* createPerBitPullConst(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ध्रुव स्वतः& bitPulls = m_varAux(varp).bitPulls;
        V3_संख्या अंक{varp, varp->width(), 0};  // Start with all zeros
        प्रति (ध्रुव स्वतः& pair : bitPulls) {
            यदि (pair.second == 1) अंक.setBit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        प्रत्यागम नूतन AstConst{varp->fileline(), अंक};
    }

    तार्किक hasPerBitPulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        प्रत्यागम !m_varAux(varp).bitPulls.empty();
    }

    शून्य checkUnhandled(Ast_गाँठ* nodep) {
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

    शून्य जोड़ो_Tristates(Ast_गाँठ_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ध्रुव TristateGraph::VarVec vars = m_tgraph.tristateVars();
        प्रति (स्वतः varp : vars) {
            यदि (!m_tgraph.isTristate(varp)) अनुवर्त;
            ध्रुव स्वतः it = m_lhsmap.find(varp);
            यदि (it != m_lhsmap.end()) अनुवर्त;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ध्रुव AstPull* ध्रुव pullp = m_varAux(varp).pullp;
            ध्रुव तार्किक pull_मूल्य = pullp && pullp->direction() == 1;
            AstConst* ध्रुव constp = newAllZerosOrOnes(varp, pull_मूल्य);
            AstVarRef* ध्रुव varrefp = नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            AstAssignW* ध्रुव newp = नूतन AstAssignW{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, असत्य));  // en=0 for floating/pullup vars
            nodep->addStmtsp(नूतन AstAlways{newp});
            map_जोड़ो_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        प्रति (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ध्रुव std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            यदि (it == m_lhsmap.end()) अनुवर्त;
            AstVar* ध्रुव invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            यदि (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_गाँठ_Module* ध्रुव ownerModp = findParentModule(invarp);
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
                            pi.refs.धकेलो_back(rs);
                            यदि (pi.inlinedDots.empty()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } अन्यथा {
                            partitions[""].refs.धकेलो_back(rs);
                        }
                    }
                    प्रति (स्वतः& kv : partitions) {
                        जोड़ो_Tristates_Signal(nodep, invarp, &kv.second.refs, सत्य, kv.first,
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
                    जोड़ो_Tristates_Signal(nodep, invarp, refsp, सत्य, "", "", nullptr);
                } अन्यथा {
                    जोड़ो_Tristates_Signal(nodep, invarp, refsp, असत्य, "", "", nullptr);
                }
            } अन्यथा {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(delete refsp, refsp);
        }
    }

    शून्य aggregateTriSameStrength(Ast_गाँठ_Module* nodep, AstVar* ध्रुव varp, AstVar* ध्रुव envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator endStrength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        संरचना DriverGroup final {
            ध्रुव Ast_गाँठ_Procedure* m_procedurep;  // Process owning these assignments
            std::vector<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::vector<DriverGroup> drivers;
        std::map<ध्रुव Ast_गाँठ_Procedure*, माप_प्रकार> procedureToDriver;
        प्रति (स्वतः it = beginStrength; it != endStrength; ++it) {
            यदि (it->m_procedurep) {
                ध्रुव स्वतः emplaced = procedureToDriver.emplace(it->m_procedurep, drivers.size());
                यदि (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.धकेलो_back(&*it);
                    अनुवर्त;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.धकेलो_back(DriverGroup{it->m_procedurep, {&*it}});
        }

        Ast_गाँठ_व्यंजक* orp = nullptr;
        Ast_गाँठ_व्यंजक* enp = nullptr;

        प्रति (DriverGroup& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ध्रुव newLhsp = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvtToStr(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->setDfgTriLowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->addStmtsp(newLhsp);
            AstVar* ध्रुव newEnLhsp
                = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvtToStr(m_unique++), envarp};  // 2-state ok
            newEnLhsp->setDfgTriLowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->addStmtsp(newEnLhsp);

            प्रति (RefStrength* ध्रुव refStrengthp : driver.m_refsp) {
                Ast_गाँठ_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                यदि (VN_IS(refp, VarXRef)) {
                    AstVarRef* ध्रुव localRefp
                        = नूतन AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(nullptr);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
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

                    Ast_गाँठ_व्यंजक* enLhsp = refStrengthp->m_enLhsp;
                    Ast_गाँठ_Var_Ref* ध्रुव enVarrefp = refStrengthp->m_enVarrefp;
                    यदि (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ध्रुव localRefp
                            = नूतन AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        यदि (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_वृक्ष(), enVarrefp);
                        } अन्यथा {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(धकेलो_Deletep(enVarrefp), enVarrefp);
                        }
                    } अन्यथा {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].धकेलो_back(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = nullptr;
                    refStrengthp->m_enVarrefp = nullptr;
                    refStrengthp->m_enRhsp = nullptr;
                } अन्यथा {
                    AstAssignW* ध्रुव enLhspAssignp = नूतन AstAssignW{
                        refp->fileline(),
                        नूतन AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    यदि (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_वृक्ष(असत्य));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->addStmtsp(नूतन AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_पंक्ति* ध्रुव fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_गाँठ_व्यंजक* ध्रुव ref1p = नूतन AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_गाँठ_व्यंजक* ध्रुव ref2p = नूतन AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_गाँठ_व्यंजक* ध्रुव andp = नूतन AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : नूतन AstOr{fl, orp, andp};

            Ast_गाँठ_व्यंजक* ध्रुव ref3p = नूतन AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : नूतन AstOr{ref3p->fileline(), enp, ref3p};
        }
        AstAssignW* ध्रुव assp = नूतन AstAssignW{
            varp->fileline(), नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->addStmtsp(नूतन AstAlways{assp});

        AstAssignW* ध्रुव enAssp = नूतन AstAssignW{
            envarp->fileline(), नूतन AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->addStmtsp(नूतन AstAlways{enAssp});
    }

    शून्य जोड़ो_Procedural_Enable_Assignments(Ast_गाँठ_Module* nodep) {
        प्रति (स्वतः& assignEnables : m_proceduralEnables) {
            Ast_गाँठ_Assign* ध्रुव assignp = assignEnables.first;
            Ast_गाँठ_व्यंजक* combinedLhsp = assignp->lhsp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* combinedRhsp = assignp->rhsp()->unlinkFrBack();
            प्रति (ProceduralEnable& enable : assignEnables.second) {
                combinedLhsp = नूतन AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = नूतन AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            यदि (!VN_IS(assignp, AssignDly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ध्रुव sampledp
                    = नूतन AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvtToStr(m_unique++), combinedRhsp->dtypep()};
                nodep->addStmtsp(sampledp);
                assignp->lhsp(नूतन AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                AstAssign* ध्रुव applyp
                    = नूतन AstAssign{assignp->fileline(), combinedLhsp,
                                    नूतन AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->addNextHere(applyp);
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
    शून्य जोड़ो_Tristates_Signal(Ast_गाँठ_Module* nodep, AstVar* ध्रुव invarp, RefStrengthVec* refsp,
                               तार्किक isIfaceTri, ध्रुव string& ifaceDottedPath,
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
        AstVar* envarp = nullptr;
        AstVar* outvarp = nullptr;  // __out
        AstVar* lhsp = invarp;  // Variable to assign drive-value to (<in> or __out)
        ध्रुव तार्किक is_शीर्ष_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_शीर्ष();
        यदि (!isIfaceTri) {
            यदि (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty())
                 && is_शीर्ष_Inout)
                || (!nodep->is_शीर्ष() && invarp->isIO())) {
                // This var becomes an input
                invarp->varType2In();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = getCreateOutVarp(invarp, is_शीर्ष_Inout);
                outvarp->varType2Out();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_शीर्ष_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                यदि (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                यदि (!m_varAux(invarp).bitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } अन्यथा यदि (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_गाँठ_व्यंजक* orp = nullptr;
        Ast_गाँठ_व्यंजक* enp = nullptr;

        std::sort(refsp->begin(), refsp->end(),
                  [](RefStrength a, RefStrength b) { प्रत्यागम a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        यावत् (beginStrength != refsp->end()) {
            RefStrengthVec::iterator endStrength = beginStrength + 1;
            यावत् (endStrength != refsp->end()
                   && endStrength->m_strength == beginStrength->m_strength)
                endStrength++;

            File_पंक्ति* ध्रुव fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strengthPrefix;
            यदि (isIfaceTri && !ifaceDottedPath.empty()) {
                strengthPrefix = ifaceDottedPath;
                std::replace(strengthPrefix.begin(), strengthPrefix.end(), '.', '_');
                strengthPrefix += "__";
            }
            ध्रुव string strengthVarName
                = strengthPrefix + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ध्रुव varStrengthp = नूतन AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->setDfgTriLowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->addStmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ध्रुव enVarStrengthp = नूतन AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->setDfgTriLowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->addStmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     endStrength);

            Ast_गाँठ_व्यंजक* व्यंजक_Current_Strengthp;
            यदि (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                व्यंजक_Current_Strengthp
                    = नूतन AstAnd{fl, नूतन AstVarRef{fl, varStrengthp, VAccess::READ},
                                 नूतन AstNot{fl, enp->clone_वृक्ष_Pure(असत्य)}};
            } अन्यथा {
                व्यंजक_Current_Strengthp = नूतन AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? व्यंजक_Current_Strengthp : नूतन AstOr{fl, orp, व्यंजक_Current_Strengthp};

            Ast_गाँठ_व्यंजक* ध्रुव enVarStrengthRefp
                = नूतन AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : नूतन AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = endStrength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        यदि (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->setDfgAllowMultidriveTri();
            Ast_गाँठ_Module* ध्रुव ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ध्रुव पूर्णाङ्क contribIdx = static_cast<पूर्णाङ्क>(m_ifaceContribs[invarp].size());
            File_पंक्ति* ध्रुव fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ध्रुव contribOutp = नूतन AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvtToStr(contribIdx), invarp};
            contribOutp->setDfgTriLowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->addStmtsp(contribOutp);

            AstVar* ध्रुव contribEnp = नूतन AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvtToStr(contribIdx), invarp};
            contribEnp->setDfgTriLowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->addStmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            यदि (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                addModportVarRefIfMissing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                addModportVarRefIfMissing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_गाँठ_Var_Ref* ध्रुव lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_गाँठ_Var_Ref*>(
                                                    नूतन AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_गाँठ_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ध्रुव assp = नूतन AstAssignW{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(नूतन AstAlways{assp});
            }
            {
                Ast_गाँठ_Var_Ref* ध्रुव lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_गाँठ_Var_Ref*>(
                                                    नूतन AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_गाँठ_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* ध्रुव assp = नूतन AstAssignW{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(नूतन AstAlways{assp});
            }

            m_ifaceContribs[invarp].धकेलो_back({contribEnp, contribOutp});
            प्रत्यागम;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ध्रुव तार्किक hasPull = m_varAux(lhsp).pullp || hasPerBitPulls(lhsp);
        यदि (!outvarp || hasPull) {
            Ast_गाँठ_व्यंजक* undrivenp;
            यदि (envarp) {
                undrivenp = नूतन AstNot{envarp->fileline(),
                                       नूतन AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } अन्यथा यदि (enp) {
                undrivenp = नूतन AstNot{enp->fileline(), enp};
                enp = nullptr;  // moved into undrivenp
            } अन्यथा {
                undrivenp = newAllZerosOrOnes(invarp, सत्य);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            यदि (hasPerBitPulls(lhsp)) {
                pullConstp = createPerBitPullConst(lhsp);
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
            AstAssignW* ध्रुव enAssp = नूतन AstAssignW{
                enp->fileline(), नूतन AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->addStmtsp(नूतन AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        AstAssignW* ध्रुव assp = नूतन AstAssignW{
            lhsp->fileline(), नूतन AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->addStmtsp(नूतन AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        यदि ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty()) && is_शीर्ष_Inout) {
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
            यदि (!v3Global.opt.libCreate().empty()) {
                ध्रुव पूर्णाङ्क id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(सत्य);
            }
        }
    }

    तार्किक isOnlyAssignmentIsToLhsVar(ध्रुव AstAssignW* ध्रुव nodep) {
        यदि (ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            स्वतः foundIt = m_assigns.find(varRefp->varp());
            यदि (foundIt != m_assigns.end()) {
                स्वतः ध्रुव& assignsToVar = foundIt->second;
                यदि (assignsToVar.size() == 1 && assignsToVar[0] == nodep) प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }

    शून्य addToAssignmentList(AstAssignW* nodep) {
        यदि (AstVarRef* ध्रुव varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            यदि (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].धकेलो_back(nodep);
            } अन्यथा यदि (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlinkFrBack()->delete_वृक्ष();
            }
        } अन्यथा यदि (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ध्रुव AstAssignW* ध्रुव nodep, तार्किक मूल्य) {
        यदि (AstStrengthSpec* ध्रुव strengthSpec = nodep->strengthSpecp()) {
            प्रत्यागम मूल्य ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        प्रत्यागम VStrength::STRONG;  // default strength is strong
    }

    तार्किक assignment_Of_मूल्य_On_All_Bits(ध्रुव AstAssignW* ध्रुव nodep, तार्किक मूल्य) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->rhsp(), Const)) {
            ध्रुव V3_संख्या अंक = constp->अंक();
            प्रत्यागम मूल्य ? अंक.isEqAllOnes() : अंक.isEqZero();
        }
        प्रत्यागम असत्य;
    }

    AstAssignW* get_Strongest_Assignment_Of_मूल्य(ध्रुव Assigns& assigns, तार्किक मूल्य) {
        स्वतः maxIt = std::max_element(
            assigns.begin(), assigns.end(), [&](ध्रुव AstAssignW* ap, ध्रुव AstAssignW* bp) {
                ध्रुव तार्किक मूल्यावली_On_Rhs_A = assignment_Of_मूल्य_On_All_Bits(ap, मूल्य);
                ध्रुव तार्किक मूल्यावली_On_Rhs_B = assignment_Of_मूल्य_On_All_Bits(bp, मूल्य);
                यदि (!मूल्यावली_On_Rhs_A) प्रत्यागम मूल्यावली_On_Rhs_B;
                यदि (!मूल्यावली_On_Rhs_B) प्रत्यागम असत्य;
                प्रत्यागम getStrength(ap, मूल्य) < getStrength(bp, मूल्य);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        प्रत्यागम assignment_Of_मूल्य_On_All_Bits(*maxIt, मूल्य) ? *maxIt : nullptr;
    }

    तार्किक isAssignmentNotStrongerThanStrength(ध्रुव AstAssignW* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ध्रुव uint8_t strength0 = getStrength(assignp, 0);
        ध्रुव uint8_t strength1 = getStrength(assignp, 1);
        प्रत्यागम (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_Of_मूल्य_On_All_Bits(assignp, 0))
               || (strength1 <= strength && assignment_Of_मूल्य_On_All_Bits(assignp, 1));
    }

    Ast_गाँठ_व्यंजक* new_Merge_व्यंजक(Ast_गाँठ_व्यंजक* ध्रुव lhsp, Ast_गाँठ_व्यंजक* ध्रुव rhsp, File_पंक्ति* ध्रुव fl,
                              तार्किक isWor) {
        Ast_गाँठ_व्यंजक* व्यंजक = nullptr;
        यदि (isWor)
            व्यंजक = नूतन AstOr{fl, lhsp, rhsp};
        अन्यथा
            व्यंजक = नूतन AstAnd{fl, lhsp, rhsp};
        प्रत्यागम व्यंजक;
    }

    शून्य mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        प्रति (स्वतः& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            यदि (assigns.size() <= 1) अनुवर्त;
            AstVar* ध्रुव varp = varpAssigns.first;
            यदि (!varp->isWiredNet()) अनुवर्त;
            स्वतः it = assigns.begin();
            AstAssignW* ध्रुव assignWp0 = *it;
            File_पंक्ति* ध्रुव fl = assignWp0->fileline();
            Ast_गाँठ_व्यंजक* wExp = nullptr;
            यावत् (++it != assigns.end()) {
                AstAssignW* assignWpi = *it;
                यदि (!wExp) {
                    wExp
                        = new_Merge_व्यंजक(assignWp0->rhsp()->clone_वृक्ष_Pure(असत्य),
                                       assignWpi->rhsp()->clone_वृक्ष_Pure(असत्य), fl, varp->isWor());
                } अन्यथा {
                    wExp = new_Merge_व्यंजक(wExp, assignWpi->rhsp()->clone_वृक्ष_Pure(असत्य), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((assignWpi->unlinkFrBack()->delete_वृक्ष()), assignWpi);
            }
            AstVarRef* ध्रुव wVarRef = नूतन AstVarRef{fl, varp, VAccess::WRITE};
            AstAssignW* ध्रुव wAssignp = नूतन AstAssignW{fl, wVarRef, wExp};
            assignWp0->replaceWith(wAssignp);
            VL_DO_DANGLING(धकेलो_Deletep(assignWp0), assignWp0);
            assigns.clear();
            assigns.धकेलो_back(wAssignp);
        }
    }

    शून्य removeNotStrongerAssignments(Assigns& assigns, ध्रुव AstAssignW* strongestp,
                                      uint8_t greatestKnownStrength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        स्वतः removedIt = std::remove_if(assigns.begin(), assigns.end(), [&](AstAssignW* assignp) {
            यदि (assignp == strongestp) प्रत्यागम असत्य;
            यदि (isAssignmentNotStrongerThanStrength(assignp, greatestKnownStrength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(धकेलो_Deletep(assignp->unlinkFrBack()), assignp);
                प्रत्यागम सत्य;
            }
            प्रत्यागम असत्य;
        });
        assigns.erase(removedIt, assigns.end());
    }

    शून्य removeAssignmentsNotStrongerThanUniformConstant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        प्रति (स्वतः& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            यदि (assigns.size() <= 1) अनुवर्त;
            ध्रुव AstAssignW* ध्रुव strongest0p = get_Strongest_Assignment_Of_मूल्य(assigns, 0);
            ध्रुव AstAssignW* ध्रुव strongest1p = get_Strongest_Assignment_Of_मूल्य(assigns, 1);
            ध्रुव AstAssignW* strongestp = nullptr;
            uint8_t greatestKnownStrength = 0;
            ध्रुव स्वतः getIfStrongest
                = [&](ध्रुव AstAssignW* ध्रुव strongestCandidatep, तार्किक मूल्य) {
                      यदि (!strongestCandidatep) प्रत्यागम;
                      ध्रुव uint8_t strength = getStrength(strongestCandidatep, मूल्य);
                      यदि (strength >= greatestKnownStrength) {
                          greatestKnownStrength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            यदि (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    शून्य removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        प्रति (स्वतः& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            यदि (assigns.size() <= 1) अनुवर्त;
            स्वतः maxIt = std::max_element(
                assigns.begin(), assigns.end(), [&](ध्रुव AstAssignW* ap, ध्रुव AstAssignW* bp) {
                    यदि (m_tgraph.isTristate(ap)) प्रत्यागम !m_tgraph.isTristate(bp);
                    यदि (m_tgraph.isTristate(bp)) प्रत्यागम असत्य;
                    ध्रुव uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ध्रुव uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    प्रत्यागम minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ध्रुव AstAssignW* ध्रुव strongestp = m_tgraph.isTristate(*maxIt) ? nullptr : *maxIt;
            यदि (strongestp) {
                ध्रुव uint8_t greatestKnownStrength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    // VISITORS
    शून्य visit(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        यदि (m_graphing) {
            यदि (!m_alhs && nodep->अंक().hasZ()) m_tgraph.setTristate(nodep);
        } अन्यथा {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            यदि (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ध्रुव varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_गाँठ* ध्रुव newp = नूतन AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा यदि (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_पंक्ति* ध्रुव fl = nodep->fileline();
                AstConst* ध्रुव enp = getNonZConstp(nodep);
                V3_संख्या num1{nodep, nodep->width()};
                num1.opAnd(nodep->अंक(), enp->अंक());  // 01X->01X, Z->0
                AstConst* ध्रुव newconstp = नूतन AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    शून्य visit(AstCond* nodep) override {
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
            Ast_गाँठ_व्यंजक* ध्रुव condp = nodep->condp();
            यदि (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_गाँठ_व्यंजक* ध्रुव thenp = nodep->thenp();
            Ast_गाँठ_व्यंजक* ध्रुव elsep = nodep->elsep();
            यदि (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_गाँठ_व्यंजक* ध्रुव en1p = getEnp(thenp);
                Ast_गाँठ_व्यंजक* ध्रुव en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_गाँठ_व्यंजक* ध्रुव enp
                    = नूतन AstCond{nodep->fileline(), condp->clone_वृक्ष(असत्य), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                यदि (thenp->user1p() && !thenp->user1p()->backp()) धकेलो_Deletep(thenp->user1p());
                यदि (elsep->user1p() && !elsep->user1p()->backp()) धकेलो_Deletep(elsep->user1p());
                thenp->user1p(nullptr);
                elsep->user1p(nullptr);
            }
        }
    }

    शून्य visit(Ast_व्यंजक_Stmt* nodep) override {
        iterateChildren(nodep);
        यदि (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } अन्यथा यदि (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(nullptr);
            m_tgraph.didProcess(nodep);
        }
    }

    शून्य visit(AstSel* nodep) override {
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
                    Ast_गाँठ_व्यंजक* ध्रुव newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), गाँठ_व्यंजक));
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
                    Ast_गाँठ_व्यंजक* ध्रुव en1p = getEnp(nodep->fromp());
                    Ast_गाँठ_व्यंजक* ध्रुव enp
                        = नूतन AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_वृक्ष_Pure(सत्य),
                                     nodep->widthConst()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    शून्य visit(AstConcat* nodep) override {
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
                    Ast_गाँठ_व्यंजक* ध्रुव enp = VN_AS(nodep->user1p(), गाँठ_व्यंजक);
                    nodep->user1p(nullptr);
                    nodep->lhsp()->user1p(नूतन AstSel{nodep->fileline(), enp->clone_वृक्ष_Pure(सत्य),
                                                     nodep->rhsp()->width(),
                                                     nodep->lhsp()->width()});
                    nodep->rhsp()->user1p(
                        नूतन AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->width()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } अन्यथा {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_गाँठ_व्यंजक* ध्रुव expr1p = nodep->lhsp();
                Ast_गाँठ_व्यंजक* ध्रुव expr2p = nodep->rhsp();
                यदि (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_गाँठ_व्यंजक* ध्रुव en1p = getEnp(expr1p);
                    Ast_गाँठ_व्यंजक* ध्रुव en2p = getEnp(expr2p);
                    Ast_गाँठ_व्यंजक* ध्रुव enp = नूतन AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(nullptr);
                    expr2p->user1p(nullptr);
                }
            }
        }
    }

    शून्य visit(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        यदि (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.setTristate(nodep);
        } अन्यथा {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            यदि (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                प्रत्यागम;
            }
            m_tgraph.didProcess(nodep);
            Ast_गाँठ_व्यंजक* ध्रुव expr1p = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* ध्रुव expr2p = nodep->rhsp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* enp;
            यदि (Ast_गाँठ_व्यंजक* ध्रुव en2p = VN_AS(expr2p->user1p(), गाँठ_व्यंजक)) {
                enp = नूतन AstAnd{nodep->fileline(), expr1p, en2p};
            } अन्यथा {
                enp = expr1p;
            }
            expr1p->user1p(nullptr);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }

    शून्य visitAndOr(Ast_गाँठ_Biop* nodep, तार्किक isAnd) {
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
            Ast_गाँठ_व्यंजक* ध्रुव expr1p = nodep->lhsp();
            Ast_गाँठ_व्यंजक* ध्रुव expr2p = nodep->rhsp();
            यदि (!expr1p->user1p() && !expr2p->user1p()) {
                प्रत्यागम;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_गाँठ_व्यंजक* en1p = getEnp(expr1p);
            Ast_गाँठ_व्यंजक* en2p = getEnp(expr2p);
            Ast_गाँठ_व्यंजक* subexpr1p = expr1p->clone_वृक्ष_Pure(असत्य);
            Ast_गाँठ_व्यंजक* subexpr2p = expr2p->clone_वृक्ष_Pure(असत्य);
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
            Ast_गाँठ_व्यंजक* ध्रुव enp = नूतन AstOr{
                nodep->fileline(),
                नूतन AstAnd{nodep->fileline(), en1p, नूतन AstOr{nodep->fileline(), en2p, subexpr1p}},
                नूतन AstAnd{nodep->fileline(), en2p->clone_वृक्ष(असत्य), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(nullptr);
            expr2p->user1p(nullptr);
        }
    }
    शून्य visit(AstAnd* nodep) override { visitAndOr(nodep, सत्य); }
    शून्य visit(AstOr* nodep) override { visitAndOr(nodep, असत्य); }

    शून्य visitAssign(Ast_गाँठ_Assign* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        यदि (VN_IS(nodep->rhsp(), CReset)) प्रत्यागम;
        यदि (AstAssignW* ध्रुव assignWp = VN_CAST(nodep, AssignW)) {
            m_currentDelayp = VN_CAST(assignWp->timingControlp(), Delay);
        }
        यदि (m_graphing) {
            यदि (AstAssignW* assignWp = VN_CAST(nodep, AssignW)) {
                यदि (assignWp->getLhsNetDelay()) प्रत्यागम;
                यदि (assignWp->timingControlp() && !m_currentDelayp) प्रत्यागम;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                यदि (m_currentDelayp && m_currentDelayp->fallDelay()) प्रत्यागम;
                यदि (!assignWp->timingControlp()) addToAssignmentList(assignWp);
            }

            यदि (nodep->user2() & U2_GRAPHING) प्रत्यागम;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterateAndNextNull(nodep->rhsp());
            m_alhs = सत्य;
            iterateAndNextNull(nodep->lhsp());
            m_alhs = असत्य;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } अन्यथा {
            यदि (nodep->user2() & U2_NONGRAPH) {
                प्रत्यागम;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterateAndNextNull(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            यदि (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, AssignW), nodep,
                            "Continuous assignment inside procedural process");
                Ast_गाँठ_व्यंजक* ध्रुव enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), गाँठ_व्यंजक)->clone_वृक्ष_Pure(असत्य)
                          : newAllZerosOrOnes(nodep->lhsp(), सत्य);
                ध्रुव तार्किक inserted = m_assignEnps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            यदि (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(nullptr);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } अन्यथा {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                यदि (AstSel* ध्रुव selp = VN_CAST(nodep->lhsp(), Sel)) {
                    यदि (Ast_गाँठ_Var_Ref* ध्रुव varrefp = VN_CAST(selp->fromp(), गाँठ_Var_Ref)) {
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
            यदि (AstAssignW* ध्रुव assignWp = VN_CAST(nodep, AssignW)) {
                यदि (AstStrengthSpec* ध्रुव specp = assignWp->strengthSpecp()) {
                    यदि (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        यदि (!isOnlyAssignmentIsToLhsVar(assignWp)) {
                            assignWp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } अन्यथा {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterateAndNextNull(nodep->lhsp());
        }
    }
    शून्य visit(AstAssignW* nodep) override { visitAssign(nodep); }
    शून्य visit(AstAssign* nodep) override { visitAssign(nodep); }
    शून्य visit(AstAssignDly* nodep) override { visitAssign(nodep); }

    शून्य visit(Ast_गाँठ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ध्रुव AstAlways* ध्रुव alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN ? nullptr : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    शून्य visit(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_inAlias);
        m_inAlias = सत्य;
        यदि (m_graphing) {
            यदि (nodep->user2() & U2_GRAPHING) प्रत्यागम;
            m_alhs = सत्य;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            प्रति (Ast_गाँठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

    शून्य visitCaseEq(Ast_गाँठ_Biop* nodep, तार्किक neq) {
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
                Ast_गाँठ_व्यंजक* ध्रुव rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                File_पंक्ति* ध्रुव fl = nodep->fileline();
                Ast_गाँठ_व्यंजक* enRhsp;
                यदि (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), गाँठ_व्यंजक);
                    rhsp->user1p(nullptr);
                } अन्यथा {
                    enRhsp = get_En_व्यंजक_Based_On_Originalp(rhsp);
                }
                Ast_गाँठ_व्यंजक* newp
                    = नूतन AstLogAnd{fl, नूतन AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    नूतन AstEqCase{fl, नूतन AstConst{fl, constp->अंक()}, rhsp}};
                constp->user1p(nullptr);
                यदि (neq) newp = नूतन AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा यदि (constp && nodep->rhsp()->user1p()) {
                File_पंक्ति* ध्रुव fl = nodep->fileline();
                constp->unlinkFrBack();
                Ast_गाँठ_व्यंजक* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_गाँठ_व्यंजक* newp = नूतन AstLogAnd{fl,
                                                  नूतन AstEq{fl, newAllZerosOrOnes(constp, असत्य),
                                                            VN_AS(rhsp->user1p(), गाँठ_व्यंजक)},
                                                  // Keep the caseeq if there are X's present
                                                  नूतन AstEqCase{fl, constp, rhsp}};
                यदि (neq) newp = नूतन AstLogNot{fl, newp};
                rhsp->user1p(nullptr);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा {
                checkUnhandled(nodep);
            }
        }
    }
    शून्य visitEqNeqWild(Ast_गाँठ_Biop* nodep) {
        यदि (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterateAndNextNull(nodep->lhsp());
        यदि (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            प्रत्यागम;
        }
    }
    शून्य visit(AstEqCase* nodep) override { visitCaseEq(nodep, असत्य); }
    शून्य visit(AstNeqCase* nodep) override { visitCaseEq(nodep, सत्य); }
    शून्य visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    शून्य visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }

    शून्य visit(Ast_गिनती_Bits* nodep) override {
        std::array<तार्किक, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->अंक().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->अंक().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->अंक().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        यदि (m_graphing) {
            iterateAndNextNull(nodep->lhsp());
            यदि (!dropop[0]) iterateAndNextNull(nodep->rhsp());
            यदि (!dropop[1]) iterateAndNextNull(nodep->thsp());
            यदि (!dropop[2]) iterateAndNextNull(nodep->fhsp());
        } अन्यथा {
            Ast_गाँठ_व्यंजक* nonXp = nullptr;
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
                nodep->unlinkFrBack(&relinkHandle);
                Ast_गाँठ_व्यंजक* newp = नूतन Ast_गिनती_Ones{
                    nodep->fileline(), नूतन AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                यदि (nonXp) {  // Need to still count '0 or '1 or 'x's
                    यदि (dropop[0]) {
                        nodep->rhsp()->unlinkFrBack()->delete_वृक्ष();
                        nodep->rhsp(nonXp->clone_वृक्ष_Pure(सत्य));
                    }
                    यदि (dropop[1]) {
                        nodep->thsp()->unlinkFrBack()->delete_वृक्ष();
                        nodep->thsp(nonXp->clone_वृक्ष_Pure(सत्य));
                    }
                    यदि (dropop[2]) {
                        nodep->fhsp()->unlinkFrBack()->delete_वृक्ष();
                        nodep->fhsp(nonXp->clone_वृक्ष_Pure(सत्य));
                    }
                    newp = नूतन AstAdd{nodep->fileline(), nodep, newp};
                } अन्यथा {
                    धकेलो_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = nullptr;
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
                m_tgraph.setTristate(nodep);
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
                ध्रुव AstConst* ध्रुव lsbp = selp ? VN_CAST(selp->lsbp(), Const) : nullptr;
                यदि (lsbp) {
                    ध्रुव पूर्णाङ्क lsb = lsbp->toSInt();
                    प्रति (पूर्णाङ्क i = 0; i < selp->widthConst(); ++i) {
                        setBitPullDirection(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } अन्यथा {
                    setPullDirection(varrefp->varp(), nodep);
                }
            }
        }
        यदि (!m_graphing) {
            nodep->unlinkFrBack();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
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
    शून्य visit(AstPin* nodep) override {
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
            ध्रुव तार्किक inDeclProcessing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::निवेश
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
                m_tgraph.setTristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } अन्यथा यदि (inDeclProcessing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ध्रुव AstAssignW* ध्रुव reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, सत्य, सत्य);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.setTristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ध्रुव enVarp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvtToStr(m_unique++),
                                              VFlagBitPacked{}, enModVarp->width()};
            यदि (inDeclProcessing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->varType2In();
            } अन्यथा {
                enModVarp->varType2Out();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ध्रुव enpinp
                = नूतन AstPin{nodep->fileline(), nodep->pin_अंक(),
                             enModVarp->name(),  // should be {var}"__en"
                             नूतन AstVarRef{nodep->fileline(), enVarp,
                                           inDeclProcessing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->addNextHere(enpinp);
            m_modp->addStmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ध्रुव AstAssignW* outAssignp = nullptr;  // If reconnected, the related assignment
            AstPin* outpinp = nullptr;
            AstVar* ध्रुव outModVarp = m_varAux(nodep->modVarp()).outVarp;
            यदि (!outModVarp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                यदि (!m_modp->is_शीर्ष()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } अन्यथा {
                Ast_गाँठ_व्यंजक* ध्रुव outexprp = VN_AS(nodep->exprp(), गाँठ_व्यंजक)
                                                  ->clone_वृक्ष_Pure(असत्य);  // Note has lvalue() set
                outpinp = नूतन AstPin{nodep->fileline(), nodep->pin_अंक(),
                                     outModVarp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(outModVarp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->addNextHere(outpinp);
                // Simplify
                यदि (inDeclProcessing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, सत्य};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                outAssignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        सत्य);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, outAssignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ध्रुव TristatePinVisitor visitor{nodep->exprp(), m_tgraph, असत्य};
            ध्रुव Ast_गाँठ* ध्रुव inAssignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, सत्य);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, inAssignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            यदि (!outAssignp) {
                यदि (!outpinp) {
                    exprrefp = nullptr;  // Primary input only
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
                exprrefp = VN_CAST(outAssignp->rhsp(),
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
                यदि (!outAssignp) {
                    map_जोड़ो_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            यदि (exprrefp) {
                AstPull* ध्रुव pullp = m_varAux(nodep->modVarp()).pullp;
                ध्रुव स्वतः& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = nullptr;
                पूर्णाङ्क selLsb = 0;
                पूर्णाङ्क selWidth = 0;
                यदि (outAssignp) {
                    यदि (AstSel* ध्रुव selp = VN_CAST(outAssignp->lhsp(), Sel)) {
                        AstVarRef* ध्रुव vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const);
                        यदि (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->toSInt();
                            selWidth = selp->widthConst();
                        }
                    }
                }

                यदि (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    setPullDirection(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    यदि (selVarp) {
                        ध्रुव पूर्णाङ्क direction = pullp->direction();
                        प्रति (पूर्णाङ्क i = 0; i < selWidth; ++i) {
                            setBitPullDirection(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                यदि (!srcBitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    प्रति (ध्रुव स्वतः& pair : srcBitPulls) {
                        setBitPullDirection(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    यदि (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        प्रति (ध्रुव स्वतः& pair : srcBitPulls) {
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
        अन्यथा {
            यदि (nodep->user2() & U2_NONGRAPH) प्रत्यागम;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    शून्य handle_गाँठ_Var_Ref(Ast_गाँठ_Var_Ref* nodep) {
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
                m_tgraph.setTristate(nodep->varp());
            }
        } अन्यथा {
            यदि (nodep->user2() & U2_NONGRAPH) प्रत्यागम;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            यदि (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                यदि (m_inAlias) {
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
                map_जोड़ो_Lhs_Var_Ref(nodep);
            } अन्यथा यदि (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_पंक्ति* ध्रुव fl = nodep->fileline();
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

    शून्य visit(AstVarRef* nodep) override { handle_गाँठ_Var_Ref(nodep); }
    शून्य visit(AstVarXRef* nodep) override { handle_गाँठ_Var_Ref(nodep); }

    शून्य visit(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        यदि (m_graphing) {
            // If tri0/1 force a pullup
            यदि (nodep->user2() & U2_GRAPHING) प्रत्यागम;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            यदि (nodep->isPulldown() || nodep->isPullup()) {
                Ast_गाँठ* ध्रुव newp = नूतन AstPull{
                    nodep->fileline(), नूतन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->addNextHere(newp);
                // We'll iterate on the new AstPull later
            }
            यदि (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.setTristate(nodep);
            }
        } अन्यथा {  // !graphing
            यदि (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    शून्य visit(Ast_गाँठ_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        यदि (m_processedIfaces && VN_IS(nodep, Iface)) प्रत्यागम;
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
        m_graphing = असत्य;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = nullptr;
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
        removeAssignmentsNotStrongerThanUniformConstant();
        // Use graph to find tristate signals
        m_tgraph.graphWalk(nodep);

        // Remove all assignments not stronger than the strongest non-tristate RHS
        removeAssignmentsNotStrongerThanNonTristate();

        // Build the LHS drivers map for this module
        iterateChildren(nodep);
        // Insert new logic for all tristates
        जोड़ो_Tristates(nodep);
        जोड़ो_Procedural_Enable_Assignments(nodep);

        प्रति (स्वतः& assignEnp : m_assignEnps) {
            VL_DO_DANGLING(assignEnp.second->delete_वृक्ष(), assignEnp.second);
        }
        m_assignEnps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    शून्य visit(AstClass* nodep) override {
        // don't deal with classes
    }
    शून्य visit(Ast_गाँठ_FTask* nodep) override {
        // don't deal with functions
    }

    शून्य visit(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterateAndNextNull(nodep->stmtsp());
    }

    शून्य visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = असत्य;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    शून्य visit(Ast_गाँठ* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    शून्य combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        प्रति (ध्रुव std::pair<AstVar* ध्रुव, std::vector<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ध्रुव invarp = kv.first;
            ध्रुव std::vector<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_गाँठ_Module* ध्रुव ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_पंक्ति* ध्रुव fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.size() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_गाँठ_व्यंजक* orp = nullptr;
            Ast_गाँठ_व्यंजक* enp = nullptr;

            प्रति (ध्रुव std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ध्रुव contribEnp = contrib.first;
                AstVar* ध्रुव contribOutp = contrib.second;

                Ast_गाँठ_व्यंजक* ध्रुव outRefp = नूतन AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_गाँठ_व्यंजक* ध्रुव enRefp = नूतन AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_गाँठ_व्यंजक* ध्रुव andp = नूतन AstAnd{fl, outRefp, enRefp};

                orp = (!orp) ? andp : नूतन AstOr{fl, orp, andp};

                Ast_गाँठ_व्यंजक* ध्रुव enRef2p = नूतन AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : नूतन AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            यदि (!envarp) {
                envarp = नूतन AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->setDfgTriLowered();
                ifaceModp->addStmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            AstAssignW* ध्रुव enAssp
                = नूतन AstAssignW{fl, नूतन AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->addStmtsp(नूतन AstAlways{enAssp});

            // Pull resolution
            ध्रुव AstPull* ध्रुव pullp = m_varAux(invarp).pullp;
            ध्रुव तार्किक pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_गाँठ_व्यंजक* undrivenp = नूतन AstNot{fl, नूतन AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = नूतन AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = नूतन AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            AstAssignW* ध्रुव assp
                = नूतन AstAssignW{fl, नूतन AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->addStmtsp(नूतन AstAlways{assp});
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::vector<Ast_गाँठ_Module*> ifacesp;
        प्रति (Ast_गाँठ* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            यदि (VN_IS(modp, Iface)) ifacesp.धकेलो_back(VN_AS(modp, गाँठ_Module));
        }
        प्रति (स्वतः it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = सत्य;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ifdef VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::vector<Ast_गाँठ*> unusedRootps;
        netlistp->foreach([&](Ast_गाँठ* nodep) {
            Ast_गाँठ* ध्रुव enp = nodep->user1p();
            यदि (!enp) प्रत्यागम;
            यदि (enp->backp()) प्रत्यागम;
            unusedRootps.emplace_back(enp);
        });
        प्रति (Ast_गाँठ* ध्रुव nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
#endif
    }
    ~TristateVisitor() override {
        V3Stats::addStat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

शून्य V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("tristate", 0, dump_वृक्ष_Either_Level() >= 3);
}
