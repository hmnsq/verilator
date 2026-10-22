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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Tristate.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Inst.h"
#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം TristateBaseVisitor VL_NOT_FINAL : പൊതു VNVisitor {
പൊതു:
    // METHODS
};

//######################################################################
// Graph support classes

വർഗ്ഗം TristateVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_കണ്ണി* സ്ഥിരം m_nodep;
    തർക്കം m_isTristate = അസത്യം;  // Logic indicates a tristate
    തർക്കം m_feedsTri = അസത്യം;  // Propagates to a tristate node (on RHS)
    തർക്കം m_processed = അസത്യം;  // Tristating was cleaned up
പൊതു:
    TristateVertex(V3Graph* graphp, Ast_കണ്ണി* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    Ast_കണ്ണി* nodep() സ്ഥിരം VL_MT_STABLE { തിരികെ m_nodep; }
    സ്ഥിരം AstVar* varp() സ്ഥിരം { തിരികെ VN_CAST(nodep(), Var); }
    string name() സ്ഥിരം override {
        തിരികെ ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvtToHex(nodep())));
    }
    string dotColor() സ്ഥിരം override {
        തിരികെ (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_വരി* fileline() സ്ഥിരം override { തിരികെ nodep()->fileline(); }
    തർക്കം isTristate() സ്ഥിരം VL_MT_SAFE { തിരികെ m_isTristate; }
    ശൂന്യം isTristate(തർക്കം flag) { m_isTristate = flag; }
    തർക്കം feedsTri() സ്ഥിരം VL_MT_SAFE { തിരികെ m_feedsTri; }
    ശൂന്യം feedsTri(തർക്കം flag) { m_feedsTri = flag; }
    തർക്കം processed() സ്ഥിരം { തിരികെ m_processed; }
    ശൂന്യം processed(തർക്കം flag) { m_processed = flag; }
};

//######################################################################

വർഗ്ഗം TristateGraph final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
പൊതു:
    using VarVec = std::vector<AstVar*>;

സ്വകാര്യ:
    // MEMBERS
    V3Graph m_graph;  // Logic graph

പൊതു:
    // CONSTRUCTORS
    TristateGraph() { clearAndCheck(); }
    മായാ ~TristateGraph() { clearAndCheck(); }
    VL_UNCOPYABLE(TristateGraph);

സ്വകാര്യ:
    // METHODS

    TristateVertex* makeVertex(Ast_കണ്ണി* nodep) {
        TristateVertex* vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        എങ്കിൽ (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = പുതിയ TristateVertex{&m_graph, nodep};
            nodep->user4p(vertexp);
        }
        തിരികെ vertexp;
    }

    // METHODS - Graph optimization
    ശൂന്യം graphWalkRecurseFwd(TristateVertex* vtxp, പൂർണ്ണസംഖ്യ level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        എങ്കിൽ (!vtxp->isTristate()) തിരികെ;  // tristate involved
        എങ്കിൽ (vtxp->user() == 1) തിരികെ;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        എങ്കിൽ (!vtxp->varp()) {  // not a var where we stop the recursion
            വേണ്ടി (V3GraphEdge& edge : vtxp->outEdges()) {
                TristateVertex* സ്ഥിരം vvertexp = static_cast<TristateVertex*>(edge.മുകൾ());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                എങ്കിൽ (!vvertexp->isTristate()) {
                    vvertexp->isTristate(സത്യം);
                    graphWalkRecurseFwd(vvertexp, level + 1);
                }
            }
        } അല്ലെങ്കിൽ {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            വേണ്ടി (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* സ്ഥിരം vvertexp = static_cast<TristateVertex*>(edge.fromp());
                എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    എങ്കിൽ (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(സത്യം);
                        graphWalkRecurseFwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    ശൂന്യം graphWalkRecurseBack(TristateVertex* vtxp, പൂർണ്ണസംഖ്യ level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        എങ്കിൽ (!(vtxp->isTristate() || vtxp->feedsTri())) തിരികെ;  // tristate involved
        എങ്കിൽ (vtxp->user() == 3) തിരികെ;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        എങ്കിൽ (!vtxp->varp()) {  // not a var where we stop the recursion
            വേണ്ടി (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* സ്ഥിരം vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                എങ്കിൽ (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(സത്യം);
                    graphWalkRecurseBack(vvertexp, level + 1);
                }
            }
        }
    }

പൊതു:
    // METHODS
    തർക്കം empty() സ്ഥിരം { തിരികെ m_graph.empty(); }
    ശൂന്യം clearAndCheck() {
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            സ്ഥിരം TristateVertex& vvertex = static_cast<TristateVertex&>(vtx);
            എങ്കിൽ (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_graph.clear();
        Ast_കണ്ണി::user4_Clear_വൃക്ഷം();  // Wipe all node user4p's that point to vertexes
    }
    ശൂന്യം graphWalk(Ast_കണ്ണി_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseBack(static_cast<TristateVertex*>(&vtx), 0);
        }
        എങ്കിൽ (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    ശൂന്യം associate(Ast_കണ്ണി* fromp, Ast_കണ്ണി* മുകൾ) {
        പുതിയ V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(മുകൾ), 1};
    }
    ശൂന്യം deleteVerticesFromSubtreeRecurse(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!nodep) തിരികെ;
        // Skip vars, because they may be connected to more than one varref
        എങ്കിൽ (!VN_IS(nodep, Var)) {
            TristateVertex* സ്ഥിരം vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            എങ്കിൽ (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    ശൂന്യം setTristate(Ast_കണ്ണി* nodep) { makeVertex(nodep)->isTristate(സത്യം); }
    തർക്കം isTristate(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം TristateVertex* സ്ഥിരം vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        തിരികെ vertexp && vertexp->isTristate();
    }
    തർക്കം feedsTri(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം TristateVertex* സ്ഥിരം vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        തിരികെ vertexp && vertexp->feedsTri();
    }
    ശൂന്യം didProcess(Ast_കണ്ണി* nodep, തർക്കം quiet = അസത്യം) {
        എങ്കിൽ (TristateVertex* സ്ഥിരം vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(സത്യം);
        } അല്ലെങ്കിൽ എങ്കിൽ (!quiet) {
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
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : m_graph.vertices()) {
            സ്ഥിരം TristateVertex& vvertex = static_cast<സ്ഥിരം TristateVertex&>(vtx);
            എങ്കിൽ (vvertex.isTristate()) {
                എങ്കിൽ (AstVar* സ്ഥിരം nodep = VN_CAST(vvertex.nodep(), Var)) v.തള്ളുക_back(nodep);
            }
        }
        തിരികെ v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

വർഗ്ഗം TristatePinVisitor final : പൊതു TristateBaseVisitor {
    TristateGraph& m_tgraph;
    സ്ഥിരം തർക്കം m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        എങ്കിൽ (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.setTristate(nodep->varp());
        }
    }
    ശൂന്യം visit(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstSliceSel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_കണ്ണി* nodep, TristateGraph& tgraph, തർക്കം lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################

വർഗ്ഗം TristateVisitor final : പൊതു TristateBaseVisitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    സ്ഥിരം VNUser3InUse m_inuser3;
    സ്ഥിരം VNUser4InUse m_inuser4;

    ഘടന AuxAstVar final {
        AstPull* pullp = nullptr;  // pullup/pulldown direction (whole variable)
        AstVar* outVarp = nullptr;  // output __out var
        തർക്കം ifaceTristate = അസത്യം;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>
            bitPulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    ഘടന RefStrength final {
        Ast_കണ്ണി_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_കണ്ണി_Assign* m_assignp;  // Assignment containing m_varrefp
        സ്ഥിരം Ast_കണ്ണി_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_കണ്ണി_വ്യഞ്ജകം* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_കണ്ണി_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_കണ്ണി_വ്യഞ്ജകം* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_കണ്ണി_Var_Ref* varrefp, Ast_കണ്ണി_Assign* assignp,
                    സ്ഥിരം Ast_കണ്ണി_Procedure* procedurep, Ast_കണ്ണി_വ്യഞ്ജകം* enLhsp,
                    Ast_കണ്ണി_Var_Ref* enVarrefp, Ast_കണ്ണി_വ്യഞ്ജകം* enRhsp, VStrength strength,
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
    ഘടന LhsProjection final {
        Ast_കണ്ണി_വ്യഞ്ജകം* m_lhsp;  // LHS giving the enable assign
        Ast_കണ്ണി_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_കണ്ണി_വ്യഞ്ജകം* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    ഘടന ProceduralEnable final {
        Ast_കണ്ണി_വ്യഞ്ജകം* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_കണ്ണി_വ്യഞ്ജകം* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    using RefStrengthVec = std::vector<RefStrength>;
    using VarMap = std::map<AstVar*, RefStrengthVec*>;
    using Assigns = std::vector<AstAssignW*>;
    using VarToAssignsMap = std::map<AstVar*, Assigns>;
    using AssignEnableMap = std::map<Ast_കണ്ണി_Assign*, Ast_കണ്ണി_വ്യഞ്ജകം*>;
    using ProceduralEnableMap = std::map<Ast_കണ്ണി_Assign*, std::vector<ProceduralEnable>>;
    എണ്ണൽ : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    തർക്കം m_graphing = അസത്യം;  // Major mode - creating graph
    //
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    AstCell* m_cellp = nullptr;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    VarToAssignsMap m_assigns;  // Assigns in current module
    AssignEnableMap m_assignEnps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    പൂർണ്ണസംഖ്യ m_unique = 0;
    തർക്കം m_alhs = അസത്യം;  // On LHS of assignment
    തർക്കം m_inAlias = അസത്യം;  // Inside alias statement
    തർക്കം m_processedIfaces = അസത്യം;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = nullptr;  // Delay of assignment currently collecting LHS drivers
    സ്ഥിരം Ast_കണ്ണി* m_logicp = nullptr;  // Current logic being built
    Ast_കണ്ണി_Assign* m_assignp = nullptr;  // Current assignment
    സ്ഥിരം Ast_കണ്ണി_Procedure* m_procedurep = nullptr;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::vector<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() സ്ഥിരം {
        string o = (m_graphing ? " gr " : " ng ");
        എങ്കിൽ (m_alhs) o += "alhs ";
        തിരികെ o;
    }
    ശൂന്യം modAddStmtp(Ast_കണ്ണി* nodep, Ast_കണ്ണി* newp) {
        എങ്കിൽ (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ {
            m_modp->addStmtsp(newp);
        }
    }
    ശൂന്യം associateLogic(Ast_കണ്ണി* fromp, Ast_കണ്ണി* മുകൾ) {
        എങ്കിൽ (m_logicp) m_tgraph.associate(fromp, മുകൾ);
    }
    AstConst* newAllZerosOrOnes(Ast_കണ്ണി* nodep, തർക്കം ones) {
        V3_സംഖ്യ അക്കം{nodep, nodep->width()};
        എങ്കിൽ (ones) അക്കം.setAllBits1();
        AstConst* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), അക്കം};
        തിരികെ newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_വരി* fl, AstVar* varp, സ്ഥിരം string& dotted, VAccess access,
                           സ്ഥിരം string& inlinedDots) {
        AstVarXRef* സ്ഥിരം xrefp = പുതിയ AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        തിരികെ xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_കണ്ണി_Module* modp, സ്ഥിരം string& dottedPath) {
        എങ്കിൽ (dottedPath.empty()) തിരികെ nullptr;
        സ്ഥിരം string firstComp = dottedPath.substr(0, dottedPath.find('.'));
        വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* സ്ഥിരം ivarp = VN_CAST(stmtp, Var);
            എങ്കിൽ (!ivarp || ivarp->name() != firstComp) തുടരുക;
            സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            എങ്കിൽ (ifaceDtp && ifaceDtp->modportp()) തിരികെ ifaceDtp->modportp();
            നിർത്തുക;
        }
        തിരികെ nullptr;
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* getEnp(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (nodep->user1p()) {
            എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp = VN_CAST(nodep, കണ്ണി_Var_Ref)) {
                എങ്കിൽ (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    തിരികെ newAllZerosOrOnes(nodep, സത്യം);
                }
            }
        } അല്ലെങ്കിൽ {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, സത്യം));
        }
        // Otherwise return the previous output enable
        തിരികെ VN_AS(nodep->user1p(), കണ്ണി_വ്യഞ്ജകം);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    ശൂന്യം addModportVarRefIfMissing(AstModport* modportp, AstVar* varp, VDirection dir) {
        വേണ്ടി (Ast_കണ്ണി* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* സ്ഥിരം mvrp = VN_CAST(mrp, ModportVarRef);
            എങ്കിൽ (mvrp && mvrp->varp() == varp) തിരികെ;  // Already present
        }
        AstModportVarRef* സ്ഥിരം mvrp = പുതിയ AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->addVarsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, തർക്കം is_മുകൾ) {
        // Return the master __en for the specified input variable
        എങ്കിൽ (!invarp->user1p()) {
            AstVar* സ്ഥിരം newp
                = പുതിയ AstVar{invarp->fileline(), is_മുകൾ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_കണ്ണി_Module* സ്ഥിരം ownerModp = findParentModule(invarp);
            എങ്കിൽ (ownerModp && ownerModp != m_modp) {
                ownerModp->addStmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                വേണ്ടി (Ast_കണ്ണി* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* സ്ഥിരം modportp = VN_CAST(stmtp, Modport);
                    എങ്കിൽ (!modportp) തുടരുക;
                    വേണ്ടി (Ast_കണ്ണി* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* സ്ഥിരം mvrp = VN_CAST(mrp, ModportVarRef);
                        എങ്കിൽ (mvrp && mvrp->varp() == invarp) {
                            addModportVarRefIfMissing(modportp, newp, VDirection::നിവേശം);
                            നിർത്തുക;
                        }
                    }
                }
            } അല്ലെങ്കിൽ {
                modAddStmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        തിരികെ VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* സ്ഥിരം constp) {
        File_വരി* സ്ഥിരം fl = constp->fileline();
        V3_സംഖ്യ numz{constp, constp->width()};
        numz.opBitsZ(constp->അക്കം());  // Z->1, else 0
        V3_സംഖ്യ numz0{constp, constp->width()};
        numz0.opNot(numz);  // Z->0, else 1
        തിരികെ പുതിയ AstConst{fl, numz0};
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* get_En_വ്യഞ്ജകം_Based_On_Originalp(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep) {
        എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep, VarRef)) {
            തിരികെ പുതിയ AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), അസത്യം),
                                 VAccess::READ};
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVarXRef* സ്ഥിരം xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* സ്ഥിരം enVarp = getCreateEnVarp(xrefp->varp(), അസത്യം);
            തിരികെ newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
            തിരികെ getNonZConstp(constp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstExtend* സ്ഥിരം extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = extendp->lhsp()->unlinkFrBack();
            AstExtend* സ്ഥിരം enExtendp = extendp->clone_വൃക്ഷം(അസത്യം);
            extendp->lhsp(lhsp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enLhsp = get_En_വ്യഞ്ജകം_Based_On_Originalp(lhsp);
            enExtendp->lhsp(പുതിയ AstNot{enLhsp->fileline(), enLhsp});
            തിരികെ പുതിയ AstNot{enExtendp->fileline(), enExtendp};
        } അല്ലെങ്കിൽ എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = selp->fromp()->unlinkFrBack();
            AstSel* സ്ഥിരം enSelp = selp->clone_വൃക്ഷം(അസത്യം);
            selp->fromp(fromp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enFromp = get_En_വ്യഞ്ജകം_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            തിരികെ enSelp;
        } അല്ലെങ്കിൽ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            തിരികെ nullptr;
        }
    }
    AstVar* getCreateOutVarp(AstVar* invarp, തർക്കം is_മുകൾ) {
        // Return the master __out for the specified input variable
        എങ്കിൽ (!m_varAux(invarp).outVarp) {
            AstVar* സ്ഥിരം newp
                = പുതിയ AstVar{invarp->fileline(), is_മുകൾ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            modAddStmtp(invarp, newp);
            m_varAux(invarp).outVarp = newp;  // find outvar given invarp
        }
        തിരികെ m_varAux(invarp).outVarp;
    }
    AstVar* getCreateUnconnVarp(Ast_കണ്ണി* fromp, Ast_കണ്ണി_DType* dtypep) {
        AstVar* സ്ഥിരം newp = പുതിയ AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvtToStr(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        modAddStmtp(newp, newp);
        തിരികെ newp;
    }

    LhsProjection newLhsProjection(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_Var_Ref* targetp, Ast_കണ്ണി_വ്യഞ്ജകം* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newLhsp = lhsp->clone_വൃക്ഷം_Pure(അസത്യം);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_കണ്ണി* nodep) { nodep->user1p(nullptr); });
        Ast_കണ്ണി_Var_Ref* സ്ഥിരം newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        തിരികെ LhsProjection{newLhsp, newVarrefp, enp};
    }

    ശൂന്യം map_തിരുകുക_Lhs_Var_Ref(Ast_കണ്ണി_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* സ്ഥിരം താക്കോൽ = nodep->varp();
        സ്ഥിരം auto pair = m_lhsmap.emplace(താക്കോൽ, nullptr);
        എങ്കിൽ (pair.second) pair.first->second = പുതിയ RefStrengthVec;
        Ast_കണ്ണി_വ്യഞ്ജകം* enLhsp = nullptr;
        Ast_കണ്ണി_Var_Ref* enVarrefp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* enRhsp = nullptr;
        സ്ഥിരം auto enIt = m_assignEnps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        സ്ഥിരം തർക്കം canCombineProcedural = !VN_IS(താക്കോൽ->dtypep()->skipRefp(), കണ്ണി_UOr_Struct_DType);
        സ്ഥിരം Ast_കണ്ണി_Procedure* സ്ഥിരം procedurep
            = enIt != m_assignEnps.end() && canCombineProcedural ? m_procedurep : nullptr;
        എങ്കിൽ (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_വൃക്ഷം_Pure(അസത്യം));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->തള്ളുക_back(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newEnableDeposit(AstSel* selp, Ast_കണ്ണി_വ്യഞ്ജകം* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstShiftL{
            selp->fileline(), പുതിയ AstExtend{selp->fileline(), enp, selp->fromp()->width()},
            selp->lsbp()->clone_വൃക്ഷം_Pure(അസത്യം), selp->fromp()->width()};
        തിരികെ newp;
    }

    ശൂന്യം setPullDirection(AstVar* varp, AstPull* pullp) {
        സ്ഥിരം AstPull* സ്ഥിരം oldpullp = m_varAux(varp).pullp;
        എങ്കിൽ (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    ശൂന്യം setBitPullDirection(AstVar* varp, പൂർണ്ണസംഖ്യ bitIndex, പൂർണ്ണസംഖ്യ direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        auto& bitPulls = m_varAux(varp).bitPulls;
        auto it = bitPulls.find(bitIndex);
        എങ്കിൽ (it != bitPulls.end() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bitIndex << " of " << varp->prettyNameQ());
        }
        bitPulls[bitIndex] = direction;
    }

    AstConst* createPerBitPullConst(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        സ്ഥിരം auto& bitPulls = m_varAux(varp).bitPulls;
        V3_സംഖ്യ അക്കം{varp, varp->width(), 0};  // Start with all zeros
        വേണ്ടി (സ്ഥിരം auto& pair : bitPulls) {
            എങ്കിൽ (pair.second == 1) അക്കം.setBit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        തിരികെ പുതിയ AstConst{varp->fileline(), അക്കം};
    }

    തർക്കം hasPerBitPulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        തിരികെ !m_varAux(varp).bitPulls.empty();
    }

    ശൂന്യം checkUnhandled(Ast_കണ്ണി* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        എങ്കിൽ (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        എങ്കിൽ ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    ശൂന്യം തിരുകുക_Tristates(Ast_കണ്ണി_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        സ്ഥിരം TristateGraph::VarVec vars = m_tgraph.tristateVars();
        വേണ്ടി (auto varp : vars) {
            എങ്കിൽ (!m_tgraph.isTristate(varp)) തുടരുക;
            സ്ഥിരം auto it = m_lhsmap.find(varp);
            എങ്കിൽ (it != m_lhsmap.end()) തുടരുക;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            സ്ഥിരം AstPull* സ്ഥിരം pullp = m_varAux(varp).pullp;
            സ്ഥിരം തർക്കം pull_മൂല്യം = pullp && pullp->direction() == 1;
            AstConst* സ്ഥിരം constp = newAllZerosOrOnes(varp, pull_മൂല്യം);
            AstVarRef* സ്ഥിരം varrefp = പുതിയ AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            AstAssignW* സ്ഥിരം newp = പുതിയ AstAssignW{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, അസത്യം));  // en=0 for floating/pullup vars
            nodep->addStmtsp(പുതിയ AstAlways{newp});
            map_തിരുകുക_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        വേണ്ടി (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            സ്ഥിരം std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            എങ്കിൽ (it == m_lhsmap.end()) തുടരുക;
            AstVar* സ്ഥിരം invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            എങ്കിൽ (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_കണ്ണി_Module* സ്ഥിരം ownerModp = findParentModule(invarp);
                സ്ഥിരം തർക്കം isCrossModule = ownerModp && ownerModp != nodep;
                സ്ഥിരം തർക്കം isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                എങ്കിൽ (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    വേണ്ടി (സ്ഥിരം RefStrength& rs : *refsp) {
                        എങ്കിൽ (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            നിർത്തുക;
                        }
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = സത്യം;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    ഘടന PartitionInfo final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, PartitionInfo> partitions;
                    വേണ്ടി (സ്ഥിരം RefStrength& rs : *refsp) {
                        എങ്കിൽ (AstVarXRef* സ്ഥിരം xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            PartitionInfo& pi = partitions[xrefp->dotted()];
                            pi.refs.തള്ളുക_back(rs);
                            എങ്കിൽ (pi.inlinedDots.empty()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } അല്ലെങ്കിൽ {
                            partitions[""].refs.തള്ളുക_back(rs);
                        }
                    }
                    വേണ്ടി (auto& kv : partitions) {
                        തിരുകുക_Tristates_Signal(nodep, invarp, &kv.second.refs, സത്യം, kv.first,
                                              kv.second.inlinedDots,
                                              findModportForDotted(nodep, kv.first));
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = സത്യം;
                    തിരുകുക_Tristates_Signal(nodep, invarp, refsp, സത്യം, "", "", nullptr);
                } അല്ലെങ്കിൽ {
                    തിരുകുക_Tristates_Signal(nodep, invarp, refsp, അസത്യം, "", "", nullptr);
                }
            } അല്ലെങ്കിൽ {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(delete refsp, refsp);
        }
    }

    ശൂന്യം aggregateTriSameStrength(Ast_കണ്ണി_Module* nodep, AstVar* സ്ഥിരം varp, AstVar* സ്ഥിരം envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator endStrength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        ഘടന DriverGroup final {
            സ്ഥിരം Ast_കണ്ണി_Procedure* m_procedurep;  // Process owning these assignments
            std::vector<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::vector<DriverGroup> drivers;
        std::map<സ്ഥിരം Ast_കണ്ണി_Procedure*, size_t> procedureToDriver;
        വേണ്ടി (auto it = beginStrength; it != endStrength; ++it) {
            എങ്കിൽ (it->m_procedurep) {
                സ്ഥിരം auto emplaced = procedureToDriver.emplace(it->m_procedurep, drivers.size());
                എങ്കിൽ (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.തള്ളുക_back(&*it);
                    തുടരുക;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.തള്ളുക_back(DriverGroup{it->m_procedurep, {&*it}});
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* orp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* enp = nullptr;

        വേണ്ടി (DriverGroup& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* സ്ഥിരം newLhsp = പുതിയ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvtToStr(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->setDfgTriLowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->addStmtsp(newLhsp);
            AstVar* സ്ഥിരം newEnLhsp
                = പുതിയ AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvtToStr(m_unique++), envarp};  // 2-state ok
            newEnLhsp->setDfgTriLowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->addStmtsp(newEnLhsp);

            വേണ്ടി (RefStrength* സ്ഥിരം refStrengthp : driver.m_refsp) {
                Ast_കണ്ണി_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                എങ്കിൽ (VN_IS(refp, VarXRef)) {
                    AstVarRef* സ്ഥിരം localRefp
                        = പുതിയ AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(nullptr);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } അല്ലെങ്കിൽ {
                    refp->varp(newLhsp);
                }

                എങ്കിൽ (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_കണ്ണി_വ്യഞ്ജകം* enLhsp = refStrengthp->m_enLhsp;
                    Ast_കണ്ണി_Var_Ref* സ്ഥിരം enVarrefp = refStrengthp->m_enVarrefp;
                    എങ്കിൽ (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* സ്ഥിരം localRefp
                            = പുതിയ AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        എങ്കിൽ (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_വൃക്ഷം(), enVarrefp);
                        } അല്ലെങ്കിൽ {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(തള്ളുക_Deletep(enVarrefp), enVarrefp);
                        }
                    } അല്ലെങ്കിൽ {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].തള്ളുക_back(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = nullptr;
                    refStrengthp->m_enVarrefp = nullptr;
                    refStrengthp->m_enRhsp = nullptr;
                } അല്ലെങ്കിൽ {
                    AstAssignW* സ്ഥിരം enLhspAssignp = പുതിയ AstAssignW{
                        refp->fileline(),
                        പുതിയ AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    എങ്കിൽ (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_വൃക്ഷം(അസത്യം));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->addStmtsp(പുതിയ AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_വരി* സ്ഥിരം fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ref1p = പുതിയ AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ref2p = പുതിയ AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം andp = പുതിയ AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : പുതിയ AstOr{fl, orp, andp};

            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ref3p = പുതിയ AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : പുതിയ AstOr{ref3p->fileline(), enp, ref3p};
        }
        AstAssignW* സ്ഥിരം assp = പുതിയ AstAssignW{
            varp->fileline(), പുതിയ AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->addStmtsp(പുതിയ AstAlways{assp});

        AstAssignW* സ്ഥിരം enAssp = പുതിയ AstAssignW{
            envarp->fileline(), പുതിയ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->addStmtsp(പുതിയ AstAlways{enAssp});
    }

    ശൂന്യം തിരുകുക_Procedural_Enable_Assignments(Ast_കണ്ണി_Module* nodep) {
        വേണ്ടി (auto& assignEnables : m_proceduralEnables) {
            Ast_കണ്ണി_Assign* സ്ഥിരം assignp = assignEnables.first;
            Ast_കണ്ണി_വ്യഞ്ജകം* combinedLhsp = assignp->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* combinedRhsp = assignp->rhsp()->unlinkFrBack();
            വേണ്ടി (ProceduralEnable& enable : assignEnables.second) {
                combinedLhsp = പുതിയ AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = പുതിയ AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            എങ്കിൽ (!VN_IS(assignp, AssignDly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* സ്ഥിരം sampledp
                    = പുതിയ AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvtToStr(m_unique++), combinedRhsp->dtypep()};
                nodep->addStmtsp(sampledp);
                assignp->lhsp(പുതിയ AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                AstAssign* സ്ഥിരം applyp
                    = പുതിയ AstAssign{assignp->fileline(), combinedLhsp,
                                    പുതിയ AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->addNextHere(applyp);
            } അല്ലെങ്കിൽ {
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
    ശൂന്യം തിരുകുക_Tristates_Signal(Ast_കണ്ണി_Module* nodep, AstVar* സ്ഥിരം invarp, RefStrengthVec* refsp,
                               തർക്കം isIfaceTri, സ്ഥിരം string& ifaceDottedPath,
                               സ്ഥിരം string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        സ്ഥിരം തർക്കം is_മുകൾ_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_മുകൾ();
        എങ്കിൽ (!isIfaceTri) {
            എങ്കിൽ (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty())
                 && is_മുകൾ_Inout)
                || (!nodep->is_മുകൾ() && invarp->isIO())) {
                // This var becomes an input
                invarp->varType2In();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = getCreateOutVarp(invarp, is_മുകൾ_Inout);
                outvarp->varType2Out();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_മുകൾ_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                എങ്കിൽ (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                എങ്കിൽ (!m_varAux(invarp).bitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* orp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* enp = nullptr;

        std::sort(refsp->begin(), refsp->end(),
                  [](RefStrength a, RefStrength b) { തിരികെ a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        ആകുമ്പോൾ (beginStrength != refsp->end()) {
            RefStrengthVec::iterator endStrength = beginStrength + 1;
            ആകുമ്പോൾ (endStrength != refsp->end()
                   && endStrength->m_strength == beginStrength->m_strength)
                endStrength++;

            File_വരി* സ്ഥിരം fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strengthPrefix;
            എങ്കിൽ (isIfaceTri && !ifaceDottedPath.empty()) {
                strengthPrefix = ifaceDottedPath;
                std::replace(strengthPrefix.begin(), strengthPrefix.end(), '.', '_');
                strengthPrefix += "__";
            }
            സ്ഥിരം string strengthVarName
                = strengthPrefix + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* സ്ഥിരം varStrengthp = പുതിയ AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->setDfgTriLowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->addStmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* സ്ഥിരം enVarStrengthp = പുതിയ AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->setDfgTriLowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->addStmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     endStrength);

            Ast_കണ്ണി_വ്യഞ്ജകം* വ്യഞ്ജകം_Current_Strengthp;
            എങ്കിൽ (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                വ്യഞ്ജകം_Current_Strengthp
                    = പുതിയ AstAnd{fl, പുതിയ AstVarRef{fl, varStrengthp, VAccess::READ},
                                 പുതിയ AstNot{fl, enp->clone_വൃക്ഷം_Pure(അസത്യം)}};
            } അല്ലെങ്കിൽ {
                വ്യഞ്ജകം_Current_Strengthp = പുതിയ AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? വ്യഞ്ജകം_Current_Strengthp : പുതിയ AstOr{fl, orp, വ്യഞ്ജകം_Current_Strengthp};

            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enVarStrengthRefp
                = പുതിയ AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : പുതിയ AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = endStrength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        എങ്കിൽ (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->setDfgAllowMultidriveTri();
            Ast_കണ്ണി_Module* സ്ഥിരം ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            സ്ഥിരം പൂർണ്ണസംഖ്യ contribIdx = static_cast<പൂർണ്ണസംഖ്യ>(m_ifaceContribs[invarp].size());
            File_വരി* സ്ഥിരം fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* സ്ഥിരം contribOutp = പുതിയ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvtToStr(contribIdx), invarp};
            contribOutp->setDfgTriLowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->addStmtsp(contribOutp);

            AstVar* സ്ഥിരം contribEnp = പുതിയ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvtToStr(contribIdx), invarp};
            contribEnp->setDfgTriLowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->addStmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            എങ്കിൽ (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                addModportVarRefIfMissing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                addModportVarRefIfMissing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_കണ്ണി_Var_Ref* സ്ഥിരം lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_കണ്ണി_Var_Ref*>(
                                                    പുതിയ AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_കണ്ണി_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* സ്ഥിരം assp = പുതിയ AstAssignW{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(പുതിയ AstAlways{assp});
            }
            {
                Ast_കണ്ണി_Var_Ref* സ്ഥിരം lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_കണ്ണി_Var_Ref*>(
                                                    പുതിയ AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_കണ്ണി_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* സ്ഥിരം assp = പുതിയ AstAssignW{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(പുതിയ AstAlways{assp});
            }

            m_ifaceContribs[invarp].തള്ളുക_back({contribEnp, contribOutp});
            തിരികെ;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        സ്ഥിരം തർക്കം hasPull = m_varAux(lhsp).pullp || hasPerBitPulls(lhsp);
        എങ്കിൽ (!outvarp || hasPull) {
            Ast_കണ്ണി_വ്യഞ്ജകം* undrivenp;
            എങ്കിൽ (envarp) {
                undrivenp = പുതിയ AstNot{envarp->fileline(),
                                       പുതിയ AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } അല്ലെങ്കിൽ എങ്കിൽ (enp) {
                undrivenp = പുതിയ AstNot{enp->fileline(), enp};
                enp = nullptr;  // moved into undrivenp
            } അല്ലെങ്കിൽ {
                undrivenp = newAllZerosOrOnes(invarp, സത്യം);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            എങ്കിൽ (hasPerBitPulls(lhsp)) {
                pullConstp = createPerBitPullConst(lhsp);
            } അല്ലെങ്കിൽ {
                സ്ഥിരം AstPull* സ്ഥിരം pullp = m_varAux(lhsp).pullp;
                സ്ഥിരം തർക്കം pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = പുതിയ AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? പുതിയ AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        എങ്കിൽ (envarp && !enp) enp = newAllZerosOrOnes(invarp, അസത്യം);

        എങ്കിൽ (envarp) {
            AstAssignW* സ്ഥിരം enAssp = പുതിയ AstAssignW{
                enp->fileline(), പുതിയ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->addStmtsp(പുതിയ AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        AstAssignW* സ്ഥിരം assp = പുതിയ AstAssignW{
            lhsp->fileline(), പുതിയ AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->addStmtsp(പുതിയ AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        എങ്കിൽ ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty()) && is_മുകൾ_Inout) {
            എങ്കിൽ (envarp) {
                envarp->primaryIO(സത്യം);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            എങ്കിൽ (outvarp) {
                outvarp->primaryIO(സത്യം);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(സത്യം);
            }
        }
    }

    തർക്കം isOnlyAssignmentIsToLhsVar(സ്ഥിരം AstAssignW* സ്ഥിരം nodep) {
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            auto foundIt = m_assigns.find(varRefp->varp());
            എങ്കിൽ (foundIt != m_assigns.end()) {
                auto സ്ഥിരം& assignsToVar = foundIt->second;
                എങ്കിൽ (assignsToVar.size() == 1 && assignsToVar[0] == nodep) തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }

    ശൂന്യം addToAssignmentList(AstAssignW* nodep) {
        എങ്കിൽ (AstVarRef* സ്ഥിരം varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            എങ്കിൽ (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].തള്ളുക_back(nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlinkFrBack()->delete_വൃക്ഷം();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(സ്ഥിരം AstAssignW* സ്ഥിരം nodep, തർക്കം മൂല്യം) {
        എങ്കിൽ (AstStrengthSpec* സ്ഥിരം strengthSpec = nodep->strengthSpecp()) {
            തിരികെ മൂല്യം ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        തിരികെ VStrength::STRONG;  // default strength is strong
    }

    തർക്കം assignment_Of_മൂല്യം_On_All_Bits(സ്ഥിരം AstAssignW* സ്ഥിരം nodep, തർക്കം മൂല്യം) {
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->rhsp(), Const)) {
            സ്ഥിരം V3_സംഖ്യ അക്കം = constp->അക്കം();
            തിരികെ മൂല്യം ? അക്കം.isEqAllOnes() : അക്കം.isEqZero();
        }
        തിരികെ അസത്യം;
    }

    AstAssignW* get_Strongest_Assignment_Of_മൂല്യം(സ്ഥിരം Assigns& assigns, തർക്കം മൂല്യം) {
        auto maxIt = std::max_element(
            assigns.begin(), assigns.end(), [&](സ്ഥിരം AstAssignW* ap, സ്ഥിരം AstAssignW* bp) {
                സ്ഥിരം തർക്കം മൂല്യങ്ങൾ_On_Rhs_A = assignment_Of_മൂല്യം_On_All_Bits(ap, മൂല്യം);
                സ്ഥിരം തർക്കം മൂല്യങ്ങൾ_On_Rhs_B = assignment_Of_മൂല്യം_On_All_Bits(bp, മൂല്യം);
                എങ്കിൽ (!മൂല്യങ്ങൾ_On_Rhs_A) തിരികെ മൂല്യങ്ങൾ_On_Rhs_B;
                എങ്കിൽ (!മൂല്യങ്ങൾ_On_Rhs_B) തിരികെ അസത്യം;
                തിരികെ getStrength(ap, മൂല്യം) < getStrength(bp, മൂല്യം);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        തിരികെ assignment_Of_മൂല്യം_On_All_Bits(*maxIt, മൂല്യം) ? *maxIt : nullptr;
    }

    തർക്കം isAssignmentNotStrongerThanStrength(സ്ഥിരം AstAssignW* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        സ്ഥിരം uint8_t strength0 = getStrength(assignp, 0);
        സ്ഥിരം uint8_t strength1 = getStrength(assignp, 1);
        തിരികെ (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_Of_മൂല്യം_On_All_Bits(assignp, 0))
               || (strength1 <= strength && assignment_Of_മൂല്യം_On_All_Bits(assignp, 1));
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* new_Merge_വ്യഞ്ജകം(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp, File_വരി* സ്ഥിരം fl,
                              തർക്കം isWor) {
        Ast_കണ്ണി_വ്യഞ്ജകം* വ്യഞ്ജകം = nullptr;
        എങ്കിൽ (isWor)
            വ്യഞ്ജകം = പുതിയ AstOr{fl, lhsp, rhsp};
        അല്ലെങ്കിൽ
            വ്യഞ്ജകം = പുതിയ AstAnd{fl, lhsp, rhsp};
        തിരികെ വ്യഞ്ജകം;
    }

    ശൂന്യം mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        വേണ്ടി (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            എങ്കിൽ (assigns.size() <= 1) തുടരുക;
            AstVar* സ്ഥിരം varp = varpAssigns.first;
            എങ്കിൽ (!varp->isWiredNet()) തുടരുക;
            auto it = assigns.begin();
            AstAssignW* സ്ഥിരം assignWp0 = *it;
            File_വരി* സ്ഥിരം fl = assignWp0->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* wExp = nullptr;
            ആകുമ്പോൾ (++it != assigns.end()) {
                AstAssignW* assignWpi = *it;
                എങ്കിൽ (!wExp) {
                    wExp
                        = new_Merge_വ്യഞ്ജകം(assignWp0->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം),
                                       assignWpi->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം), fl, varp->isWor());
                } അല്ലെങ്കിൽ {
                    wExp = new_Merge_വ്യഞ്ജകം(wExp, assignWpi->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((assignWpi->unlinkFrBack()->delete_വൃക്ഷം()), assignWpi);
            }
            AstVarRef* സ്ഥിരം wVarRef = പുതിയ AstVarRef{fl, varp, VAccess::WRITE};
            AstAssignW* സ്ഥിരം wAssignp = പുതിയ AstAssignW{fl, wVarRef, wExp};
            assignWp0->replaceWith(wAssignp);
            VL_DO_DANGLING(തള്ളുക_Deletep(assignWp0), assignWp0);
            assigns.clear();
            assigns.തള്ളുക_back(wAssignp);
        }
    }

    ശൂന്യം removeNotStrongerAssignments(Assigns& assigns, സ്ഥിരം AstAssignW* strongestp,
                                      uint8_t greatestKnownStrength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        auto removedIt = std::remove_if(assigns.begin(), assigns.end(), [&](AstAssignW* assignp) {
            എങ്കിൽ (assignp == strongestp) തിരികെ അസത്യം;
            എങ്കിൽ (isAssignmentNotStrongerThanStrength(assignp, greatestKnownStrength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(തള്ളുക_Deletep(assignp->unlinkFrBack()), assignp);
                തിരികെ സത്യം;
            }
            തിരികെ അസത്യം;
        });
        assigns.erase(removedIt, assigns.end());
    }

    ശൂന്യം removeAssignmentsNotStrongerThanUniformConstant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        വേണ്ടി (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            എങ്കിൽ (assigns.size() <= 1) തുടരുക;
            സ്ഥിരം AstAssignW* സ്ഥിരം strongest0p = get_Strongest_Assignment_Of_മൂല്യം(assigns, 0);
            സ്ഥിരം AstAssignW* സ്ഥിരം strongest1p = get_Strongest_Assignment_Of_മൂല്യം(assigns, 1);
            സ്ഥിരം AstAssignW* strongestp = nullptr;
            uint8_t greatestKnownStrength = 0;
            സ്ഥിരം auto getIfStrongest
                = [&](സ്ഥിരം AstAssignW* സ്ഥിരം strongestCandidatep, തർക്കം മൂല്യം) {
                      എങ്കിൽ (!strongestCandidatep) തിരികെ;
                      സ്ഥിരം uint8_t strength = getStrength(strongestCandidatep, മൂല്യം);
                      എങ്കിൽ (strength >= greatestKnownStrength) {
                          greatestKnownStrength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            എങ്കിൽ (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    ശൂന്യം removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        വേണ്ടി (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            എങ്കിൽ (assigns.size() <= 1) തുടരുക;
            auto maxIt = std::max_element(
                assigns.begin(), assigns.end(), [&](സ്ഥിരം AstAssignW* ap, സ്ഥിരം AstAssignW* bp) {
                    എങ്കിൽ (m_tgraph.isTristate(ap)) തിരികെ !m_tgraph.isTristate(bp);
                    എങ്കിൽ (m_tgraph.isTristate(bp)) തിരികെ അസത്യം;
                    സ്ഥിരം uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    സ്ഥിരം uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    തിരികെ minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            സ്ഥിരം AstAssignW* സ്ഥിരം strongestp = m_tgraph.isTristate(*maxIt) ? nullptr : *maxIt;
            എങ്കിൽ (strongestp) {
                സ്ഥിരം uint8_t greatestKnownStrength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    // VISITORS
    ശൂന്യം visit(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        എങ്കിൽ (m_graphing) {
            എങ്കിൽ (!m_alhs && nodep->അക്കം().hasZ()) m_tgraph.setTristate(nodep);
        } അല്ലെങ്കിൽ {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            എങ്കിൽ (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* സ്ഥിരം varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_വരി* സ്ഥിരം fl = nodep->fileline();
                AstConst* സ്ഥിരം enp = getNonZConstp(nodep);
                V3_സംഖ്യ num1{nodep, nodep->width()};
                num1.opAnd(nodep->അക്കം(), enp->അക്കം());  // 01X->01X, Z->0
                AstConst* സ്ഥിരം newconstp = പുതിയ AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    ശൂന്യം visit(AstCond* nodep) override {
        എങ്കിൽ (m_graphing) {
            iterateChildren(nodep);
            എങ്കിൽ (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } അല്ലെങ്കിൽ {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                തിരികെ;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = nodep->condp();
            എങ്കിൽ (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thenp = nodep->thenp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elsep = nodep->elsep();
            എങ്കിൽ (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം en1p = getEnp(thenp);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enp
                    = പുതിയ AstCond{nodep->fileline(), condp->clone_വൃക്ഷം(അസത്യം), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                എങ്കിൽ (thenp->user1p() && !thenp->user1p()->backp()) തള്ളുക_Deletep(thenp->user1p());
                എങ്കിൽ (elsep->user1p() && !elsep->user1p()->backp()) തള്ളുക_Deletep(elsep->user1p());
                thenp->user1p(nullptr);
                elsep->user1p(nullptr);
            }
        }
    }

    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(nullptr);
            m_tgraph.didProcess(nodep);
        }
    }

    ശൂന്യം visit(AstSel* nodep) override {
        എങ്കിൽ (m_graphing) {
            iterateChildren(nodep);
            എങ്കിൽ (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } അല്ലെങ്കിൽ {
                associateLogic(nodep->fromp(), nodep);
            }
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                എങ്കിൽ (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), കണ്ണി_വ്യഞ്ജകം));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } അല്ലെങ്കിൽ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                എങ്കിൽ (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                എങ്കിൽ (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം en1p = getEnp(nodep->fromp());
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enp
                        = പുതിയ AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_വൃക്ഷം_Pure(സത്യം),
                                     nodep->widthConst()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    ശൂന്യം visit(AstConcat* nodep) override {
        എങ്കിൽ (m_graphing) {
            iterateChildren(nodep);
            എങ്കിൽ (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } അല്ലെങ്കിൽ {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                എങ്കിൽ (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enp = VN_AS(nodep->user1p(), കണ്ണി_വ്യഞ്ജകം);
                    nodep->user1p(nullptr);
                    nodep->lhsp()->user1p(പുതിയ AstSel{nodep->fileline(), enp->clone_വൃക്ഷം_Pure(സത്യം),
                                                     nodep->rhsp()->width(),
                                                     nodep->lhsp()->width()});
                    nodep->rhsp()->user1p(
                        പുതിയ AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->width()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } അല്ലെങ്കിൽ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p = nodep->lhsp();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr2p = nodep->rhsp();
                എങ്കിൽ (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം en1p = getEnp(expr1p);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം en2p = getEnp(expr2p);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enp = പുതിയ AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(nullptr);
                    expr2p->user1p(nullptr);
                }
            }
        }
    }

    ശൂന്യം visit(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        എങ്കിൽ (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.setTristate(nodep);
        } അല്ലെങ്കിൽ {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            എങ്കിൽ (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                തിരികെ;
            }
            m_tgraph.didProcess(nodep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr2p = nodep->rhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* enp;
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം en2p = VN_AS(expr2p->user1p(), കണ്ണി_വ്യഞ്ജകം)) {
                enp = പുതിയ AstAnd{nodep->fileline(), expr1p, en2p};
            } അല്ലെങ്കിൽ {
                enp = expr1p;
            }
            expr1p->user1p(nullptr);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    ശൂന്യം visitAndOr(Ast_കണ്ണി_Biop* nodep, തർക്കം isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        എങ്കിൽ (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                തിരികെ;
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
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p = nodep->lhsp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr2p = nodep->rhsp();
            എങ്കിൽ (!expr1p->user1p() && !expr2p->user1p()) {
                തിരികെ;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_കണ്ണി_വ്യഞ്ജകം* en1p = getEnp(expr1p);
            Ast_കണ്ണി_വ്യഞ്ജകം* en2p = getEnp(expr2p);
            Ast_കണ്ണി_വ്യഞ്ജകം* subexpr1p = expr1p->clone_വൃക്ഷം_Pure(അസത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* subexpr2p = expr2p->clone_വൃക്ഷം_Pure(അസത്യം);
            എങ്കിൽ (isAnd) {
                subexpr1p = പുതിയ AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = പുതിയ AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            എങ്കിൽ (!en1p->op1p() && en2p->op1p()) {
                std::swap(en1p, en2p);
                std::swap(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enp = പുതിയ AstOr{
                nodep->fileline(),
                പുതിയ AstAnd{nodep->fileline(), en1p, പുതിയ AstOr{nodep->fileline(), en2p, subexpr1p}},
                പുതിയ AstAnd{nodep->fileline(), en2p->clone_വൃക്ഷം(അസത്യം), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(nullptr);
            expr2p->user1p(nullptr);
        }
    }
    ശൂന്യം visit(AstAnd* nodep) override { visitAndOr(nodep, സത്യം); }
    ശൂന്യം visit(AstOr* nodep) override { visitAndOr(nodep, അസത്യം); }

    ശൂന്യം visitAssign(Ast_കണ്ണി_Assign* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        എങ്കിൽ (VN_IS(nodep->rhsp(), CReset)) തിരികെ;
        എങ്കിൽ (AstAssignW* സ്ഥിരം assignWp = VN_CAST(nodep, AssignW)) {
            m_currentDelayp = VN_CAST(assignWp->timingControlp(), Delay);
        }
        എങ്കിൽ (m_graphing) {
            എങ്കിൽ (AstAssignW* assignWp = VN_CAST(nodep, AssignW)) {
                എങ്കിൽ (assignWp->getLhsNetDelay()) തിരികെ;
                എങ്കിൽ (assignWp->timingControlp() && !m_currentDelayp) തിരികെ;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                എങ്കിൽ (m_currentDelayp && m_currentDelayp->fallDelay()) തിരികെ;
                എങ്കിൽ (!assignWp->timingControlp()) addToAssignmentList(assignWp);
            }

            എങ്കിൽ (nodep->user2() & U2_GRAPHING) തിരികെ;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterateAndNextNull(nodep->rhsp());
            m_alhs = സത്യം;
            iterateAndNextNull(nodep->lhsp());
            m_alhs = അസത്യം;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->user2() & U2_NONGRAPH) {
                തിരികെ;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterateAndNextNull(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            എങ്കിൽ (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, AssignW), nodep,
                            "Continuous assignment inside procedural process");
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), കണ്ണി_വ്യഞ്ജകം)->clone_വൃക്ഷം_Pure(അസത്യം)
                          : newAllZerosOrOnes(nodep->lhsp(), സത്യം);
                സ്ഥിരം തർക്കം inserted = m_assignEnps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            എങ്കിൽ (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(nullptr);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } അല്ലെങ്കിൽ {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(nodep->lhsp(), Sel)) {
                    എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(selp->fromp(), കണ്ണി_Var_Ref)) {
                        എങ്കിൽ (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), സത്യം));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } അല്ലെങ്കിൽ {
                            m_tgraph.didProcess(nodep, സത്യം);
                        }
                    } അല്ലെങ്കിൽ {
                        m_tgraph.didProcess(nodep, സത്യം);  // LCOV_EXCL_LINE
                    }
                } അല്ലെങ്കിൽ {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, സത്യം);
                }
            }
            m_alhs = സത്യം;  // And user1p() will indicate tristate equation, if any
            എങ്കിൽ (AstAssignW* സ്ഥിരം assignWp = VN_CAST(nodep, AssignW)) {
                എങ്കിൽ (AstStrengthSpec* സ്ഥിരം specp = assignWp->strengthSpecp()) {
                    എങ്കിൽ (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        എങ്കിൽ (!isOnlyAssignmentIsToLhsVar(assignWp)) {
                            assignWp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } അല്ലെങ്കിൽ {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterateAndNextNull(nodep->lhsp());
        }
    }
    ശൂന്യം visit(AstAssignW* nodep) override { visitAssign(nodep); }
    ശൂന്യം visit(AstAssign* nodep) override { visitAssign(nodep); }
    ശൂന്യം visit(AstAssignDly* nodep) override { visitAssign(nodep); }

    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        സ്ഥിരം AstAlways* സ്ഥിരം alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN ? nullptr : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    ശൂന്യം visit(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_inAlias);
        m_inAlias = സത്യം;
        എങ്കിൽ (m_graphing) {
            എങ്കിൽ (nodep->user2() & U2_GRAPHING) തിരികെ;
            m_alhs = സത്യം;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            വേണ്ടി (Ast_കണ്ണി* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }

    ശൂന്യം visitCaseEq(Ast_കണ്ണി_Biop* nodep, തർക്കം neq) {
        എങ്കിൽ (m_graphing) {
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* സ്ഥിരം constp = VN_CAST(nodep->lhsp(), Const);
            എങ്കിൽ (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                File_വരി* സ്ഥിരം fl = nodep->fileline();
                Ast_കണ്ണി_വ്യഞ്ജകം* enRhsp;
                എങ്കിൽ (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), കണ്ണി_വ്യഞ്ജകം);
                    rhsp->user1p(nullptr);
                } അല്ലെങ്കിൽ {
                    enRhsp = get_En_വ്യഞ്ജകം_Based_On_Originalp(rhsp);
                }
                Ast_കണ്ണി_വ്യഞ്ജകം* newp
                    = പുതിയ AstLogAnd{fl, പുതിയ AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    പുതിയ AstEqCase{fl, പുതിയ AstConst{fl, constp->അക്കം()}, rhsp}};
                constp->user1p(nullptr);
                എങ്കിൽ (neq) newp = പുതിയ AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (constp && nodep->rhsp()->user1p()) {
                File_വരി* സ്ഥിരം fl = nodep->fileline();
                constp->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* newp = പുതിയ AstLogAnd{fl,
                                                  പുതിയ AstEq{fl, newAllZerosOrOnes(constp, അസത്യം),
                                                            VN_AS(rhsp->user1p(), കണ്ണി_വ്യഞ്ജകം)},
                                                  // Keep the caseeq if there are X's present
                                                  പുതിയ AstEqCase{fl, constp, rhsp}};
                എങ്കിൽ (neq) newp = പുതിയ AstLogNot{fl, newp};
                rhsp->user1p(nullptr);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ {
                checkUnhandled(nodep);
            }
        }
    }
    ശൂന്യം visitEqNeqWild(Ast_കണ്ണി_Biop* nodep) {
        എങ്കിൽ (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterateAndNextNull(nodep->lhsp());
        എങ്കിൽ (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            തിരികെ;
        }
    }
    ശൂന്യം visit(AstEqCase* nodep) override { visitCaseEq(nodep, അസത്യം); }
    ശൂന്യം visit(AstNeqCase* nodep) override { visitCaseEq(nodep, സത്യം); }
    ശൂന്യം visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    ശൂന്യം visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }

    ശൂന്യം visit(Ast_എണ്ണം_Bits* nodep) override {
        std::array<തർക്കം, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->അക്കം().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->അക്കം().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->അക്കം().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        എങ്കിൽ (m_graphing) {
            iterateAndNextNull(nodep->lhsp());
            എങ്കിൽ (!dropop[0]) iterateAndNextNull(nodep->rhsp());
            എങ്കിൽ (!dropop[1]) iterateAndNextNull(nodep->thsp());
            എങ്കിൽ (!dropop[2]) iterateAndNextNull(nodep->fhsp());
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_വ്യഞ്ജകം* nonXp = nullptr;
            എങ്കിൽ (!dropop[0]) {
                nonXp = nodep->rhsp();
            } അല്ലെങ്കിൽ എങ്കിൽ (!dropop[1]) {
                nonXp = nodep->thsp();
            } അല്ലെങ്കിൽ എങ്കിൽ (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            എങ്കിൽ (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                എങ്കിൽ (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    തിരികെ;
                }
                AstVar* സ്ഥിരം envarp = getCreateEnVarp(varrefp->varp(), അസത്യം);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                Ast_കണ്ണി_വ്യഞ്ജകം* newp = പുതിയ Ast_എണ്ണം_Ones{
                    nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                എങ്കിൽ (nonXp) {  // Need to still count '0 or '1 or 'x's
                    എങ്കിൽ (dropop[0]) {
                        nodep->rhsp()->unlinkFrBack()->delete_വൃക്ഷം();
                        nodep->rhsp(nonXp->clone_വൃക്ഷം_Pure(സത്യം));
                    }
                    എങ്കിൽ (dropop[1]) {
                        nodep->thsp()->unlinkFrBack()->delete_വൃക്ഷം();
                        nodep->thsp(nonXp->clone_വൃക്ഷം_Pure(സത്യം));
                    }
                    എങ്കിൽ (dropop[2]) {
                        nodep->fhsp()->unlinkFrBack()->delete_വൃക്ഷം();
                        nodep->fhsp(nonXp->clone_വൃക്ഷം_Pure(സത്യം));
                    }
                    newp = പുതിയ AstAdd{nodep->fileline(), nodep, newp};
                } അല്ലെങ്കിൽ {
                    തള്ളുക_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = nullptr;
        എങ്കിൽ (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        എങ്കിൽ (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.setTristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } അല്ലെങ്കിൽ {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(nodep->lhsp(), Sel);
                സ്ഥിരം AstConst* സ്ഥിരം lsbp = selp ? VN_CAST(selp->lsbp(), Const) : nullptr;
                എങ്കിൽ (lsbp) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = lsbp->toSInt();
                    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < selp->widthConst(); ++i) {
                        setBitPullDirection(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } അല്ലെങ്കിൽ {
                    setPullDirection(varrefp->varp(), nodep);
                }
            }
        }
        എങ്കിൽ (!m_graphing) {
            nodep->unlinkFrBack();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    ശൂന്യം iteratePinGuts(AstPin* nodep) {
        എങ്കിൽ (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            എങ്കിൽ (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            // All heavy lifting completed in graph visitor.
            എങ്കിൽ (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    ശൂന്യം visit(AstPin* nodep) override {
        എങ്കിൽ (m_graphing) {
            എങ്കിൽ (nodep->user2() & U2_GRAPHING) തിരികെ;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* സ്ഥിരം enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            എങ്കിൽ (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, അസത്യം);
                iteratePinGuts(nodep);
                തിരികെ;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            സ്ഥിരം തർക്കം inDeclProcessing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::നിവേശം
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            എങ്കിൽ (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* സ്ഥിരം ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(പുതിയ AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.setTristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } അല്ലെങ്കിൽ എങ്കിൽ (inDeclProcessing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                സ്ഥിരം AstAssignW* സ്ഥിരം reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, സത്യം, സത്യം);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.setTristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* സ്ഥിരം enVarp = പുതിയ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvtToStr(m_unique++),
                                              VFlagBitPacked{}, enModVarp->width()};
            എങ്കിൽ (inDeclProcessing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->varType2In();
            } അല്ലെങ്കിൽ {
                enModVarp->varType2Out();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* സ്ഥിരം enpinp
                = പുതിയ AstPin{nodep->fileline(), nodep->pin_അക്കം(),
                             enModVarp->name(),  // should be {var}"__en"
                             പുതിയ AstVarRef{nodep->fileline(), enVarp,
                                           inDeclProcessing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->addNextHere(enpinp);
            m_modp->addStmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            സ്ഥിരം AstAssignW* outAssignp = nullptr;  // If reconnected, the related assignment
            AstPin* outpinp = nullptr;
            AstVar* സ്ഥിരം outModVarp = m_varAux(nodep->modVarp()).outVarp;
            എങ്കിൽ (!outModVarp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                എങ്കിൽ (!m_modp->is_മുകൾ()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } അല്ലെങ്കിൽ {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം outexprp = VN_AS(nodep->exprp(), കണ്ണി_വ്യഞ്ജകം)
                                                  ->clone_വൃക്ഷം_Pure(അസത്യം);  // Note has lvalue() set
                outpinp = പുതിയ AstPin{nodep->fileline(), nodep->pin_അക്കം(),
                                     outModVarp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(outModVarp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->addNextHere(outpinp);
                // Simplify
                എങ്കിൽ (inDeclProcessing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, സത്യം};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                outAssignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        സത്യം);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, outAssignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            സ്ഥിരം TristatePinVisitor visitor{nodep->exprp(), m_tgraph, അസത്യം};
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം inAssignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, സത്യം);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, inAssignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            എങ്കിൽ (!outAssignp) {
                എങ്കിൽ (!outpinp) {
                    exprrefp = nullptr;  // Primary input only
                } അല്ലെങ്കിൽ {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    എങ്കിൽ (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } അല്ലെങ്കിൽ {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(outAssignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                എങ്കിൽ (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            എങ്കിൽ (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(പുതിയ AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                എങ്കിൽ (!outAssignp) {
                    map_തിരുകുക_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            എങ്കിൽ (exprrefp) {
                AstPull* സ്ഥിരം pullp = m_varAux(nodep->modVarp()).pullp;
                സ്ഥിരം auto& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = nullptr;
                പൂർണ്ണസംഖ്യ selLsb = 0;
                പൂർണ്ണസംഖ്യ selWidth = 0;
                എങ്കിൽ (outAssignp) {
                    എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(outAssignp->lhsp(), Sel)) {
                        AstVarRef* സ്ഥിരം vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* സ്ഥിരം lsbp = VN_CAST(selp->lsbp(), Const);
                        എങ്കിൽ (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->toSInt();
                            selWidth = selp->widthConst();
                        }
                    }
                }

                എങ്കിൽ (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    setPullDirection(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    എങ്കിൽ (selVarp) {
                        സ്ഥിരം പൂർണ്ണസംഖ്യ direction = pullp->direction();
                        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < selWidth; ++i) {
                            setBitPullDirection(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                എങ്കിൽ (!srcBitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    വേണ്ടി (സ്ഥിരം auto& pair : srcBitPulls) {
                        setBitPullDirection(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    എങ്കിൽ (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        വേണ്ടി (സ്ഥിരം auto& pair : srcBitPulls) {
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
        അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->user2() & U2_NONGRAPH) തിരികെ;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    ശൂന്യം handle_കണ്ണി_Var_Ref(Ast_കണ്ണി_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        എങ്കിൽ (m_graphing) {
            എങ്കിൽ (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            എങ്കിൽ (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            എങ്കിൽ (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.setTristate(nodep->varp());
            }
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->user2() & U2_NONGRAPH) തിരികെ;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            എങ്കിൽ (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                എങ്കിൽ (m_inAlias) {
                    എങ്കിൽ (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } അല്ലെങ്കിൽ {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    തിരികെ;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_തിരുകുക_Lhs_Var_Ref(nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_വരി* സ്ഥിരം fl = nodep->fileline();
                AstVar* സ്ഥിരം enVarp = getCreateEnVarp(nodep->varp(), അസത്യം);
                എങ്കിൽ (AstVarXRef* സ്ഥിരം xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } അല്ലെങ്കിൽ {
                    nodep->user1p(പുതിയ AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    ശൂന്യം visit(AstVarRef* nodep) override { handle_കണ്ണി_Var_Ref(nodep); }
    ശൂന്യം visit(AstVarXRef* nodep) override { handle_കണ്ണി_Var_Ref(nodep); }

    ശൂന്യം visit(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        എങ്കിൽ (m_graphing) {
            // If tri0/1 force a pullup
            എങ്കിൽ (nodep->user2() & U2_GRAPHING) തിരികെ;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            എങ്കിൽ (nodep->isPulldown() || nodep->isPullup()) {
                Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstPull{
                    nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->addNextHere(newp);
                // We'll iterate on the new AstPull later
            }
            എങ്കിൽ (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.setTristate(nodep);
            }
        } അല്ലെങ്കിൽ {  // !graphing
            എങ്കിൽ (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        എങ്കിൽ (m_processedIfaces && VN_IS(nodep, Iface)) തിരികെ;
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
        m_graphing = അസത്യം;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = nullptr;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = സത്യം;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = അസത്യം;
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
        തിരുകുക_Tristates(nodep);
        തിരുകുക_Procedural_Enable_Assignments(nodep);

        വേണ്ടി (auto& assignEnp : m_assignEnps) {
            VL_DO_DANGLING(assignEnp.second->delete_വൃക്ഷം(), assignEnp.second);
        }
        m_assignEnps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    ശൂന്യം visit(AstClass* nodep) override {
        // don't deal with classes
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        // don't deal with functions
    }

    ശൂന്യം visit(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterateAndNextNull(nodep->stmtsp());
    }

    ശൂന്യം visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = അസത്യം;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    ശൂന്യം combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        വേണ്ടി (സ്ഥിരം std::pair<AstVar* സ്ഥിരം, std::vector<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* സ്ഥിരം invarp = kv.first;
            സ്ഥിരം std::vector<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_കണ്ണി_Module* സ്ഥിരം ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_വരി* സ്ഥിരം fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.size() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_കണ്ണി_വ്യഞ്ജകം* orp = nullptr;
            Ast_കണ്ണി_വ്യഞ്ജകം* enp = nullptr;

            വേണ്ടി (സ്ഥിരം std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* സ്ഥിരം contribEnp = contrib.first;
                AstVar* സ്ഥിരം contribOutp = contrib.second;

                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം outRefp = പുതിയ AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enRefp = പുതിയ AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം andp = പുതിയ AstAnd{fl, outRefp, enRefp};

                orp = (!orp) ? andp : പുതിയ AstOr{fl, orp, andp};

                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enRef2p = പുതിയ AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : പുതിയ AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            എങ്കിൽ (!envarp) {
                envarp = പുതിയ AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->setDfgTriLowered();
                ifaceModp->addStmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            AstAssignW* സ്ഥിരം enAssp
                = പുതിയ AstAssignW{fl, പുതിയ AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->addStmtsp(പുതിയ AstAlways{enAssp});

            // Pull resolution
            സ്ഥിരം AstPull* സ്ഥിരം pullp = m_varAux(invarp).pullp;
            സ്ഥിരം തർക്കം pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_കണ്ണി_വ്യഞ്ജകം* undrivenp = പുതിയ AstNot{fl, പുതിയ AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = പുതിയ AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = പുതിയ AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            AstAssignW* സ്ഥിരം assp
                = പുതിയ AstAssignW{fl, പുതിയ AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->addStmtsp(പുതിയ AstAlways{assp});
        }
    }

പൊതു:
    // CONSTRUCTORS
    explicit TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::vector<Ast_കണ്ണി_Module*> ifacesp;
        വേണ്ടി (Ast_കണ്ണി* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            എങ്കിൽ (VN_IS(modp, Iface)) ifacesp.തള്ളുക_back(VN_AS(modp, കണ്ണി_Module));
        }
        വേണ്ടി (auto it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = സത്യം;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ifdef VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::vector<Ast_കണ്ണി*> unusedRootps;
        netlistp->foreach([&](Ast_കണ്ണി* nodep) {
            Ast_കണ്ണി* സ്ഥിരം enp = nodep->user1p();
            എങ്കിൽ (!enp) തിരികെ;
            എങ്കിൽ (enp->backp()) തിരികെ;
            unusedRootps.emplace_back(enp);
        });
        വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
#endif
    }
    ~TristateVisitor() override {
        V3Stats::addStat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

ശൂന്യം V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("tristate", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
