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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Tristate.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Inst.h"
#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு TristateBaseVisitor VL_NOT_FINAL : பொது VNVisitor {
பொது:
    // METHODS
};

//######################################################################
// Graph support classes

வகுப்பு TristateVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(TristateVertex, V3GraphVertex)
    Ast_முனை* மாறிலி m_nodep;
    தருக்க m_isTristate = பொய்;  // Logic indicates a tristate
    தருக்க m_feedsTri = பொய்;  // Propagates to a tristate node (on RHS)
    தருக்க m_processed = பொய்;  // Tristating was cleaned up
பொது:
    TristateVertex(V3Graph* graphp, Ast_முனை* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TristateVertex() override = இயல்பு;
    // ACCESSORS
    Ast_முனை* nodep() மாறிலி VL_MT_STABLE { திரும்பு m_nodep; }
    மாறிலி AstVar* varp() மாறிலி { திரும்பு VN_CAST(nodep(), Var); }
    string name() மாறிலி override {
        திரும்பு ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvtToHex(nodep())));
    }
    string dotColor() மாறிலி override {
        திரும்பு (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_வரி* fileline() மாறிலி override { திரும்பு nodep()->fileline(); }
    தருக்க isTristate() மாறிலி VL_MT_SAFE { திரும்பு m_isTristate; }
    வெறுமை isTristate(தருக்க flag) { m_isTristate = flag; }
    தருக்க feedsTri() மாறிலி VL_MT_SAFE { திரும்பு m_feedsTri; }
    வெறுமை feedsTri(தருக்க flag) { m_feedsTri = flag; }
    தருக்க processed() மாறிலி { திரும்பு m_processed; }
    வெறுமை processed(தருக்க flag) { m_processed = flag; }
};

//######################################################################

வகுப்பு TristateGraph final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
பொது:
    using VarVec = std::vector<AstVar*>;

தனிப்பட்ட:
    // MEMBERS
    V3Graph m_graph;  // Logic graph

பொது:
    // CONSTRUCTORS
    TristateGraph() { clearAndCheck(); }
    மெய்நிகர் ~TristateGraph() { clearAndCheck(); }
    VL_UNCOPYABLE(TristateGraph);

தனிப்பட்ட:
    // METHODS

    TristateVertex* makeVertex(Ast_முனை* nodep) {
        TristateVertex* vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        எனில் (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = புதிய TristateVertex{&m_graph, nodep};
            nodep->user4p(vertexp);
        }
        திரும்பு vertexp;
    }

    // METHODS - Graph optimization
    வெறுமை graphWalkRecurseFwd(TristateVertex* vtxp, முழு level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        எனில் (!vtxp->isTristate()) திரும்பு;  // tristate involved
        எனில் (vtxp->user() == 1) திரும்பு;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        எனில் (!vtxp->varp()) {  // not a var where we stop the recursion
            க்கு (V3GraphEdge& edge : vtxp->outEdges()) {
                TristateVertex* மாறிலி vvertexp = static_cast<TristateVertex*>(edge.உச்சி());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                எனில் (!vvertexp->isTristate()) {
                    vvertexp->isTristate(உண்மை);
                    graphWalkRecurseFwd(vvertexp, level + 1);
                }
            }
        } இல்லையெனில் {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            க்கு (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* மாறிலி vvertexp = static_cast<TristateVertex*>(edge.fromp());
                எனில் (மாறிலி AstVarRef* மாறிலி refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    எனில் (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(உண்மை);
                        graphWalkRecurseFwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    வெறுமை graphWalkRecurseBack(TristateVertex* vtxp, முழு level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        எனில் (!(vtxp->isTristate() || vtxp->feedsTri())) திரும்பு;  // tristate involved
        எனில் (vtxp->user() == 3) திரும்பு;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        எனில் (!vtxp->varp()) {  // not a var where we stop the recursion
            க்கு (V3GraphEdge& edge : vtxp->inEdges()) {
                TristateVertex* மாறிலி vvertexp = static_cast<TristateVertex*>(edge.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                எனில் (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(உண்மை);
                    graphWalkRecurseBack(vvertexp, level + 1);
                }
            }
        }
    }

பொது:
    // METHODS
    தருக்க empty() மாறிலி { திரும்பு m_graph.empty(); }
    வெறுமை clearAndCheck() {
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            மாறிலி TristateVertex& vvertex = static_cast<TristateVertex&>(vtx);
            எனில் (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_graph.clear();
        Ast_முனை::user4_Clear_மரம்();  // Wipe all node user4p's that point to vertexes
    }
    வெறுமை graphWalk(Ast_முனை_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseFwd(static_cast<TristateVertex*>(&vtx), 0);
        }
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            graphWalkRecurseBack(static_cast<TristateVertex*>(&vtx), 0);
        }
        எனில் (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    வெறுமை associate(Ast_முனை* fromp, Ast_முனை* உச்சி) {
        புதிய V3GraphEdge{&m_graph, makeVertex(fromp), makeVertex(உச்சி), 1};
    }
    வெறுமை deleteVerticesFromSubtreeRecurse(Ast_முனை* nodep) {
        எனில் (!nodep) திரும்பு;
        // Skip vars, because they may be connected to more than one varref
        எனில் (!VN_IS(nodep, Var)) {
            TristateVertex* மாறிலி vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
            எனில் (vertexp) vertexp->unlinkDelete(&m_graph);
        }
        எனில் (Ast_முனை* மாறிலி refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    வெறுமை setTristate(Ast_முனை* nodep) { makeVertex(nodep)->isTristate(உண்மை); }
    தருக்க isTristate(மாறிலி Ast_முனை* nodep) {
        மாறிலி TristateVertex* மாறிலி vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        திரும்பு vertexp && vertexp->isTristate();
    }
    தருக்க feedsTri(மாறிலி Ast_முனை* nodep) {
        மாறிலி TristateVertex* மாறிலி vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p());
        திரும்பு vertexp && vertexp->feedsTri();
    }
    வெறுமை didProcess(Ast_முனை* nodep, தருக்க quiet = பொய்) {
        எனில் (TristateVertex* மாறிலி vertexp = reinterpret_cast<TristateVertex*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(உண்மை);
        } இல்லையெனில் எனில் (!quiet) {
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
        க்கு (மாறிலி V3GraphVertex& vtx : m_graph.vertices()) {
            மாறிலி TristateVertex& vvertex = static_cast<மாறிலி TristateVertex&>(vtx);
            எனில் (vvertex.isTristate()) {
                எனில் (AstVar* மாறிலி nodep = VN_CAST(vvertex.nodep(), Var)) v.தள்ளு_back(nodep);
            }
        }
        திரும்பு v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

வகுப்பு TristatePinVisitor final : பொது TristateBaseVisitor {
    TristateGraph& m_tgraph;
    மாறிலி தருக்க m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        எனில் (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } இல்லையெனில் எனில் (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.setTristate(nodep->varp());
        }
    }
    வெறுமை visit(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    வெறுமை visit(AstSliceSel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_முனை* nodep, TristateGraph& tgraph, தருக்க lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = இயல்பு;
};

//######################################################################

வகுப்பு TristateVisitor final : பொது TristateBaseVisitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    மாறிலி VNUser3InUse m_inuser3;
    மாறிலி VNUser4InUse m_inuser4;

    கட்டமைப்பு AuxAstVar final {
        AstPull* pullp = nullptr;  // pullup/pulldown direction (whole variable)
        AstVar* outVarp = nullptr;  // output __out var
        தருக்க ifaceTristate = பொய்;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<முழு, முழு>
            bitPulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    கட்டமைப்பு RefStrength final {
        Ast_முனை_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_முனை_Assign* m_assignp;  // Assignment containing m_varrefp
        மாறிலி Ast_முனை_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_முனை_கோவை* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_முனை_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_முனை_கோவை* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_முனை_Var_Ref* varrefp, Ast_முனை_Assign* assignp,
                    மாறிலி Ast_முனை_Procedure* procedurep, Ast_முனை_கோவை* enLhsp,
                    Ast_முனை_Var_Ref* enVarrefp, Ast_முனை_கோவை* enRhsp, VStrength strength,
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
    கட்டமைப்பு LhsProjection final {
        Ast_முனை_கோவை* m_lhsp;  // LHS giving the enable assign
        Ast_முனை_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_முனை_கோவை* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    கட்டமைப்பு ProceduralEnable final {
        Ast_முனை_கோவை* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_முனை_கோவை* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    using RefStrengthVec = std::vector<RefStrength>;
    using VarMap = std::map<AstVar*, RefStrengthVec*>;
    using Assigns = std::vector<AstAssignW*>;
    using VarToAssignsMap = std::map<AstVar*, Assigns>;
    using AssignEnableMap = std::map<Ast_முனை_Assign*, Ast_முனை_கோவை*>;
    using ProceduralEnableMap = std::map<Ast_முனை_Assign*, std::vector<ProceduralEnable>>;
    எண்ணிக்கை : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    தருக்க m_graphing = பொய்;  // Major mode - creating graph
    //
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    AstCell* m_cellp = nullptr;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    VarToAssignsMap m_assigns;  // Assigns in current module
    AssignEnableMap m_assignEnps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    முழு m_unique = 0;
    தருக்க m_alhs = பொய்;  // On LHS of assignment
    தருக்க m_inAlias = பொய்;  // Inside alias statement
    தருக்க m_processedIfaces = பொய்;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = nullptr;  // Delay of assignment currently collecting LHS drivers
    மாறிலி Ast_முனை* m_logicp = nullptr;  // Current logic being built
    Ast_முனை_Assign* m_assignp = nullptr;  // Current assignment
    மாறிலி Ast_முனை_Procedure* m_procedurep = nullptr;  // Current procedural process
    TristateGraph m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::vector<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() மாறிலி {
        string o = (m_graphing ? " gr " : " ng ");
        எனில் (m_alhs) o += "alhs ";
        திரும்பு o;
    }
    வெறுமை modAddStmtp(Ast_முனை* nodep, Ast_முனை* newp) {
        எனில் (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } இல்லையெனில் {
            m_modp->addStmtsp(newp);
        }
    }
    வெறுமை associateLogic(Ast_முனை* fromp, Ast_முனை* உச்சி) {
        எனில் (m_logicp) m_tgraph.associate(fromp, உச்சி);
    }
    AstConst* newAllZerosOrOnes(Ast_முனை* nodep, தருக்க ones) {
        V3_எண் இலக்கம்{nodep, nodep->width()};
        எனில் (ones) இலக்கம்.setAllBits1();
        AstConst* மாறிலி newp = புதிய AstConst{nodep->fileline(), இலக்கம்};
        திரும்பு newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_வரி* fl, AstVar* varp, மாறிலி string& dotted, VAccess access,
                           மாறிலி string& inlinedDots) {
        AstVarXRef* மாறிலி xrefp = புதிய AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        திரும்பு xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* findModportForDotted(Ast_முனை_Module* modp, மாறிலி string& dottedPath) {
        எனில் (dottedPath.empty()) திரும்பு nullptr;
        மாறிலி string firstComp = dottedPath.substr(0, dottedPath.find('.'));
        க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* மாறிலி ivarp = VN_CAST(stmtp, Var);
            எனில் (!ivarp || ivarp->name() != firstComp) தொடர்;
            மாறிலி AstIfaceRefDType* மாறிலி ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            எனில் (ifaceDtp && ifaceDtp->modportp()) திரும்பு ifaceDtp->modportp();
            நிறுத்து;
        }
        திரும்பு nullptr;
    }
    Ast_முனை_கோவை* getEnp(Ast_முனை* nodep) {
        எனில் (nodep->user1p()) {
            எனில் (Ast_முனை_Var_Ref* மாறிலி refp = VN_CAST(nodep, முனை_Var_Ref)) {
                எனில் (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    திரும்பு newAllZerosOrOnes(nodep, உண்மை);
                }
            }
        } இல்லையெனில் {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, உண்மை));
        }
        // Otherwise return the previous output enable
        திரும்பு VN_AS(nodep->user1p(), முனை_கோவை);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    வெறுமை addModportVarRefIfMissing(AstModport* modportp, AstVar* varp, VDirection dir) {
        க்கு (Ast_முனை* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* மாறிலி mvrp = VN_CAST(mrp, ModportVarRef);
            எனில் (mvrp && mvrp->varp() == varp) திரும்பு;  // Already present
        }
        AstModportVarRef* மாறிலி mvrp = புதிய AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->addVarsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, தருக்க is_உச்சி) {
        // Return the master __en for the specified input variable
        எனில் (!invarp->user1p()) {
            AstVar* மாறிலி newp
                = புதிய AstVar{invarp->fileline(), is_உச்சி ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_முனை_Module* மாறிலி ownerModp = findParentModule(invarp);
            எனில் (ownerModp && ownerModp != m_modp) {
                ownerModp->addStmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                க்கு (Ast_முனை* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* மாறிலி modportp = VN_CAST(stmtp, Modport);
                    எனில் (!modportp) தொடர்;
                    க்கு (Ast_முனை* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* மாறிலி mvrp = VN_CAST(mrp, ModportVarRef);
                        எனில் (mvrp && mvrp->varp() == invarp) {
                            addModportVarRefIfMissing(modportp, newp, VDirection::உள்ளீடு);
                            நிறுத்து;
                        }
                    }
                }
            } இல்லையெனில் {
                modAddStmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        திரும்பு VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* மாறிலி constp) {
        File_வரி* மாறிலி fl = constp->fileline();
        V3_எண் numz{constp, constp->width()};
        numz.opBitsZ(constp->இலக்கம்());  // Z->1, else 0
        V3_எண் numz0{constp, constp->width()};
        numz0.opNot(numz);  // Z->0, else 1
        திரும்பு புதிய AstConst{fl, numz0};
    }
    Ast_முனை_கோவை* get_En_கோவை_Based_On_Originalp(Ast_முனை_கோவை* மாறிலி nodep) {
        எனில் (AstVarRef* மாறிலி varrefp = VN_CAST(nodep, VarRef)) {
            திரும்பு புதிய AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), பொய்),
                                 VAccess::READ};
        } இல்லையெனில் எனில் (AstVarXRef* மாறிலி xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* மாறிலி enVarp = getCreateEnVarp(xrefp->varp(), பொய்);
            திரும்பு newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } இல்லையெனில் எனில் (AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
            திரும்பு getNonZConstp(constp);
        } இல்லையெனில் எனில் (AstExtend* மாறிலி extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_முனை_கோவை* மாறிலி lhsp = extendp->lhsp()->unlinkFrBack();
            AstExtend* மாறிலி enExtendp = extendp->clone_மரம்(பொய்);
            extendp->lhsp(lhsp);
            Ast_முனை_கோவை* மாறிலி enLhsp = get_En_கோவை_Based_On_Originalp(lhsp);
            enExtendp->lhsp(புதிய AstNot{enLhsp->fileline(), enLhsp});
            திரும்பு புதிய AstNot{enExtendp->fileline(), enExtendp};
        } இல்லையெனில் எனில் (AstSel* மாறிலி selp = VN_CAST(nodep, Sel)) {
            Ast_முனை_கோவை* மாறிலி fromp = selp->fromp()->unlinkFrBack();
            AstSel* மாறிலி enSelp = selp->clone_மரம்(பொய்);
            selp->fromp(fromp);
            Ast_முனை_கோவை* மாறிலி enFromp = get_En_கோவை_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            திரும்பு enSelp;
        } இல்லையெனில் {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            திரும்பு nullptr;
        }
    }
    AstVar* getCreateOutVarp(AstVar* invarp, தருக்க is_உச்சி) {
        // Return the master __out for the specified input variable
        எனில் (!m_varAux(invarp).outVarp) {
            AstVar* மாறிலி newp
                = புதிய AstVar{invarp->fileline(), is_உச்சி ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            modAddStmtp(invarp, newp);
            m_varAux(invarp).outVarp = newp;  // find outvar given invarp
        }
        திரும்பு m_varAux(invarp).outVarp;
    }
    AstVar* getCreateUnconnVarp(Ast_முனை* fromp, Ast_முனை_DType* dtypep) {
        AstVar* மாறிலி newp = புதிய AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvtToStr(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        modAddStmtp(newp, newp);
        திரும்பு newp;
    }

    LhsProjection newLhsProjection(Ast_முனை_கோவை* lhsp, Ast_முனை_Var_Ref* targetp, Ast_முனை_கோவை* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_முனை_கோவை* மாறிலி newLhsp = lhsp->clone_மரம்_Pure(பொய்);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_முனை* nodep) { nodep->user1p(nullptr); });
        Ast_முனை_Var_Ref* மாறிலி newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        திரும்பு LhsProjection{newLhsp, newVarrefp, enp};
    }

    வெறுமை map_செருகு_Lhs_Var_Ref(Ast_முனை_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* மாறிலி சாவி = nodep->varp();
        மாறிலி auto pair = m_lhsmap.emplace(சாவி, nullptr);
        எனில் (pair.second) pair.first->second = புதிய RefStrengthVec;
        Ast_முனை_கோவை* enLhsp = nullptr;
        Ast_முனை_Var_Ref* enVarrefp = nullptr;
        Ast_முனை_கோவை* enRhsp = nullptr;
        மாறிலி auto enIt = m_assignEnps.find(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        மாறிலி தருக்க canCombineProcedural = !VN_IS(சாவி->dtypep()->skipRefp(), முனை_UOr_Struct_DType);
        மாறிலி Ast_முனை_Procedure* மாறிலி procedurep
            = enIt != m_assignEnps.end() && canCombineProcedural ? m_procedurep : nullptr;
        எனில் (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->second->clone_மரம்_Pure(பொய்));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.first->second->தள்ளு_back(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_முனை_கோவை* newEnableDeposit(AstSel* selp, Ast_முனை_கோவை* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_முனை_கோவை* மாறிலி newp = புதிய AstShiftL{
            selp->fileline(), புதிய AstExtend{selp->fileline(), enp, selp->fromp()->width()},
            selp->lsbp()->clone_மரம்_Pure(பொய்), selp->fromp()->width()};
        திரும்பு newp;
    }

    வெறுமை setPullDirection(AstVar* varp, AstPull* pullp) {
        மாறிலி AstPull* மாறிலி oldpullp = m_varAux(varp).pullp;
        எனில் (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } இல்லையெனில் {
            எனில் (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    வெறுமை setBitPullDirection(AstVar* varp, முழு bitIndex, முழு direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        auto& bitPulls = m_varAux(varp).bitPulls;
        auto it = bitPulls.find(bitIndex);
        எனில் (it != bitPulls.end() && it->second != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << bitIndex << " of " << varp->prettyNameQ());
        }
        bitPulls[bitIndex] = direction;
    }

    AstConst* createPerBitPullConst(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        மாறிலி auto& bitPulls = m_varAux(varp).bitPulls;
        V3_எண் இலக்கம்{varp, varp->width(), 0};  // Start with all zeros
        க்கு (மாறிலி auto& pair : bitPulls) {
            எனில் (pair.second == 1) இலக்கம்.setBit(pair.first, 1);
        }  // LCOV_EXCL_LINE
        திரும்பு புதிய AstConst{varp->fileline(), இலக்கம்};
    }

    தருக்க hasPerBitPulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        திரும்பு !m_varAux(varp).bitPulls.empty();
    }

    வெறுமை checkUnhandled(Ast_முனை* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        எனில் (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        எனில் ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    வெறுமை செருகு_Tristates(Ast_முனை_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        மாறிலி TristateGraph::VarVec vars = m_tgraph.tristateVars();
        க்கு (auto varp : vars) {
            எனில் (!m_tgraph.isTristate(varp)) தொடர்;
            மாறிலி auto it = m_lhsmap.find(varp);
            எனில் (it != m_lhsmap.end()) தொடர்;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            மாறிலி AstPull* மாறிலி pullp = m_varAux(varp).pullp;
            மாறிலி தருக்க pull_மதிப்பு = pullp && pullp->direction() == 1;
            AstConst* மாறிலி constp = newAllZerosOrOnes(varp, pull_மதிப்பு);
            AstVarRef* மாறிலி varrefp = புதிய AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            AstAssignW* மாறிலி newp = புதிய AstAssignW{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, பொய்));  // en=0 for floating/pullup vars
            nodep->addStmtsp(புதிய AstAlways{newp});
            map_செருகு_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        க்கு (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            மாறிலி std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.find(varp);
            எனில் (it == m_lhsmap.end()) தொடர்;
            AstVar* மாறிலி invarp = it->first;
            RefStrengthVec* refsp = it->second;
            // Figure out if this var needs tristate expanded.
            எனில் (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_முனை_Module* மாறிலி ownerModp = findParentModule(invarp);
                மாறிலி தருக்க isCrossModule = ownerModp && ownerModp != nodep;
                மாறிலி தருக்க isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                எனில் (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    க்கு (மாறிலி RefStrength& rs : *refsp) {
                        எனில் (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            நிறுத்து;
                        }
                    }
                } இல்லையெனில் எனில் (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = உண்மை;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    கட்டமைப்பு PartitionInfo final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, PartitionInfo> partitions;
                    க்கு (மாறிலி RefStrength& rs : *refsp) {
                        எனில் (AstVarXRef* மாறிலி xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            PartitionInfo& pi = partitions[xrefp->dotted()];
                            pi.refs.தள்ளு_back(rs);
                            எனில் (pi.inlinedDots.empty()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } இல்லையெனில் {
                            partitions[""].refs.தள்ளு_back(rs);
                        }
                    }
                    க்கு (auto& kv : partitions) {
                        செருகு_Tristates_Signal(nodep, invarp, &kv.second.refs, உண்மை, kv.first,
                                              kv.second.inlinedDots,
                                              findModportForDotted(nodep, kv.first));
                    }
                } இல்லையெனில் எனில் (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = உண்மை;
                    செருகு_Tristates_Signal(nodep, invarp, refsp, உண்மை, "", "", nullptr);
                } இல்லையெனில் {
                    செருகு_Tristates_Signal(nodep, invarp, refsp, பொய், "", "", nullptr);
                }
            } இல்லையெனில் {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(delete refsp, refsp);
        }
    }

    வெறுமை aggregateTriSameStrength(Ast_முனை_Module* nodep, AstVar* மாறிலி varp, AstVar* மாறிலி envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator endStrength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        கட்டமைப்பு DriverGroup final {
            மாறிலி Ast_முனை_Procedure* m_procedurep;  // Process owning these assignments
            std::vector<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::vector<DriverGroup> drivers;
        std::map<மாறிலி Ast_முனை_Procedure*, size_t> procedureToDriver;
        க்கு (auto it = beginStrength; it != endStrength; ++it) {
            எனில் (it->m_procedurep) {
                மாறிலி auto emplaced = procedureToDriver.emplace(it->m_procedurep, drivers.size());
                எனில் (!emplaced.second) {
                    drivers[emplaced.first->second].m_refsp.தள்ளு_back(&*it);
                    தொடர்;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.தள்ளு_back(DriverGroup{it->m_procedurep, {&*it}});
        }

        Ast_முனை_கோவை* orp = nullptr;
        Ast_முனை_கோவை* enp = nullptr;

        க்கு (DriverGroup& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* மாறிலி newLhsp = புதிய AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvtToStr(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->setDfgTriLowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->addStmtsp(newLhsp);
            AstVar* மாறிலி newEnLhsp
                = புதிய AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvtToStr(m_unique++), envarp};  // 2-state ok
            newEnLhsp->setDfgTriLowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->addStmtsp(newEnLhsp);

            க்கு (RefStrength* மாறிலி refStrengthp : driver.m_refsp) {
                Ast_முனை_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                எனில் (VN_IS(refp, VarXRef)) {
                    AstVarRef* மாறிலி localRefp
                        = புதிய AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(nullptr);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } இல்லையெனில் {
                    refp->varp(newLhsp);
                }

                எனில் (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_முனை_கோவை* enLhsp = refStrengthp->m_enLhsp;
                    Ast_முனை_Var_Ref* மாறிலி enVarrefp = refStrengthp->m_enVarrefp;
                    எனில் (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* மாறிலி localRefp
                            = புதிய AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        எனில் (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_மரம்(), enVarrefp);
                        } இல்லையெனில் {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(தள்ளு_Deletep(enVarrefp), enVarrefp);
                        }
                    } இல்லையெனில் {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].தள்ளு_back(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = nullptr;
                    refStrengthp->m_enVarrefp = nullptr;
                    refStrengthp->m_enRhsp = nullptr;
                } இல்லையெனில் {
                    AstAssignW* மாறிலி enLhspAssignp = புதிய AstAssignW{
                        refp->fileline(),
                        புதிய AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    எனில் (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_மரம்(பொய்));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->addStmtsp(புதிய AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_வரி* மாறிலி fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_முனை_கோவை* மாறிலி ref1p = புதிய AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_முனை_கோவை* மாறிலி ref2p = புதிய AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_முனை_கோவை* மாறிலி andp = புதிய AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : புதிய AstOr{fl, orp, andp};

            Ast_முனை_கோவை* மாறிலி ref3p = புதிய AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : புதிய AstOr{ref3p->fileline(), enp, ref3p};
        }
        AstAssignW* மாறிலி assp = புதிய AstAssignW{
            varp->fileline(), புதிய AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->addStmtsp(புதிய AstAlways{assp});

        AstAssignW* மாறிலி enAssp = புதிய AstAssignW{
            envarp->fileline(), புதிய AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->addStmtsp(புதிய AstAlways{enAssp});
    }

    வெறுமை செருகு_Procedural_Enable_Assignments(Ast_முனை_Module* nodep) {
        க்கு (auto& assignEnables : m_proceduralEnables) {
            Ast_முனை_Assign* மாறிலி assignp = assignEnables.first;
            Ast_முனை_கோவை* combinedLhsp = assignp->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* combinedRhsp = assignp->rhsp()->unlinkFrBack();
            க்கு (ProceduralEnable& enable : assignEnables.second) {
                combinedLhsp = புதிய AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = புதிய AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            எனில் (!VN_IS(assignp, AssignDly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* மாறிலி sampledp
                    = புதிய AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvtToStr(m_unique++), combinedRhsp->dtypep()};
                nodep->addStmtsp(sampledp);
                assignp->lhsp(புதிய AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                AstAssign* மாறிலி applyp
                    = புதிய AstAssign{assignp->fileline(), combinedLhsp,
                                    புதிய AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->addNextHere(applyp);
            } இல்லையெனில் {
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
    வெறுமை செருகு_Tristates_Signal(Ast_முனை_Module* nodep, AstVar* மாறிலி invarp, RefStrengthVec* refsp,
                               தருக்க isIfaceTri, மாறிலி string& ifaceDottedPath,
                               மாறிலி string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        மாறிலி தருக்க is_உச்சி_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_உச்சி();
        எனில் (!isIfaceTri) {
            எனில் (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty())
                 && is_உச்சி_Inout)
                || (!nodep->is_உச்சி() && invarp->isIO())) {
                // This var becomes an input
                invarp->varType2In();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = getCreateOutVarp(invarp, is_உச்சி_Inout);
                outvarp->varType2Out();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_உச்சி_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).bitPulls
                    = m_varAux(invarp).bitPulls;  // Per-bit pull propagation
                எனில் (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                எனில் (!m_varAux(invarp).bitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } இல்லையெனில் எனில் (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_முனை_கோவை* orp = nullptr;
        Ast_முனை_கோவை* enp = nullptr;

        std::sort(refsp->begin(), refsp->end(),
                  [](RefStrength a, RefStrength b) { திரும்பு a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        போது (beginStrength != refsp->end()) {
            RefStrengthVec::iterator endStrength = beginStrength + 1;
            போது (endStrength != refsp->end()
                   && endStrength->m_strength == beginStrength->m_strength)
                endStrength++;

            File_வரி* மாறிலி fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strengthPrefix;
            எனில் (isIfaceTri && !ifaceDottedPath.empty()) {
                strengthPrefix = ifaceDottedPath;
                std::replace(strengthPrefix.begin(), strengthPrefix.end(), '.', '_');
                strengthPrefix += "__";
            }
            மாறிலி string strengthVarName
                = strengthPrefix + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* மாறிலி varStrengthp = புதிய AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->setDfgTriLowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->addStmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* மாறிலி enVarStrengthp = புதிய AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->setDfgTriLowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->addStmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     endStrength);

            Ast_முனை_கோவை* கோவை_Current_Strengthp;
            எனில் (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                கோவை_Current_Strengthp
                    = புதிய AstAnd{fl, புதிய AstVarRef{fl, varStrengthp, VAccess::READ},
                                 புதிய AstNot{fl, enp->clone_மரம்_Pure(பொய்)}};
            } இல்லையெனில் {
                கோவை_Current_Strengthp = புதிய AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? கோவை_Current_Strengthp : புதிய AstOr{fl, orp, கோவை_Current_Strengthp};

            Ast_முனை_கோவை* மாறிலி enVarStrengthRefp
                = புதிய AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : புதிய AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = endStrength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        எனில் (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->setDfgAllowMultidriveTri();
            Ast_முனை_Module* மாறிலி ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            மாறிலி முழு contribIdx = static_cast<முழு>(m_ifaceContribs[invarp].size());
            File_வரி* மாறிலி fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* மாறிலி contribOutp = புதிய AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvtToStr(contribIdx), invarp};
            contribOutp->setDfgTriLowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->addStmtsp(contribOutp);

            AstVar* மாறிலி contribEnp = புதிய AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvtToStr(contribIdx), invarp};
            contribEnp->setDfgTriLowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->addStmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            எனில் (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                addModportVarRefIfMissing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                addModportVarRefIfMissing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_முனை_Var_Ref* மாறிலி lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_முனை_Var_Ref*>(
                                                    புதிய AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_முனை_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* மாறிலி assp = புதிய AstAssignW{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(புதிய AstAlways{assp});
            }
            {
                Ast_முனை_Var_Ref* மாறிலி lhsp
                    = ifaceDottedPath.empty() ? static_cast<Ast_முனை_Var_Ref*>(
                                                    புதிய AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_முனை_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, ifaceDottedPath,
                                                               VAccess::WRITE, ifaceInlinedDots));
                AstAssignW* மாறிலி assp = புதிய AstAssignW{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->addStmtsp(புதிய AstAlways{assp});
            }

            m_ifaceContribs[invarp].தள்ளு_back({contribEnp, contribOutp});
            திரும்பு;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        மாறிலி தருக்க hasPull = m_varAux(lhsp).pullp || hasPerBitPulls(lhsp);
        எனில் (!outvarp || hasPull) {
            Ast_முனை_கோவை* undrivenp;
            எனில் (envarp) {
                undrivenp = புதிய AstNot{envarp->fileline(),
                                       புதிய AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } இல்லையெனில் எனில் (enp) {
                undrivenp = புதிய AstNot{enp->fileline(), enp};
                enp = nullptr;  // moved into undrivenp
            } இல்லையெனில் {
                undrivenp = newAllZerosOrOnes(invarp, உண்மை);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            எனில் (hasPerBitPulls(lhsp)) {
                pullConstp = createPerBitPullConst(lhsp);
            } இல்லையெனில் {
                மாறிலி AstPull* மாறிலி pullp = m_varAux(lhsp).pullp;
                மாறிலி தருக்க pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = புதிய AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? புதிய AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        எனில் (envarp && !enp) enp = newAllZerosOrOnes(invarp, பொய்);

        எனில் (envarp) {
            AstAssignW* மாறிலி enAssp = புதிய AstAssignW{
                enp->fileline(), புதிய AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->addStmtsp(புதிய AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        AstAssignW* மாறிலி assp = புதிய AstAssignW{
            lhsp->fileline(), புதிய AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->addStmtsp(புதிய AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        எனில் ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().empty()) && is_உச்சி_Inout) {
            எனில் (envarp) {
                envarp->primaryIO(உண்மை);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            எனில் (outvarp) {
                outvarp->primaryIO(உண்மை);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            எனில் (!v3Global.opt.libCreate().empty()) {
                மாறிலி முழு id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(உண்மை);
            }
        }
    }

    தருக்க isOnlyAssignmentIsToLhsVar(மாறிலி AstAssignW* மாறிலி nodep) {
        எனில் (மாறிலி AstVarRef* மாறிலி varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            auto foundIt = m_assigns.find(varRefp->varp());
            எனில் (foundIt != m_assigns.end()) {
                auto மாறிலி& assignsToVar = foundIt->second;
                எனில் (assignsToVar.size() == 1 && assignsToVar[0] == nodep) திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }

    வெறுமை addToAssignmentList(AstAssignW* nodep) {
        எனில் (AstVarRef* மாறிலி varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            எனில் (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].தள்ளு_back(nodep);
            } இல்லையெனில் எனில் (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlinkFrBack()->delete_மரம்();
            }
        } இல்லையெனில் எனில் (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(மாறிலி AstAssignW* மாறிலி nodep, தருக்க மதிப்பு) {
        எனில் (AstStrengthSpec* மாறிலி strengthSpec = nodep->strengthSpecp()) {
            திரும்பு மதிப்பு ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        திரும்பு VStrength::STRONG;  // default strength is strong
    }

    தருக்க assignment_Of_மதிப்பு_On_All_Bits(மாறிலி AstAssignW* மாறிலி nodep, தருக்க மதிப்பு) {
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->rhsp(), Const)) {
            மாறிலி V3_எண் இலக்கம் = constp->இலக்கம்();
            திரும்பு மதிப்பு ? இலக்கம்.isEqAllOnes() : இலக்கம்.isEqZero();
        }
        திரும்பு பொய்;
    }

    AstAssignW* get_Strongest_Assignment_Of_மதிப்பு(மாறிலி Assigns& assigns, தருக்க மதிப்பு) {
        auto maxIt = std::max_element(
            assigns.begin(), assigns.end(), [&](மாறிலி AstAssignW* ap, மாறிலி AstAssignW* bp) {
                மாறிலி தருக்க மதிப்புகள்_On_Rhs_A = assignment_Of_மதிப்பு_On_All_Bits(ap, மதிப்பு);
                மாறிலி தருக்க மதிப்புகள்_On_Rhs_B = assignment_Of_மதிப்பு_On_All_Bits(bp, மதிப்பு);
                எனில் (!மதிப்புகள்_On_Rhs_A) திரும்பு மதிப்புகள்_On_Rhs_B;
                எனில் (!மதிப்புகள்_On_Rhs_B) திரும்பு பொய்;
                திரும்பு getStrength(ap, மதிப்பு) < getStrength(bp, மதிப்பு);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        திரும்பு assignment_Of_மதிப்பு_On_All_Bits(*maxIt, மதிப்பு) ? *maxIt : nullptr;
    }

    தருக்க isAssignmentNotStrongerThanStrength(மாறிலி AstAssignW* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        மாறிலி uint8_t strength0 = getStrength(assignp, 0);
        மாறிலி uint8_t strength1 = getStrength(assignp, 1);
        திரும்பு (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_Of_மதிப்பு_On_All_Bits(assignp, 0))
               || (strength1 <= strength && assignment_Of_மதிப்பு_On_All_Bits(assignp, 1));
    }

    Ast_முனை_கோவை* new_Merge_கோவை(Ast_முனை_கோவை* மாறிலி lhsp, Ast_முனை_கோவை* மாறிலி rhsp, File_வரி* மாறிலி fl,
                              தருக்க isWor) {
        Ast_முனை_கோவை* கோவை = nullptr;
        எனில் (isWor)
            கோவை = புதிய AstOr{fl, lhsp, rhsp};
        இல்லையெனில்
            கோவை = புதிய AstAnd{fl, lhsp, rhsp};
        திரும்பு கோவை;
    }

    வெறுமை mergeWiredNetsAssignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        க்கு (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            எனில் (assigns.size() <= 1) தொடர்;
            AstVar* மாறிலி varp = varpAssigns.first;
            எனில் (!varp->isWiredNet()) தொடர்;
            auto it = assigns.begin();
            AstAssignW* மாறிலி assignWp0 = *it;
            File_வரி* மாறிலி fl = assignWp0->fileline();
            Ast_முனை_கோவை* wExp = nullptr;
            போது (++it != assigns.end()) {
                AstAssignW* assignWpi = *it;
                எனில் (!wExp) {
                    wExp
                        = new_Merge_கோவை(assignWp0->rhsp()->clone_மரம்_Pure(பொய்),
                                       assignWpi->rhsp()->clone_மரம்_Pure(பொய்), fl, varp->isWor());
                } இல்லையெனில் {
                    wExp = new_Merge_கோவை(wExp, assignWpi->rhsp()->clone_மரம்_Pure(பொய்), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((assignWpi->unlinkFrBack()->delete_மரம்()), assignWpi);
            }
            AstVarRef* மாறிலி wVarRef = புதிய AstVarRef{fl, varp, VAccess::WRITE};
            AstAssignW* மாறிலி wAssignp = புதிய AstAssignW{fl, wVarRef, wExp};
            assignWp0->replaceWith(wAssignp);
            VL_DO_DANGLING(தள்ளு_Deletep(assignWp0), assignWp0);
            assigns.clear();
            assigns.தள்ளு_back(wAssignp);
        }
    }

    வெறுமை removeNotStrongerAssignments(Assigns& assigns, மாறிலி AstAssignW* strongestp,
                                      uint8_t greatestKnownStrength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        auto removedIt = std::remove_if(assigns.begin(), assigns.end(), [&](AstAssignW* assignp) {
            எனில் (assignp == strongestp) திரும்பு பொய்;
            எனில் (isAssignmentNotStrongerThanStrength(assignp, greatestKnownStrength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(தள்ளு_Deletep(assignp->unlinkFrBack()), assignp);
                திரும்பு உண்மை;
            }
            திரும்பு பொய்;
        });
        assigns.erase(removedIt, assigns.end());
    }

    வெறுமை removeAssignmentsNotStrongerThanUniformConstant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        க்கு (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            எனில் (assigns.size() <= 1) தொடர்;
            மாறிலி AstAssignW* மாறிலி strongest0p = get_Strongest_Assignment_Of_மதிப்பு(assigns, 0);
            மாறிலி AstAssignW* மாறிலி strongest1p = get_Strongest_Assignment_Of_மதிப்பு(assigns, 1);
            மாறிலி AstAssignW* strongestp = nullptr;
            uint8_t greatestKnownStrength = 0;
            மாறிலி auto getIfStrongest
                = [&](மாறிலி AstAssignW* மாறிலி strongestCandidatep, தருக்க மதிப்பு) {
                      எனில் (!strongestCandidatep) திரும்பு;
                      மாறிலி uint8_t strength = getStrength(strongestCandidatep, மதிப்பு);
                      எனில் (strength >= greatestKnownStrength) {
                          greatestKnownStrength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            எனில் (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    வெறுமை removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        க்கு (auto& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.second;
            எனில் (assigns.size() <= 1) தொடர்;
            auto maxIt = std::max_element(
                assigns.begin(), assigns.end(), [&](மாறிலி AstAssignW* ap, மாறிலி AstAssignW* bp) {
                    எனில் (m_tgraph.isTristate(ap)) திரும்பு !m_tgraph.isTristate(bp);
                    எனில் (m_tgraph.isTristate(bp)) திரும்பு பொய்;
                    மாறிலி uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    மாறிலி uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    திரும்பு minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            மாறிலி AstAssignW* மாறிலி strongestp = m_tgraph.isTristate(*maxIt) ? nullptr : *maxIt;
            எனில் (strongestp) {
                மாறிலி uint8_t greatestKnownStrength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatestKnownStrength);
            }
        }
    }

    // VISITORS
    வெறுமை visit(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        எனில் (m_graphing) {
            எனில் (!m_alhs && nodep->இலக்கம்().hasZ()) m_tgraph.setTristate(nodep);
        } இல்லையெனில் {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            எனில் (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* மாறிலி varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_முனை* மாறிலி newp = புதிய AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் எனில் (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_வரி* மாறிலி fl = nodep->fileline();
                AstConst* மாறிலி enp = getNonZConstp(nodep);
                V3_எண் num1{nodep, nodep->width()};
                num1.opAnd(nodep->இலக்கம்(), enp->இலக்கம்());  // 01X->01X, Z->0
                AstConst* மாறிலி newconstp = புதிய AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    வெறுமை visit(AstCond* nodep) override {
        எனில் (m_graphing) {
            iterateChildren(nodep);
            எனில் (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } இல்லையெனில் {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } இல்லையெனில் {
            எனில் (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                திரும்பு;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_முனை_கோவை* மாறிலி condp = nodep->condp();
            எனில் (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_முனை_கோவை* மாறிலி thenp = nodep->thenp();
            Ast_முனை_கோவை* மாறிலி elsep = nodep->elsep();
            எனில் (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_முனை_கோவை* மாறிலி en1p = getEnp(thenp);
                Ast_முனை_கோவை* மாறிலி en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_முனை_கோவை* மாறிலி enp
                    = புதிய AstCond{nodep->fileline(), condp->clone_மரம்(பொய்), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                எனில் (thenp->user1p() && !thenp->user1p()->backp()) தள்ளு_Deletep(thenp->user1p());
                எனில் (elsep->user1p() && !elsep->user1p()->backp()) தள்ளு_Deletep(elsep->user1p());
                thenp->user1p(nullptr);
                elsep->user1p(nullptr);
            }
        }
    }

    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        iterateChildren(nodep);
        எனில் (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } இல்லையெனில் எனில் (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(nullptr);
            m_tgraph.didProcess(nodep);
        }
    }

    வெறுமை visit(AstSel* nodep) override {
        எனில் (m_graphing) {
            iterateChildren(nodep);
            எனில் (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } இல்லையெனில் {
                associateLogic(nodep->fromp(), nodep);
            }
        } இல்லையெனில் {
            எனில் (m_alhs) {
                UINFO(9, dbgState() << nodep);
                எனில் (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_முனை_கோவை* மாறிலி newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), முனை_கோவை));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } இல்லையெனில் {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                எனில் (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                எனில் (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_முனை_கோவை* மாறிலி en1p = getEnp(nodep->fromp());
                    Ast_முனை_கோவை* மாறிலி enp
                        = புதிய AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_மரம்_Pure(உண்மை),
                                     nodep->widthConst()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    வெறுமை visit(AstConcat* nodep) override {
        எனில் (m_graphing) {
            iterateChildren(nodep);
            எனில் (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } இல்லையெனில் {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } இல்லையெனில் {
            எனில் (m_alhs) {
                UINFO(9, dbgState() << nodep);
                எனில் (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_முனை_கோவை* மாறிலி enp = VN_AS(nodep->user1p(), முனை_கோவை);
                    nodep->user1p(nullptr);
                    nodep->lhsp()->user1p(புதிய AstSel{nodep->fileline(), enp->clone_மரம்_Pure(உண்மை),
                                                     nodep->rhsp()->width(),
                                                     nodep->lhsp()->width()});
                    nodep->rhsp()->user1p(
                        புதிய AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->width()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } இல்லையெனில் {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_முனை_கோவை* மாறிலி expr1p = nodep->lhsp();
                Ast_முனை_கோவை* மாறிலி expr2p = nodep->rhsp();
                எனில் (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_முனை_கோவை* மாறிலி en1p = getEnp(expr1p);
                    Ast_முனை_கோவை* மாறிலி en2p = getEnp(expr2p);
                    Ast_முனை_கோவை* மாறிலி enp = புதிய AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(nullptr);
                    expr2p->user1p(nullptr);
                }
            }
        }
    }

    வெறுமை visit(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        எனில் (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.setTristate(nodep);
        } இல்லையெனில் {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            எனில் (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                திரும்பு;
            }
            m_tgraph.didProcess(nodep);
            Ast_முனை_கோவை* மாறிலி expr1p = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி expr2p = nodep->rhsp()->unlinkFrBack();
            Ast_முனை_கோவை* enp;
            எனில் (Ast_முனை_கோவை* மாறிலி en2p = VN_AS(expr2p->user1p(), முனை_கோவை)) {
                enp = புதிய AstAnd{nodep->fileline(), expr1p, en2p};
            } இல்லையெனில் {
                enp = expr1p;
            }
            expr1p->user1p(nullptr);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    வெறுமை visitAndOr(Ast_முனை_Biop* nodep, தருக்க isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        எனில் (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } இல்லையெனில் {
            எனில் (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                திரும்பு;
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
            Ast_முனை_கோவை* மாறிலி expr1p = nodep->lhsp();
            Ast_முனை_கோவை* மாறிலி expr2p = nodep->rhsp();
            எனில் (!expr1p->user1p() && !expr2p->user1p()) {
                திரும்பு;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_முனை_கோவை* en1p = getEnp(expr1p);
            Ast_முனை_கோவை* en2p = getEnp(expr2p);
            Ast_முனை_கோவை* subexpr1p = expr1p->clone_மரம்_Pure(பொய்);
            Ast_முனை_கோவை* subexpr2p = expr2p->clone_மரம்_Pure(பொய்);
            எனில் (isAnd) {
                subexpr1p = புதிய AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = புதிய AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            எனில் (!en1p->op1p() && en2p->op1p()) {
                std::swap(en1p, en2p);
                std::swap(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_முனை_கோவை* மாறிலி enp = புதிய AstOr{
                nodep->fileline(),
                புதிய AstAnd{nodep->fileline(), en1p, புதிய AstOr{nodep->fileline(), en2p, subexpr1p}},
                புதிய AstAnd{nodep->fileline(), en2p->clone_மரம்(பொய்), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(nullptr);
            expr2p->user1p(nullptr);
        }
    }
    வெறுமை visit(AstAnd* nodep) override { visitAndOr(nodep, உண்மை); }
    வெறுமை visit(AstOr* nodep) override { visitAndOr(nodep, பொய்); }

    வெறுமை visitAssign(Ast_முனை_Assign* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        எனில் (VN_IS(nodep->rhsp(), CReset)) திரும்பு;
        எனில் (AstAssignW* மாறிலி assignWp = VN_CAST(nodep, AssignW)) {
            m_currentDelayp = VN_CAST(assignWp->timingControlp(), Delay);
        }
        எனில் (m_graphing) {
            எனில் (AstAssignW* assignWp = VN_CAST(nodep, AssignW)) {
                எனில் (assignWp->getLhsNetDelay()) திரும்பு;
                எனில் (assignWp->timingControlp() && !m_currentDelayp) திரும்பு;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                எனில் (m_currentDelayp && m_currentDelayp->fallDelay()) திரும்பு;
                எனில் (!assignWp->timingControlp()) addToAssignmentList(assignWp);
            }

            எனில் (nodep->user2() & U2_GRAPHING) திரும்பு;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterateAndNextNull(nodep->rhsp());
            m_alhs = உண்மை;
            iterateAndNextNull(nodep->lhsp());
            m_alhs = பொய்;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } இல்லையெனில் {
            எனில் (nodep->user2() & U2_NONGRAPH) {
                திரும்பு;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterateAndNextNull(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            எனில் (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, AssignW), nodep,
                            "Continuous assignment inside procedural process");
                Ast_முனை_கோவை* மாறிலி enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), முனை_கோவை)->clone_மரம்_Pure(பொய்)
                          : newAllZerosOrOnes(nodep->lhsp(), உண்மை);
                மாறிலி தருக்க inserted = m_assignEnps.emplace(nodep, enp).second;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            எனில் (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(nullptr);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } இல்லையெனில் {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                எனில் (AstSel* மாறிலி selp = VN_CAST(nodep->lhsp(), Sel)) {
                    எனில் (Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(selp->fromp(), முனை_Var_Ref)) {
                        எனில் (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), உண்மை));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } இல்லையெனில் {
                            m_tgraph.didProcess(nodep, உண்மை);
                        }
                    } இல்லையெனில் {
                        m_tgraph.didProcess(nodep, உண்மை);  // LCOV_EXCL_LINE
                    }
                } இல்லையெனில் {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, உண்மை);
                }
            }
            m_alhs = உண்மை;  // And user1p() will indicate tristate equation, if any
            எனில் (AstAssignW* மாறிலி assignWp = VN_CAST(nodep, AssignW)) {
                எனில் (AstStrengthSpec* மாறிலி specp = assignWp->strengthSpecp()) {
                    எனில் (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        எனில் (!isOnlyAssignmentIsToLhsVar(assignWp)) {
                            assignWp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } இல்லையெனில் {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterateAndNextNull(nodep->lhsp());
        }
    }
    வெறுமை visit(AstAssignW* nodep) override { visitAssign(nodep); }
    வெறுமை visit(AstAssign* nodep) override { visitAssign(nodep); }
    வெறுமை visit(AstAssignDly* nodep) override { visitAssign(nodep); }

    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        மாறிலி AstAlways* மாறிலி alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN ? nullptr : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    வெறுமை visit(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_inAlias);
        m_inAlias = உண்மை;
        எனில் (m_graphing) {
            எனில் (nodep->user2() & U2_GRAPHING) திரும்பு;
            m_alhs = உண்மை;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            க்கு (Ast_முனை* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }

    வெறுமை visitCaseEq(Ast_முனை_Biop* nodep, தருக்க neq) {
        எனில் (m_graphing) {
            iterateChildren(nodep);
        } இல்லையெனில் {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* மாறிலி constp = VN_CAST(nodep->lhsp(), Const);
            எனில் (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                File_வரி* மாறிலி fl = nodep->fileline();
                Ast_முனை_கோவை* enRhsp;
                எனில் (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), முனை_கோவை);
                    rhsp->user1p(nullptr);
                } இல்லையெனில் {
                    enRhsp = get_En_கோவை_Based_On_Originalp(rhsp);
                }
                Ast_முனை_கோவை* newp
                    = புதிய AstLogAnd{fl, புதிய AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    புதிய AstEqCase{fl, புதிய AstConst{fl, constp->இலக்கம்()}, rhsp}};
                constp->user1p(nullptr);
                எனில் (neq) newp = புதிய AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் எனில் (constp && nodep->rhsp()->user1p()) {
                File_வரி* மாறிலி fl = nodep->fileline();
                constp->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_முனை_கோவை* newp = புதிய AstLogAnd{fl,
                                                  புதிய AstEq{fl, newAllZerosOrOnes(constp, பொய்),
                                                            VN_AS(rhsp->user1p(), முனை_கோவை)},
                                                  // Keep the caseeq if there are X's present
                                                  புதிய AstEqCase{fl, constp, rhsp}};
                எனில் (neq) newp = புதிய AstLogNot{fl, newp};
                rhsp->user1p(nullptr);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் {
                checkUnhandled(nodep);
            }
        }
    }
    வெறுமை visitEqNeqWild(Ast_முனை_Biop* nodep) {
        எனில் (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterateAndNextNull(nodep->lhsp());
        எனில் (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            திரும்பு;
        }
    }
    வெறுமை visit(AstEqCase* nodep) override { visitCaseEq(nodep, பொய்); }
    வெறுமை visit(AstNeqCase* nodep) override { visitCaseEq(nodep, உண்மை); }
    வெறுமை visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    வெறுமை visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }

    வெறுமை visit(Ast_மொத்தம்_Bits* nodep) override {
        std::array<தருக்க, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->இலக்கம்().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->இலக்கம்().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->இலக்கம்().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        எனில் (m_graphing) {
            iterateAndNextNull(nodep->lhsp());
            எனில் (!dropop[0]) iterateAndNextNull(nodep->rhsp());
            எனில் (!dropop[1]) iterateAndNextNull(nodep->thsp());
            எனில் (!dropop[2]) iterateAndNextNull(nodep->fhsp());
        } இல்லையெனில் {
            Ast_முனை_கோவை* nonXp = nullptr;
            எனில் (!dropop[0]) {
                nonXp = nodep->rhsp();
            } இல்லையெனில் எனில் (!dropop[1]) {
                nonXp = nodep->thsp();
            } இல்லையெனில் எனில் (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            எனில் (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                மாறிலி AstVarRef* மாறிலி varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                எனில் (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    திரும்பு;
                }
                AstVar* மாறிலி envarp = getCreateEnVarp(varrefp->varp(), பொய்);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                Ast_முனை_கோவை* newp = புதிய Ast_மொத்தம்_Ones{
                    nodep->fileline(), புதிய AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                எனில் (nonXp) {  // Need to still count '0 or '1 or 'x's
                    எனில் (dropop[0]) {
                        nodep->rhsp()->unlinkFrBack()->delete_மரம்();
                        nodep->rhsp(nonXp->clone_மரம்_Pure(உண்மை));
                    }
                    எனில் (dropop[1]) {
                        nodep->thsp()->unlinkFrBack()->delete_மரம்();
                        nodep->thsp(nonXp->clone_மரம்_Pure(உண்மை));
                    }
                    எனில் (dropop[2]) {
                        nodep->fhsp()->unlinkFrBack()->delete_மரம்();
                        nodep->fhsp(nonXp->clone_மரம்_Pure(உண்மை));
                    }
                    newp = புதிய AstAdd{nodep->fileline(), nodep, newp};
                } இல்லையெனில் {
                    தள்ளு_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = nullptr;
        எனில் (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } இல்லையெனில் எனில் (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        எனில் (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } இல்லையெனில் {
            எனில் (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.setTristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } இல்லையெனில் {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                மாறிலி AstSel* மாறிலி selp = VN_CAST(nodep->lhsp(), Sel);
                மாறிலி AstConst* மாறிலி lsbp = selp ? VN_CAST(selp->lsbp(), Const) : nullptr;
                எனில் (lsbp) {
                    மாறிலி முழு lsb = lsbp->toSInt();
                    க்கு (முழு i = 0; i < selp->widthConst(); ++i) {
                        setBitPullDirection(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } இல்லையெனில் {
                    setPullDirection(varrefp->varp(), nodep);
                }
            }
        }
        எனில் (!m_graphing) {
            nodep->unlinkFrBack();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    வெறுமை iteratePinGuts(AstPin* nodep) {
        எனில் (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            எனில் (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } இல்லையெனில் {
            // All heavy lifting completed in graph visitor.
            எனில் (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    வெறுமை visit(AstPin* nodep) override {
        எனில் (m_graphing) {
            எனில் (nodep->user2() & U2_GRAPHING) திரும்பு;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* மாறிலி enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            எனில் (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, பொய்);
                iteratePinGuts(nodep);
                திரும்பு;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            மாறிலி தருக்க inDeclProcessing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::உள்ளீடு
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            எனில் (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* மாறிலி ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(புதிய AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.setTristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } இல்லையெனில் எனில் (inDeclProcessing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                மாறிலி AstAssignW* மாறிலி reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, உண்மை, உண்மை);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.setTristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* மாறிலி enVarp = புதிய AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvtToStr(m_unique++),
                                              VFlagBitPacked{}, enModVarp->width()};
            எனில் (inDeclProcessing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->varType2In();
            } இல்லையெனில் {
                enModVarp->varType2Out();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* மாறிலி enpinp
                = புதிய AstPin{nodep->fileline(), nodep->pin_இலக்கம்(),
                             enModVarp->name(),  // should be {var}"__en"
                             புதிய AstVarRef{nodep->fileline(), enVarp,
                                           inDeclProcessing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->addNextHere(enpinp);
            m_modp->addStmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            மாறிலி AstAssignW* outAssignp = nullptr;  // If reconnected, the related assignment
            AstPin* outpinp = nullptr;
            AstVar* மாறிலி outModVarp = m_varAux(nodep->modVarp()).outVarp;
            எனில் (!outModVarp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                எனில் (!m_modp->is_உச்சி()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } இல்லையெனில் {
                Ast_முனை_கோவை* மாறிலி outexprp = VN_AS(nodep->exprp(), முனை_கோவை)
                                                  ->clone_மரம்_Pure(பொய்);  // Note has lvalue() set
                outpinp = புதிய AstPin{nodep->fileline(), nodep->pin_இலக்கம்(),
                                     outModVarp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(outModVarp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->addNextHere(outpinp);
                // Simplify
                எனில் (inDeclProcessing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, உண்மை};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                outAssignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        உண்மை);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, outAssignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            மாறிலி TristatePinVisitor visitor{nodep->exprp(), m_tgraph, பொய்};
            மாறிலி Ast_முனை* மாறிலி inAssignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, உண்மை);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, inAssignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            எனில் (!outAssignp) {
                எனில் (!outpinp) {
                    exprrefp = nullptr;  // Primary input only
                } இல்லையெனில் {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    எனில் (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } இல்லையெனில் {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(outAssignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                எனில் (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            எனில் (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(புதிய AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                எனில் (!outAssignp) {
                    map_செருகு_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            எனில் (exprrefp) {
                AstPull* மாறிலி pullp = m_varAux(nodep->modVarp()).pullp;
                மாறிலி auto& srcBitPulls = m_varAux(nodep->modVarp()).bitPulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = nullptr;
                முழு selLsb = 0;
                முழு selWidth = 0;
                எனில் (outAssignp) {
                    எனில் (AstSel* மாறிலி selp = VN_CAST(outAssignp->lhsp(), Sel)) {
                        AstVarRef* மாறிலி vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* மாறிலி lsbp = VN_CAST(selp->lsbp(), Const);
                        எனில் (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->toSInt();
                            selWidth = selp->widthConst();
                        }
                    }
                }

                எனில் (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    setPullDirection(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    எனில் (selVarp) {
                        மாறிலி முழு direction = pullp->direction();
                        க்கு (முழு i = 0; i < selWidth; ++i) {
                            setBitPullDirection(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                எனில் (!srcBitPulls.empty()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    க்கு (மாறிலி auto& pair : srcBitPulls) {
                        setBitPullDirection(exprrefp->varp(), pair.first, pair.second);
                    }  // LCOV_EXCL_LINE
                    எனில் (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        க்கு (மாறிலி auto& pair : srcBitPulls) {
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
        இல்லையெனில் {
            எனில் (nodep->user2() & U2_NONGRAPH) திரும்பு;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    வெறுமை handle_முனை_Var_Ref(Ast_முனை_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        எனில் (m_graphing) {
            எனில் (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            எனில் (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            எனில் (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.setTristate(nodep->varp());
            }
        } இல்லையெனில் {
            எனில் (nodep->user2() & U2_NONGRAPH) திரும்பு;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            எனில் (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                எனில் (m_inAlias) {
                    எனில் (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } இல்லையெனில் {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    திரும்பு;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_செருகு_Lhs_Var_Ref(nodep);
            } இல்லையெனில் எனில் (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_வரி* மாறிலி fl = nodep->fileline();
                AstVar* மாறிலி enVarp = getCreateEnVarp(nodep->varp(), பொய்);
                எனில் (AstVarXRef* மாறிலி xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } இல்லையெனில் {
                    nodep->user1p(புதிய AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    வெறுமை visit(AstVarRef* nodep) override { handle_முனை_Var_Ref(nodep); }
    வெறுமை visit(AstVarXRef* nodep) override { handle_முனை_Var_Ref(nodep); }

    வெறுமை visit(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        எனில் (m_graphing) {
            // If tri0/1 force a pullup
            எனில் (nodep->user2() & U2_GRAPHING) திரும்பு;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            எனில் (nodep->isPulldown() || nodep->isPullup()) {
                Ast_முனை* மாறிலி newp = புதிய AstPull{
                    nodep->fileline(), புதிய AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->addNextHere(newp);
                // We'll iterate on the new AstPull later
            }
            எனில் (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.setTristate(nodep);
            }
        } இல்லையெனில் {  // !graphing
            எனில் (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    வெறுமை visit(Ast_முனை_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        எனில் (m_processedIfaces && VN_IS(nodep, Iface)) திரும்பு;
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
        m_graphing = பொய்;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = nullptr;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = உண்மை;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = பொய்;
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
        செருகு_Tristates(nodep);
        செருகு_Procedural_Enable_Assignments(nodep);

        க்கு (auto& assignEnp : m_assignEnps) {
            VL_DO_DANGLING(assignEnp.second->delete_மரம்(), assignEnp.second);
        }
        m_assignEnps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    வெறுமை visit(AstClass* nodep) override {
        // don't deal with classes
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        // don't deal with functions
    }

    வெறுமை visit(AstCaseItem* nodep) override {
        // don't deal with casez compare '???? values
        iterateAndNextNull(nodep->stmtsp());
    }

    வெறுமை visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = பொய்;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    வெறுமை visit(Ast_முனை* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    வெறுமை combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        க்கு (மாறிலி std::pair<AstVar* மாறிலி, std::vector<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* மாறிலி invarp = kv.first;
            மாறிலி std::vector<std::pair<AstVar*, AstVar*>>& contribs = kv.second;
            Ast_முனை_Module* மாறிலி ifaceModp = findParentModule(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_வரி* மாறிலி fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.size() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_முனை_கோவை* orp = nullptr;
            Ast_முனை_கோவை* enp = nullptr;

            க்கு (மாறிலி std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* மாறிலி contribEnp = contrib.first;
                AstVar* மாறிலி contribOutp = contrib.second;

                Ast_முனை_கோவை* மாறிலி outRefp = புதிய AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_முனை_கோவை* மாறிலி enRefp = புதிய AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_முனை_கோவை* மாறிலி andp = புதிய AstAnd{fl, outRefp, enRefp};

                orp = (!orp) ? andp : புதிய AstOr{fl, orp, andp};

                Ast_முனை_கோவை* மாறிலி enRef2p = புதிய AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : புதிய AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            எனில் (!envarp) {
                envarp = புதிய AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->setDfgTriLowered();
                ifaceModp->addStmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            AstAssignW* மாறிலி enAssp
                = புதிய AstAssignW{fl, புதிய AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->addStmtsp(புதிய AstAlways{enAssp});

            // Pull resolution
            மாறிலி AstPull* மாறிலி pullp = m_varAux(invarp).pullp;
            மாறிலி தருக்க pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_முனை_கோவை* undrivenp = புதிய AstNot{fl, புதிய AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = புதிய AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = புதிய AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            AstAssignW* மாறிலி assp
                = புதிய AstAssignW{fl, புதிய AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->addStmtsp(புதிய AstAlways{assp});
        }
    }

பொது:
    // CONSTRUCTORS
    explicit TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::vector<Ast_முனை_Module*> ifacesp;
        க்கு (Ast_முனை* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            எனில் (VN_IS(modp, Iface)) ifacesp.தள்ளு_back(VN_AS(modp, முனை_Module));
        }
        க்கு (auto it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = உண்மை;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ifdef VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::vector<Ast_முனை*> unusedRootps;
        netlistp->foreach([&](Ast_முனை* nodep) {
            Ast_முனை* மாறிலி enp = nodep->user1p();
            எனில் (!enp) திரும்பு;
            எனில் (enp->backp()) திரும்பு;
            unusedRootps.emplace_back(enp);
        });
        க்கு (Ast_முனை* மாறிலி nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
#endif
    }
    ~TristateVisitor() override {
        V3Stats::addStat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

வெறுமை V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("tristate", 0, dump_மரம்_Either_Level() >= 3);
}
