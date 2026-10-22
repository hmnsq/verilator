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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Tristate.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Inst.h"
#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి Tristate_పునాది_Visitor VL_NOT_FINAL : బహిరంగ VNVisitor {
బహిరంగ:
    // METHODS
};

//######################################################################
// Graph support classes

తరగతి Tristate_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Tristate_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    Ast_కణుపు* స్థిరరాశి m_nodep;
    తార్కిక m_isTristate = అబద్ధం;  // Logic indicates a tristate
    తార్కిక m_feedsTri = అబద్ధం;  // Propagates to a tristate node (on RHS)
    తార్కిక m_processed = అబద్ధం;  // Tristating was cleaned up
బహిరంగ:
    Tristate_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_nodep{nodep} {}
    ~Tristate_శీర్షం() override = అప్రమేయ;
    // ACCESSORS
    Ast_కణుపు* nodep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_nodep; }
    స్థిరరాశి AstVar* varp() స్థిరరాశి { తిరిగి VN_CAST(nodep(), Var); }
    string name() స్థిరరాశి override {
        తిరిగి ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvt_గమ్యం_Hex(nodep())));
    }
    string dotColor() స్థిరరాశి override {
        తిరిగి (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి nodep()->fileline(); }
    తార్కిక isTristate() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_isTristate; }
    శూన్య isTristate(తార్కిక flag) { m_isTristate = flag; }
    తార్కిక feedsTri() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_feedsTri; }
    శూన్య feedsTri(తార్కిక flag) { m_feedsTri = flag; }
    తార్కిక processed() స్థిరరాశి { తిరిగి m_processed; }
    శూన్య processed(తార్కిక flag) { m_processed = flag; }
};

//######################################################################

తరగతి Tristate_లేఖాచిత్రం final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
బహిరంగ:
    ఉపయోగించి VarVec = std::సదిశ<AstVar*>;

అంతరంగ:
    // MEMBERS
    V3_లేఖాచిత్రం m_లేఖాచిత్రం;  // Logic graph

బహిరంగ:
    // CONSTRUCTORS
    Tristate_లేఖాచిత్రం() { clearAndCheck(); }
    మాయా ~Tristate_లేఖాచిత్రం() { clearAndCheck(); }
    VL_UNCOPYABLE(Tristate_లేఖాచిత్రం);

అంతరంగ:
    // METHODS

    Tristate_శీర్షం* make_శీర్షం(Ast_కణుపు* nodep) {
        Tristate_శీర్షం* vertexp = reinterpret_cast<Tristate_శీర్షం*>(nodep->user4p());
        ఒకవేళ (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = కొత్త Tristate_శీర్షం{&m_లేఖాచిత్రం, nodep};
            nodep->user4p(vertexp);
        }
        తిరిగి vertexp;
    }

    // METHODS - Graph optimization
    శూన్య లేఖాచిత్రం_Walk_Recurse_Fwd(Tristate_శీర్షం* vtxp, పూర్ణాంకం level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        ఒకవేళ (!vtxp->isTristate()) తిరిగి;  // tristate involved
        ఒకవేళ (vtxp->user() == 1) తిరిగి;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        ఒకవేళ (!vtxp->varp()) {  // not a var where we stop the recursion
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->బహిర్గతం_అంచులు()) {
                Tristate_శీర్షం* స్థిరరాశి vvertexp = static_cast<Tristate_శీర్షం*>(అంచు.శిఖరం());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ఒకవేళ (!vvertexp->isTristate()) {
                    vvertexp->isTristate(నిజం);
                    లేఖాచిత్రం_Walk_Recurse_Fwd(vvertexp, level + 1);
                }
            }
        } లేకపోతే {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->లోపల_అంచులు()) {
                Tristate_శీర్షం* స్థిరరాశి vvertexp = static_cast<Tristate_శీర్షం*>(అంచు.fromp());
                ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    ఒకవేళ (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(నిజం);
                        లేఖాచిత్రం_Walk_Recurse_Fwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    శూన్య లేఖాచిత్రం_Walk_Recurse_వెనుక(Tristate_శీర్షం* vtxp, పూర్ణాంకం level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        ఒకవేళ (!(vtxp->isTristate() || vtxp->feedsTri())) తిరిగి;  // tristate involved
        ఒకవేళ (vtxp->user() == 3) తిరిగి;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        ఒకవేళ (!vtxp->varp()) {  // not a var where we stop the recursion
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->లోపల_అంచులు()) {
                Tristate_శీర్షం* స్థిరరాశి vvertexp = static_cast<Tristate_శీర్షం*>(అంచు.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ఒకవేళ (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(నిజం);
                    లేఖాచిత్రం_Walk_Recurse_వెనుక(vvertexp, level + 1);
                }
            }
        }
    }

బహిరంగ:
    // METHODS
    తార్కిక ఖాళీలు() స్థిరరాశి { తిరిగి m_లేఖాచిత్రం.ఖాళీలు(); }
    శూన్య clearAndCheck() {
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            స్థిరరాశి Tristate_శీర్షం& vvertex = static_cast<Tristate_శీర్షం&>(vtx);
            ఒకవేళ (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_లేఖాచిత్రం.clear();
        Ast_కణుపు::user4_Clear_వృక్షం();  // Wipe all node user4p's that point to vertexes
    }
    శూన్య లేఖాచిత్రం_Walk(Ast_కణుపు_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            లేఖాచిత్రం_Walk_Recurse_Fwd(static_cast<Tristate_శీర్షం*>(&vtx), 0);
        }
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            లేఖాచిత్రం_Walk_Recurse_వెనుక(static_cast<Tristate_శీర్షం*>(&vtx), 0);
        }
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_లేఖాచిత్రం.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    శూన్య associate(Ast_కణుపు* fromp, Ast_కణుపు* శిఖరం) {
        కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, make_శీర్షం(fromp), make_శీర్షం(శిఖరం), 1};
    }
    శూన్య deleteVerticesFromSubtreeRecurse(Ast_కణుపు* nodep) {
        ఒకవేళ (!nodep) తిరిగి;
        // Skip vars, because they may be connected to more than one varref
        ఒకవేళ (!VN_IS(nodep, Var)) {
            Tristate_శీర్షం* స్థిరరాశి vertexp = reinterpret_cast<Tristate_శీర్షం*>(nodep->user4p());
            ఒకవేళ (vertexp) vertexp->unlinkDelete(&m_లేఖాచిత్రం);
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    శూన్య సమితి_Tristate(Ast_కణుపు* nodep) { make_శీర్షం(nodep)->isTristate(నిజం); }
    తార్కిక isTristate(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి Tristate_శీర్షం* స్థిరరాశి vertexp = reinterpret_cast<Tristate_శీర్షం*>(nodep->user4p());
        తిరిగి vertexp && vertexp->isTristate();
    }
    తార్కిక feedsTri(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి Tristate_శీర్షం* స్థిరరాశి vertexp = reinterpret_cast<Tristate_శీర్షం*>(nodep->user4p());
        తిరిగి vertexp && vertexp->feedsTri();
    }
    శూన్య didProcess(Ast_కణుపు* nodep, తార్కిక quiet = అబద్ధం) {
        ఒకవేళ (Tristate_శీర్షం* స్థిరరాశి vertexp = reinterpret_cast<Tristate_శీర్షం*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(నిజం);
        } లేకపోతే ఒకవేళ (!quiet) {
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
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            స్థిరరాశి Tristate_శీర్షం& vvertex = static_cast<స్థిరరాశి Tristate_శీర్షం&>(vtx);
            ఒకవేళ (vvertex.isTristate()) {
                ఒకవేళ (AstVar* స్థిరరాశి nodep = VN_CAST(vvertex.nodep(), Var)) v.నెట్టు_వెనుక(nodep);
            }
        }
        తిరిగి v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

తరగతి TristatePinVisitor final : బహిరంగ Tristate_పునాది_Visitor {
    Tristate_లేఖాచిత్రం& m_tgraph;
    స్థిరరాశి తార్కిక m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        ఒకవేళ (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } లేకపోతే ఒకవేళ (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.సమితి_Tristate(nodep->varp());
        }
    }
    శూన్య సందర్శించు(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_ముక్క_Sel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_కణుపు* nodep, Tristate_లేఖాచిత్రం& tgraph, తార్కిక lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = అప్రమేయ;
};

//######################################################################

తరగతి TristateVisitor final : బహిరంగ Tristate_పునాది_Visitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;
    స్థిరరాశి VNUser4_లోపల_Use m_inuser4;

    నిర్మాణం AuxAstVar final {
        AstPull* pullp = శూన్యసూచిక;  // pullup/pulldown direction (whole variable)
        AstVar* బహిర్గతం_Varp = శూన్యసూచిక;  // output __out var
        తార్కిక ifaceTristate = అబద్ధం;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<పూర్ణాంకం, పూర్ణాంకం>
            బిట్టు_Pulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    నిర్మాణం RefStrength final {
        Ast_కణుపు_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_కణుపు_కేటాయించు* m_assignp;  // Assignment containing m_varrefp
        స్థిరరాశి Ast_కణుపు_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_కణుపు_సమాసం* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_కణుపు_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_కణుపు_సమాసం* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_కణుపు_Var_Ref* varrefp, Ast_కణుపు_కేటాయించు* assignp,
                    స్థిరరాశి Ast_కణుపు_Procedure* procedurep, Ast_కణుపు_సమాసం* enLhsp,
                    Ast_కణుపు_Var_Ref* enVarrefp, Ast_కణుపు_సమాసం* enRhsp, VStrength strength,
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
    నిర్మాణం LhsProjection final {
        Ast_కణుపు_సమాసం* m_lhsp;  // LHS giving the enable assign
        Ast_కణుపు_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_కణుపు_సమాసం* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    నిర్మాణం ProceduralEnable final {
        Ast_కణుపు_సమాసం* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_కణుపు_సమాసం* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    ఉపయోగించి RefStrengthVec = std::సదిశ<RefStrength>;
    ఉపయోగించి VarMap = std::map<AstVar*, RefStrengthVec*>;
    ఉపయోగించి Assigns = std::సదిశ<Ast_కేటాయించు_W*>;
    ఉపయోగించి Var_గమ్యం_Assigns_Map = std::map<AstVar*, Assigns>;
    ఉపయోగించి కేటాయించు_Enable_Map = std::map<Ast_కణుపు_కేటాయించు*, Ast_కణుపు_సమాసం*>;
    ఉపయోగించి ProceduralEnableMap = std::map<Ast_కణుపు_కేటాయించు*, std::సదిశ<ProceduralEnable>>;
    గణన : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    తార్కిక m_graphing = అబద్ధం;  // Major mode - creating graph
    //
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    AstCell* m_cellp = శూన్యసూచిక;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    Var_గమ్యం_Assigns_Map m_assigns;  // Assigns in current module
    కేటాయించు_Enable_Map m_కేటాయించు_Enps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    పూర్ణాంకం m_unique = 0;
    తార్కిక m_alhs = అబద్ధం;  // On LHS of assignment
    తార్కిక m_లోపల_Alias = అబద్ధం;  // Inside alias statement
    తార్కిక m_processedIfaces = అబద్ధం;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = శూన్యసూచిక;  // Delay of assignment currently collecting LHS drivers
    స్థిరరాశి Ast_కణుపు* m_logicp = శూన్యసూచిక;  // Current logic being built
    Ast_కణుపు_కేటాయించు* m_assignp = శూన్యసూచిక;  // Current assignment
    స్థిరరాశి Ast_కణుపు_Procedure* m_procedurep = శూన్యసూచిక;  // Current procedural process
    Tristate_లేఖాచిత్రం m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::సదిశ<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() స్థిరరాశి {
        string o = (m_graphing ? " gr " : " ng ");
        ఒకవేళ (m_alhs) o += "alhs ";
        తిరిగి o;
    }
    శూన్య mod_కూడు_Stmtp(Ast_కణుపు* nodep, Ast_కణుపు* newp) {
        ఒకవేళ (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } లేకపోతే {
            m_modp->కూడు_Stmtsp(newp);
        }
    }
    శూన్య associateLogic(Ast_కణుపు* fromp, Ast_కణుపు* శిఖరం) {
        ఒకవేళ (m_logicp) m_tgraph.associate(fromp, శిఖరం);
    }
    AstConst* newAllZerosOrOnes(Ast_కణుపు* nodep, తార్కిక ones) {
        V3_సంఖ్య అంకె{nodep, nodep->వెడల్పు()};
        ఒకవేళ (ones) అంకె.సమితి_All_Bits1();
        AstConst* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), అంకె};
        తిరిగి newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_పంక్తి* fl, AstVar* varp, స్థిరరాశి string& dotted, VAccess access,
                           స్థిరరాశి string& inlinedDots) {
        AstVarXRef* స్థిరరాశి xrefp = కొత్త AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        తిరిగి xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* కనుగొను_Modport_For_Dotted(Ast_కణుపు_Module* modp, స్థిరరాశి string& dotted_మార్గం) {
        ఒకవేళ (dotted_మార్గం.ఖాళీలు()) తిరిగి శూన్యసూచిక;
        స్థిరరాశి string మొదటి_Comp = dotted_మార్గం.substr(0, dotted_మార్గం.కనుగొను('.'));
        కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* స్థిరరాశి ivarp = VN_CAST(stmtp, Var);
            ఒకవేళ (!ivarp || ivarp->name() != మొదటి_Comp) కొనసాగు;
            స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            ఒకవేళ (ifaceDtp && ifaceDtp->modportp()) తిరిగి ifaceDtp->modportp();
            ఆపు;
        }
        తిరిగి శూన్యసూచిక;
    }
    Ast_కణుపు_సమాసం* getEnp(Ast_కణుపు* nodep) {
        ఒకవేళ (nodep->user1p()) {
            ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి refp = VN_CAST(nodep, కణుపు_Var_Ref)) {
                ఒకవేళ (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    తిరిగి newAllZerosOrOnes(nodep, నిజం);
                }
            }
        } లేకపోతే {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, నిజం));
        }
        // Otherwise return the previous output enable
        తిరిగి VN_AS(nodep->user1p(), కణుపు_సమాసం);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    శూన్య కూడు_Modport_Var_Ref_If_Missing(AstModport* modportp, AstVar* varp, VDirection dir) {
        కొరకు (Ast_కణుపు* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* స్థిరరాశి mvrp = VN_CAST(mrp, ModportVarRef);
            ఒకవేళ (mvrp && mvrp->varp() == varp) తిరిగి;  // Already present
        }
        AstModportVarRef* స్థిరరాశి mvrp = కొత్త AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->కూడు_Varsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, తార్కిక is_శిఖరం) {
        // Return the master __en for the specified input variable
        ఒకవేళ (!invarp->user1p()) {
            AstVar* స్థిరరాశి newp
                = కొత్త AstVar{invarp->fileline(), is_శిఖరం ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_కణుపు_Module* స్థిరరాశి ownerModp = కనుగొను_మాతృ_Module(invarp);
            ఒకవేళ (ownerModp && ownerModp != m_modp) {
                ownerModp->కూడు_Stmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                కొరకు (Ast_కణుపు* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* స్థిరరాశి modportp = VN_CAST(stmtp, Modport);
                    ఒకవేళ (!modportp) కొనసాగు;
                    కొరకు (Ast_కణుపు* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* స్థిరరాశి mvrp = VN_CAST(mrp, ModportVarRef);
                        ఒకవేళ (mvrp && mvrp->varp() == invarp) {
                            కూడు_Modport_Var_Ref_If_Missing(modportp, newp, VDirection::ఆదానం);
                            ఆపు;
                        }
                    }
                }
            } లేకపోతే {
                mod_కూడు_Stmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        తిరిగి VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* స్థిరరాశి constp) {
        File_పంక్తి* స్థిరరాశి fl = constp->fileline();
        V3_సంఖ్య numz{constp, constp->వెడల్పు()};
        numz.op_బిట్లు_Z(constp->అంకె());  // Z->1, else 0
        V3_సంఖ్య numz0{constp, constp->వెడల్పు()};
        numz0.opNot(numz);  // Z->0, else 1
        తిరిగి కొత్త AstConst{fl, numz0};
    }
    Ast_కణుపు_సమాసం* get_En_సమాసం_Based_On_Originalp(Ast_కణుపు_సమాసం* స్థిరరాశి nodep) {
        ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep, VarRef)) {
            తిరిగి కొత్త AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), అబద్ధం),
                                 VAccess::READ};
        } లేకపోతే ఒకవేళ (AstVarXRef* స్థిరరాశి xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* స్థిరరాశి enVarp = getCreateEnVarp(xrefp->varp(), అబద్ధం);
            తిరిగి newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } లేకపోతే ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
            తిరిగి getNonZConstp(constp);
        } లేకపోతే ఒకవేళ (AstExtend* స్థిరరాశి extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = extendp->lhsp()->unlink_Fr_వెనుక();
            AstExtend* స్థిరరాశి enExtendp = extendp->clone_వృక్షం(అబద్ధం);
            extendp->lhsp(lhsp);
            Ast_కణుపు_సమాసం* స్థిరరాశి enLhsp = get_En_సమాసం_Based_On_Originalp(lhsp);
            enExtendp->lhsp(కొత్త AstNot{enLhsp->fileline(), enLhsp});
            తిరిగి కొత్త AstNot{enExtendp->fileline(), enExtendp};
        } లేకపోతే ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = selp->fromp()->unlink_Fr_వెనుక();
            AstSel* స్థిరరాశి enSelp = selp->clone_వృక్షం(అబద్ధం);
            selp->fromp(fromp);
            Ast_కణుపు_సమాసం* స్థిరరాశి enFromp = get_En_సమాసం_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            తిరిగి enSelp;
        } లేకపోతే {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            తిరిగి శూన్యసూచిక;
        }
    }
    AstVar* get_Create_బహిర్గతం_Varp(AstVar* invarp, తార్కిక is_శిఖరం) {
        // Return the master __out for the specified input variable
        ఒకవేళ (!m_varAux(invarp).బహిర్గతం_Varp) {
            AstVar* స్థిరరాశి newp
                = కొత్త AstVar{invarp->fileline(), is_శిఖరం ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            mod_కూడు_Stmtp(invarp, newp);
            m_varAux(invarp).బహిర్గతం_Varp = newp;  // find outvar given invarp
        }
        తిరిగి m_varAux(invarp).బహిర్గతం_Varp;
    }
    AstVar* getCreateUnconnVarp(Ast_కణుపు* fromp, Ast_కణుపు_DType* dtypep) {
        AstVar* స్థిరరాశి newp = కొత్త AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvt_గమ్యం_Str(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        mod_కూడు_Stmtp(newp, newp);
        తిరిగి newp;
    }

    LhsProjection newLhsProjection(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_Var_Ref* targetp, Ast_కణుపు_సమాసం* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_కణుపు_సమాసం* స్థిరరాశి newLhsp = lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_కణుపు* nodep) { nodep->user1p(శూన్యసూచిక); });
        Ast_కణుపు_Var_Ref* స్థిరరాశి newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        తిరిగి LhsProjection{newLhsp, newVarrefp, enp};
    }

    శూన్య map_చొప్పించు_Lhs_Var_Ref(Ast_కణుపు_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* స్థిరరాశి కీలకం = nodep->varp();
        స్థిరరాశి స్వయంచాలక pair = m_lhsmap.emplace(కీలకం, శూన్యసూచిక);
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ = కొత్త RefStrengthVec;
        Ast_కణుపు_సమాసం* enLhsp = శూన్యసూచిక;
        Ast_కణుపు_Var_Ref* enVarrefp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* enRhsp = శూన్యసూచిక;
        స్థిరరాశి స్వయంచాలక enIt = m_కేటాయించు_Enps.కనుగొను(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        స్థిరరాశి తార్కిక canCombineProcedural = !VN_IS(కీలకం->dtypep()->skipRefp(), కణుపు_UOr_Struct_DType);
        స్థిరరాశి Ast_కణుపు_Procedure* స్థిరరాశి procedurep
            = enIt != m_కేటాయించు_Enps.చివర() && canCombineProcedural ? m_procedurep : శూన్యసూచిక;
        ఒకవేళ (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->రెండవ->clone_వృక్షం_శుద్ధ(అబద్ధం));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.మొదటి->రెండవ->నెట్టు_వెనుక(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_కణుపు_సమాసం* newEnableDeposit(AstSel* selp, Ast_కణుపు_సమాసం* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త Ast_జరుపు_L{
            selp->fileline(), కొత్త AstExtend{selp->fileline(), enp, selp->fromp()->వెడల్పు()},
            selp->lsbp()->clone_వృక్షం_శుద్ధ(అబద్ధం), selp->fromp()->వెడల్పు()};
        తిరిగి newp;
    }

    శూన్య సమితి_Pull_Direction(AstVar* varp, AstPull* pullp) {
        స్థిరరాశి AstPull* స్థిరరాశి oldpullp = m_varAux(varp).pullp;
        ఒకవేళ (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } లేకపోతే {
            ఒకవేళ (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    శూన్య సమితి_బిట్టు_Pull_Direction(AstVar* varp, పూర్ణాంకం బిట్టు_సూచీ, పూర్ణాంకం direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        స్వయంచాలక& బిట్టు_Pulls = m_varAux(varp).బిట్టు_Pulls;
        స్వయంచాలక it = బిట్టు_Pulls.కనుగొను(బిట్టు_సూచీ);
        ఒకవేళ (it != బిట్టు_Pulls.చివర() && it->రెండవ != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << బిట్టు_సూచీ << " of " << varp->prettyNameQ());
        }
        బిట్టు_Pulls[బిట్టు_సూచీ] = direction;
    }

    AstConst* create_చొప్పున_బిట్టు_Pull_Const(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        స్థిరరాశి స్వయంచాలక& బిట్టు_Pulls = m_varAux(varp).బిట్టు_Pulls;
        V3_సంఖ్య అంకె{varp, varp->వెడల్పు(), 0};  // Start with all zeros
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : బిట్టు_Pulls) {
            ఒకవేళ (pair.రెండవ == 1) అంకె.సమితి_బిట్టు(pair.మొదటి, 1);
        }  // LCOV_EXCL_LINE
        తిరిగి కొత్త AstConst{varp->fileline(), అంకె};
    }

    తార్కిక has_చొప్పున_బిట్టు_Pulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        తిరిగి !m_varAux(varp).బిట్టు_Pulls.ఖాళీలు();
    }

    శూన్య checkUnhandled(Ast_కణుపు* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        ఒకవేళ (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        ఒకవేళ ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    శూన్య చొప్పించు_Tristates(Ast_కణుపు_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        స్థిరరాశి Tristate_లేఖాచిత్రం::VarVec vars = m_tgraph.tristateVars();
        కొరకు (స్వయంచాలక varp : vars) {
            ఒకవేళ (!m_tgraph.isTristate(varp)) కొనసాగు;
            స్థిరరాశి స్వయంచాలక it = m_lhsmap.కనుగొను(varp);
            ఒకవేళ (it != m_lhsmap.చివర()) కొనసాగు;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            స్థిరరాశి AstPull* స్థిరరాశి pullp = m_varAux(varp).pullp;
            స్థిరరాశి తార్కిక pull_విలువ = pullp && pullp->direction() == 1;
            AstConst* స్థిరరాశి constp = newAllZerosOrOnes(varp, pull_విలువ);
            AstVarRef* స్థిరరాశి varrefp = కొత్త AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            Ast_కేటాయించు_W* స్థిరరాశి newp = కొత్త Ast_కేటాయించు_W{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, అబద్ధం));  // en=0 for floating/pullup vars
            nodep->కూడు_Stmtsp(కొత్త AstAlways{newp});
            map_చొప్పించు_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        కొరకు (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            స్థిరరాశి std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.కనుగొను(varp);
            ఒకవేళ (it == m_lhsmap.చివర()) కొనసాగు;
            AstVar* స్థిరరాశి invarp = it->మొదటి;
            RefStrengthVec* refsp = it->రెండవ;
            // Figure out if this var needs tristate expanded.
            ఒకవేళ (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_కణుపు_Module* స్థిరరాశి ownerModp = కనుగొను_మాతృ_Module(invarp);
                స్థిరరాశి తార్కిక isCrossModule = ownerModp && ownerModp != nodep;
                స్థిరరాశి తార్కిక isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                ఒకవేళ (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    కొరకు (స్థిరరాశి RefStrength& rs : *refsp) {
                        ఒకవేళ (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            ఆపు;
                        }
                    }
                } లేకపోతే ఒకవేళ (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = నిజం;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    నిర్మాణం విభజన_Info final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, విభజన_Info> partitions;
                    కొరకు (స్థిరరాశి RefStrength& rs : *refsp) {
                        ఒకవేళ (AstVarXRef* స్థిరరాశి xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            విభజన_Info& pi = partitions[xrefp->dotted()];
                            pi.refs.నెట్టు_వెనుక(rs);
                            ఒకవేళ (pi.inlinedDots.ఖాళీలు()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } లేకపోతే {
                            partitions[""].refs.నెట్టు_వెనుక(rs);
                        }
                    }
                    కొరకు (స్వయంచాలక& kv : partitions) {
                        చొప్పించు_Tristates_Signal(nodep, invarp, &kv.రెండవ.refs, నిజం, kv.మొదటి,
                                              kv.రెండవ.inlinedDots,
                                              కనుగొను_Modport_For_Dotted(nodep, kv.మొదటి));
                    }
                } లేకపోతే ఒకవేళ (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = నిజం;
                    చొప్పించు_Tristates_Signal(nodep, invarp, refsp, నిజం, "", "", శూన్యసూచిక);
                } లేకపోతే {
                    చొప్పించు_Tristates_Signal(nodep, invarp, refsp, అబద్ధం, "", "", శూన్యసూచిక);
                }
            } లేకపోతే {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(తొలగించు refsp, refsp);
        }
    }

    శూన్య aggregateTriSameStrength(Ast_కణుపు_Module* nodep, AstVar* స్థిరరాశి varp, AstVar* స్థిరరాశి envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator చివర_Strength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        నిర్మాణం Driver_సమూహం final {
            స్థిరరాశి Ast_కణుపు_Procedure* m_procedurep;  // Process owning these assignments
            std::సదిశ<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::సదిశ<Driver_సమూహం> drivers;
        std::map<స్థిరరాశి Ast_కణుపు_Procedure*, కొలత_t> procedure_గమ్యం_Driver;
        కొరకు (స్వయంచాలక it = beginStrength; it != చివర_Strength; ++it) {
            ఒకవేళ (it->m_procedurep) {
                స్థిరరాశి స్వయంచాలక emplaced = procedure_గమ్యం_Driver.emplace(it->m_procedurep, drivers.కొలత());
                ఒకవేళ (!emplaced.రెండవ) {
                    drivers[emplaced.మొదటి->రెండవ].m_refsp.నెట్టు_వెనుక(&*it);
                    కొనసాగు;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.నెట్టు_వెనుక(Driver_సమూహం{it->m_procedurep, {&*it}});
        }

        Ast_కణుపు_సమాసం* orp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* enp = శూన్యసూచిక;

        కొరకు (Driver_సమూహం& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* స్థిరరాశి newLhsp = కొత్త AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvt_గమ్యం_Str(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->సమితి_Dfg_Tri_Lowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->కూడు_Stmtsp(newLhsp);
            AstVar* స్థిరరాశి newEnLhsp
                = కొత్త AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvt_గమ్యం_Str(m_unique++), envarp};  // 2-state ok
            newEnLhsp->సమితి_Dfg_Tri_Lowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->కూడు_Stmtsp(newEnLhsp);

            కొరకు (RefStrength* స్థిరరాశి refStrengthp : driver.m_refsp) {
                Ast_కణుపు_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                ఒకవేళ (VN_IS(refp, VarXRef)) {
                    AstVarRef* స్థిరరాశి localRefp
                        = కొత్త AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(శూన్యసూచిక);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } లేకపోతే {
                    refp->varp(newLhsp);
                }

                ఒకవేళ (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_కణుపు_సమాసం* enLhsp = refStrengthp->m_enLhsp;
                    Ast_కణుపు_Var_Ref* స్థిరరాశి enVarrefp = refStrengthp->m_enVarrefp;
                    ఒకవేళ (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* స్థిరరాశి localRefp
                            = కొత్త AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        ఒకవేళ (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_వృక్షం(), enVarrefp);
                        } లేకపోతే {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(నెట్టు_Deletep(enVarrefp), enVarrefp);
                        }
                    } లేకపోతే {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].నెట్టు_వెనుక(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = శూన్యసూచిక;
                    refStrengthp->m_enVarrefp = శూన్యసూచిక;
                    refStrengthp->m_enRhsp = శూన్యసూచిక;
                } లేకపోతే {
                    Ast_కేటాయించు_W* స్థిరరాశి enLhspAssignp = కొత్త Ast_కేటాయించు_W{
                        refp->fileline(),
                        కొత్త AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    ఒకవేళ (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_వృక్షం(అబద్ధం));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->కూడు_Stmtsp(కొత్త AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_పంక్తి* స్థిరరాశి fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_కణుపు_సమాసం* స్థిరరాశి ref1p = కొత్త AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_కణుపు_సమాసం* స్థిరరాశి ref2p = కొత్త AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_కణుపు_సమాసం* స్థిరరాశి andp = కొత్త AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : కొత్త AstOr{fl, orp, andp};

            Ast_కణుపు_సమాసం* స్థిరరాశి ref3p = కొత్త AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : కొత్త AstOr{ref3p->fileline(), enp, ref3p};
        }
        Ast_కేటాయించు_W* స్థిరరాశి assp = కొత్త Ast_కేటాయించు_W{
            varp->fileline(), కొత్త AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->కూడు_Stmtsp(కొత్త AstAlways{assp});

        Ast_కేటాయించు_W* స్థిరరాశి enAssp = కొత్త Ast_కేటాయించు_W{
            envarp->fileline(), కొత్త AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->కూడు_Stmtsp(కొత్త AstAlways{enAssp});
    }

    శూన్య చొప్పించు_Procedural_Enable_Assignments(Ast_కణుపు_Module* nodep) {
        కొరకు (స్వయంచాలక& కేటాయించు_Enables : m_proceduralEnables) {
            Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = కేటాయించు_Enables.మొదటి;
            Ast_కణుపు_సమాసం* combinedLhsp = assignp->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* combinedRhsp = assignp->rhsp()->unlink_Fr_వెనుక();
            కొరకు (ProceduralEnable& enable : కేటాయించు_Enables.రెండవ) {
                combinedLhsp = కొత్త AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = కొత్త AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            ఒకవేళ (!VN_IS(assignp, కేటాయించు_Dly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* స్థిరరాశి sampledp
                    = కొత్త AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvt_గమ్యం_Str(m_unique++), combinedRhsp->dtypep()};
                nodep->కూడు_Stmtsp(sampledp);
                assignp->lhsp(కొత్త AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                Ast_కేటాయించు* స్థిరరాశి applyp
                    = కొత్త Ast_కేటాయించు{assignp->fileline(), combinedLhsp,
                                    కొత్త AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->కూడు_తదుపరి_ఇక్కడ(applyp);
            } లేకపోతే {
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
    శూన్య చొప్పించు_Tristates_Signal(Ast_కణుపు_Module* nodep, AstVar* స్థిరరాశి invarp, RefStrengthVec* refsp,
                               తార్కిక isIfaceTri, స్థిరరాశి string& iface_Dotted_మార్గం,
                               స్థిరరాశి string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        AstVar* envarp = శూన్యసూచిక;
        AstVar* outvarp = శూన్యసూచిక;  // __out
        AstVar* lhsp = invarp;  // Variable to assign drive-value to (<in> or __out)
        స్థిరరాశి తార్కిక is_శిఖరం_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_శిఖరం();
        ఒకవేళ (!isIfaceTri) {
            ఒకవేళ (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().ఖాళీలు())
                 && is_శిఖరం_Inout)
                || (!nodep->is_శిఖరం() && invarp->isIO())) {
                // This var becomes an input
                invarp->var_Type2_లోపల();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = get_Create_బహిర్గతం_Varp(invarp, is_శిఖరం_Inout);
                outvarp->var_Type2_బహిర్గతం();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_శిఖరం_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).బిట్టు_Pulls
                    = m_varAux(invarp).బిట్టు_Pulls;  // Per-bit pull propagation
                ఒకవేళ (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                ఒకవేళ (!m_varAux(invarp).బిట్టు_Pulls.ఖాళీలు()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } లేకపోతే ఒకవేళ (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_కణుపు_సమాసం* orp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* enp = శూన్యసూచిక;

        std::క్రమీకరించు(refsp->begin(), refsp->చివర(),
                  [](RefStrength a, RefStrength b) { తిరిగి a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        అయితే (beginStrength != refsp->చివర()) {
            RefStrengthVec::iterator చివర_Strength = beginStrength + 1;
            అయితే (చివర_Strength != refsp->చివర()
                   && చివర_Strength->m_strength == beginStrength->m_strength)
                చివర_Strength++;

            File_పంక్తి* స్థిరరాశి fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strength_ఉపసర్గ;
            ఒకవేళ (isIfaceTri && !iface_Dotted_మార్గం.ఖాళీలు()) {
                strength_ఉపసర్గ = iface_Dotted_మార్గం;
                std::replace(strength_ఉపసర్గ.begin(), strength_ఉపసర్గ.చివర(), '.', '_');
                strength_ఉపసర్గ += "__";
            }
            స్థిరరాశి string strengthVarName
                = strength_ఉపసర్గ + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* స్థిరరాశి varStrengthp = కొత్త AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->సమితి_Dfg_Tri_Lowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->కూడు_Stmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* స్థిరరాశి enVarStrengthp = కొత్త AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->సమితి_Dfg_Tri_Lowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->కూడు_Stmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     చివర_Strength);

            Ast_కణుపు_సమాసం* సమాసం_Current_Strengthp;
            ఒకవేళ (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                సమాసం_Current_Strengthp
                    = కొత్త AstAnd{fl, కొత్త AstVarRef{fl, varStrengthp, VAccess::READ},
                                 కొత్త AstNot{fl, enp->clone_వృక్షం_శుద్ధ(అబద్ధం)}};
            } లేకపోతే {
                సమాసం_Current_Strengthp = కొత్త AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? సమాసం_Current_Strengthp : కొత్త AstOr{fl, orp, సమాసం_Current_Strengthp};

            Ast_కణుపు_సమాసం* స్థిరరాశి enVarStrengthRefp
                = కొత్త AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : కొత్త AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = చివర_Strength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        ఒకవేళ (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->సమితి_Dfg_Allow_Multidrive_Tri();
            Ast_కణుపు_Module* స్థిరరాశి ifaceModp = కనుగొను_మాతృ_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            స్థిరరాశి పూర్ణాంకం contribIdx = static_cast<పూర్ణాంకం>(m_ifaceContribs[invarp].కొలత());
            File_పంక్తి* స్థిరరాశి fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* స్థిరరాశి contribOutp = కొత్త AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvt_గమ్యం_Str(contribIdx), invarp};
            contribOutp->సమితి_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->కూడు_Stmtsp(contribOutp);

            AstVar* స్థిరరాశి contribEnp = కొత్త AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvt_గమ్యం_Str(contribIdx), invarp};
            contribEnp->సమితి_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->కూడు_Stmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            ఒకవేళ (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                కూడు_Modport_Var_Ref_If_Missing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                కూడు_Modport_Var_Ref_If_Missing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_కణుపు_Var_Ref* స్థిరరాశి lhsp
                    = iface_Dotted_మార్గం.ఖాళీలు() ? static_cast<Ast_కణుపు_Var_Ref*>(
                                                    కొత్త AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_కణుపు_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, iface_Dotted_మార్గం,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_కేటాయించు_W* స్థిరరాశి assp = కొత్త Ast_కేటాయించు_W{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->కూడు_Stmtsp(కొత్త AstAlways{assp});
            }
            {
                Ast_కణుపు_Var_Ref* స్థిరరాశి lhsp
                    = iface_Dotted_మార్గం.ఖాళీలు() ? static_cast<Ast_కణుపు_Var_Ref*>(
                                                    కొత్త AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_కణుపు_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, iface_Dotted_మార్గం,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_కేటాయించు_W* స్థిరరాశి assp = కొత్త Ast_కేటాయించు_W{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->కూడు_Stmtsp(కొత్త AstAlways{assp});
            }

            m_ifaceContribs[invarp].నెట్టు_వెనుక({contribEnp, contribOutp});
            తిరిగి;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        స్థిరరాశి తార్కిక hasPull = m_varAux(lhsp).pullp || has_చొప్పున_బిట్టు_Pulls(lhsp);
        ఒకవేళ (!outvarp || hasPull) {
            Ast_కణుపు_సమాసం* undrivenp;
            ఒకవేళ (envarp) {
                undrivenp = కొత్త AstNot{envarp->fileline(),
                                       కొత్త AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } లేకపోతే ఒకవేళ (enp) {
                undrivenp = కొత్త AstNot{enp->fileline(), enp};
                enp = శూన్యసూచిక;  // moved into undrivenp
            } లేకపోతే {
                undrivenp = newAllZerosOrOnes(invarp, నిజం);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            ఒకవేళ (has_చొప్పున_బిట్టు_Pulls(lhsp)) {
                pullConstp = create_చొప్పున_బిట్టు_Pull_Const(lhsp);
            } లేకపోతే {
                స్థిరరాశి AstPull* స్థిరరాశి pullp = m_varAux(lhsp).pullp;
                స్థిరరాశి తార్కిక pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = కొత్త AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? కొత్త AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        ఒకవేళ (envarp && !enp) enp = newAllZerosOrOnes(invarp, అబద్ధం);

        ఒకవేళ (envarp) {
            Ast_కేటాయించు_W* స్థిరరాశి enAssp = కొత్త Ast_కేటాయించు_W{
                enp->fileline(), కొత్త AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->కూడు_Stmtsp(కొత్త AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        Ast_కేటాయించు_W* స్థిరరాశి assp = కొత్త Ast_కేటాయించు_W{
            lhsp->fileline(), కొత్త AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->కూడు_Stmtsp(కొత్త AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        ఒకవేళ ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().ఖాళీలు()) && is_శిఖరం_Inout) {
            ఒకవేళ (envarp) {
                envarp->primaryIO(నిజం);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            ఒకవేళ (outvarp) {
                outvarp->primaryIO(నిజం);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
                స్థిరరాశి పూర్ణాంకం id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(నిజం);
            }
        }
    }

    తార్కిక is_Only_Assignment_Is_గమ్యం_Lhs_Var(స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి nodep) {
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            స్వయంచాలక దొరికింది_It = m_assigns.కనుగొను(varRefp->varp());
            ఒకవేళ (దొరికింది_It != m_assigns.చివర()) {
                స్వయంచాలక స్థిరరాశి& assigns_గమ్యం_Var = దొరికింది_It->రెండవ;
                ఒకవేళ (assigns_గమ్యం_Var.కొలత() == 1 && assigns_గమ్యం_Var[0] == nodep) తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }

    శూన్య కూడు_గమ్యం_Assignment_List(Ast_కేటాయించు_W* nodep) {
        ఒకవేళ (AstVarRef* స్థిరరాశి varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            ఒకవేళ (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].నెట్టు_వెనుక(nodep);
            } లేకపోతే ఒకవేళ (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlink_Fr_వెనుక()->delete_వృక్షం();
            }
        } లేకపోతే ఒకవేళ (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి nodep, తార్కిక విలువ) {
        ఒకవేళ (AstStrengthSpec* స్థిరరాశి strengthSpec = nodep->strengthSpecp()) {
            తిరిగి విలువ ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        తిరిగి VStrength::STRONG;  // default strength is strong
    }

    తార్కిక assignment_యొక్క_విలువ_On_All_బిట్లు(స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి nodep, తార్కిక విలువ) {
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->rhsp(), Const)) {
            స్థిరరాశి V3_సంఖ్య అంకె = constp->అంకె();
            తిరిగి విలువ ? అంకె.isEqAllOnes() : అంకె.isEqZero();
        }
        తిరిగి అబద్ధం;
    }

    Ast_కేటాయించు_W* get_Strongest_Assignment_యొక్క_విలువ(స్థిరరాశి Assigns& assigns, తార్కిక విలువ) {
        స్వయంచాలక maxIt = std::max_element(
            assigns.begin(), assigns.చివర(), [&](స్థిరరాశి Ast_కేటాయించు_W* ap, స్థిరరాశి Ast_కేటాయించు_W* bp) {
                స్థిరరాశి తార్కిక విలువలు_On_Rhs_A = assignment_యొక్క_విలువ_On_All_బిట్లు(ap, విలువ);
                స్థిరరాశి తార్కిక విలువలు_On_Rhs_B = assignment_యొక్క_విలువ_On_All_బిట్లు(bp, విలువ);
                ఒకవేళ (!విలువలు_On_Rhs_A) తిరిగి విలువలు_On_Rhs_B;
                ఒకవేళ (!విలువలు_On_Rhs_B) తిరిగి అబద్ధం;
                తిరిగి getStrength(ap, విలువ) < getStrength(bp, విలువ);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        తిరిగి assignment_యొక్క_విలువ_On_All_బిట్లు(*maxIt, విలువ) ? *maxIt : శూన్యసూచిక;
    }

    తార్కిక isAssignmentNotStrongerThanStrength(స్థిరరాశి Ast_కేటాయించు_W* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        స్థిరరాశి uint8_t strength0 = getStrength(assignp, 0);
        స్థిరరాశి uint8_t strength1 = getStrength(assignp, 1);
        తిరిగి (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_యొక్క_విలువ_On_All_బిట్లు(assignp, 0))
               || (strength1 <= strength && assignment_యొక్క_విలువ_On_All_బిట్లు(assignp, 1));
    }

    Ast_కణుపు_సమాసం* new_విలీనం_సమాసం(Ast_కణుపు_సమాసం* స్థిరరాశి lhsp, Ast_కణుపు_సమాసం* స్థిరరాశి rhsp, File_పంక్తి* స్థిరరాశి fl,
                              తార్కిక isWor) {
        Ast_కణుపు_సమాసం* సమాసం = శూన్యసూచిక;
        ఒకవేళ (isWor)
            సమాసం = కొత్త AstOr{fl, lhsp, rhsp};
        లేకపోతే
            సమాసం = కొత్త AstAnd{fl, lhsp, rhsp};
        తిరిగి సమాసం;
    }

    శూన్య విలీనం_Wired_Nets_Assignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        కొరకు (స్వయంచాలక& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.రెండవ;
            ఒకవేళ (assigns.కొలత() <= 1) కొనసాగు;
            AstVar* స్థిరరాశి varp = varpAssigns.మొదటి;
            ఒకవేళ (!varp->isWiredNet()) కొనసాగు;
            స్వయంచాలక it = assigns.begin();
            Ast_కేటాయించు_W* స్థిరరాశి కేటాయించు_Wp0 = *it;
            File_పంక్తి* స్థిరరాశి fl = కేటాయించు_Wp0->fileline();
            Ast_కణుపు_సమాసం* wExp = శూన్యసూచిక;
            అయితే (++it != assigns.చివర()) {
                Ast_కేటాయించు_W* కేటాయించు_Wpi = *it;
                ఒకవేళ (!wExp) {
                    wExp
                        = new_విలీనం_సమాసం(కేటాయించు_Wp0->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                       కేటాయించు_Wpi->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), fl, varp->isWor());
                } లేకపోతే {
                    wExp = new_విలీనం_సమాసం(wExp, కేటాయించు_Wpi->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((కేటాయించు_Wpi->unlink_Fr_వెనుక()->delete_వృక్షం()), కేటాయించు_Wpi);
            }
            AstVarRef* స్థిరరాశి wVarRef = కొత్త AstVarRef{fl, varp, VAccess::WRITE};
            Ast_కేటాయించు_W* స్థిరరాశి wAssignp = కొత్త Ast_కేటాయించు_W{fl, wVarRef, wExp};
            కేటాయించు_Wp0->replaceWith(wAssignp);
            VL_DO_DANGLING(నెట్టు_Deletep(కేటాయించు_Wp0), కేటాయించు_Wp0);
            assigns.clear();
            assigns.నెట్టు_వెనుక(wAssignp);
        }
    }

    శూన్య removeNotStrongerAssignments(Assigns& assigns, స్థిరరాశి Ast_కేటాయించు_W* strongestp,
                                      uint8_t greatest_తెలిసిన_Strength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        స్వయంచాలక removedIt = std::remove_if(assigns.begin(), assigns.చివర(), [&](Ast_కేటాయించు_W* assignp) {
            ఒకవేళ (assignp == strongestp) తిరిగి అబద్ధం;
            ఒకవేళ (isAssignmentNotStrongerThanStrength(assignp, greatest_తెలిసిన_Strength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(నెట్టు_Deletep(assignp->unlink_Fr_వెనుక()), assignp);
                తిరిగి నిజం;
            }
            తిరిగి అబద్ధం;
        });
        assigns.erase(removedIt, assigns.చివర());
    }

    శూన్య remove_Assignments_Not_Stronger_Than_ఏకరీతి_Constant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        కొరకు (స్వయంచాలక& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.రెండవ;
            ఒకవేళ (assigns.కొలత() <= 1) కొనసాగు;
            స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి strongest0p = get_Strongest_Assignment_యొక్క_విలువ(assigns, 0);
            స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి strongest1p = get_Strongest_Assignment_యొక్క_విలువ(assigns, 1);
            స్థిరరాశి Ast_కేటాయించు_W* strongestp = శూన్యసూచిక;
            uint8_t greatest_తెలిసిన_Strength = 0;
            స్థిరరాశి స్వయంచాలక getIfStrongest
                = [&](స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి strongestCandidatep, తార్కిక విలువ) {
                      ఒకవేళ (!strongestCandidatep) తిరిగి;
                      స్థిరరాశి uint8_t strength = getStrength(strongestCandidatep, విలువ);
                      ఒకవేళ (strength >= greatest_తెలిసిన_Strength) {
                          greatest_తెలిసిన_Strength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            ఒకవేళ (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatest_తెలిసిన_Strength);
            }
        }
    }

    శూన్య removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        కొరకు (స్వయంచాలక& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.రెండవ;
            ఒకవేళ (assigns.కొలత() <= 1) కొనసాగు;
            స్వయంచాలక maxIt = std::max_element(
                assigns.begin(), assigns.చివర(), [&](స్థిరరాశి Ast_కేటాయించు_W* ap, స్థిరరాశి Ast_కేటాయించు_W* bp) {
                    ఒకవేళ (m_tgraph.isTristate(ap)) తిరిగి !m_tgraph.isTristate(bp);
                    ఒకవేళ (m_tgraph.isTristate(bp)) తిరిగి అబద్ధం;
                    స్థిరరాశి uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    స్థిరరాశి uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    తిరిగి minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి strongestp = m_tgraph.isTristate(*maxIt) ? శూన్యసూచిక : *maxIt;
            ఒకవేళ (strongestp) {
                స్థిరరాశి uint8_t greatest_తెలిసిన_Strength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatest_తెలిసిన_Strength);
            }
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        ఒకవేళ (m_graphing) {
            ఒకవేళ (!m_alhs && nodep->అంకె().hasZ()) m_tgraph.సమితి_Tristate(nodep);
        } లేకపోతే {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            ఒకవేళ (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* స్థిరరాశి varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_కణుపు* స్థిరరాశి newp = కొత్త AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే ఒకవేళ (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                AstConst* స్థిరరాశి enp = getNonZConstp(nodep);
                V3_సంఖ్య num1{nodep, nodep->వెడల్పు()};
                num1.opAnd(nodep->అంకె(), enp->అంకె());  // 01X->01X, Z->0
                AstConst* స్థిరరాశి newconstp = కొత్త AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    శూన్య సందర్శించు(AstCond* nodep) override {
        ఒకవేళ (m_graphing) {
            iterateChildren(nodep);
            ఒకవేళ (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } లేకపోతే {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } లేకపోతే {
            ఒకవేళ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                తిరిగి;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_కణుపు_సమాసం* స్థిరరాశి condp = nodep->condp();
            ఒకవేళ (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_కణుపు_సమాసం* స్థిరరాశి thenp = nodep->thenp();
            Ast_కణుపు_సమాసం* స్థిరరాశి elsep = nodep->elsep();
            ఒకవేళ (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_కణుపు_సమాసం* స్థిరరాశి en1p = getEnp(thenp);
                Ast_కణుపు_సమాసం* స్థిరరాశి en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_కణుపు_సమాసం* స్థిరరాశి enp
                    = కొత్త AstCond{nodep->fileline(), condp->clone_వృక్షం(అబద్ధం), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                ఒకవేళ (thenp->user1p() && !thenp->user1p()->backp()) నెట్టు_Deletep(thenp->user1p());
                ఒకవేళ (elsep->user1p() && !elsep->user1p()->backp()) నెట్టు_Deletep(elsep->user1p());
                thenp->user1p(శూన్యసూచిక);
                elsep->user1p(శూన్యసూచిక);
            }
        }
    }

    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } లేకపోతే ఒకవేళ (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(శూన్యసూచిక);
            m_tgraph.didProcess(nodep);
        }
    }

    శూన్య సందర్శించు(AstSel* nodep) override {
        ఒకవేళ (m_graphing) {
            iterateChildren(nodep);
            ఒకవేళ (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } లేకపోతే {
                associateLogic(nodep->fromp(), nodep);
            }
        } లేకపోతే {
            ఒకవేళ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ఒకవేళ (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_కణుపు_సమాసం* స్థిరరాశి newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), కణుపు_సమాసం));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } లేకపోతే {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                ఒకవేళ (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                ఒకవేళ (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_కణుపు_సమాసం* స్థిరరాశి en1p = getEnp(nodep->fromp());
                    Ast_కణుపు_సమాసం* స్థిరరాశి enp
                        = కొత్త AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_వృక్షం_శుద్ధ(నిజం),
                                     nodep->వెడల్పు_Const()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    శూన్య సందర్శించు(AstConcat* nodep) override {
        ఒకవేళ (m_graphing) {
            iterateChildren(nodep);
            ఒకవేళ (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } లేకపోతే {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } లేకపోతే {
            ఒకవేళ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ఒకవేళ (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_కణుపు_సమాసం* స్థిరరాశి enp = VN_AS(nodep->user1p(), కణుపు_సమాసం);
                    nodep->user1p(శూన్యసూచిక);
                    nodep->lhsp()->user1p(కొత్త AstSel{nodep->fileline(), enp->clone_వృక్షం_శుద్ధ(నిజం),
                                                     nodep->rhsp()->వెడల్పు(),
                                                     nodep->lhsp()->వెడల్పు()});
                    nodep->rhsp()->user1p(
                        కొత్త AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->వెడల్పు()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } లేకపోతే {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_కణుపు_సమాసం* స్థిరరాశి expr1p = nodep->lhsp();
                Ast_కణుపు_సమాసం* స్థిరరాశి expr2p = nodep->rhsp();
                ఒకవేళ (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_కణుపు_సమాసం* స్థిరరాశి en1p = getEnp(expr1p);
                    Ast_కణుపు_సమాసం* స్థిరరాశి en2p = getEnp(expr2p);
                    Ast_కణుపు_సమాసం* స్థిరరాశి enp = కొత్త AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(శూన్యసూచిక);
                    expr2p->user1p(శూన్యసూచిక);
                }
            }
        }
    }

    శూన్య సందర్శించు(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ఒకవేళ (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.సమితి_Tristate(nodep);
        } లేకపోతే {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            ఒకవేళ (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                తిరిగి;
            }
            m_tgraph.didProcess(nodep);
            Ast_కణుపు_సమాసం* స్థిరరాశి expr1p = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి expr2p = nodep->rhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* enp;
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి en2p = VN_AS(expr2p->user1p(), కణుపు_సమాసం)) {
                enp = కొత్త AstAnd{nodep->fileline(), expr1p, en2p};
            } లేకపోతే {
                enp = expr1p;
            }
            expr1p->user1p(శూన్యసూచిక);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    శూన్య సందర్శించు_And_Or(Ast_కణుపు_Biop* nodep, తార్కిక isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ఒకవేళ (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } లేకపోతే {
            ఒకవేళ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                తిరిగి;
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
            Ast_కణుపు_సమాసం* స్థిరరాశి expr1p = nodep->lhsp();
            Ast_కణుపు_సమాసం* స్థిరరాశి expr2p = nodep->rhsp();
            ఒకవేళ (!expr1p->user1p() && !expr2p->user1p()) {
                తిరిగి;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_కణుపు_సమాసం* en1p = getEnp(expr1p);
            Ast_కణుపు_సమాసం* en2p = getEnp(expr2p);
            Ast_కణుపు_సమాసం* subexpr1p = expr1p->clone_వృక్షం_శుద్ధ(అబద్ధం);
            Ast_కణుపు_సమాసం* subexpr2p = expr2p->clone_వృక్షం_శుద్ధ(అబద్ధం);
            ఒకవేళ (isAnd) {
                subexpr1p = కొత్త AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = కొత్త AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            ఒకవేళ (!en1p->op1p() && en2p->op1p()) {
                std::మార్పిడి(en1p, en2p);
                std::మార్పిడి(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_కణుపు_సమాసం* స్థిరరాశి enp = కొత్త AstOr{
                nodep->fileline(),
                కొత్త AstAnd{nodep->fileline(), en1p, కొత్త AstOr{nodep->fileline(), en2p, subexpr1p}},
                కొత్త AstAnd{nodep->fileline(), en2p->clone_వృక్షం(అబద్ధం), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(శూన్యసూచిక);
            expr2p->user1p(శూన్యసూచిక);
        }
    }
    శూన్య సందర్శించు(AstAnd* nodep) override { సందర్శించు_And_Or(nodep, నిజం); }
    శూన్య సందర్శించు(AstOr* nodep) override { సందర్శించు_And_Or(nodep, అబద్ధం); }

    శూన్య సందర్శించు_కేటాయించు(Ast_కణుపు_కేటాయించు* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        ఒకవేళ (VN_IS(nodep->rhsp(), CReset)) తిరిగి;
        ఒకవేళ (Ast_కేటాయించు_W* స్థిరరాశి కేటాయించు_Wp = VN_CAST(nodep, కేటాయించు_W)) {
            m_currentDelayp = VN_CAST(కేటాయించు_Wp->timingControlp(), Delay);
        }
        ఒకవేళ (m_graphing) {
            ఒకవేళ (Ast_కేటాయించు_W* కేటాయించు_Wp = VN_CAST(nodep, కేటాయించు_W)) {
                ఒకవేళ (కేటాయించు_Wp->getLhsNetDelay()) తిరిగి;
                ఒకవేళ (కేటాయించు_Wp->timingControlp() && !m_currentDelayp) తిరిగి;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                ఒకవేళ (m_currentDelayp && m_currentDelayp->fallDelay()) తిరిగి;
                ఒకవేళ (!కేటాయించు_Wp->timingControlp()) కూడు_గమ్యం_Assignment_List(కేటాయించు_Wp);
            }

            ఒకవేళ (nodep->user2() & U2_GRAPHING) తిరిగి;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterate_And_తదుపరి_Null(nodep->rhsp());
            m_alhs = నిజం;
            iterate_And_తదుపరి_Null(nodep->lhsp());
            m_alhs = అబద్ధం;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } లేకపోతే {
            ఒకవేళ (nodep->user2() & U2_NONGRAPH) {
                తిరిగి;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterate_And_తదుపరి_Null(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            ఒకవేళ (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, కేటాయించు_W), nodep,
                            "Continuous assignment inside procedural process");
                Ast_కణుపు_సమాసం* స్థిరరాశి enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), కణుపు_సమాసం)->clone_వృక్షం_శుద్ధ(అబద్ధం)
                          : newAllZerosOrOnes(nodep->lhsp(), నిజం);
                స్థిరరాశి తార్కిక inserted = m_కేటాయించు_Enps.emplace(nodep, enp).రెండవ;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            ఒకవేళ (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(శూన్యసూచిక);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } లేకపోతే {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(nodep->lhsp(), Sel)) {
                    ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(selp->fromp(), కణుపు_Var_Ref)) {
                        ఒకవేళ (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), నిజం));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } లేకపోతే {
                            m_tgraph.didProcess(nodep, నిజం);
                        }
                    } లేకపోతే {
                        m_tgraph.didProcess(nodep, నిజం);  // LCOV_EXCL_LINE
                    }
                } లేకపోతే {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, నిజం);
                }
            }
            m_alhs = నిజం;  // And user1p() will indicate tristate equation, if any
            ఒకవేళ (Ast_కేటాయించు_W* స్థిరరాశి కేటాయించు_Wp = VN_CAST(nodep, కేటాయించు_W)) {
                ఒకవేళ (AstStrengthSpec* స్థిరరాశి specp = కేటాయించు_Wp->strengthSpecp()) {
                    ఒకవేళ (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        ఒకవేళ (!is_Only_Assignment_Is_గమ్యం_Lhs_Var(కేటాయించు_Wp)) {
                            కేటాయించు_Wp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } లేకపోతే {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterate_And_తదుపరి_Null(nodep->lhsp());
        }
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override { సందర్శించు_కేటాయించు(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override { సందర్శించు_కేటాయించు(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override { సందర్శించు_కేటాయించు(nodep); }

    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        స్థిరరాశి AstAlways* స్థిరరాశి alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_కేటాయించు ? శూన్యసూచిక : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    శూన్య సందర్శించు(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_లోపల_Alias);
        m_లోపల_Alias = నిజం;
        ఒకవేళ (m_graphing) {
            ఒకవేళ (nodep->user2() & U2_GRAPHING) తిరిగి;
            m_alhs = నిజం;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            కొరకు (Ast_కణుపు* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } లేకపోతే {
            iterateChildren(nodep);
        }
    }

    శూన్య సందర్శించు_Case_Eq(Ast_కణుపు_Biop* nodep, తార్కిక neq) {
        ఒకవేళ (m_graphing) {
            iterateChildren(nodep);
        } లేకపోతే {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* స్థిరరాశి constp = VN_CAST(nodep->lhsp(), Const);
            ఒకవేళ (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp();
                rhsp->unlink_Fr_వెనుక();
                File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                Ast_కణుపు_సమాసం* enRhsp;
                ఒకవేళ (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), కణుపు_సమాసం);
                    rhsp->user1p(శూన్యసూచిక);
                } లేకపోతే {
                    enRhsp = get_En_సమాసం_Based_On_Originalp(rhsp);
                }
                Ast_కణుపు_సమాసం* newp
                    = కొత్త AstLogAnd{fl, కొత్త AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    కొత్త AstEqCase{fl, కొత్త AstConst{fl, constp->అంకె()}, rhsp}};
                constp->user1p(శూన్యసూచిక);
                ఒకవేళ (neq) newp = కొత్త AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే ఒకవేళ (constp && nodep->rhsp()->user1p()) {
                File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                constp->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* newp = కొత్త AstLogAnd{fl,
                                                  కొత్త AstEq{fl, newAllZerosOrOnes(constp, అబద్ధం),
                                                            VN_AS(rhsp->user1p(), కణుపు_సమాసం)},
                                                  // Keep the caseeq if there are X's present
                                                  కొత్త AstEqCase{fl, constp, rhsp}};
                ఒకవేళ (neq) newp = కొత్త AstLogNot{fl, newp};
                rhsp->user1p(శూన్యసూచిక);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే {
                checkUnhandled(nodep);
            }
        }
    }
    శూన్య సందర్శించు_Eq_Neq_Wild(Ast_కణుపు_Biop* nodep) {
        ఒకవేళ (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterate_And_తదుపరి_Null(nodep->lhsp());
        ఒకవేళ (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            తిరిగి;
        }
    }
    శూన్య సందర్శించు(AstEqCase* nodep) override { సందర్శించు_Case_Eq(nodep, అబద్ధం); }
    శూన్య సందర్శించు(AstNeqCase* nodep) override { సందర్శించు_Case_Eq(nodep, నిజం); }
    శూన్య సందర్శించు(AstEqWild* nodep) override { సందర్శించు_Eq_Neq_Wild(nodep); }
    శూన్య సందర్శించు(AstNeqWild* nodep) override { సందర్శించు_Eq_Neq_Wild(nodep); }

    శూన్య సందర్శించు(Ast_లెక్క_బిట్లు* nodep) override {
        std::array<తార్కిక, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->అంకె().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->అంకె().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->అంకె().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        ఒకవేళ (m_graphing) {
            iterate_And_తదుపరి_Null(nodep->lhsp());
            ఒకవేళ (!dropop[0]) iterate_And_తదుపరి_Null(nodep->rhsp());
            ఒకవేళ (!dropop[1]) iterate_And_తదుపరి_Null(nodep->thsp());
            ఒకవేళ (!dropop[2]) iterate_And_తదుపరి_Null(nodep->fhsp());
        } లేకపోతే {
            Ast_కణుపు_సమాసం* nonXp = శూన్యసూచిక;
            ఒకవేళ (!dropop[0]) {
                nonXp = nodep->rhsp();
            } లేకపోతే ఒకవేళ (!dropop[1]) {
                nonXp = nodep->thsp();
            } లేకపోతే ఒకవేళ (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            ఒకవేళ (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                ఒకవేళ (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    తిరిగి;
                }
                AstVar* స్థిరరాశి envarp = getCreateEnVarp(varrefp->varp(), అబద్ధం);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlink_Fr_వెనుక(&relinkHandle);
                Ast_కణుపు_సమాసం* newp = కొత్త Ast_లెక్క_Ones{
                    nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                ఒకవేళ (nonXp) {  // Need to still count '0 or '1 or 'x's
                    ఒకవేళ (dropop[0]) {
                        nodep->rhsp()->unlink_Fr_వెనుక()->delete_వృక్షం();
                        nodep->rhsp(nonXp->clone_వృక్షం_శుద్ధ(నిజం));
                    }
                    ఒకవేళ (dropop[1]) {
                        nodep->thsp()->unlink_Fr_వెనుక()->delete_వృక్షం();
                        nodep->thsp(nonXp->clone_వృక్షం_శుద్ధ(నిజం));
                    }
                    ఒకవేళ (dropop[2]) {
                        nodep->fhsp()->unlink_Fr_వెనుక()->delete_వృక్షం();
                        nodep->fhsp(nonXp->clone_వృక్షం_శుద్ధ(నిజం));
                    }
                    newp = కొత్త Ast_కూడు{nodep->fileline(), nodep, newp};
                } లేకపోతే {
                    నెట్టు_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = శూన్యసూచిక;
        ఒకవేళ (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } లేకపోతే ఒకవేళ (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        ఒకవేళ (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } లేకపోతే {
            ఒకవేళ (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.సమితి_Tristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } లేకపోతే {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(nodep->lhsp(), Sel);
                స్థిరరాశి AstConst* స్థిరరాశి lsbp = selp ? VN_CAST(selp->lsbp(), Const) : శూన్యసూచిక;
                ఒకవేళ (lsbp) {
                    స్థిరరాశి పూర్ణాంకం lsb = lsbp->గమ్యం_SInt();
                    కొరకు (పూర్ణాంకం i = 0; i < selp->వెడల్పు_Const(); ++i) {
                        సమితి_బిట్టు_Pull_Direction(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } లేకపోతే {
                    సమితి_Pull_Direction(varrefp->varp(), nodep);
                }
            }
        }
        ఒకవేళ (!m_graphing) {
            nodep->unlink_Fr_వెనుక();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    శూన్య iteratePinGuts(AstPin* nodep) {
        ఒకవేళ (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            ఒకవేళ (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } లేకపోతే {
            // All heavy lifting completed in graph visitor.
            ఒకవేళ (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    శూన్య సందర్శించు(AstPin* nodep) override {
        ఒకవేళ (m_graphing) {
            ఒకవేళ (nodep->user2() & U2_GRAPHING) తిరిగి;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* స్థిరరాశి enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            ఒకవేళ (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, అబద్ధం);
                iteratePinGuts(nodep);
                తిరిగి;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            స్థిరరాశి తార్కిక లోపల_Decl_Processing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::ఆదానం
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            ఒకవేళ (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* స్థిరరాశి ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(కొత్త AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.సమితి_Tristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } లేకపోతే ఒకవేళ (లోపల_Decl_Processing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, నిజం, నిజం);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.సమితి_Tristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* స్థిరరాశి enVarp = కొత్త AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvt_గమ్యం_Str(m_unique++),
                                              VFlag_బిట్టు_Packed{}, enModVarp->వెడల్పు()};
            ఒకవేళ (లోపల_Decl_Processing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->var_Type2_లోపల();
            } లేకపోతే {
                enModVarp->var_Type2_బహిర్గతం();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* స్థిరరాశి enpinp
                = కొత్త AstPin{nodep->fileline(), nodep->pin_అంకె(),
                             enModVarp->name(),  // should be {var}"__en"
                             కొత్త AstVarRef{nodep->fileline(), enVarp,
                                           లోపల_Decl_Processing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->కూడు_తదుపరి_ఇక్కడ(enpinp);
            m_modp->కూడు_Stmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            స్థిరరాశి Ast_కేటాయించు_W* బహిర్గతం_Assignp = శూన్యసూచిక;  // If reconnected, the related assignment
            AstPin* outpinp = శూన్యసూచిక;
            AstVar* స్థిరరాశి బహిర్గతం_Mod_Varp = m_varAux(nodep->modVarp()).బహిర్గతం_Varp;
            ఒకవేళ (!బహిర్గతం_Mod_Varp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                ఒకవేళ (!m_modp->is_శిఖరం()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } లేకపోతే {
                Ast_కణుపు_సమాసం* స్థిరరాశి outexprp = VN_AS(nodep->exprp(), కణుపు_సమాసం)
                                                  ->clone_వృక్షం_శుద్ధ(అబద్ధం);  // Note has lvalue() set
                outpinp = కొత్త AstPin{nodep->fileline(), nodep->pin_అంకె(),
                                     బహిర్గతం_Mod_Varp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(బహిర్గతం_Mod_Varp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->కూడు_తదుపరి_ఇక్కడ(outpinp);
                // Simplify
                ఒకవేళ (లోపల_Decl_Processing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, నిజం};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                బహిర్గతం_Assignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        నిజం);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, బహిర్గతం_Assignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            స్థిరరాశి TristatePinVisitor visitor{nodep->exprp(), m_tgraph, అబద్ధం};
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి లోపల_Assignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, నిజం);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, లోపల_Assignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            ఒకవేళ (!బహిర్గతం_Assignp) {
                ఒకవేళ (!outpinp) {
                    exprrefp = శూన్యసూచిక;  // Primary input only
                } లేకపోతే {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    ఒకవేళ (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } లేకపోతే {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(బహిర్గతం_Assignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                ఒకవేళ (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            ఒకవేళ (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(కొత్త AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                ఒకవేళ (!బహిర్గతం_Assignp) {
                    map_చొప్పించు_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            ఒకవేళ (exprrefp) {
                AstPull* స్థిరరాశి pullp = m_varAux(nodep->modVarp()).pullp;
                స్థిరరాశి స్వయంచాలక& src_బిట్టు_Pulls = m_varAux(nodep->modVarp()).బిట్టు_Pulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = శూన్యసూచిక;
                పూర్ణాంకం selLsb = 0;
                పూర్ణాంకం sel_వెడల్పు = 0;
                ఒకవేళ (బహిర్గతం_Assignp) {
                    ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(బహిర్గతం_Assignp->lhsp(), Sel)) {
                        AstVarRef* స్థిరరాశి vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* స్థిరరాశి lsbp = VN_CAST(selp->lsbp(), Const);
                        ఒకవేళ (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->గమ్యం_SInt();
                            sel_వెడల్పు = selp->వెడల్పు_Const();
                        }
                    }
                }

                ఒకవేళ (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    సమితి_Pull_Direction(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    ఒకవేళ (selVarp) {
                        స్థిరరాశి పూర్ణాంకం direction = pullp->direction();
                        కొరకు (పూర్ణాంకం i = 0; i < sel_వెడల్పు; ++i) {
                            సమితి_బిట్టు_Pull_Direction(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                ఒకవేళ (!src_బిట్టు_Pulls.ఖాళీలు()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    కొరకు (స్థిరరాశి స్వయంచాలక& pair : src_బిట్టు_Pulls) {
                        సమితి_బిట్టు_Pull_Direction(exprrefp->varp(), pair.మొదటి, pair.రెండవ);
                    }  // LCOV_EXCL_LINE
                    ఒకవేళ (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        కొరకు (స్థిరరాశి స్వయంచాలక& pair : src_బిట్టు_Pulls) {
                            సమితి_బిట్టు_Pull_Direction(selVarp, selLsb + pair.మొదటి, pair.రెండవ);
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
        లేకపోతే {
            ఒకవేళ (nodep->user2() & U2_NONGRAPH) తిరిగి;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    శూన్య handle_కణుపు_Var_Ref(Ast_కణుపు_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        ఒకవేళ (m_graphing) {
            ఒకవేళ (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            ఒకవేళ (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            ఒకవేళ (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.సమితి_Tristate(nodep->varp());
            }
        } లేకపోతే {
            ఒకవేళ (nodep->user2() & U2_NONGRAPH) తిరిగి;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            ఒకవేళ (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                ఒకవేళ (m_లోపల_Alias) {
                    ఒకవేళ (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } లేకపోతే {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    తిరిగి;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_చొప్పించు_Lhs_Var_Ref(nodep);
            } లేకపోతే ఒకవేళ (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                AstVar* స్థిరరాశి enVarp = getCreateEnVarp(nodep->varp(), అబద్ధం);
                ఒకవేళ (AstVarXRef* స్థిరరాశి xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } లేకపోతే {
                    nodep->user1p(కొత్త AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override { handle_కణుపు_Var_Ref(nodep); }
    శూన్య సందర్శించు(AstVarXRef* nodep) override { handle_కణుపు_Var_Ref(nodep); }

    శూన్య సందర్శించు(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ఒకవేళ (m_graphing) {
            // If tri0/1 force a pullup
            ఒకవేళ (nodep->user2() & U2_GRAPHING) తిరిగి;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            ఒకవేళ (nodep->isPulldown() || nodep->isPullup()) {
                Ast_కణుపు* స్థిరరాశి newp = కొత్త AstPull{
                    nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->కూడు_తదుపరి_ఇక్కడ(newp);
                // We'll iterate on the new AstPull later
            }
            ఒకవేళ (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.సమితి_Tristate(nodep);
            }
        } లేకపోతే {  // !graphing
            ఒకవేళ (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        ఒకవేళ (m_processedIfaces && VN_IS(nodep, Iface)) తిరిగి;
        UINFO(8, dbgState() << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_graphing);
        VL_RESTORER(m_unique);
        VL_RESTORER_CLEAR(m_lhsmap);
        VL_RESTORER_CLEAR(m_assigns);
        VL_RESTORER_CLEAR(m_కేటాయించు_Enps);
        VL_RESTORER_CLEAR(m_proceduralEnables);
        // Not preserved, needs pointer instead: TristateGraph origTgraph = m_tgraph;
        UASSERT_OBJ(m_tgraph.ఖాళీలు(), nodep, "Unsupported: NodeModule under NodeModule");

        // Clear state
        m_graphing = అబద్ధం;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = శూన్యసూచిక;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = నిజం;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = అబద్ధం;
        UINFO(9, dbgState() << "processing mod " << nodep);

        // Merge the assignments for very Wired net LHS : wor, trior, wand and triand
        విలీనం_Wired_Nets_Assignments();
        // Remove all assignments not stronger than the strongest uniform constant
        remove_Assignments_Not_Stronger_Than_ఏకరీతి_Constant();
        // Use graph to find tristate signals
        m_tgraph.లేఖాచిత్రం_Walk(nodep);

        // Remove all assignments not stronger than the strongest non-tristate RHS
        removeAssignmentsNotStrongerThanNonTristate();

        // Build the LHS drivers map for this module
        iterateChildren(nodep);
        // Insert new logic for all tristates
        చొప్పించు_Tristates(nodep);
        చొప్పించు_Procedural_Enable_Assignments(nodep);

        కొరకు (స్వయంచాలక& కేటాయించు_Enp : m_కేటాయించు_Enps) {
            VL_DO_DANGLING(కేటాయించు_Enp.రెండవ->delete_వృక్షం(), కేటాయించు_Enp.రెండవ);
        }
        m_కేటాయించు_Enps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    శూన్య సందర్శించు(AstClass* nodep) override {
        // don't deal with classes
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        // don't deal with functions
    }

    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        // don't deal with casez compare '???? values
        iterate_And_తదుపరి_Null(nodep->stmtsp());
    }

    శూన్య సందర్శించు(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = అబద్ధం;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    శూన్య combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        కొరకు (స్థిరరాశి std::pair<AstVar* స్థిరరాశి, std::సదిశ<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* స్థిరరాశి invarp = kv.మొదటి;
            స్థిరరాశి std::సదిశ<std::pair<AstVar*, AstVar*>>& contribs = kv.రెండవ;
            Ast_కణుపు_Module* స్థిరరాశి ifaceModp = కనుగొను_మాతృ_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_పంక్తి* స్థిరరాశి fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.కొలత() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_కణుపు_సమాసం* orp = శూన్యసూచిక;
            Ast_కణుపు_సమాసం* enp = శూన్యసూచిక;

            కొరకు (స్థిరరాశి std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* స్థిరరాశి contribEnp = contrib.మొదటి;
                AstVar* స్థిరరాశి contribOutp = contrib.రెండవ;

                Ast_కణుపు_సమాసం* స్థిరరాశి బహిర్గతం_Refp = కొత్త AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_కణుపు_సమాసం* స్థిరరాశి enRefp = కొత్త AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_కణుపు_సమాసం* స్థిరరాశి andp = కొత్త AstAnd{fl, బహిర్గతం_Refp, enRefp};

                orp = (!orp) ? andp : కొత్త AstOr{fl, orp, andp};

                Ast_కణుపు_సమాసం* స్థిరరాశి enRef2p = కొత్త AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : కొత్త AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            ఒకవేళ (!envarp) {
                envarp = కొత్త AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->సమితి_Dfg_Tri_Lowered();
                ifaceModp->కూడు_Stmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            Ast_కేటాయించు_W* స్థిరరాశి enAssp
                = కొత్త Ast_కేటాయించు_W{fl, కొత్త AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->కూడు_Stmtsp(కొత్త AstAlways{enAssp});

            // Pull resolution
            స్థిరరాశి AstPull* స్థిరరాశి pullp = m_varAux(invarp).pullp;
            స్థిరరాశి తార్కిక pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_కణుపు_సమాసం* undrivenp = కొత్త AstNot{fl, కొత్త AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = కొత్త AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = కొత్త AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            Ast_కేటాయించు_W* స్థిరరాశి assp
                = కొత్త Ast_కేటాయించు_W{fl, కొత్త AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->కూడు_Stmtsp(కొత్త AstAlways{assp});
        }
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::సదిశ<Ast_కణుపు_Module*> ifacesp;
        కొరకు (Ast_కణుపు* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            ఒకవేళ (VN_IS(modp, Iface)) ifacesp.నెట్టు_వెనుక(VN_AS(modp, కణుపు_Module));
        }
        కొరకు (స్వయంచాలక it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = నిజం;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#నిర్వచితమైతే VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::సదిశ<Ast_కణుపు*> వాడని_Rootps;
        netlistp->foreach([&](Ast_కణుపు* nodep) {
            Ast_కణుపు* స్థిరరాశి enp = nodep->user1p();
            ఒకవేళ (!enp) తిరిగి;
            ఒకవేళ (enp->backp()) తిరిగి;
            వాడని_Rootps.emplace_వెనుక(enp);
        });
        కొరకు (Ast_కణుపు* స్థిరరాశి nodep : వాడని_Rootps) VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
#అయితే_ముగింపు
    }
    ~TristateVisitor() override {
        V3Stats::కూడు_Stat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

శూన్య V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("tristate", 0, dump_వృక్షం_Either_Level() >= 3);
}
