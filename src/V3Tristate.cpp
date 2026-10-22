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

ವರ್ಗ Tristate_ತಳ_Visitor VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ VNVisitor {
ಸಾರ್ವಜನಿಕ:
    // METHODS
};

//######################################################################
// Graph support classes

ವರ್ಗ Tristate_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Tristate_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;
    ತಾರ್ಕಿಕ m_isTristate = ಸುಳ್ಳು;  // Logic indicates a tristate
    ತಾರ್ಕಿಕ m_feedsTri = ಸುಳ್ಳು;  // Propagates to a tristate node (on RHS)
    ತಾರ್ಕಿಕ m_processed = ಸುಳ್ಳು;  // Tristating was cleaned up
ಸಾರ್ವಜನಿಕ:
    Tristate_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_nodep{nodep} {}
    ~Tristate_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_nodep; }
    ಸ್ಥಿರಾಂಕ AstVar* varp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_CAST(nodep(), Var); }
    string name() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvt_ಗಮ್ಯ_Hex(nodep())));
    }
    string dotColor() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->fileline(); }
    ತಾರ್ಕಿಕ isTristate() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_isTristate; }
    ಶೂನ್ಯ isTristate(ತಾರ್ಕಿಕ flag) { m_isTristate = flag; }
    ತಾರ್ಕಿಕ feedsTri() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_feedsTri; }
    ಶೂನ್ಯ feedsTri(ತಾರ್ಕಿಕ flag) { m_feedsTri = flag; }
    ತಾರ್ಕಿಕ processed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_processed; }
    ಶೂನ್ಯ processed(ತಾರ್ಕಿಕ flag) { m_processed = flag; }
};

//######################################################################

ವರ್ಗ Tristate_ನಕ್ಷೆ final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
ಸಾರ್ವಜನಿಕ:
    ಬಳಸಿ VarVec = std::ಸದಿಶ<AstVar*>;

ಖಾಸಗಿ:
    // MEMBERS
    V3_ನಕ್ಷೆ m_ನಕ್ಷೆ;  // Logic graph

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Tristate_ನಕ್ಷೆ() { clearAndCheck(); }
    ವಾಸ್ತವಿಕ ~Tristate_ನಕ್ಷೆ() { clearAndCheck(); }
    VL_UNCOPYABLE(Tristate_ನಕ್ಷೆ);

ಖಾಸಗಿ:
    // METHODS

    Tristate_ಶೃಂಗ* make_ಶೃಂಗ(Ast_ಗಂಟು* nodep) {
        Tristate_ಶೃಂಗ* vertexp = reinterpret_cast<Tristate_ಶೃಂಗ*>(nodep->user4p());
        ಆದರೆ (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = ಹೊಸ Tristate_ಶೃಂಗ{&m_ನಕ್ಷೆ, nodep};
            nodep->user4p(vertexp);
        }
        ಹಿಂತಿರುಗಿ vertexp;
    }

    // METHODS - Graph optimization
    ಶೂನ್ಯ ನಕ್ಷೆ_Walk_Recurse_Fwd(Tristate_ಶೃಂಗ* vtxp, ಪೂರ್ಣಾಂಕ level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        ಆದರೆ (!vtxp->isTristate()) ಹಿಂತಿರುಗಿ;  // tristate involved
        ಆದರೆ (vtxp->user() == 1) ಹಿಂತಿರುಗಿ;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        ಆದರೆ (!vtxp->varp()) {  // not a var where we stop the recursion
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                Tristate_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = static_cast<Tristate_ಶೃಂಗ*>(ಅಂಚು.ತುದಿ());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ಆದರೆ (!vvertexp->isTristate()) {
                    vvertexp->isTristate(ನಿಜ);
                    ನಕ್ಷೆ_Walk_Recurse_Fwd(vvertexp, level + 1);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
                Tristate_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = static_cast<Tristate_ಶೃಂಗ*>(ಅಂಚು.fromp());
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    ಆದರೆ (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(ನಿಜ);
                        ನಕ್ಷೆ_Walk_Recurse_Fwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ ನಕ್ಷೆ_Walk_Recurse_ಹಿಂಭಾಗ(Tristate_ಶೃಂಗ* vtxp, ಪೂರ್ಣಾಂಕ level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        ಆದರೆ (!(vtxp->isTristate() || vtxp->feedsTri())) ಹಿಂತಿರುಗಿ;  // tristate involved
        ಆದರೆ (vtxp->user() == 3) ಹಿಂತಿರುಗಿ;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        ಆದರೆ (!vtxp->varp()) {  // not a var where we stop the recursion
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
                Tristate_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = static_cast<Tristate_ಶೃಂಗ*>(ಅಂಚು.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ಆದರೆ (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(ನಿಜ);
                    ನಕ್ಷೆ_Walk_Recurse_ಹಿಂಭಾಗ(vvertexp, level + 1);
                }
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ತಾರ್ಕಿಕ ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ನಕ್ಷೆ.ಖಾಲಿಗಳು(); }
    ಶೂನ್ಯ clearAndCheck() {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಸ್ಥಿರಾಂಕ Tristate_ಶೃಂಗ& vvertex = static_cast<Tristate_ಶೃಂಗ&>(vtx);
            ಆದರೆ (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_ನಕ್ಷೆ.clear();
        Ast_ಗಂಟು::user4_Clear_ವೃಕ್ಷ();  // Wipe all node user4p's that point to vertexes
    }
    ಶೂನ್ಯ ನಕ್ಷೆ_Walk(Ast_ಗಂಟು_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ನಕ್ಷೆ_Walk_Recurse_Fwd(static_cast<Tristate_ಶೃಂಗ*>(&vtx), 0);
        }
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ನಕ್ಷೆ_Walk_Recurse_ಹಿಂಭಾಗ(static_cast<Tristate_ಶೃಂಗ*>(&vtx), 0);
        }
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_ನಕ್ಷೆ.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    ಶೂನ್ಯ associate(Ast_ಗಂಟು* fromp, Ast_ಗಂಟು* ತುದಿ) {
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, make_ಶೃಂಗ(fromp), make_ಶೃಂಗ(ತುದಿ), 1};
    }
    ಶೂನ್ಯ deleteVerticesFromSubtreeRecurse(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        // Skip vars, because they may be connected to more than one varref
        ಆದರೆ (!VN_IS(nodep, Var)) {
            Tristate_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = reinterpret_cast<Tristate_ಶೃಂಗ*>(nodep->user4p());
            ಆದರೆ (vertexp) vertexp->unlinkDelete(&m_ನಕ್ಷೆ);
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    ಶೂನ್ಯ ಗಣ_Tristate(Ast_ಗಂಟು* nodep) { make_ಶೃಂಗ(nodep)->isTristate(ನಿಜ); }
    ತಾರ್ಕಿಕ isTristate(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ Tristate_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = reinterpret_cast<Tristate_ಶೃಂಗ*>(nodep->user4p());
        ಹಿಂತಿರುಗಿ vertexp && vertexp->isTristate();
    }
    ತಾರ್ಕಿಕ feedsTri(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ Tristate_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = reinterpret_cast<Tristate_ಶೃಂಗ*>(nodep->user4p());
        ಹಿಂತಿರುಗಿ vertexp && vertexp->feedsTri();
    }
    ಶೂನ್ಯ didProcess(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ quiet = ಸುಳ್ಳು) {
        ಆದರೆ (Tristate_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = reinterpret_cast<Tristate_ಶೃಂಗ*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!quiet) {
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
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಸ್ಥಿರಾಂಕ Tristate_ಶೃಂಗ& vvertex = static_cast<ಸ್ಥಿರಾಂಕ Tristate_ಶೃಂಗ&>(vtx);
            ಆದರೆ (vvertex.isTristate()) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ nodep = VN_CAST(vvertex.nodep(), Var)) v.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
            }
        }
        ಹಿಂತಿರುಗಿ v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

ವರ್ಗ TristatePinVisitor final : ಸಾರ್ವಜನಿಕ Tristate_ತಳ_Visitor {
    Tristate_ನಕ್ಷೆ& m_tgraph;
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        ಆದರೆ (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.ಗಣ_Tristate(nodep->varp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುಂಡು_Sel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_ಗಂಟು* nodep, Tristate_ನಕ್ಷೆ& tgraph, ತಾರ್ಕಿಕ lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################

ವರ್ಗ TristateVisitor final : ಸಾರ್ವಜನಿಕ Tristate_ತಳ_Visitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_inuser4;

    ರಚನೆ AuxAstVar final {
        AstPull* pullp = ಶೂನ್ಯಸೂಚಕ;  // pullup/pulldown direction (whole variable)
        AstVar* ಹೊರಹೋಗು_Varp = ಶೂನ್ಯಸೂಚಕ;  // output __out var
        ತಾರ್ಕಿಕ ifaceTristate = ಸುಳ್ಳು;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>
            ಬಿಟ್_Pulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    ರಚನೆ RefStrength final {
        Ast_ಗಂಟು_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* m_assignp;  // Assignment containing m_varrefp
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_ಗಂಟು_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_ಗಂಟು_Var_Ref* varrefp, Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assignp,
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Procedure* procedurep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enLhsp,
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
    ರಚನೆ LhsProjection final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_lhsp;  // LHS giving the enable assign
        Ast_ಗಂಟು_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    ರಚನೆ ProceduralEnable final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    ಬಳಸಿ RefStrengthVec = std::ಸದಿಶ<RefStrength>;
    ಬಳಸಿ VarMap = std::map<AstVar*, RefStrengthVec*>;
    ಬಳಸಿ Assigns = std::ಸದಿಶ<Ast_ನಿಯೋಜಿಸು_W*>;
    ಬಳಸಿ Var_ಗಮ್ಯ_Assigns_Map = std::map<AstVar*, Assigns>;
    ಬಳಸಿ ನಿಯೋಜಿಸು_Enable_Map = std::map<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>;
    ಬಳಸಿ ProceduralEnableMap = std::map<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*, std::ಸದಿಶ<ProceduralEnable>>;
    ಎಣಿಕೆ : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    ತಾರ್ಕಿಕ m_graphing = ಸುಳ್ಳು;  // Major mode - creating graph
    //
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    AstCell* m_cellp = ಶೂನ್ಯಸೂಚಕ;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    Var_ಗಮ್ಯ_Assigns_Map m_assigns;  // Assigns in current module
    ನಿಯೋಜಿಸು_Enable_Map m_ನಿಯೋಜಿಸು_Enps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    ಪೂರ್ಣಾಂಕ m_unique = 0;
    ತಾರ್ಕಿಕ m_alhs = ಸುಳ್ಳು;  // On LHS of assignment
    ತಾರ್ಕಿಕ m_ಒಳ_Alias = ಸುಳ್ಳು;  // Inside alias statement
    ತಾರ್ಕಿಕ m_processedIfaces = ಸುಳ್ಳು;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = ಶೂನ್ಯಸೂಚಕ;  // Delay of assignment currently collecting LHS drivers
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_logicp = ಶೂನ್ಯಸೂಚಕ;  // Current logic being built
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* m_assignp = ಶೂನ್ಯಸೂಚಕ;  // Current assignment
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Procedure* m_procedurep = ಶೂನ್ಯಸೂಚಕ;  // Current procedural process
    Tristate_ನಕ್ಷೆ m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::ಸದಿಶ<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ಸ್ಥಿರಾಂಕ {
        string o = (m_graphing ? " gr " : " ng ");
        ಆದರೆ (m_alhs) o += "alhs ";
        ಹಿಂತಿರುಗಿ o;
    }
    ಶೂನ್ಯ mod_ಕೂಡಿಸು_Stmtp(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        ಆದರೆ (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_modp->ಕೂಡಿಸು_Stmtsp(newp);
        }
    }
    ಶೂನ್ಯ associateLogic(Ast_ಗಂಟು* fromp, Ast_ಗಂಟು* ತುದಿ) {
        ಆದರೆ (m_logicp) m_tgraph.associate(fromp, ತುದಿ);
    }
    AstConst* newAllZerosOrOnes(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ ones) {
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, nodep->ಅಗಲ()};
        ಆದರೆ (ones) ಅಂಕೆ.ಗಣ_All_Bits1();
        AstConst* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), ಅಂಕೆ};
        ಹಿಂತಿರುಗಿ newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_ಸಾಲು* fl, AstVar* varp, ಸ್ಥಿರಾಂಕ string& dotted, VAccess access,
                           ಸ್ಥಿರಾಂಕ string& inlinedDots) {
        AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = ಹೊಸ AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        ಹಿಂತಿರುಗಿ xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* ಕಂಡುಹಿಡಿ_Modport_For_Dotted(Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ string& dotted_ಮಾರ್ಗ) {
        ಆದರೆ (dotted_ಮಾರ್ಗ.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ string ಮೊದಲ_Comp = dotted_ಮಾರ್ಗ.substr(0, dotted_ಮಾರ್ಗ.ಕಂಡುಹಿಡಿ('.'));
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ ivarp = VN_CAST(stmtp, Var);
            ಆದರೆ (!ivarp || ivarp->name() != ಮೊದಲ_Comp) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            ಆದರೆ (ifaceDtp && ifaceDtp->modportp()) ಹಿಂತಿರುಗಿ ifaceDtp->modportp();
            ನಿಲ್ಲಿಸು;
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getEnp(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (nodep->user1p()) {
            ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
                ಆದರೆ (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    ಹಿಂತಿರುಗಿ newAllZerosOrOnes(nodep, ನಿಜ);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, ನಿಜ));
        }
        // Otherwise return the previous output enable
        ಹಿಂತಿರುಗಿ VN_AS(nodep->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    ಶೂನ್ಯ ಕೂಡಿಸು_Modport_Var_Ref_If_Missing(AstModport* modportp, AstVar* varp, VDirection dir) {
        ಗಾಗಿ (Ast_ಗಂಟು* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ಸ್ಥಿರಾಂಕ mvrp = VN_CAST(mrp, ModportVarRef);
            ಆದರೆ (mvrp && mvrp->varp() == varp) ಹಿಂತಿರುಗಿ;  // Already present
        }
        AstModportVarRef* ಸ್ಥಿರಾಂಕ mvrp = ಹೊಸ AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->ಕೂಡಿಸು_Varsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, ತಾರ್ಕಿಕ is_ತುದಿ) {
        // Return the master __en for the specified input variable
        ಆದರೆ (!invarp->user1p()) {
            AstVar* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstVar{invarp->fileline(), is_ತುದಿ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp = ಕಂಡುಹಿಡಿ_ಪೋಷಕ_Module(invarp);
            ಆದರೆ (ownerModp && ownerModp != m_modp) {
                ownerModp->ಕೂಡಿಸು_Stmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ಸ್ಥಿರಾಂಕ modportp = VN_CAST(stmtp, Modport);
                    ಆದರೆ (!modportp) ಮುಂದುವರಿ;
                    ಗಾಗಿ (Ast_ಗಂಟು* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ಸ್ಥಿರಾಂಕ mvrp = VN_CAST(mrp, ModportVarRef);
                        ಆದರೆ (mvrp && mvrp->varp() == invarp) {
                            ಕೂಡಿಸು_Modport_Var_Ref_If_Missing(modportp, newp, VDirection::ಒಳಹರಿವು);
                            ನಿಲ್ಲಿಸು;
                        }
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                mod_ಕೂಡಿಸು_Stmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        ಹಿಂತಿರುಗಿ VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ಸ್ಥಿರಾಂಕ constp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = constp->fileline();
        V3_ಸಂಖ್ಯೆ numz{constp, constp->ಅಗಲ()};
        numz.op_ಬಿಟ್ಟುಗಳು_Z(constp->ಅಂಕೆ());  // Z->1, else 0
        V3_ಸಂಖ್ಯೆ numz0{constp, constp->ಅಗಲ()};
        numz0.opNot(numz);  // Z->0, else 1
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, numz0};
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* get_En_ಅಭಿವ್ಯಕ್ತಿ_Based_On_Originalp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep, VarRef)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), ಸುಳ್ಳು),
                                 VAccess::READ};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ಸ್ಥಿರಾಂಕ enVarp = getCreateEnVarp(xrefp->varp(), ಸುಳ್ಳು);
            ಹಿಂತಿರುಗಿ newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const)) {
            ಹಿಂತಿರುಗಿ getNonZConstp(constp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstExtend* ಸ್ಥಿರಾಂಕ extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = extendp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstExtend* ಸ್ಥಿರಾಂಕ enExtendp = extendp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            extendp->lhsp(lhsp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enLhsp = get_En_ಅಭಿವ್ಯಕ್ತಿ_Based_On_Originalp(lhsp);
            enExtendp->lhsp(ಹೊಸ AstNot{enLhsp->fileline(), enLhsp});
            ಹಿಂತಿರುಗಿ ಹೊಸ AstNot{enExtendp->fileline(), enExtendp};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = selp->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstSel* ಸ್ಥಿರಾಂಕ enSelp = selp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            selp->fromp(fromp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enFromp = get_En_ಅಭಿವ್ಯಕ್ತಿ_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            ಹಿಂತಿರುಗಿ enSelp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
    }
    AstVar* get_Create_ಹೊರಹೋಗು_Varp(AstVar* invarp, ತಾರ್ಕಿಕ is_ತುದಿ) {
        // Return the master __out for the specified input variable
        ಆದರೆ (!m_varAux(invarp).ಹೊರಹೋಗು_Varp) {
            AstVar* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstVar{invarp->fileline(), is_ತುದಿ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            mod_ಕೂಡಿಸು_Stmtp(invarp, newp);
            m_varAux(invarp).ಹೊರಹೋಗು_Varp = newp;  // find outvar given invarp
        }
        ಹಿಂತಿರುಗಿ m_varAux(invarp).ಹೊರಹೋಗು_Varp;
    }
    AstVar* getCreateUnconnVarp(Ast_ಗಂಟು* fromp, Ast_ಗಂಟು_DType* dtypep) {
        AstVar* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvt_ಗಮ್ಯ_Str(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        mod_ಕೂಡಿಸು_Stmtp(newp, newp);
        ಹಿಂತಿರುಗಿ newp;
    }

    LhsProjection newLhsProjection(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_Var_Ref* targetp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newLhsp = lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_ಗಂಟು* nodep) { nodep->user1p(ಶೂನ್ಯಸೂಚಕ); });
        Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        ಹಿಂತಿರುಗಿ LhsProjection{newLhsp, newVarrefp, enp};
    }

    ಶೂನ್ಯ map_ಸೇರಿಸು_Lhs_Var_Ref(Ast_ಗಂಟು_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ಸ್ಥಿರಾಂಕ ಕೀಲಿ = nodep->varp();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_lhsmap.emplace(ಕೀಲಿ, ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ = ಹೊಸ RefStrengthVec;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enLhsp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_Var_Ref* enVarrefp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enRhsp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ enIt = m_ನಿಯೋಜಿಸು_Enps.ಕಂಡುಹಿಡಿ(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ canCombineProcedural = !VN_IS(ಕೀಲಿ->dtypep()->skipRefp(), ಗಂಟು_UOr_Struct_DType);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ procedurep
            = enIt != m_ನಿಯೋಜಿಸು_Enps.ಕೊನೆ() && canCombineProcedural ? m_procedurep : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->ಎರಡನೇ->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.ಮೊದಲ->ಎರಡನೇ->ತಳ್ಳು_ಹಿಂಭಾಗ(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newEnableDeposit(AstSel* selp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ಸರಿಸು_L{
            selp->fileline(), ಹೊಸ AstExtend{selp->fileline(), enp, selp->fromp()->ಅಗಲ()},
            selp->lsbp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), selp->fromp()->ಅಗಲ()};
        ಹಿಂತಿರುಗಿ newp;
    }

    ಶೂನ್ಯ ಗಣ_Pull_Direction(AstVar* varp, AstPull* pullp) {
        ಸ್ಥಿರಾಂಕ AstPull* ಸ್ಥಿರಾಂಕ oldpullp = m_varAux(varp).pullp;
        ಆದರೆ (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    ಶೂನ್ಯ ಗಣ_ಬಿಟ್_Pull_Direction(AstVar* varp, ಪೂರ್ಣಾಂಕ ಬಿಟ್_ಸೂಚ್ಯಂಕ, ಪೂರ್ಣಾಂಕ direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        ಸ್ವಯಂಚಾಲಿತ& ಬಿಟ್_Pulls = m_varAux(varp).ಬಿಟ್_Pulls;
        ಸ್ವಯಂಚಾಲಿತ it = ಬಿಟ್_Pulls.ಕಂಡುಹಿಡಿ(ಬಿಟ್_ಸೂಚ್ಯಂಕ);
        ಆದರೆ (it != ಬಿಟ್_Pulls.ಕೊನೆ() && it->ಎರಡನೇ != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << ಬಿಟ್_ಸೂಚ್ಯಂಕ << " of " << varp->prettyNameQ());
        }
        ಬಿಟ್_Pulls[ಬಿಟ್_ಸೂಚ್ಯಂಕ] = direction;
    }

    AstConst* create_ಪ್ರತಿ_ಬಿಟ್_Pull_Const(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಬಿಟ್_Pulls = m_varAux(varp).ಬಿಟ್_Pulls;
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{varp, varp->ಅಗಲ(), 0};  // Start with all zeros
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : ಬಿಟ್_Pulls) {
            ಆದರೆ (pair.ಎರಡನೇ == 1) ಅಂಕೆ.ಗಣ_ಬಿಟ್(pair.ಮೊದಲ, 1);
        }  // LCOV_EXCL_LINE
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{varp->fileline(), ಅಂಕೆ};
    }

    ತಾರ್ಕಿಕ has_ಪ್ರತಿ_ಬಿಟ್_Pulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        ಹಿಂತಿರುಗಿ !m_varAux(varp).ಬಿಟ್_Pulls.ಖಾಲಿಗಳು();
    }

    ಶೂನ್ಯ checkUnhandled(Ast_ಗಂಟು* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        ಆದರೆ (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        ಆದರೆ ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
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
        ಸ್ಥಿರಾಂಕ Tristate_ನಕ್ಷೆ::VarVec vars = m_tgraph.tristateVars();
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ varp : vars) {
            ಆದರೆ (!m_tgraph.isTristate(varp)) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_lhsmap.ಕಂಡುಹಿಡಿ(varp);
            ಆದರೆ (it != m_lhsmap.ಕೊನೆ()) ಮುಂದುವರಿ;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ಸ್ಥಿರಾಂಕ AstPull* ಸ್ಥಿರಾಂಕ pullp = m_varAux(varp).pullp;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ pull_ಮೌಲ್ಯ = pullp && pullp->direction() == 1;
            AstConst* ಸ್ಥಿರಾಂಕ constp = newAllZerosOrOnes(varp, pull_ಮೌಲ್ಯ);
            AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = ಹೊಸ AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, ಸುಳ್ಳು));  // en=0 for floating/pullup vars
            nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{newp});
            map_ಸೇರಿಸು_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        ಗಾಗಿ (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ಸ್ಥಿರಾಂಕ std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.ಕಂಡುಹಿಡಿ(varp);
            ಆದರೆ (it == m_lhsmap.ಕೊನೆ()) ಮುಂದುವರಿ;
            AstVar* ಸ್ಥಿರಾಂಕ invarp = it->ಮೊದಲ;
            RefStrengthVec* refsp = it->ಎರಡನೇ;
            // Figure out if this var needs tristate expanded.
            ಆದರೆ (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp = ಕಂಡುಹಿಡಿ_ಪೋಷಕ_Module(invarp);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isCrossModule = ownerModp && ownerModp != nodep;
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                ಆದರೆ (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ RefStrength& rs : *refsp) {
                        ಆದರೆ (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            ನಿಲ್ಲಿಸು;
                        }
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = ನಿಜ;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    ರಚನೆ ವಿಭಜನೆ_Info final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, ವಿಭಜನೆ_Info> partitions;
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ RefStrength& rs : *refsp) {
                        ಆದರೆ (AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            ವಿಭಜನೆ_Info& pi = partitions[xrefp->dotted()];
                            pi.refs.ತಳ್ಳು_ಹಿಂಭಾಗ(rs);
                            ಆದರೆ (pi.inlinedDots.ಖಾಲಿಗಳು()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            partitions[""].refs.ತಳ್ಳು_ಹಿಂಭಾಗ(rs);
                        }
                    }
                    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& kv : partitions) {
                        ಸೇರಿಸು_Tristates_Signal(nodep, invarp, &kv.ಎರಡನೇ.refs, ನಿಜ, kv.ಮೊದಲ,
                                              kv.ಎರಡನೇ.inlinedDots,
                                              ಕಂಡುಹಿಡಿ_Modport_For_Dotted(nodep, kv.ಮೊದಲ));
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = ನಿಜ;
                    ಸೇರಿಸು_Tristates_Signal(nodep, invarp, refsp, ನಿಜ, "", "", ಶೂನ್ಯಸೂಚಕ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಸೇರಿಸು_Tristates_Signal(nodep, invarp, refsp, ಸುಳ್ಳು, "", "", ಶೂನ್ಯಸೂಚಕ);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(ಅಳಿಸು refsp, refsp);
        }
    }

    ಶೂನ್ಯ aggregateTriSameStrength(Ast_ಗಂಟು_Module* nodep, AstVar* ಸ್ಥಿರಾಂಕ varp, AstVar* ಸ್ಥಿರಾಂಕ envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator ಕೊನೆ_Strength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        ರಚನೆ Driver_ಗುಂಪು final {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Procedure* m_procedurep;  // Process owning these assignments
            std::ಸದಿಶ<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::ಸದಿಶ<Driver_ಗುಂಪು> drivers;
        std::map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Procedure*, ಗಾತ್ರ_t> procedure_ಗಮ್ಯ_Driver;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = beginStrength; it != ಕೊನೆ_Strength; ++it) {
            ಆದರೆ (it->m_procedurep) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ emplaced = procedure_ಗಮ್ಯ_Driver.emplace(it->m_procedurep, drivers.ಗಾತ್ರ_2());
                ಆದರೆ (!emplaced.ಎರಡನೇ) {
                    drivers[emplaced.ಮೊದಲ->ಎರಡನೇ].m_refsp.ತಳ್ಳು_ಹಿಂಭಾಗ(&*it);
                    ಮುಂದುವರಿ;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ತಳ್ಳು_ಹಿಂಭಾಗ(Driver_ಗುಂಪು{it->m_procedurep, {&*it}});
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* orp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp = ಶೂನ್ಯಸೂಚಕ;

        ಗಾಗಿ (Driver_ಗುಂಪು& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ಸ್ಥಿರಾಂಕ newLhsp = ಹೊಸ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvt_ಗಮ್ಯ_Str(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->ಗಣ_Dfg_Tri_Lowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->ಕೂಡಿಸು_Stmtsp(newLhsp);
            AstVar* ಸ್ಥಿರಾಂಕ newEnLhsp
                = ಹೊಸ AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvt_ಗಮ್ಯ_Str(m_unique++), envarp};  // 2-state ok
            newEnLhsp->ಗಣ_Dfg_Tri_Lowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->ಕೂಡಿಸು_Stmtsp(newEnLhsp);

            ಗಾಗಿ (RefStrength* ಸ್ಥಿರಾಂಕ refStrengthp : driver.m_refsp) {
                Ast_ಗಂಟು_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                ಆದರೆ (VN_IS(refp, VarXRef)) {
                    AstVarRef* ಸ್ಥಿರಾಂಕ localRefp
                        = ಹೊಸ AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(ಶೂನ್ಯಸೂಚಕ);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    refp->varp(newLhsp);
                }

                ಆದರೆ (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enLhsp = refStrengthp->m_enLhsp;
                    Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ enVarrefp = refStrengthp->m_enVarrefp;
                    ಆದರೆ (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ಸ್ಥಿರಾಂಕ localRefp
                            = ಹೊಸ AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        ಆದರೆ (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_ವೃಕ್ಷ(), enVarrefp);
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(enVarrefp), enVarrefp);
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ತಳ್ಳು_ಹಿಂಭಾಗ(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = ಶೂನ್ಯಸೂಚಕ;
                    refStrengthp->m_enVarrefp = ಶೂನ್ಯಸೂಚಕ;
                    refStrengthp->m_enRhsp = ಶೂನ್ಯಸೂಚಕ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ enLhspAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{
                        refp->fileline(),
                        ಹೊಸ AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    ಆದರೆ (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ref1p = ಹೊಸ AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ref2p = ಹೊಸ AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ andp = ಹೊಸ AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : ಹೊಸ AstOr{fl, orp, andp};

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ref3p = ಹೊಸ AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : ಹೊಸ AstOr{ref3p->fileline(), enp, ref3p};
        }
        Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{
            varp->fileline(), ಹೊಸ AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{assp});

        Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ enAssp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{
            envarp->fileline(), ಹೊಸ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{enAssp});
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Procedural_Enable_Assignments(Ast_ಗಂಟು_Module* nodep) {
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& ನಿಯೋಜಿಸು_Enables : m_proceduralEnables) {
            Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ನಿಯೋಜಿಸು_Enables.ಮೊದಲ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* combinedLhsp = assignp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* combinedRhsp = assignp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಗಾಗಿ (ProceduralEnable& enable : ನಿಯೋಜಿಸು_Enables.ಎರಡನೇ) {
                combinedLhsp = ಹೊಸ AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = ಹೊಸ AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            ಆದರೆ (!VN_IS(assignp, ನಿಯೋಜಿಸು_Dly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ಸ್ಥಿರಾಂಕ sampledp
                    = ಹೊಸ AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvt_ಗಮ್ಯ_Str(m_unique++), combinedRhsp->dtypep()};
                nodep->ಕೂಡಿಸು_Stmtsp(sampledp);
                assignp->lhsp(ಹೊಸ AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ applyp
                    = ಹೊಸ Ast_ನಿಯೋಜಿಸು{assignp->fileline(), combinedLhsp,
                                    ಹೊಸ AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(applyp);
            } ಇಲ್ಲದಿದ್ದರೆ {
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
    ಶೂನ್ಯ ಸೇರಿಸು_Tristates_Signal(Ast_ಗಂಟು_Module* nodep, AstVar* ಸ್ಥಿರಾಂಕ invarp, RefStrengthVec* refsp,
                               ತಾರ್ಕಿಕ isIfaceTri, ಸ್ಥಿರಾಂಕ string& iface_Dotted_ಮಾರ್ಗ,
                               ಸ್ಥಿರಾಂಕ string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        AstVar* envarp = ಶೂನ್ಯಸೂಚಕ;
        AstVar* outvarp = ಶೂನ್ಯಸೂಚಕ;  // __out
        AstVar* lhsp = invarp;  // Variable to assign drive-value to (<in> or __out)
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_ತುದಿ_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_ತುದಿ();
        ಆದರೆ (!isIfaceTri) {
            ಆದರೆ (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().ಖಾಲಿಗಳು())
                 && is_ತುದಿ_Inout)
                || (!nodep->is_ತುದಿ() && invarp->isIO())) {
                // This var becomes an input
                invarp->var_Type2_ಒಳ();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = get_Create_ಹೊರಹೋಗು_Varp(invarp, is_ತುದಿ_Inout);
                outvarp->var_Type2_ಹೊರಹೋಗು();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_ತುದಿ_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).ಬಿಟ್_Pulls
                    = m_varAux(invarp).ಬಿಟ್_Pulls;  // Per-bit pull propagation
                ಆದರೆ (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                ಆದರೆ (!m_varAux(invarp).ಬಿಟ್_Pulls.ಖಾಲಿಗಳು()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* orp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp = ಶೂನ್ಯಸೂಚಕ;

        std::ವಿಂಗಡಿಸು(refsp->begin(), refsp->ಕೊನೆ(),
                  [](RefStrength a, RefStrength b) { ಹಿಂತಿರುಗಿ a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        ಆಗ (beginStrength != refsp->ಕೊನೆ()) {
            RefStrengthVec::iterator ಕೊನೆ_Strength = beginStrength + 1;
            ಆಗ (ಕೊನೆ_Strength != refsp->ಕೊನೆ()
                   && ಕೊನೆ_Strength->m_strength == beginStrength->m_strength)
                ಕೊನೆ_Strength++;

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strength_ಪೂರ್ವಪ್ರತ್ಯಯ;
            ಆದರೆ (isIfaceTri && !iface_Dotted_ಮಾರ್ಗ.ಖಾಲಿಗಳು()) {
                strength_ಪೂರ್ವಪ್ರತ್ಯಯ = iface_Dotted_ಮಾರ್ಗ;
                std::replace(strength_ಪೂರ್ವಪ್ರತ್ಯಯ.begin(), strength_ಪೂರ್ವಪ್ರತ್ಯಯ.ಕೊನೆ(), '.', '_');
                strength_ಪೂರ್ವಪ್ರತ್ಯಯ += "__";
            }
            ಸ್ಥಿರಾಂಕ string strengthVarName
                = strength_ಪೂರ್ವಪ್ರತ್ಯಯ + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ಸ್ಥಿರಾಂಕ varStrengthp = ಹೊಸ AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->ಗಣ_Dfg_Tri_Lowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->ಕೂಡಿಸು_Stmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ಸ್ಥಿರಾಂಕ enVarStrengthp = ಹೊಸ AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->ಗಣ_Dfg_Tri_Lowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->ಕೂಡಿಸು_Stmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     ಕೊನೆ_Strength);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp;
            ಆದರೆ (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp
                    = ಹೊಸ AstAnd{fl, ಹೊಸ AstVarRef{fl, varStrengthp, VAccess::READ},
                                 ಹೊಸ AstNot{fl, enp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}};
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp = ಹೊಸ AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp : ಹೊಸ AstOr{fl, orp, ಅಭಿವ್ಯಕ್ತಿ_Current_Strengthp};

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enVarStrengthRefp
                = ಹೊಸ AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : ಹೊಸ AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = ಕೊನೆ_Strength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        ಆದರೆ (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->ಗಣ_Dfg_Allow_Multidrive_Tri();
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ifaceModp = ಕಂಡುಹಿಡಿ_ಪೋಷಕ_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ contribIdx = static_cast<ಪೂರ್ಣಾಂಕ>(m_ifaceContribs[invarp].ಗಾತ್ರ_2());
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ಸ್ಥಿರಾಂಕ contribOutp = ಹೊಸ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvt_ಗಮ್ಯ_Str(contribIdx), invarp};
            contribOutp->ಗಣ_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->ಕೂಡಿಸು_Stmtsp(contribOutp);

            AstVar* ಸ್ಥಿರಾಂಕ contribEnp = ಹೊಸ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvt_ಗಮ್ಯ_Str(contribIdx), invarp};
            contribEnp->ಗಣ_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->ಕೂಡಿಸು_Stmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            ಆದರೆ (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                ಕೂಡಿಸು_Modport_Var_Ref_If_Missing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                ಕೂಡಿಸು_Modport_Var_Ref_If_Missing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ lhsp
                    = iface_Dotted_ಮಾರ್ಗ.ಖಾಲಿಗಳು() ? static_cast<Ast_ಗಂಟು_Var_Ref*>(
                                                    ಹೊಸ AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_ಗಂಟು_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, iface_Dotted_ಮಾರ್ಗ,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{assp});
            }
            {
                Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ lhsp
                    = iface_Dotted_ಮಾರ್ಗ.ಖಾಲಿಗಳು() ? static_cast<Ast_ಗಂಟು_Var_Ref*>(
                                                    ಹೊಸ AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_ಗಂಟು_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, iface_Dotted_ಮಾರ್ಗ,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{assp});
            }

            m_ifaceContribs[invarp].ತಳ್ಳು_ಹಿಂಭಾಗ({contribEnp, contribOutp});
            ಹಿಂತಿರುಗಿ;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasPull = m_varAux(lhsp).pullp || has_ಪ್ರತಿ_ಬಿಟ್_Pulls(lhsp);
        ಆದರೆ (!outvarp || hasPull) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* undrivenp;
            ಆದರೆ (envarp) {
                undrivenp = ಹೊಸ AstNot{envarp->fileline(),
                                       ಹೊಸ AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (enp) {
                undrivenp = ಹೊಸ AstNot{enp->fileline(), enp};
                enp = ಶೂನ್ಯಸೂಚಕ;  // moved into undrivenp
            } ಇಲ್ಲದಿದ್ದರೆ {
                undrivenp = newAllZerosOrOnes(invarp, ನಿಜ);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            ಆದರೆ (has_ಪ್ರತಿ_ಬಿಟ್_Pulls(lhsp)) {
                pullConstp = create_ಪ್ರತಿ_ಬಿಟ್_Pull_Const(lhsp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ AstPull* ಸ್ಥಿರಾಂಕ pullp = m_varAux(lhsp).pullp;
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = ಹೊಸ AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? ಹೊಸ AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        ಆದರೆ (envarp && !enp) enp = newAllZerosOrOnes(invarp, ಸುಳ್ಳು);

        ಆದರೆ (envarp) {
            Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ enAssp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{
                enp->fileline(), ಹೊಸ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{
            lhsp->fileline(), ಹೊಸ AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        ಆದರೆ ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().ಖಾಲಿಗಳು()) && is_ತುದಿ_Inout) {
            ಆದರೆ (envarp) {
                envarp->primaryIO(ನಿಜ);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            ಆದರೆ (outvarp) {
                outvarp->primaryIO(ನಿಜ);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(ನಿಜ);
            }
        }
    }

    ತಾರ್ಕಿಕ is_Only_Assignment_Is_ಗಮ್ಯ_Lhs_Var(ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            ಸ್ವಯಂಚಾಲಿತ ಸಿಕ್ಕಿತು_It = m_assigns.ಕಂಡುಹಿಡಿ(varRefp->varp());
            ಆದರೆ (ಸಿಕ್ಕಿತು_It != m_assigns.ಕೊನೆ()) {
                ಸ್ವಯಂಚಾಲಿತ ಸ್ಥಿರಾಂಕ& assigns_ಗಮ್ಯ_Var = ಸಿಕ್ಕಿತು_It->ಎರಡನೇ;
                ಆದರೆ (assigns_ಗಮ್ಯ_Var.ಗಾತ್ರ_2() == 1 && assigns_ಗಮ್ಯ_Var[0] == nodep) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಗಮ್ಯ_Assignment_List(Ast_ನಿಯೋಜಿಸು_W* nodep) {
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            ಆದರೆ (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ nodep, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
        ಆದರೆ (AstStrengthSpec* ಸ್ಥಿರಾಂಕ strengthSpec = nodep->strengthSpecp()) {
            ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        ಹಿಂತಿರುಗಿ VStrength::STRONG;  // default strength is strong
    }

    ತಾರ್ಕಿಕ assignment_ದ_ಮೌಲ್ಯ_On_All_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ nodep, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->rhsp(), Const)) {
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ = constp->ಅಂಕೆ();
            ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ ? ಅಂಕೆ.isEqAllOnes() : ಅಂಕೆ.isEqZero();
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    Ast_ನಿಯೋಜಿಸು_W* get_Strongest_Assignment_ದ_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ Assigns& assigns, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
        ಸ್ವಯಂಚಾಲಿತ maxIt = std::max_element(
            assigns.begin(), assigns.ಕೊನೆ(), [&](ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ap, ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* bp) {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮೌಲ್ಯಗಳು_On_Rhs_A = assignment_ದ_ಮೌಲ್ಯ_On_All_ಬಿಟ್ಟುಗಳು(ap, ಮೌಲ್ಯ);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮೌಲ್ಯಗಳು_On_Rhs_B = assignment_ದ_ಮೌಲ್ಯ_On_All_ಬಿಟ್ಟುಗಳು(bp, ಮೌಲ್ಯ);
                ಆದರೆ (!ಮೌಲ್ಯಗಳು_On_Rhs_A) ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು_On_Rhs_B;
                ಆದರೆ (!ಮೌಲ್ಯಗಳು_On_Rhs_B) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                ಹಿಂತಿರುಗಿ getStrength(ap, ಮೌಲ್ಯ) < getStrength(bp, ಮೌಲ್ಯ);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        ಹಿಂತಿರುಗಿ assignment_ದ_ಮೌಲ್ಯ_On_All_ಬಿಟ್ಟುಗಳು(*maxIt, ಮೌಲ್ಯ) ? *maxIt : ಶೂನ್ಯಸೂಚಕ;
    }

    ತಾರ್ಕಿಕ isAssignmentNotStrongerThanStrength(ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ಸ್ಥಿರಾಂಕ uint8_t strength0 = getStrength(assignp, 0);
        ಸ್ಥಿರಾಂಕ uint8_t strength1 = getStrength(assignp, 1);
        ಹಿಂತಿರುಗಿ (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_ದ_ಮೌಲ್ಯ_On_All_ಬಿಟ್ಟುಗಳು(assignp, 0))
               || (strength1 <= strength && assignment_ದ_ಮೌಲ್ಯ_On_All_ಬಿಟ್ಟುಗಳು(assignp, 1));
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_ವಿಲೀನ_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl,
                              ತಾರ್ಕಿಕ isWor) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಅಭಿವ್ಯಕ್ತಿ = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (isWor)
            ಅಭಿವ್ಯಕ್ತಿ = ಹೊಸ AstOr{fl, lhsp, rhsp};
        ಇಲ್ಲದಿದ್ದರೆ
            ಅಭಿವ್ಯಕ್ತಿ = ಹೊಸ AstAnd{fl, lhsp, rhsp};
        ಹಿಂತಿರುಗಿ ಅಭಿವ್ಯಕ್ತಿ;
    }

    ಶೂನ್ಯ ವಿಲೀನ_Wired_Nets_Assignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ಎರಡನೇ;
            ಆದರೆ (assigns.ಗಾತ್ರ_2() <= 1) ಮುಂದುವರಿ;
            AstVar* ಸ್ಥಿರಾಂಕ varp = varpAssigns.ಮೊದಲ;
            ಆದರೆ (!varp->isWiredNet()) ಮುಂದುವರಿ;
            ಸ್ವಯಂಚಾಲಿತ it = assigns.begin();
            Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ನಿಯೋಜಿಸು_Wp0 = *it;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ನಿಯೋಜಿಸು_Wp0->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wExp = ಶೂನ್ಯಸೂಚಕ;
            ಆಗ (++it != assigns.ಕೊನೆ()) {
                Ast_ನಿಯೋಜಿಸು_W* ನಿಯೋಜಿಸು_Wpi = *it;
                ಆದರೆ (!wExp) {
                    wExp
                        = new_ವಿಲೀನ_ಅಭಿವ್ಯಕ್ತಿ(ನಿಯೋಜಿಸು_Wp0->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                       ನಿಯೋಜಿಸು_Wpi->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), fl, varp->isWor());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    wExp = new_ವಿಲೀನ_ಅಭಿವ್ಯಕ್ತಿ(wExp, ನಿಯೋಜಿಸು_Wpi->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((ನಿಯೋಜಿಸು_Wpi->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ()), ನಿಯೋಜಿಸು_Wpi);
            }
            AstVarRef* ಸ್ಥಿರಾಂಕ wVarRef = ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE};
            Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ wAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{fl, wVarRef, wExp};
            ನಿಯೋಜಿಸು_Wp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ನಿಯೋಜಿಸು_Wp0), ನಿಯೋಜಿಸು_Wp0);
            assigns.clear();
            assigns.ತಳ್ಳು_ಹಿಂಭಾಗ(wAssignp);
        }
    }

    ಶೂನ್ಯ removeNotStrongerAssignments(Assigns& assigns, ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* strongestp,
                                      uint8_t greatest_ತಿಳಿದ_Strength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        ಸ್ವಯಂಚಾಲಿತ removedIt = std::remove_if(assigns.begin(), assigns.ಕೊನೆ(), [&](Ast_ನಿಯೋಜಿಸು_W* assignp) {
            ಆದರೆ (assignp == strongestp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (isAssignmentNotStrongerThanStrength(assignp, greatest_ತಿಳಿದ_Strength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(assignp->unlink_Fr_ಹಿಂಭಾಗ()), assignp);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        assigns.erase(removedIt, assigns.ಕೊನೆ());
    }

    ಶೂನ್ಯ remove_Assignments_Not_Stronger_Than_ಏಕರೂಪ_Constant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ಎರಡನೇ;
            ಆದರೆ (assigns.ಗಾತ್ರ_2() <= 1) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ strongest0p = get_Strongest_Assignment_ದ_ಮೌಲ್ಯ(assigns, 0);
            ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ strongest1p = get_Strongest_Assignment_ದ_ಮೌಲ್ಯ(assigns, 1);
            ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* strongestp = ಶೂನ್ಯಸೂಚಕ;
            uint8_t greatest_ತಿಳಿದ_Strength = 0;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ getIfStrongest
                = [&](ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ strongestCandidatep, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
                      ಆದರೆ (!strongestCandidatep) ಹಿಂತಿರುಗಿ;
                      ಸ್ಥಿರಾಂಕ uint8_t strength = getStrength(strongestCandidatep, ಮೌಲ್ಯ);
                      ಆದರೆ (strength >= greatest_ತಿಳಿದ_Strength) {
                          greatest_ತಿಳಿದ_Strength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            ಆದರೆ (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatest_ತಿಳಿದ_Strength);
            }
        }
    }

    ಶೂನ್ಯ removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ಎರಡನೇ;
            ಆದರೆ (assigns.ಗಾತ್ರ_2() <= 1) ಮುಂದುವರಿ;
            ಸ್ವಯಂಚಾಲಿತ maxIt = std::max_element(
                assigns.begin(), assigns.ಕೊನೆ(), [&](ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ap, ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* bp) {
                    ಆದರೆ (m_tgraph.isTristate(ap)) ಹಿಂತಿರುಗಿ !m_tgraph.isTristate(bp);
                    ಆದರೆ (m_tgraph.isTristate(bp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    ಸ್ಥಿರಾಂಕ uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ಸ್ಥಿರಾಂಕ uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    ಹಿಂತಿರುಗಿ minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ strongestp = m_tgraph.isTristate(*maxIt) ? ಶೂನ್ಯಸೂಚಕ : *maxIt;
            ಆದರೆ (strongestp) {
                ಸ್ಥಿರಾಂಕ uint8_t greatest_ತಿಳಿದ_Strength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatest_ತಿಳಿದ_Strength);
            }
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        ಆದರೆ (m_graphing) {
            ಆದರೆ (!m_alhs && nodep->ಅಂಕೆ().hasZ()) m_tgraph.ಗಣ_Tristate(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            ಆದರೆ (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ಸ್ಥಿರಾಂಕ varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                AstConst* ಸ್ಥಿರಾಂಕ enp = getNonZConstp(nodep);
                V3_ಸಂಖ್ಯೆ num1{nodep, nodep->ಅಗಲ()};
                num1.opAnd(nodep->ಅಂಕೆ(), enp->ಅಂಕೆ());  // 01X->01X, Z->0
                AstConst* ಸ್ಥಿರಾಂಕ newconstp = ಹೊಸ AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        ಆದರೆ (m_graphing) {
            iterateChildren(nodep);
            ಆದರೆ (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } ಇಲ್ಲದಿದ್ದರೆ {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ಹಿಂತಿರುಗಿ;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = nodep->condp();
            ಆದರೆ (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thenp = nodep->thenp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elsep = nodep->elsep();
            ಆದರೆ (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ en1p = getEnp(thenp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enp
                    = ಹೊಸ AstCond{nodep->fileline(), condp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                ಆದರೆ (thenp->user1p() && !thenp->user1p()->backp()) ತಳ್ಳು_Deletep(thenp->user1p());
                ಆದರೆ (elsep->user1p() && !elsep->user1p()->backp()) ತಳ್ಳು_Deletep(elsep->user1p());
                thenp->user1p(ಶೂನ್ಯಸೂಚಕ);
                elsep->user1p(ಶೂನ್ಯಸೂಚಕ);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(ಶೂನ್ಯಸೂಚಕ);
            m_tgraph.didProcess(nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        ಆದರೆ (m_graphing) {
            iterateChildren(nodep);
            ಆದರೆ (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } ಇಲ್ಲದಿದ್ದರೆ {
                associateLogic(nodep->fromp(), nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ಆದರೆ (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                ಆದರೆ (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                ಆದರೆ (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ en1p = getEnp(nodep->fromp());
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enp
                        = ಹೊಸ AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ),
                                     nodep->ಅಗಲ_Const()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstConcat* nodep) override {
        ಆದರೆ (m_graphing) {
            iterateChildren(nodep);
            ಆದರೆ (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } ಇಲ್ಲದಿದ್ದರೆ {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ಆದರೆ (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enp = VN_AS(nodep->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    nodep->user1p(ಶೂನ್ಯಸೂಚಕ);
                    nodep->lhsp()->user1p(ಹೊಸ AstSel{nodep->fileline(), enp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ),
                                                     nodep->rhsp()->ಅಗಲ(),
                                                     nodep->lhsp()->ಅಗಲ()});
                    nodep->rhsp()->user1p(
                        ಹೊಸ AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->ಅಗಲ()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p = nodep->lhsp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr2p = nodep->rhsp();
                ಆದರೆ (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ en1p = getEnp(expr1p);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ en2p = getEnp(expr2p);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enp = ಹೊಸ AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(ಶೂನ್ಯಸೂಚಕ);
                    expr2p->user1p(ಶೂನ್ಯಸೂಚಕ);
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ಆದರೆ (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.ಗಣ_Tristate(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            ಆದರೆ (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ಹಿಂತಿರುಗಿ;
            }
            m_tgraph.didProcess(nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr2p = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp;
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ en2p = VN_AS(expr2p->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                enp = ಹೊಸ AstAnd{nodep->fileline(), expr1p, en2p};
            } ಇಲ್ಲದಿದ್ದರೆ {
                enp = expr1p;
            }
            expr1p->user1p(ಶೂನ್ಯಸೂಚಕ);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_And_Or(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ಆದರೆ (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ಹಿಂತಿರುಗಿ;
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
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p = nodep->lhsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr2p = nodep->rhsp();
            ಆದರೆ (!expr1p->user1p() && !expr2p->user1p()) {
                ಹಿಂತಿರುಗಿ;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* en1p = getEnp(expr1p);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* en2p = getEnp(expr2p);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subexpr1p = expr1p->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subexpr2p = expr2p->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            ಆದರೆ (isAnd) {
                subexpr1p = ಹೊಸ AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = ಹೊಸ AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            ಆದರೆ (!en1p->op1p() && en2p->op1p()) {
                std::ಅದಲುಬದಲು(en1p, en2p);
                std::ಅದಲುಬದಲು(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enp = ಹೊಸ AstOr{
                nodep->fileline(),
                ಹೊಸ AstAnd{nodep->fileline(), en1p, ಹೊಸ AstOr{nodep->fileline(), en2p, subexpr1p}},
                ಹೊಸ AstAnd{nodep->fileline(), en2p->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(ಶೂನ್ಯಸೂಚಕ);
            expr2p->user1p(ಶೂನ್ಯಸೂಚಕ);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAnd* nodep) override { ಭೇಟಿ_And_Or(nodep, ನಿಜ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstOr* nodep) override { ಭೇಟಿ_And_Or(nodep, ಸುಳ್ಳು); }

    ಶೂನ್ಯ ಭೇಟಿ_ನಿಯೋಜಿಸು(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        ಆದರೆ (VN_IS(nodep->rhsp(), CReset)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ನಿಯೋಜಿಸು_Wp = VN_CAST(nodep, ನಿಯೋಜಿಸು_W)) {
            m_currentDelayp = VN_CAST(ನಿಯೋಜಿಸು_Wp->timingControlp(), Delay);
        }
        ಆದರೆ (m_graphing) {
            ಆದರೆ (Ast_ನಿಯೋಜಿಸು_W* ನಿಯೋಜಿಸು_Wp = VN_CAST(nodep, ನಿಯೋಜಿಸು_W)) {
                ಆದರೆ (ನಿಯೋಜಿಸು_Wp->getLhsNetDelay()) ಹಿಂತಿರುಗಿ;
                ಆದರೆ (ನಿಯೋಜಿಸು_Wp->timingControlp() && !m_currentDelayp) ಹಿಂತಿರುಗಿ;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                ಆದರೆ (m_currentDelayp && m_currentDelayp->fallDelay()) ಹಿಂತಿರುಗಿ;
                ಆದರೆ (!ನಿಯೋಜಿಸು_Wp->timingControlp()) ಕೂಡಿಸು_ಗಮ್ಯ_Assignment_List(ನಿಯೋಜಿಸು_Wp);
            }

            ಆದರೆ (nodep->user2() & U2_GRAPHING) ಹಿಂತಿರುಗಿ;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
            m_alhs = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
            m_alhs = ಸುಳ್ಳು;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->user2() & U2_NONGRAPH) {
                ಹಿಂತಿರುಗಿ;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            ಆದರೆ (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, ನಿಯೋಜಿಸು_W), nodep,
                            "Continuous assignment inside procedural process");
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)
                          : newAllZerosOrOnes(nodep->lhsp(), ನಿಜ);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ inserted = m_ನಿಯೋಜಿಸು_Enps.emplace(nodep, enp).ಎರಡನೇ;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            ಆದರೆ (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(ಶೂನ್ಯಸೂಚಕ);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep->lhsp(), Sel)) {
                    ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(selp->fromp(), ಗಂಟು_Var_Ref)) {
                        ಆದರೆ (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), ನಿಜ));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            m_tgraph.didProcess(nodep, ನಿಜ);
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        m_tgraph.didProcess(nodep, ನಿಜ);  // LCOV_EXCL_LINE
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, ನಿಜ);
                }
            }
            m_alhs = ನಿಜ;  // And user1p() will indicate tristate equation, if any
            ಆದರೆ (Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ನಿಯೋಜಿಸು_Wp = VN_CAST(nodep, ನಿಯೋಜಿಸು_W)) {
                ಆದರೆ (AstStrengthSpec* ಸ್ಥಿರಾಂಕ specp = ನಿಯೋಜಿಸು_Wp->strengthSpecp()) {
                    ಆದರೆ (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        ಆದರೆ (!is_Only_Assignment_Is_ಗಮ್ಯ_Lhs_Var(ನಿಯೋಜಿಸು_Wp)) {
                            ನಿಯೋಜಿಸು_Wp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override { ಭೇಟಿ_ನಿಯೋಜಿಸು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override { ಭೇಟಿ_ನಿಯೋಜಿಸು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override { ಭೇಟಿ_ನಿಯೋಜಿಸು(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ಸ್ಥಿರಾಂಕ AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ನಿಯೋಜಿಸು ? ಶೂನ್ಯಸೂಚಕ : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_ಒಳ_Alias);
        m_ಒಳ_Alias = ನಿಜ;
        ಆದರೆ (m_graphing) {
            ಆದರೆ (nodep->user2() & U2_GRAPHING) ಹಿಂತಿರುಗಿ;
            m_alhs = ನಿಜ;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_Case_Eq(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ neq) {
        ಆದರೆ (m_graphing) {
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->lhsp(), Const);
            ಆದರೆ (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();
                rhsp->unlink_Fr_ಹಿಂಭಾಗ();
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enRhsp;
                ಆದರೆ (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    rhsp->user1p(ಶೂನ್ಯಸೂಚಕ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    enRhsp = get_En_ಅಭಿವ್ಯಕ್ತಿ_Based_On_Originalp(rhsp);
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp
                    = ಹೊಸ AstLogAnd{fl, ಹೊಸ AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    ಹೊಸ AstEqCase{fl, ಹೊಸ AstConst{fl, constp->ಅಂಕೆ()}, rhsp}};
                constp->user1p(ಶೂನ್ಯಸೂಚಕ);
                ಆದರೆ (neq) newp = ಹೊಸ AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp && nodep->rhsp()->user1p()) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                constp->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಹೊಸ AstLogAnd{fl,
                                                  ಹೊಸ AstEq{fl, newAllZerosOrOnes(constp, ಸುಳ್ಳು),
                                                            VN_AS(rhsp->user1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)},
                                                  // Keep the caseeq if there are X's present
                                                  ಹೊಸ AstEqCase{fl, constp, rhsp}};
                ಆದರೆ (neq) newp = ಹೊಸ AstLogNot{fl, newp};
                rhsp->user1p(ಶೂನ್ಯಸೂಚಕ);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                checkUnhandled(nodep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_Eq_Neq_Wild(Ast_ಗಂಟು_Biop* nodep) {
        ಆದರೆ (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        ಆದರೆ (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            ಹಿಂತಿರುಗಿ;
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEqCase* nodep) override { ಭೇಟಿ_Case_Eq(nodep, ಸುಳ್ಳು); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqCase* nodep) override { ಭೇಟಿ_Case_Eq(nodep, ನಿಜ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstEqWild* nodep) override { ಭೇಟಿ_Eq_Neq_Wild(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqWild* nodep) override { ಭೇಟಿ_Eq_Neq_Wild(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು* nodep) override {
        std::array<ತಾರ್ಕಿಕ, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->ಅಂಕೆ().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->ಅಂಕೆ().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        ಆದರೆ (m_graphing) {
            iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
            ಆದರೆ (!dropop[0]) iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
            ಆದರೆ (!dropop[1]) iterate_And_ಮುಂದಿನ_Null(nodep->thsp());
            ಆದರೆ (!dropop[2]) iterate_And_ಮುಂದಿನ_Null(nodep->fhsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nonXp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!dropop[0]) {
                nonXp = nodep->rhsp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!dropop[1]) {
                nonXp = nodep->thsp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            ಆದರೆ (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                ಆದರೆ (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    ಹಿಂತಿರುಗಿ;
                }
                AstVar* ಸ್ಥಿರಾಂಕ envarp = getCreateEnVarp(varrefp->varp(), ಸುಳ್ಳು);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಹೊಸ Ast_ಎಣಿಕೆ_Ones{
                    nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                ಆದರೆ (nonXp) {  // Need to still count '0 or '1 or 'x's
                    ಆದರೆ (dropop[0]) {
                        nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                        nodep->rhsp(nonXp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
                    }
                    ಆದರೆ (dropop[1]) {
                        nodep->thsp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                        nodep->thsp(nonXp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
                    }
                    ಆದರೆ (dropop[2]) {
                        nodep->fhsp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                        nodep->fhsp(nonXp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
                    }
                    newp = ಹೊಸ Ast_ಕೂಡಿಸು{nodep->fileline(), nodep, newp};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ತಳ್ಳು_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        ಆದರೆ (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.ಗಣ_Tristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep->lhsp(), Sel);
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lsbp = selp ? VN_CAST(selp->lsbp(), Const) : ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (lsbp) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = lsbp->ಗಮ್ಯ_SInt();
                    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < selp->ಅಗಲ_Const(); ++i) {
                        ಗಣ_ಬಿಟ್_Pull_Direction(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಗಣ_Pull_Direction(varrefp->varp(), nodep);
                }
            }
        }
        ಆದರೆ (!m_graphing) {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ iteratePinGuts(AstPin* nodep) {
        ಆದರೆ (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            ಆದರೆ (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // All heavy lifting completed in graph visitor.
            ಆದರೆ (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {
        ಆದರೆ (m_graphing) {
            ಆದರೆ (nodep->user2() & U2_GRAPHING) ಹಿಂತಿರುಗಿ;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ಸ್ಥಿರಾಂಕ enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            ಆದರೆ (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, ಸುಳ್ಳು);
                iteratePinGuts(nodep);
                ಹಿಂತಿರುಗಿ;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Decl_Processing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::ಒಳಹರಿವು
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            ಆದರೆ (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ಸ್ಥಿರಾಂಕ ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(ಹೊಸ AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.ಗಣ_Tristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Decl_Processing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, ನಿಜ, ನಿಜ);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.ಗಣ_Tristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ಸ್ಥಿರಾಂಕ enVarp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvt_ಗಮ್ಯ_Str(m_unique++),
                                              VFlag_ಬಿಟ್_Packed{}, enModVarp->ಅಗಲ()};
            ಆದರೆ (ಒಳ_Decl_Processing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->var_Type2_ಒಳ();
            } ಇಲ್ಲದಿದ್ದರೆ {
                enModVarp->var_Type2_ಹೊರಹೋಗು();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ಸ್ಥಿರಾಂಕ enpinp
                = ಹೊಸ AstPin{nodep->fileline(), nodep->pin_ಅಂಕೆ(),
                             enModVarp->name(),  // should be {var}"__en"
                             ಹೊಸ AstVarRef{nodep->fileline(), enVarp,
                                           ಒಳ_Decl_Processing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(enpinp);
            m_modp->ಕೂಡಿಸು_Stmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಹೊರಹೋಗು_Assignp = ಶೂನ್ಯಸೂಚಕ;  // If reconnected, the related assignment
            AstPin* outpinp = ಶೂನ್ಯಸೂಚಕ;
            AstVar* ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Mod_Varp = m_varAux(nodep->modVarp()).ಹೊರಹೋಗು_Varp;
            ಆದರೆ (!ಹೊರಹೋಗು_Mod_Varp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                ಆದರೆ (!m_modp->is_ತುದಿ()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ outexprp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)
                                                  ->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);  // Note has lvalue() set
                outpinp = ಹೊಸ AstPin{nodep->fileline(), nodep->pin_ಅಂಕೆ(),
                                     ಹೊರಹೋಗು_Mod_Varp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(ಹೊರಹೋಗು_Mod_Varp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(outpinp);
                // Simplify
                ಆದರೆ (ಒಳ_Decl_Processing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, ನಿಜ};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                ಹೊರಹೋಗು_Assignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        ನಿಜ);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, ಹೊರಹೋಗು_Assignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ಸ್ಥಿರಾಂಕ TristatePinVisitor visitor{nodep->exprp(), m_tgraph, ಸುಳ್ಳು};
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಒಳ_Assignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, ನಿಜ);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, ಒಳ_Assignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            ಆದರೆ (!ಹೊರಹೋಗು_Assignp) {
                ಆದರೆ (!outpinp) {
                    exprrefp = ಶೂನ್ಯಸೂಚಕ;  // Primary input only
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    ಆದರೆ (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(ಹೊರಹೋಗು_Assignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                ಆದರೆ (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            ಆದರೆ (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(ಹೊಸ AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                ಆದರೆ (!ಹೊರಹೋಗು_Assignp) {
                    map_ಸೇರಿಸು_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            ಆದರೆ (exprrefp) {
                AstPull* ಸ್ಥಿರಾಂಕ pullp = m_varAux(nodep->modVarp()).pullp;
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& src_ಬಿಟ್_Pulls = m_varAux(nodep->modVarp()).ಬಿಟ್_Pulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = ಶೂನ್ಯಸೂಚಕ;
                ಪೂರ್ಣಾಂಕ selLsb = 0;
                ಪೂರ್ಣಾಂಕ sel_ಅಗಲ = 0;
                ಆದರೆ (ಹೊರಹೋಗು_Assignp) {
                    ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(ಹೊರಹೋಗು_Assignp->lhsp(), Sel)) {
                        AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(selp->lsbp(), Const);
                        ಆದರೆ (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->ಗಮ್ಯ_SInt();
                            sel_ಅಗಲ = selp->ಅಗಲ_Const();
                        }
                    }
                }

                ಆದರೆ (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    ಗಣ_Pull_Direction(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    ಆದರೆ (selVarp) {
                        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ direction = pullp->direction();
                        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < sel_ಅಗಲ; ++i) {
                            ಗಣ_ಬಿಟ್_Pull_Direction(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                ಆದರೆ (!src_ಬಿಟ್_Pulls.ಖಾಲಿಗಳು()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : src_ಬಿಟ್_Pulls) {
                        ಗಣ_ಬಿಟ್_Pull_Direction(exprrefp->varp(), pair.ಮೊದಲ, pair.ಎರಡನೇ);
                    }  // LCOV_EXCL_LINE
                    ಆದರೆ (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : src_ಬಿಟ್_Pulls) {
                            ಗಣ_ಬಿಟ್_Pull_Direction(selVarp, selLsb + pair.ಮೊದಲ, pair.ಎರಡನೇ);
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
        ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->user2() & U2_NONGRAPH) ಹಿಂತಿರುಗಿ;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    ಶೂನ್ಯ handle_ಗಂಟು_Var_Ref(Ast_ಗಂಟು_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        ಆದರೆ (m_graphing) {
            ಆದರೆ (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            ಆದರೆ (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            ಆದರೆ (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.ಗಣ_Tristate(nodep->varp());
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->user2() & U2_NONGRAPH) ಹಿಂತಿರುಗಿ;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            ಆದರೆ (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                ಆದರೆ (m_ಒಳ_Alias) {
                    ಆದರೆ (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    ಹಿಂತಿರುಗಿ;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_ಸೇರಿಸು_Lhs_Var_Ref(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                AstVar* ಸ್ಥಿರಾಂಕ enVarp = getCreateEnVarp(nodep->varp(), ಸುಳ್ಳು);
                ಆದರೆ (AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->user1p(ಹೊಸ AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override { handle_ಗಂಟು_Var_Ref(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef* nodep) override { handle_ಗಂಟು_Var_Ref(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ಆದರೆ (m_graphing) {
            // If tri0/1 force a pullup
            ಆದರೆ (nodep->user2() & U2_GRAPHING) ಹಿಂತಿರುಗಿ;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            ಆದರೆ (nodep->isPulldown() || nodep->isPullup()) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstPull{
                    nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newp);
                // We'll iterate on the new AstPull later
            }
            ಆದರೆ (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.ಗಣ_Tristate(nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // !graphing
            ಆದರೆ (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        ಆದರೆ (m_processedIfaces && VN_IS(nodep, Iface)) ಹಿಂತಿರುಗಿ;
        UINFO(8, dbgState() << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_graphing);
        VL_RESTORER(m_unique);
        VL_RESTORER_CLEAR(m_lhsmap);
        VL_RESTORER_CLEAR(m_assigns);
        VL_RESTORER_CLEAR(m_ನಿಯೋಜಿಸು_Enps);
        VL_RESTORER_CLEAR(m_proceduralEnables);
        // Not preserved, needs pointer instead: TristateGraph origTgraph = m_tgraph;
        UASSERT_OBJ(m_tgraph.ಖಾಲಿಗಳು(), nodep, "Unsupported: NodeModule under NodeModule");

        // Clear state
        m_graphing = ಸುಳ್ಳು;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = ಶೂನ್ಯಸೂಚಕ;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = ನಿಜ;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = ಸುಳ್ಳು;
        UINFO(9, dbgState() << "processing mod " << nodep);

        // Merge the assignments for very Wired net LHS : wor, trior, wand and triand
        ವಿಲೀನ_Wired_Nets_Assignments();
        // Remove all assignments not stronger than the strongest uniform constant
        remove_Assignments_Not_Stronger_Than_ಏಕರೂಪ_Constant();
        // Use graph to find tristate signals
        m_tgraph.ನಕ್ಷೆ_Walk(nodep);

        // Remove all assignments not stronger than the strongest non-tristate RHS
        removeAssignmentsNotStrongerThanNonTristate();

        // Build the LHS drivers map for this module
        iterateChildren(nodep);
        // Insert new logic for all tristates
        ಸೇರಿಸು_Tristates(nodep);
        ಸೇರಿಸು_Procedural_Enable_Assignments(nodep);

        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& ನಿಯೋಜಿಸು_Enp : m_ನಿಯೋಜಿಸು_Enps) {
            VL_DO_DANGLING(ನಿಯೋಜಿಸು_Enp.ಎರಡನೇ->delete_ವೃಕ್ಷ(), ನಿಯೋಜಿಸು_Enp.ಎರಡನೇ);
        }
        m_ನಿಯೋಜಿಸು_Enps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        // don't deal with classes
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        // don't deal with functions
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        // don't deal with casez compare '???? values
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    ಶೂನ್ಯ combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<AstVar* ಸ್ಥಿರಾಂಕ, std::ಸದಿಶ<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ಸ್ಥಿರಾಂಕ invarp = kv.ಮೊದಲ;
            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::pair<AstVar*, AstVar*>>& contribs = kv.ಎರಡನೇ;
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ifaceModp = ಕಂಡುಹಿಡಿ_ಪೋಷಕ_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.ಗಾತ್ರ_2() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* orp = ಶೂನ್ಯಸೂಚಕ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enp = ಶೂನ್ಯಸೂಚಕ;

            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ಸ್ಥಿರಾಂಕ contribEnp = contrib.ಮೊದಲ;
                AstVar* ಸ್ಥಿರಾಂಕ contribOutp = contrib.ಎರಡನೇ;

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Refp = ಹೊಸ AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enRefp = ಹೊಸ AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ andp = ಹೊಸ AstAnd{fl, ಹೊರಹೋಗು_Refp, enRefp};

                orp = (!orp) ? andp : ಹೊಸ AstOr{fl, orp, andp};

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enRef2p = ಹೊಸ AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : ಹೊಸ AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            ಆದರೆ (!envarp) {
                envarp = ಹೊಸ AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->ಗಣ_Dfg_Tri_Lowered();
                ifaceModp->ಕೂಡಿಸು_Stmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ enAssp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{fl, ಹೊಸ AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{enAssp});

            // Pull resolution
            ಸ್ಥಿರಾಂಕ AstPull* ಸ್ಥಿರಾಂಕ pullp = m_varAux(invarp).pullp;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* undrivenp = ಹೊಸ AstNot{fl, ಹೊಸ AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = ಹೊಸ AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = ಹೊಸ AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{fl, ಹೊಸ AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{assp});
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::ಸದಿಶ<Ast_ಗಂಟು_Module*> ifacesp;
        ಗಾಗಿ (Ast_ಗಂಟು* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            ಆದರೆ (VN_IS(modp, Iface)) ifacesp.ತಳ್ಳು_ಹಿಂಭಾಗ(VN_AS(modp, ಗಂಟು_Module));
        }
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = ನಿಜ;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::ಸದಿಶ<Ast_ಗಂಟು*> ಬಳಸದ_Rootps;
        netlistp->foreach([&](Ast_ಗಂಟು* nodep) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ enp = nodep->user1p();
            ಆದರೆ (!enp) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (enp->backp()) ಹಿಂತಿರುಗಿ;
            ಬಳಸದ_Rootps.emplace_ಹಿಂಭಾಗ(enp);
        });
        ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep : ಬಳಸದ_Rootps) VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
#ಆದರೆ_ಅಂತ್ಯ
    }
    ~TristateVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

ಶೂನ್ಯ V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("tristate", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
