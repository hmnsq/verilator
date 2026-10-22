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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Tristate.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Inst.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ Tristate_ਅਧਾਰ_Visitor VL_NOT_FINAL : ਜਨਤਕ VNVisitor {
ਜਨਤਕ:
    // METHODS
};

//######################################################################
// Graph support classes

ਵਰਗ Tristate_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Tristate_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ* ਅਚਲ m_nodep;
    ਤਾਰਕਿਕ m_isTristate = ਝੂਠ;  // Logic indicates a tristate
    ਤਾਰਕਿਕ m_feedsTri = ਝੂਠ;  // Propagates to a tristate node (on RHS)
    ਤਾਰਕਿਕ m_processed = ਝੂਠ;  // Tristating was cleaned up
ਜਨਤਕ:
    Tristate_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep} {}
    ~Tristate_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    // ACCESSORS
    Ast_ਗੰਢ* nodep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_nodep; }
    ਅਚਲ AstVar* varp() ਅਚਲ { ਵਾਪਸ VN_CAST(nodep(), Var); }
    string name() ਅਚਲ override {
        ਵਾਪਸ ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvt_ਮੰਜ਼ਿਲ_Hex(nodep())));
    }
    string dotColor() ਅਚਲ override {
        ਵਾਪਸ (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ nodep()->fileline(); }
    ਤਾਰਕਿਕ isTristate() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_isTristate; }
    ਖਾਲੀ isTristate(ਤਾਰਕਿਕ flag) { m_isTristate = flag; }
    ਤਾਰਕਿਕ feedsTri() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_feedsTri; }
    ਖਾਲੀ feedsTri(ਤਾਰਕਿਕ flag) { m_feedsTri = flag; }
    ਤਾਰਕਿਕ processed() ਅਚਲ { ਵਾਪਸ m_processed; }
    ਖਾਲੀ processed(ਤਾਰਕਿਕ flag) { m_processed = flag; }
};

//######################################################################

ਵਰਗ Tristate_ਗ੍ਰਾਫ final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
ਜਨਤਕ:
    ਵਰਤੋਂ VarVec = std::ਸਦਿਸ਼<AstVar*>;

ਨਿਜੀ:
    // MEMBERS
    V3_ਗ੍ਰਾਫ m_ਗ੍ਰਾਫ;  // Logic graph

ਜਨਤਕ:
    // CONSTRUCTORS
    Tristate_ਗ੍ਰਾਫ() { clearAndCheck(); }
    ਆਭਾਸੀ ~Tristate_ਗ੍ਰਾਫ() { clearAndCheck(); }
    VL_UNCOPYABLE(Tristate_ਗ੍ਰਾਫ);

ਨਿਜੀ:
    // METHODS

    Tristate_ਸ਼ੀਰਸ਼* make_ਸ਼ੀਰਸ਼(Ast_ਗੰਢ* nodep) {
        Tristate_ਸ਼ੀਰਸ਼* vertexp = reinterpret_cast<Tristate_ਸ਼ੀਰਸ਼*>(nodep->user4p());
        ਜੇ (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = ਨਵਾਂ Tristate_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ, nodep};
            nodep->user4p(vertexp);
        }
        ਵਾਪਸ vertexp;
    }

    // METHODS - Graph optimization
    ਖਾਲੀ ਗ੍ਰਾਫ_Walk_Recurse_Fwd(Tristate_ਸ਼ੀਰਸ਼* vtxp, ਪੂਰਨ level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        ਜੇ (!vtxp->isTristate()) ਵਾਪਸ;  // tristate involved
        ਜੇ (vtxp->user() == 1) ਵਾਪਸ;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        ਜੇ (!vtxp->varp()) {  // not a var where we stop the recursion
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                Tristate_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = static_cast<Tristate_ਸ਼ੀਰਸ਼*>(ਕਿਨਾਰਾ.ਸਿਖਰ());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ਜੇ (!vvertexp->isTristate()) {
                    vvertexp->isTristate(ਸੱਚ);
                    ਗ੍ਰਾਫ_Walk_Recurse_Fwd(vvertexp, level + 1);
                }
            }
        } ਨਹੀਂਤਾਂ {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                Tristate_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = static_cast<Tristate_ਸ਼ੀਰਸ਼*>(ਕਿਨਾਰਾ.fromp());
                ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    ਜੇ (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(ਸੱਚ);
                        ਗ੍ਰਾਫ_Walk_Recurse_Fwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    ਖਾਲੀ ਗ੍ਰਾਫ_Walk_Recurse_ਪਿੱਠ(Tristate_ਸ਼ੀਰਸ਼* vtxp, ਪੂਰਨ level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        ਜੇ (!(vtxp->isTristate() || vtxp->feedsTri())) ਵਾਪਸ;  // tristate involved
        ਜੇ (vtxp->user() == 3) ਵਾਪਸ;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        ਜੇ (!vtxp->varp()) {  // not a var where we stop the recursion
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                Tristate_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = static_cast<Tristate_ਸ਼ੀਰਸ਼*>(ਕਿਨਾਰਾ.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                ਜੇ (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(ਸੱਚ);
                    ਗ੍ਰਾਫ_Walk_Recurse_ਪਿੱਠ(vvertexp, level + 1);
                }
            }
        }
    }

ਜਨਤਕ:
    // METHODS
    ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_ਗ੍ਰਾਫ.ਖਾਲੀ_ਥਾਵਾਂ(); }
    ਖਾਲੀ clearAndCheck() {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਅਚਲ Tristate_ਸ਼ੀਰਸ਼& vvertex = static_cast<Tristate_ਸ਼ੀਰਸ਼&>(vtx);
            ਜੇ (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_ਗ੍ਰਾਫ.clear();
        Ast_ਗੰਢ::user4_Clear_ਰੁੱਖ();  // Wipe all node user4p's that point to vertexes
    }
    ਖਾਲੀ ਗ੍ਰਾਫ_Walk(Ast_ਗੰਢ_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਗ੍ਰਾਫ_Walk_Recurse_Fwd(static_cast<Tristate_ਸ਼ੀਰਸ਼*>(&vtx), 0);
        }
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਗ੍ਰਾਫ_Walk_Recurse_ਪਿੱਠ(static_cast<Tristate_ਸ਼ੀਰਸ਼*>(&vtx), 0);
        }
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    ਖਾਲੀ associate(Ast_ਗੰਢ* fromp, Ast_ਗੰਢ* ਸਿਖਰ) {
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, make_ਸ਼ੀਰਸ਼(fromp), make_ਸ਼ੀਰਸ਼(ਸਿਖਰ), 1};
    }
    ਖਾਲੀ deleteVerticesFromSubtreeRecurse(Ast_ਗੰਢ* nodep) {
        ਜੇ (!nodep) ਵਾਪਸ;
        // Skip vars, because they may be connected to more than one varref
        ਜੇ (!VN_IS(nodep, Var)) {
            Tristate_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = reinterpret_cast<Tristate_ਸ਼ੀਰਸ਼*>(nodep->user4p());
            ਜੇ (vertexp) vertexp->unlinkDelete(&m_ਗ੍ਰਾਫ);
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    ਖਾਲੀ ਸੈੱਟ_Tristate(Ast_ਗੰਢ* nodep) { make_ਸ਼ੀਰਸ਼(nodep)->isTristate(ਸੱਚ); }
    ਤਾਰਕਿਕ isTristate(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ Tristate_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = reinterpret_cast<Tristate_ਸ਼ੀਰਸ਼*>(nodep->user4p());
        ਵਾਪਸ vertexp && vertexp->isTristate();
    }
    ਤਾਰਕਿਕ feedsTri(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ Tristate_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = reinterpret_cast<Tristate_ਸ਼ੀਰਸ਼*>(nodep->user4p());
        ਵਾਪਸ vertexp && vertexp->feedsTri();
    }
    ਖਾਲੀ didProcess(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ quiet = ਝੂਠ) {
        ਜੇ (Tristate_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = reinterpret_cast<Tristate_ਸ਼ੀਰਸ਼*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(ਸੱਚ);
        } ਨਹੀਂਤਾਂ ਜੇ (!quiet) {
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
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਅਚਲ Tristate_ਸ਼ੀਰਸ਼& vvertex = static_cast<ਅਚਲ Tristate_ਸ਼ੀਰਸ਼&>(vtx);
            ਜੇ (vvertex.isTristate()) {
                ਜੇ (AstVar* ਅਚਲ nodep = VN_CAST(vvertex.nodep(), Var)) v.ਧੱਕੋ_ਪਿੱਠ(nodep);
            }
        }
        ਵਾਪਸ v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

ਵਰਗ TristatePinVisitor final : ਜਨਤਕ Tristate_ਅਧਾਰ_Visitor {
    Tristate_ਗ੍ਰਾਫ& m_tgraph;
    ਅਚਲ ਤਾਰਕਿਕ m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        ਜੇ (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } ਨਹੀਂਤਾਂ ਜੇ (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.ਸੈੱਟ_Tristate(nodep->varp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਟੁਕੜਾ_Sel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_ਗੰਢ* nodep, Tristate_ਗ੍ਰਾਫ& tgraph, ਤਾਰਕਿਕ lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = ਮੂਲ;
};

//######################################################################

ਵਰਗ TristateVisitor final : ਜਨਤਕ Tristate_ਅਧਾਰ_Visitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_inuser4;

    ਬਣਤਰ AuxAstVar final {
        AstPull* pullp = ਖਾਲੀ_ਸੂਚਕ;  // pullup/pulldown direction (whole variable)
        AstVar* ਬਾਹਰਲਾ_Varp = ਖਾਲੀ_ਸੂਚਕ;  // output __out var
        ਤਾਰਕਿਕ ifaceTristate = ਝੂਠ;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<ਪੂਰਨ, ਪੂਰਨ>
            ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    ਬਣਤਰ RefStrength final {
        Ast_ਗੰਢ_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_ਗੰਢ_ਸੌਂਪੋ* m_assignp;  // Assignment containing m_varrefp
        ਅਚਲ Ast_ਗੰਢ_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_ਗੰਢ_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_ਗੰਢ_Var_Ref* varrefp, Ast_ਗੰਢ_ਸੌਂਪੋ* assignp,
                    ਅਚਲ Ast_ਗੰਢ_Procedure* procedurep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* enLhsp,
                    Ast_ਗੰਢ_Var_Ref* enVarrefp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* enRhsp, VStrength strength,
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
    ਬਣਤਰ LhsProjection final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_lhsp;  // LHS giving the enable assign
        Ast_ਗੰਢ_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    ਬਣਤਰ ProceduralEnable final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    ਵਰਤੋਂ RefStrengthVec = std::ਸਦਿਸ਼<RefStrength>;
    ਵਰਤੋਂ VarMap = std::map<AstVar*, RefStrengthVec*>;
    ਵਰਤੋਂ Assigns = std::ਸਦਿਸ਼<Ast_ਸੌਂਪੋ_W*>;
    ਵਰਤੋਂ Var_ਮੰਜ਼ਿਲ_Assigns_Map = std::map<AstVar*, Assigns>;
    ਵਰਤੋਂ ਸੌਂਪੋ_Enable_Map = std::map<Ast_ਗੰਢ_ਸੌਂਪੋ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>;
    ਵਰਤੋਂ ProceduralEnableMap = std::map<Ast_ਗੰਢ_ਸੌਂਪੋ*, std::ਸਦਿਸ਼<ProceduralEnable>>;
    ਗਿਣਤੀ : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    ਤਾਰਕਿਕ m_graphing = ਝੂਠ;  // Major mode - creating graph
    //
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    AstCell* m_cellp = ਖਾਲੀ_ਸੂਚਕ;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    Var_ਮੰਜ਼ਿਲ_Assigns_Map m_assigns;  // Assigns in current module
    ਸੌਂਪੋ_Enable_Map m_ਸੌਂਪੋ_Enps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    ਪੂਰਨ m_unique = 0;
    ਤਾਰਕਿਕ m_alhs = ਝੂਠ;  // On LHS of assignment
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Alias = ਝੂਠ;  // Inside alias statement
    ਤਾਰਕਿਕ m_processedIfaces = ਝੂਠ;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = ਖਾਲੀ_ਸੂਚਕ;  // Delay of assignment currently collecting LHS drivers
    ਅਚਲ Ast_ਗੰਢ* m_logicp = ਖਾਲੀ_ਸੂਚਕ;  // Current logic being built
    Ast_ਗੰਢ_ਸੌਂਪੋ* m_assignp = ਖਾਲੀ_ਸੂਚਕ;  // Current assignment
    ਅਚਲ Ast_ਗੰਢ_Procedure* m_procedurep = ਖਾਲੀ_ਸੂਚਕ;  // Current procedural process
    Tristate_ਗ੍ਰਾਫ m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::ਸਦਿਸ਼<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ਅਚਲ {
        string o = (m_graphing ? " gr " : " ng ");
        ਜੇ (m_alhs) o += "alhs ";
        ਵਾਪਸ o;
    }
    ਖਾਲੀ mod_ਜੋੜੋ_Stmtp(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* newp) {
        ਜੇ (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ {
            m_modp->ਜੋੜੋ_Stmtsp(newp);
        }
    }
    ਖਾਲੀ associateLogic(Ast_ਗੰਢ* fromp, Ast_ਗੰਢ* ਸਿਖਰ) {
        ਜੇ (m_logicp) m_tgraph.associate(fromp, ਸਿਖਰ);
    }
    AstConst* newAllZerosOrOnes(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ ones) {
        V3_ਸੰਖਿਆ ਅੰਕ{nodep, nodep->ਚੌੜਾਈ()};
        ਜੇ (ones) ਅੰਕ.ਸੈੱਟ_All_Bits1();
        AstConst* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), ਅੰਕ};
        ਵਾਪਸ newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_ਸਤਰ* fl, AstVar* varp, ਅਚਲ string& dotted, VAccess access,
                           ਅਚਲ string& inlinedDots) {
        AstVarXRef* ਅਚਲ xrefp = ਨਵਾਂ AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        ਵਾਪਸ xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* ਲੱਭੋ_Modport_For_Dotted(Ast_ਗੰਢ_Module* modp, ਅਚਲ string& dotted_ਰਸਤਾ) {
        ਜੇ (dotted_ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ string ਪਹਿਲਾ_Comp = dotted_ਰਸਤਾ.substr(0, dotted_ਰਸਤਾ.ਲੱਭੋ('.'));
        ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ਅਚਲ ivarp = VN_CAST(stmtp, Var);
            ਜੇ (!ivarp || ivarp->name() != ਪਹਿਲਾ_Comp) ਜਾਰੀ;
            ਅਚਲ AstIfaceRefDType* ਅਚਲ ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            ਜੇ (ifaceDtp && ifaceDtp->modportp()) ਵਾਪਸ ifaceDtp->modportp();
            ਰੋਕੋ;
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* getEnp(Ast_ਗੰਢ* nodep) {
        ਜੇ (nodep->user1p()) {
            ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp = VN_CAST(nodep, ਗੰਢ_Var_Ref)) {
                ਜੇ (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    ਵਾਪਸ newAllZerosOrOnes(nodep, ਸੱਚ);
                }
            }
        } ਨਹੀਂਤਾਂ {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, ਸੱਚ));
        }
        // Otherwise return the previous output enable
        ਵਾਪਸ VN_AS(nodep->user1p(), ਗੰਢ_ਵਿਅੰਜਕ);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    ਖਾਲੀ ਜੋੜੋ_Modport_Var_Ref_If_Missing(AstModport* modportp, AstVar* varp, VDirection dir) {
        ਲਈ (Ast_ਗੰਢ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ਅਚਲ mvrp = VN_CAST(mrp, ModportVarRef);
            ਜੇ (mvrp && mvrp->varp() == varp) ਵਾਪਸ;  // Already present
        }
        AstModportVarRef* ਅਚਲ mvrp = ਨਵਾਂ AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->ਜੋੜੋ_Varsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, ਤਾਰਕਿਕ is_ਸਿਖਰ) {
        // Return the master __en for the specified input variable
        ਜੇ (!invarp->user1p()) {
            AstVar* ਅਚਲ newp
                = ਨਵਾਂ AstVar{invarp->fileline(), is_ਸਿਖਰ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_ਗੰਢ_Module* ਅਚਲ ownerModp = ਲੱਭੋ_ਮਾਪਾ_Module(invarp);
            ਜੇ (ownerModp && ownerModp != m_modp) {
                ownerModp->ਜੋੜੋ_Stmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                ਲਈ (Ast_ਗੰਢ* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ਅਚਲ modportp = VN_CAST(stmtp, Modport);
                    ਜੇ (!modportp) ਜਾਰੀ;
                    ਲਈ (Ast_ਗੰਢ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ਅਚਲ mvrp = VN_CAST(mrp, ModportVarRef);
                        ਜੇ (mvrp && mvrp->varp() == invarp) {
                            ਜੋੜੋ_Modport_Var_Ref_If_Missing(modportp, newp, VDirection::ਨਿਵੇਸ਼);
                            ਰੋਕੋ;
                        }
                    }
                }
            } ਨਹੀਂਤਾਂ {
                mod_ਜੋੜੋ_Stmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        ਵਾਪਸ VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ਅਚਲ constp) {
        File_ਸਤਰ* ਅਚਲ fl = constp->fileline();
        V3_ਸੰਖਿਆ numz{constp, constp->ਚੌੜਾਈ()};
        numz.op_ਬਿੱਟ_Z(constp->ਅੰਕ());  // Z->1, else 0
        V3_ਸੰਖਿਆ numz0{constp, constp->ਚੌੜਾਈ()};
        numz0.opNot(numz);  // Z->0, else 1
        ਵਾਪਸ ਨਵਾਂ AstConst{fl, numz0};
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* get_En_ਵਿਅੰਜਕ_Based_On_Originalp(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep) {
        ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep, VarRef)) {
            ਵਾਪਸ ਨਵਾਂ AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), ਝੂਠ),
                                 VAccess::READ};
        } ਨਹੀਂਤਾਂ ਜੇ (AstVarXRef* ਅਚਲ xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ਅਚਲ enVarp = getCreateEnVarp(xrefp->varp(), ਝੂਠ);
            ਵਾਪਸ newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } ਨਹੀਂਤਾਂ ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
            ਵਾਪਸ getNonZConstp(constp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstExtend* ਅਚਲ extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = extendp->lhsp()->unlink_Fr_ਪਿੱਠ();
            AstExtend* ਅਚਲ enExtendp = extendp->clone_ਰੁੱਖ(ਝੂਠ);
            extendp->lhsp(lhsp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enLhsp = get_En_ਵਿਅੰਜਕ_Based_On_Originalp(lhsp);
            enExtendp->lhsp(ਨਵਾਂ AstNot{enLhsp->fileline(), enLhsp});
            ਵਾਪਸ ਨਵਾਂ AstNot{enExtendp->fileline(), enExtendp};
        } ਨਹੀਂਤਾਂ ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = selp->fromp()->unlink_Fr_ਪਿੱਠ();
            AstSel* ਅਚਲ enSelp = selp->clone_ਰੁੱਖ(ਝੂਠ);
            selp->fromp(fromp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enFromp = get_En_ਵਿਅੰਜਕ_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            ਵਾਪਸ enSelp;
        } ਨਹੀਂਤਾਂ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
    }
    AstVar* get_Create_ਬਾਹਰਲਾ_Varp(AstVar* invarp, ਤਾਰਕਿਕ is_ਸਿਖਰ) {
        // Return the master __out for the specified input variable
        ਜੇ (!m_varAux(invarp).ਬਾਹਰਲਾ_Varp) {
            AstVar* ਅਚਲ newp
                = ਨਵਾਂ AstVar{invarp->fileline(), is_ਸਿਖਰ ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            mod_ਜੋੜੋ_Stmtp(invarp, newp);
            m_varAux(invarp).ਬਾਹਰਲਾ_Varp = newp;  // find outvar given invarp
        }
        ਵਾਪਸ m_varAux(invarp).ਬਾਹਰਲਾ_Varp;
    }
    AstVar* getCreateUnconnVarp(Ast_ਗੰਢ* fromp, Ast_ਗੰਢ_DType* dtypep) {
        AstVar* ਅਚਲ newp = ਨਵਾਂ AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvt_ਮੰਜ਼ਿਲ_Str(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        mod_ਜੋੜੋ_Stmtp(newp, newp);
        ਵਾਪਸ newp;
    }

    LhsProjection newLhsProjection(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_Var_Ref* targetp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newLhsp = lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_ਗੰਢ* nodep) { nodep->user1p(ਖਾਲੀ_ਸੂਚਕ); });
        Ast_ਗੰਢ_Var_Ref* ਅਚਲ newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        ਵਾਪਸ LhsProjection{newLhsp, newVarrefp, enp};
    }

    ਖਾਲੀ map_ਪਾਓ_Lhs_Var_Ref(Ast_ਗੰਢ_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ਅਚਲ ਕੁੰਜੀ = nodep->varp();
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_lhsmap.emplace(ਕੁੰਜੀ, ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ = ਨਵਾਂ RefStrengthVec;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* enLhsp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_Var_Ref* enVarrefp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* enRhsp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ enIt = m_ਸੌਂਪੋ_Enps.ਲੱਭੋ(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ਅਚਲ ਤਾਰਕਿਕ canCombineProcedural = !VN_IS(ਕੁੰਜੀ->dtypep()->skipRefp(), ਗੰਢ_UOr_Struct_DType);
        ਅਚਲ Ast_ਗੰਢ_Procedure* ਅਚਲ procedurep
            = enIt != m_ਸੌਂਪੋ_Enps.ਸਿਰਾ() && canCombineProcedural ? m_procedurep : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->ਦੂਜਾ->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.ਪਹਿਲਾ->ਦੂਜਾ->ਧੱਕੋ_ਪਿੱਠ(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newEnableDeposit(AstSel* selp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{
            selp->fileline(), ਨਵਾਂ AstExtend{selp->fileline(), enp, selp->fromp()->ਚੌੜਾਈ()},
            selp->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), selp->fromp()->ਚੌੜਾਈ()};
        ਵਾਪਸ newp;
    }

    ਖਾਲੀ ਸੈੱਟ_Pull_Direction(AstVar* varp, AstPull* pullp) {
        ਅਚਲ AstPull* ਅਚਲ oldpullp = m_varAux(varp).pullp;
        ਜੇ (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } ਨਹੀਂਤਾਂ {
            ਜੇ (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    ਖਾਲੀ ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pull_Direction(AstVar* varp, ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ, ਪੂਰਨ direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        ਸਵੈਚਾਲਿਤ& ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls = m_varAux(varp).ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls;
        ਸਵੈਚਾਲਿਤ it = ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls.ਲੱਭੋ(ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ);
        ਜੇ (it != ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls.ਸਿਰਾ() && it->ਦੂਜਾ != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ << " of " << varp->prettyNameQ());
        }
        ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls[ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੂਚਕਾਂਕ] = direction;
    }

    AstConst* create_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pull_Const(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ਅਚਲ ਸਵੈਚਾਲਿਤ& ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls = m_varAux(varp).ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls;
        V3_ਸੰਖਿਆ ਅੰਕ{varp, varp->ਚੌੜਾਈ(), 0};  // Start with all zeros
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls) {
            ਜੇ (pair.ਦੂਜਾ == 1) ਅੰਕ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(pair.ਪਹਿਲਾ, 1);
        }  // LCOV_EXCL_LINE
        ਵਾਪਸ ਨਵਾਂ AstConst{varp->fileline(), ਅੰਕ};
    }

    ਤਾਰਕਿਕ has_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        ਵਾਪਸ !m_varAux(varp).ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls.ਖਾਲੀ_ਥਾਵਾਂ();
    }

    ਖਾਲੀ checkUnhandled(Ast_ਗੰਢ* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        ਜੇ (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        ਜੇ ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    ਖਾਲੀ ਪਾਓ_Tristates(Ast_ਗੰਢ_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ਅਚਲ Tristate_ਗ੍ਰਾਫ::VarVec vars = m_tgraph.tristateVars();
        ਲਈ (ਸਵੈਚਾਲਿਤ varp : vars) {
            ਜੇ (!m_tgraph.isTristate(varp)) ਜਾਰੀ;
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_lhsmap.ਲੱਭੋ(varp);
            ਜੇ (it != m_lhsmap.ਸਿਰਾ()) ਜਾਰੀ;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ਅਚਲ AstPull* ਅਚਲ pullp = m_varAux(varp).pullp;
            ਅਚਲ ਤਾਰਕਿਕ pull_ਮੁੱਲ = pullp && pullp->direction() == 1;
            AstConst* ਅਚਲ constp = newAllZerosOrOnes(varp, pull_ਮੁੱਲ);
            AstVarRef* ਅਚਲ varrefp = ਨਵਾਂ AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            Ast_ਸੌਂਪੋ_W* ਅਚਲ newp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, ਝੂਠ));  // en=0 for floating/pullup vars
            nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{newp});
            map_ਪਾਓ_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        ਲਈ (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ਅਚਲ std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.ਲੱਭੋ(varp);
            ਜੇ (it == m_lhsmap.ਸਿਰਾ()) ਜਾਰੀ;
            AstVar* ਅਚਲ invarp = it->ਪਹਿਲਾ;
            RefStrengthVec* refsp = it->ਦੂਜਾ;
            // Figure out if this var needs tristate expanded.
            ਜੇ (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_ਗੰਢ_Module* ਅਚਲ ownerModp = ਲੱਭੋ_ਮਾਪਾ_Module(invarp);
                ਅਚਲ ਤਾਰਕਿਕ isCrossModule = ownerModp && ownerModp != nodep;
                ਅਚਲ ਤਾਰਕਿਕ isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                ਜੇ (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    ਲਈ (ਅਚਲ RefStrength& rs : *refsp) {
                        ਜੇ (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            ਰੋਕੋ;
                        }
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = ਸੱਚ;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    ਬਣਤਰ ਹਿੱਸੇ_ਕਰੋ_Info final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, ਹਿੱਸੇ_ਕਰੋ_Info> partitions;
                    ਲਈ (ਅਚਲ RefStrength& rs : *refsp) {
                        ਜੇ (AstVarXRef* ਅਚਲ xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            ਹਿੱਸੇ_ਕਰੋ_Info& pi = partitions[xrefp->dotted()];
                            pi.refs.ਧੱਕੋ_ਪਿੱਠ(rs);
                            ਜੇ (pi.inlinedDots.ਖਾਲੀ_ਥਾਵਾਂ()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } ਨਹੀਂਤਾਂ {
                            partitions[""].refs.ਧੱਕੋ_ਪਿੱਠ(rs);
                        }
                    }
                    ਲਈ (ਸਵੈਚਾਲਿਤ& kv : partitions) {
                        ਪਾਓ_Tristates_Signal(nodep, invarp, &kv.ਦੂਜਾ.refs, ਸੱਚ, kv.ਪਹਿਲਾ,
                                              kv.ਦੂਜਾ.inlinedDots,
                                              ਲੱਭੋ_Modport_For_Dotted(nodep, kv.ਪਹਿਲਾ));
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = ਸੱਚ;
                    ਪਾਓ_Tristates_Signal(nodep, invarp, refsp, ਸੱਚ, "", "", ਖਾਲੀ_ਸੂਚਕ);
                } ਨਹੀਂਤਾਂ {
                    ਪਾਓ_Tristates_Signal(nodep, invarp, refsp, ਝੂਠ, "", "", ਖਾਲੀ_ਸੂਚਕ);
                }
            } ਨਹੀਂਤਾਂ {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(ਮਿਟਾਓ refsp, refsp);
        }
    }

    ਖਾਲੀ aggregateTriSameStrength(Ast_ਗੰਢ_Module* nodep, AstVar* ਅਚਲ varp, AstVar* ਅਚਲ envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator ਸਿਰਾ_Strength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        ਬਣਤਰ Driver_ਸਮੂਹ final {
            ਅਚਲ Ast_ਗੰਢ_Procedure* m_procedurep;  // Process owning these assignments
            std::ਸਦਿਸ਼<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::ਸਦਿਸ਼<Driver_ਸਮੂਹ> drivers;
        std::map<ਅਚਲ Ast_ਗੰਢ_Procedure*, ਮਾਪ_t> procedure_ਮੰਜ਼ਿਲ_Driver;
        ਲਈ (ਸਵੈਚਾਲਿਤ it = beginStrength; it != ਸਿਰਾ_Strength; ++it) {
            ਜੇ (it->m_procedurep) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ emplaced = procedure_ਮੰਜ਼ਿਲ_Driver.emplace(it->m_procedurep, drivers.ਮਾਪ());
                ਜੇ (!emplaced.ਦੂਜਾ) {
                    drivers[emplaced.ਪਹਿਲਾ->ਦੂਜਾ].m_refsp.ਧੱਕੋ_ਪਿੱਠ(&*it);
                    ਜਾਰੀ;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ਧੱਕੋ_ਪਿੱਠ(Driver_ਸਮੂਹ{it->m_procedurep, {&*it}});
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* orp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* enp = ਖਾਲੀ_ਸੂਚਕ;

        ਲਈ (Driver_ਸਮੂਹ& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ਅਚਲ newLhsp = ਨਵਾਂ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvt_ਮੰਜ਼ਿਲ_Str(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->ਸੈੱਟ_Dfg_Tri_Lowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->ਜੋੜੋ_Stmtsp(newLhsp);
            AstVar* ਅਚਲ newEnLhsp
                = ਨਵਾਂ AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvt_ਮੰਜ਼ਿਲ_Str(m_unique++), envarp};  // 2-state ok
            newEnLhsp->ਸੈੱਟ_Dfg_Tri_Lowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->ਜੋੜੋ_Stmtsp(newEnLhsp);

            ਲਈ (RefStrength* ਅਚਲ refStrengthp : driver.m_refsp) {
                Ast_ਗੰਢ_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                ਜੇ (VN_IS(refp, VarXRef)) {
                    AstVarRef* ਅਚਲ localRefp
                        = ਨਵਾਂ AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(ਖਾਲੀ_ਸੂਚਕ);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } ਨਹੀਂਤਾਂ {
                    refp->varp(newLhsp);
                }

                ਜੇ (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* enLhsp = refStrengthp->m_enLhsp;
                    Ast_ਗੰਢ_Var_Ref* ਅਚਲ enVarrefp = refStrengthp->m_enVarrefp;
                    ਜੇ (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ਅਚਲ localRefp
                            = ਨਵਾਂ AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        ਜੇ (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_ਰੁੱਖ(), enVarrefp);
                        } ਨਹੀਂਤਾਂ {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ਧੱਕੋ_Deletep(enVarrefp), enVarrefp);
                        }
                    } ਨਹੀਂਤਾਂ {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ਧੱਕੋ_ਪਿੱਠ(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = ਖਾਲੀ_ਸੂਚਕ;
                    refStrengthp->m_enVarrefp = ਖਾਲੀ_ਸੂਚਕ;
                    refStrengthp->m_enRhsp = ਖਾਲੀ_ਸੂਚਕ;
                } ਨਹੀਂਤਾਂ {
                    Ast_ਸੌਂਪੋ_W* ਅਚਲ enLhspAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{
                        refp->fileline(),
                        ਨਵਾਂ AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    ਜੇ (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_ਰੁੱਖ(ਝੂਠ));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_ਸਤਰ* ਅਚਲ fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ref1p = ਨਵਾਂ AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ref2p = ਨਵਾਂ AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ andp = ਨਵਾਂ AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : ਨਵਾਂ AstOr{fl, orp, andp};

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ref3p = ਨਵਾਂ AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : ਨਵਾਂ AstOr{ref3p->fileline(), enp, ref3p};
        }
        Ast_ਸੌਂਪੋ_W* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{
            varp->fileline(), ਨਵਾਂ AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{assp});

        Ast_ਸੌਂਪੋ_W* ਅਚਲ enAssp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{
            envarp->fileline(), ਨਵਾਂ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{enAssp});
    }

    ਖਾਲੀ ਪਾਓ_Procedural_Enable_Assignments(Ast_ਗੰਢ_Module* nodep) {
        ਲਈ (ਸਵੈਚਾਲਿਤ& ਸੌਂਪੋ_Enables : m_proceduralEnables) {
            Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = ਸੌਂਪੋ_Enables.ਪਹਿਲਾ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* combinedLhsp = assignp->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* combinedRhsp = assignp->rhsp()->unlink_Fr_ਪਿੱਠ();
            ਲਈ (ProceduralEnable& enable : ਸੌਂਪੋ_Enables.ਦੂਜਾ) {
                combinedLhsp = ਨਵਾਂ AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = ਨਵਾਂ AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            ਜੇ (!VN_IS(assignp, ਸੌਂਪੋ_Dly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ਅਚਲ sampledp
                    = ਨਵਾਂ AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvt_ਮੰਜ਼ਿਲ_Str(m_unique++), combinedRhsp->dtypep()};
                nodep->ਜੋੜੋ_Stmtsp(sampledp);
                assignp->lhsp(ਨਵਾਂ AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                Ast_ਸੌਂਪੋ* ਅਚਲ applyp
                    = ਨਵਾਂ Ast_ਸੌਂਪੋ{assignp->fileline(), combinedLhsp,
                                    ਨਵਾਂ AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(applyp);
            } ਨਹੀਂਤਾਂ {
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
    ਖਾਲੀ ਪਾਓ_Tristates_Signal(Ast_ਗੰਢ_Module* nodep, AstVar* ਅਚਲ invarp, RefStrengthVec* refsp,
                               ਤਾਰਕਿਕ isIfaceTri, ਅਚਲ string& iface_Dotted_ਰਸਤਾ,
                               ਅਚਲ string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        AstVar* envarp = ਖਾਲੀ_ਸੂਚਕ;
        AstVar* outvarp = ਖਾਲੀ_ਸੂਚਕ;  // __out
        AstVar* lhsp = invarp;  // Variable to assign drive-value to (<in> or __out)
        ਅਚਲ ਤਾਰਕਿਕ is_ਸਿਖਰ_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_ਸਿਖਰ();
        ਜੇ (!isIfaceTri) {
            ਜੇ (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ())
                 && is_ਸਿਖਰ_Inout)
                || (!nodep->is_ਸਿਖਰ() && invarp->isIO())) {
                // This var becomes an input
                invarp->var_Type2_ਵਿੱਚ();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = get_Create_ਬਾਹਰਲਾ_Varp(invarp, is_ਸਿਖਰ_Inout);
                outvarp->var_Type2_ਬਾਹਰਲਾ();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_ਸਿਖਰ_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls
                    = m_varAux(invarp).ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls;  // Per-bit pull propagation
                ਜੇ (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                ਜੇ (!m_varAux(invarp).ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* orp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* enp = ਖਾਲੀ_ਸੂਚਕ;

        std::ਛਾਂਟੋ(refsp->begin(), refsp->ਸਿਰਾ(),
                  [](RefStrength a, RefStrength b) { ਵਾਪਸ a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        ਜਦੋਂ (beginStrength != refsp->ਸਿਰਾ()) {
            RefStrengthVec::iterator ਸਿਰਾ_Strength = beginStrength + 1;
            ਜਦੋਂ (ਸਿਰਾ_Strength != refsp->ਸਿਰਾ()
                   && ਸਿਰਾ_Strength->m_strength == beginStrength->m_strength)
                ਸਿਰਾ_Strength++;

            File_ਸਤਰ* ਅਚਲ fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strength_ਅਗੇਤਰ;
            ਜੇ (isIfaceTri && !iface_Dotted_ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                strength_ਅਗੇਤਰ = iface_Dotted_ਰਸਤਾ;
                std::replace(strength_ਅਗੇਤਰ.begin(), strength_ਅਗੇਤਰ.ਸਿਰਾ(), '.', '_');
                strength_ਅਗੇਤਰ += "__";
            }
            ਅਚਲ string strengthVarName
                = strength_ਅਗੇਤਰ + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ਅਚਲ varStrengthp = ਨਵਾਂ AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->ਸੈੱਟ_Dfg_Tri_Lowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->ਜੋੜੋ_Stmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ਅਚਲ enVarStrengthp = ਨਵਾਂ AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->ਸੈੱਟ_Dfg_Tri_Lowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->ਜੋੜੋ_Stmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     ਸਿਰਾ_Strength);

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿਅੰਜਕ_Current_Strengthp;
            ਜੇ (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                ਵਿਅੰਜਕ_Current_Strengthp
                    = ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstVarRef{fl, varStrengthp, VAccess::READ},
                                 ਨਵਾਂ AstNot{fl, enp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}};
            } ਨਹੀਂਤਾਂ {
                ਵਿਅੰਜਕ_Current_Strengthp = ਨਵਾਂ AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? ਵਿਅੰਜਕ_Current_Strengthp : ਨਵਾਂ AstOr{fl, orp, ਵਿਅੰਜਕ_Current_Strengthp};

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enVarStrengthRefp
                = ਨਵਾਂ AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : ਨਵਾਂ AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = ਸਿਰਾ_Strength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        ਜੇ (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->ਸੈੱਟ_Dfg_Allow_Multidrive_Tri();
            Ast_ਗੰਢ_Module* ਅਚਲ ifaceModp = ਲੱਭੋ_ਮਾਪਾ_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ਅਚਲ ਪੂਰਨ contribIdx = static_cast<ਪੂਰਨ>(m_ifaceContribs[invarp].ਮਾਪ());
            File_ਸਤਰ* ਅਚਲ fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ਅਚਲ contribOutp = ਨਵਾਂ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvt_ਮੰਜ਼ਿਲ_Str(contribIdx), invarp};
            contribOutp->ਸੈੱਟ_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->ਜੋੜੋ_Stmtsp(contribOutp);

            AstVar* ਅਚਲ contribEnp = ਨਵਾਂ AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvt_ਮੰਜ਼ਿਲ_Str(contribIdx), invarp};
            contribEnp->ਸੈੱਟ_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->ਜੋੜੋ_Stmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            ਜੇ (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                ਜੋੜੋ_Modport_Var_Ref_If_Missing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                ਜੋੜੋ_Modport_Var_Ref_If_Missing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_ਗੰਢ_Var_Ref* ਅਚਲ lhsp
                    = iface_Dotted_ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ() ? static_cast<Ast_ਗੰਢ_Var_Ref*>(
                                                    ਨਵਾਂ AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_ਗੰਢ_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, iface_Dotted_ਰਸਤਾ,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_ਸੌਂਪੋ_W* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{assp});
            }
            {
                Ast_ਗੰਢ_Var_Ref* ਅਚਲ lhsp
                    = iface_Dotted_ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ() ? static_cast<Ast_ਗੰਢ_Var_Ref*>(
                                                    ਨਵਾਂ AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_ਗੰਢ_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, iface_Dotted_ਰਸਤਾ,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_ਸੌਂਪੋ_W* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{assp});
            }

            m_ifaceContribs[invarp].ਧੱਕੋ_ਪਿੱਠ({contribEnp, contribOutp});
            ਵਾਪਸ;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ਅਚਲ ਤਾਰਕਿਕ hasPull = m_varAux(lhsp).pullp || has_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls(lhsp);
        ਜੇ (!outvarp || hasPull) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* undrivenp;
            ਜੇ (envarp) {
                undrivenp = ਨਵਾਂ AstNot{envarp->fileline(),
                                       ਨਵਾਂ AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } ਨਹੀਂਤਾਂ ਜੇ (enp) {
                undrivenp = ਨਵਾਂ AstNot{enp->fileline(), enp};
                enp = ਖਾਲੀ_ਸੂਚਕ;  // moved into undrivenp
            } ਨਹੀਂਤਾਂ {
                undrivenp = newAllZerosOrOnes(invarp, ਸੱਚ);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            ਜੇ (has_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls(lhsp)) {
                pullConstp = create_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pull_Const(lhsp);
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ AstPull* ਅਚਲ pullp = m_varAux(lhsp).pullp;
                ਅਚਲ ਤਾਰਕਿਕ pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = ਨਵਾਂ AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? ਨਵਾਂ AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        ਜੇ (envarp && !enp) enp = newAllZerosOrOnes(invarp, ਝੂਠ);

        ਜੇ (envarp) {
            Ast_ਸੌਂਪੋ_W* ਅਚਲ enAssp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{
                enp->fileline(), ਨਵਾਂ AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        Ast_ਸੌਂਪੋ_W* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{
            lhsp->fileline(), ਨਵਾਂ AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        ਜੇ ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) && is_ਸਿਖਰ_Inout) {
            ਜੇ (envarp) {
                envarp->primaryIO(ਸੱਚ);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            ਜੇ (outvarp) {
                outvarp->primaryIO(ਸੱਚ);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
                ਅਚਲ ਪੂਰਨ id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(ਸੱਚ);
            }
        }
    }

    ਤਾਰਕਿਕ is_Only_Assignment_Is_ਮੰਜ਼ਿਲ_Lhs_Var(ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ nodep) {
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            ਸਵੈਚਾਲਿਤ ਮਿਲਿਆ_It = m_assigns.ਲੱਭੋ(varRefp->varp());
            ਜੇ (ਮਿਲਿਆ_It != m_assigns.ਸਿਰਾ()) {
                ਸਵੈਚਾਲਿਤ ਅਚਲ& assigns_ਮੰਜ਼ਿਲ_Var = ਮਿਲਿਆ_It->ਦੂਜਾ;
                ਜੇ (assigns_ਮੰਜ਼ਿਲ_Var.ਮਾਪ() == 1 && assigns_ਮੰਜ਼ਿਲ_Var[0] == nodep) ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ ਜੋੜੋ_ਮੰਜ਼ਿਲ_Assignment_List(Ast_ਸੌਂਪੋ_W* nodep) {
        ਜੇ (AstVarRef* ਅਚਲ varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            ਜੇ (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ਧੱਕੋ_ਪਿੱਠ(nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ nodep, ਤਾਰਕਿਕ ਮੁੱਲ) {
        ਜੇ (AstStrengthSpec* ਅਚਲ strengthSpec = nodep->strengthSpecp()) {
            ਵਾਪਸ ਮੁੱਲ ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        ਵਾਪਸ VStrength::STRONG;  // default strength is strong
    }

    ਤਾਰਕਿਕ assignment_ਦਾ_ਮੁੱਲ_On_All_ਬਿੱਟ(ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ nodep, ਤਾਰਕਿਕ ਮੁੱਲ) {
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->rhsp(), Const)) {
            ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ = constp->ਅੰਕ();
            ਵਾਪਸ ਮੁੱਲ ? ਅੰਕ.isEqAllOnes() : ਅੰਕ.isEqZero();
        }
        ਵਾਪਸ ਝੂਠ;
    }

    Ast_ਸੌਂਪੋ_W* get_Strongest_Assignment_ਦਾ_ਮੁੱਲ(ਅਚਲ Assigns& assigns, ਤਾਰਕਿਕ ਮੁੱਲ) {
        ਸਵੈਚਾਲਿਤ maxIt = std::max_element(
            assigns.begin(), assigns.ਸਿਰਾ(), [&](ਅਚਲ Ast_ਸੌਂਪੋ_W* ap, ਅਚਲ Ast_ਸੌਂਪੋ_W* bp) {
                ਅਚਲ ਤਾਰਕਿਕ ਮੁੱਲਾਂ_On_Rhs_A = assignment_ਦਾ_ਮੁੱਲ_On_All_ਬਿੱਟ(ap, ਮੁੱਲ);
                ਅਚਲ ਤਾਰਕਿਕ ਮੁੱਲਾਂ_On_Rhs_B = assignment_ਦਾ_ਮੁੱਲ_On_All_ਬਿੱਟ(bp, ਮੁੱਲ);
                ਜੇ (!ਮੁੱਲਾਂ_On_Rhs_A) ਵਾਪਸ ਮੁੱਲਾਂ_On_Rhs_B;
                ਜੇ (!ਮੁੱਲਾਂ_On_Rhs_B) ਵਾਪਸ ਝੂਠ;
                ਵਾਪਸ getStrength(ap, ਮੁੱਲ) < getStrength(bp, ਮੁੱਲ);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        ਵਾਪਸ assignment_ਦਾ_ਮੁੱਲ_On_All_ਬਿੱਟ(*maxIt, ਮੁੱਲ) ? *maxIt : ਖਾਲੀ_ਸੂਚਕ;
    }

    ਤਾਰਕਿਕ isAssignmentNotStrongerThanStrength(ਅਚਲ Ast_ਸੌਂਪੋ_W* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ਅਚਲ uint8_t strength0 = getStrength(assignp, 0);
        ਅਚਲ uint8_t strength1 = getStrength(assignp, 1);
        ਵਾਪਸ (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_ਦਾ_ਮੁੱਲ_On_All_ਬਿੱਟ(assignp, 0))
               || (strength1 <= strength && assignment_ਦਾ_ਮੁੱਲ_On_All_ਬਿੱਟ(assignp, 1));
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਮਿਲਾਓ_ਵਿਅੰਜਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp, File_ਸਤਰ* ਅਚਲ fl,
                              ਤਾਰਕਿਕ isWor) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿਅੰਜਕ = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (isWor)
            ਵਿਅੰਜਕ = ਨਵਾਂ AstOr{fl, lhsp, rhsp};
        ਨਹੀਂਤਾਂ
            ਵਿਅੰਜਕ = ਨਵਾਂ AstAnd{fl, lhsp, rhsp};
        ਵਾਪਸ ਵਿਅੰਜਕ;
    }

    ਖਾਲੀ ਮਿਲਾਓ_Wired_Nets_Assignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        ਲਈ (ਸਵੈਚਾਲਿਤ& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ਦੂਜਾ;
            ਜੇ (assigns.ਮਾਪ() <= 1) ਜਾਰੀ;
            AstVar* ਅਚਲ varp = varpAssigns.ਪਹਿਲਾ;
            ਜੇ (!varp->isWiredNet()) ਜਾਰੀ;
            ਸਵੈਚਾਲਿਤ it = assigns.begin();
            Ast_ਸੌਂਪੋ_W* ਅਚਲ ਸੌਂਪੋ_Wp0 = *it;
            File_ਸਤਰ* ਅਚਲ fl = ਸੌਂਪੋ_Wp0->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* wExp = ਖਾਲੀ_ਸੂਚਕ;
            ਜਦੋਂ (++it != assigns.ਸਿਰਾ()) {
                Ast_ਸੌਂਪੋ_W* ਸੌਂਪੋ_Wpi = *it;
                ਜੇ (!wExp) {
                    wExp
                        = new_ਮਿਲਾਓ_ਵਿਅੰਜਕ(ਸੌਂਪੋ_Wp0->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                       ਸੌਂਪੋ_Wpi->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), fl, varp->isWor());
                } ਨਹੀਂਤਾਂ {
                    wExp = new_ਮਿਲਾਓ_ਵਿਅੰਜਕ(wExp, ਸੌਂਪੋ_Wpi->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((ਸੌਂਪੋ_Wpi->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ()), ਸੌਂਪੋ_Wpi);
            }
            AstVarRef* ਅਚਲ wVarRef = ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE};
            Ast_ਸੌਂਪੋ_W* ਅਚਲ wAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{fl, wVarRef, wExp};
            ਸੌਂਪੋ_Wp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਸੌਂਪੋ_Wp0), ਸੌਂਪੋ_Wp0);
            assigns.clear();
            assigns.ਧੱਕੋ_ਪਿੱਠ(wAssignp);
        }
    }

    ਖਾਲੀ removeNotStrongerAssignments(Assigns& assigns, ਅਚਲ Ast_ਸੌਂਪੋ_W* strongestp,
                                      uint8_t greatest_ਗਿਆਤ_Strength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        ਸਵੈਚਾਲਿਤ removedIt = std::remove_if(assigns.begin(), assigns.ਸਿਰਾ(), [&](Ast_ਸੌਂਪੋ_W* assignp) {
            ਜੇ (assignp == strongestp) ਵਾਪਸ ਝੂਠ;
            ਜੇ (isAssignmentNotStrongerThanStrength(assignp, greatest_ਗਿਆਤ_Strength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(assignp->unlink_Fr_ਪਿੱਠ()), assignp);
                ਵਾਪਸ ਸੱਚ;
            }
            ਵਾਪਸ ਝੂਠ;
        });
        assigns.erase(removedIt, assigns.ਸਿਰਾ());
    }

    ਖਾਲੀ remove_Assignments_Not_Stronger_Than_ਇਕਸਾਰ_Constant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        ਲਈ (ਸਵੈਚਾਲਿਤ& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ਦੂਜਾ;
            ਜੇ (assigns.ਮਾਪ() <= 1) ਜਾਰੀ;
            ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ strongest0p = get_Strongest_Assignment_ਦਾ_ਮੁੱਲ(assigns, 0);
            ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ strongest1p = get_Strongest_Assignment_ਦਾ_ਮੁੱਲ(assigns, 1);
            ਅਚਲ Ast_ਸੌਂਪੋ_W* strongestp = ਖਾਲੀ_ਸੂਚਕ;
            uint8_t greatest_ਗਿਆਤ_Strength = 0;
            ਅਚਲ ਸਵੈਚਾਲਿਤ getIfStrongest
                = [&](ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ strongestCandidatep, ਤਾਰਕਿਕ ਮੁੱਲ) {
                      ਜੇ (!strongestCandidatep) ਵਾਪਸ;
                      ਅਚਲ uint8_t strength = getStrength(strongestCandidatep, ਮੁੱਲ);
                      ਜੇ (strength >= greatest_ਗਿਆਤ_Strength) {
                          greatest_ਗਿਆਤ_Strength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            ਜੇ (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatest_ਗਿਆਤ_Strength);
            }
        }
    }

    ਖਾਲੀ removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        ਲਈ (ਸਵੈਚਾਲਿਤ& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ਦੂਜਾ;
            ਜੇ (assigns.ਮਾਪ() <= 1) ਜਾਰੀ;
            ਸਵੈਚਾਲਿਤ maxIt = std::max_element(
                assigns.begin(), assigns.ਸਿਰਾ(), [&](ਅਚਲ Ast_ਸੌਂਪੋ_W* ap, ਅਚਲ Ast_ਸੌਂਪੋ_W* bp) {
                    ਜੇ (m_tgraph.isTristate(ap)) ਵਾਪਸ !m_tgraph.isTristate(bp);
                    ਜੇ (m_tgraph.isTristate(bp)) ਵਾਪਸ ਝੂਠ;
                    ਅਚਲ uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ਅਚਲ uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    ਵਾਪਸ minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ strongestp = m_tgraph.isTristate(*maxIt) ? ਖਾਲੀ_ਸੂਚਕ : *maxIt;
            ਜੇ (strongestp) {
                ਅਚਲ uint8_t greatest_ਗਿਆਤ_Strength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatest_ਗਿਆਤ_Strength);
            }
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        ਜੇ (m_graphing) {
            ਜੇ (!m_alhs && nodep->ਅੰਕ().hasZ()) m_tgraph.ਸੈੱਟ_Tristate(nodep);
        } ਨਹੀਂਤਾਂ {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            ਜੇ (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ਅਚਲ varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                AstConst* ਅਚਲ enp = getNonZConstp(nodep);
                V3_ਸੰਖਿਆ num1{nodep, nodep->ਚੌੜਾਈ()};
                num1.opAnd(nodep->ਅੰਕ(), enp->ਅੰਕ());  // 01X->01X, Z->0
                AstConst* ਅਚਲ newconstp = ਨਵਾਂ AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        ਜੇ (m_graphing) {
            iterateChildren(nodep);
            ਜੇ (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } ਨਹੀਂਤਾਂ {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ਵਾਪਸ;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = nodep->condp();
            ਜੇ (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thenp = nodep->thenp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elsep = nodep->elsep();
            ਜੇ (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ en1p = getEnp(thenp);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enp
                    = ਨਵਾਂ AstCond{nodep->fileline(), condp->clone_ਰੁੱਖ(ਝੂਠ), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                ਜੇ (thenp->user1p() && !thenp->user1p()->backp()) ਧੱਕੋ_Deletep(thenp->user1p());
                ਜੇ (elsep->user1p() && !elsep->user1p()->backp()) ਧੱਕੋ_Deletep(elsep->user1p());
                thenp->user1p(ਖਾਲੀ_ਸੂਚਕ);
                elsep->user1p(ਖਾਲੀ_ਸੂਚਕ);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(ਖਾਲੀ_ਸੂਚਕ);
            m_tgraph.didProcess(nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        ਜੇ (m_graphing) {
            iterateChildren(nodep);
            ਜੇ (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } ਨਹੀਂਤਾਂ {
                associateLogic(nodep->fromp(), nodep);
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ਜੇ (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), ਗੰਢ_ਵਿਅੰਜਕ));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ਨਹੀਂਤਾਂ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                ਜੇ (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                ਜੇ (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ en1p = getEnp(nodep->fromp());
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enp
                        = ਨਵਾਂ AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                     nodep->ਚੌੜਾਈ_Const()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstConcat* nodep) override {
        ਜੇ (m_graphing) {
            iterateChildren(nodep);
            ਜੇ (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } ਨਹੀਂਤਾਂ {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_alhs) {
                UINFO(9, dbgState() << nodep);
                ਜੇ (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enp = VN_AS(nodep->user1p(), ਗੰਢ_ਵਿਅੰਜਕ);
                    nodep->user1p(ਖਾਲੀ_ਸੂਚਕ);
                    nodep->lhsp()->user1p(ਨਵਾਂ AstSel{nodep->fileline(), enp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                                     nodep->rhsp()->ਚੌੜਾਈ(),
                                                     nodep->lhsp()->ਚੌੜਾਈ()});
                    nodep->rhsp()->user1p(
                        ਨਵਾਂ AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->ਚੌੜਾਈ()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ਨਹੀਂਤਾਂ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p = nodep->lhsp();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr2p = nodep->rhsp();
                ਜੇ (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ en1p = getEnp(expr1p);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ en2p = getEnp(expr2p);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enp = ਨਵਾਂ AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(ਖਾਲੀ_ਸੂਚਕ);
                    expr2p->user1p(ਖਾਲੀ_ਸੂਚਕ);
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ਜੇ (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.ਸੈੱਟ_Tristate(nodep);
        } ਨਹੀਂਤਾਂ {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            ਜੇ (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ਵਾਪਸ;
            }
            m_tgraph.didProcess(nodep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr2p = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* enp;
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ en2p = VN_AS(expr2p->user1p(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                enp = ਨਵਾਂ AstAnd{nodep->fileline(), expr1p, en2p};
            } ਨਹੀਂਤਾਂ {
                enp = expr1p;
            }
            expr1p->user1p(ਖਾਲੀ_ਸੂਚਕ);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ_And_Or(Ast_ਗੰਢ_Biop* nodep, ਤਾਰਕਿਕ isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ਜੇ (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                ਵਾਪਸ;
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
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p = nodep->lhsp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr2p = nodep->rhsp();
            ਜੇ (!expr1p->user1p() && !expr2p->user1p()) {
                ਵਾਪਸ;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* en1p = getEnp(expr1p);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* en2p = getEnp(expr2p);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* subexpr1p = expr1p->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* subexpr2p = expr2p->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            ਜੇ (isAnd) {
                subexpr1p = ਨਵਾਂ AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = ਨਵਾਂ AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            ਜੇ (!en1p->op1p() && en2p->op1p()) {
                std::ਅਦਲਾ_ਬਦਲੀ(en1p, en2p);
                std::ਅਦਲਾ_ਬਦਲੀ(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enp = ਨਵਾਂ AstOr{
                nodep->fileline(),
                ਨਵਾਂ AstAnd{nodep->fileline(), en1p, ਨਵਾਂ AstOr{nodep->fileline(), en2p, subexpr1p}},
                ਨਵਾਂ AstAnd{nodep->fileline(), en2p->clone_ਰੁੱਖ(ਝੂਠ), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(ਖਾਲੀ_ਸੂਚਕ);
            expr2p->user1p(ਖਾਲੀ_ਸੂਚਕ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstAnd* nodep) override { ਦੇਖੋ_And_Or(nodep, ਸੱਚ); }
    ਖਾਲੀ ਦੇਖੋ(AstOr* nodep) override { ਦੇਖੋ_And_Or(nodep, ਝੂਠ); }

    ਖਾਲੀ ਦੇਖੋ_ਸੌਂਪੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        ਜੇ (VN_IS(nodep->rhsp(), CReset)) ਵਾਪਸ;
        ਜੇ (Ast_ਸੌਂਪੋ_W* ਅਚਲ ਸੌਂਪੋ_Wp = VN_CAST(nodep, ਸੌਂਪੋ_W)) {
            m_currentDelayp = VN_CAST(ਸੌਂਪੋ_Wp->timingControlp(), Delay);
        }
        ਜੇ (m_graphing) {
            ਜੇ (Ast_ਸੌਂਪੋ_W* ਸੌਂਪੋ_Wp = VN_CAST(nodep, ਸੌਂਪੋ_W)) {
                ਜੇ (ਸੌਂਪੋ_Wp->getLhsNetDelay()) ਵਾਪਸ;
                ਜੇ (ਸੌਂਪੋ_Wp->timingControlp() && !m_currentDelayp) ਵਾਪਸ;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                ਜੇ (m_currentDelayp && m_currentDelayp->fallDelay()) ਵਾਪਸ;
                ਜੇ (!ਸੌਂਪੋ_Wp->timingControlp()) ਜੋੜੋ_ਮੰਜ਼ਿਲ_Assignment_List(ਸੌਂਪੋ_Wp);
            }

            ਜੇ (nodep->user2() & U2_GRAPHING) ਵਾਪਸ;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
            m_alhs = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
            m_alhs = ਝੂਠ;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->user2() & U2_NONGRAPH) {
                ਵਾਪਸ;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            ਜੇ (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, ਸੌਂਪੋ_W), nodep,
                            "Continuous assignment inside procedural process");
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), ਗੰਢ_ਵਿਅੰਜਕ)->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)
                          : newAllZerosOrOnes(nodep->lhsp(), ਸੱਚ);
                ਅਚਲ ਤਾਰਕਿਕ inserted = m_ਸੌਂਪੋ_Enps.emplace(nodep, enp).ਦੂਜਾ;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            ਜੇ (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(ਖਾਲੀ_ਸੂਚਕ);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } ਨਹੀਂਤਾਂ {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(nodep->lhsp(), Sel)) {
                    ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(selp->fromp(), ਗੰਢ_Var_Ref)) {
                        ਜੇ (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), ਸੱਚ));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } ਨਹੀਂਤਾਂ {
                            m_tgraph.didProcess(nodep, ਸੱਚ);
                        }
                    } ਨਹੀਂਤਾਂ {
                        m_tgraph.didProcess(nodep, ਸੱਚ);  // LCOV_EXCL_LINE
                    }
                } ਨਹੀਂਤਾਂ {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, ਸੱਚ);
                }
            }
            m_alhs = ਸੱਚ;  // And user1p() will indicate tristate equation, if any
            ਜੇ (Ast_ਸੌਂਪੋ_W* ਅਚਲ ਸੌਂਪੋ_Wp = VN_CAST(nodep, ਸੌਂਪੋ_W)) {
                ਜੇ (AstStrengthSpec* ਅਚਲ specp = ਸੌਂਪੋ_Wp->strengthSpecp()) {
                    ਜੇ (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        ਜੇ (!is_Only_Assignment_Is_ਮੰਜ਼ਿਲ_Lhs_Var(ਸੌਂਪੋ_Wp)) {
                            ਸੌਂਪੋ_Wp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } ਨਹੀਂਤਾਂ {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override { ਦੇਖੋ_ਸੌਂਪੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override { ਦੇਖੋ_ਸੌਂਪੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override { ਦੇਖੋ_ਸੌਂਪੋ(nodep); }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ਅਚਲ AstAlways* ਅਚਲ alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_ਸੌਂਪੋ ? ਖਾਲੀ_ਸੂਚਕ : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_ਵਿੱਚ_Alias);
        m_ਵਿੱਚ_Alias = ਸੱਚ;
        ਜੇ (m_graphing) {
            ਜੇ (nodep->user2() & U2_GRAPHING) ਵਾਪਸ;
            m_alhs = ਸੱਚ;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            ਲਈ (Ast_ਗੰਢ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ_Case_Eq(Ast_ਗੰਢ_Biop* nodep, ਤਾਰਕਿਕ neq) {
        ਜੇ (m_graphing) {
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ਅਚਲ constp = VN_CAST(nodep->lhsp(), Const);
            ਜੇ (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp();
                rhsp->unlink_Fr_ਪਿੱਠ();
                File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* enRhsp;
                ਜੇ (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), ਗੰਢ_ਵਿਅੰਜਕ);
                    rhsp->user1p(ਖਾਲੀ_ਸੂਚਕ);
                } ਨਹੀਂਤਾਂ {
                    enRhsp = get_En_ਵਿਅੰਜਕ_Based_On_Originalp(rhsp);
                }
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp
                    = ਨਵਾਂ AstLogAnd{fl, ਨਵਾਂ AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    ਨਵਾਂ AstEqCase{fl, ਨਵਾਂ AstConst{fl, constp->ਅੰਕ()}, rhsp}};
                constp->user1p(ਖਾਲੀ_ਸੂਚਕ);
                ਜੇ (neq) newp = ਨਵਾਂ AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (constp && nodep->rhsp()->user1p()) {
                File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                constp->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਨਵਾਂ AstLogAnd{fl,
                                                  ਨਵਾਂ AstEq{fl, newAllZerosOrOnes(constp, ਝੂਠ),
                                                            VN_AS(rhsp->user1p(), ਗੰਢ_ਵਿਅੰਜਕ)},
                                                  // Keep the caseeq if there are X's present
                                                  ਨਵਾਂ AstEqCase{fl, constp, rhsp}};
                ਜੇ (neq) newp = ਨਵਾਂ AstLogNot{fl, newp};
                rhsp->user1p(ਖਾਲੀ_ਸੂਚਕ);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ {
                checkUnhandled(nodep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ_Eq_Neq_Wild(Ast_ਗੰਢ_Biop* nodep) {
        ਜੇ (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        ਜੇ (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            ਵਾਪਸ;
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstEqCase* nodep) override { ਦੇਖੋ_Case_Eq(nodep, ਝੂਠ); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqCase* nodep) override { ਦੇਖੋ_Case_Eq(nodep, ਸੱਚ); }
    ਖਾਲੀ ਦੇਖੋ(AstEqWild* nodep) override { ਦੇਖੋ_Eq_Neq_Wild(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqWild* nodep) override { ਦੇਖੋ_Eq_Neq_Wild(nodep); }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗਿਣਤੀ_ਬਿੱਟ* nodep) override {
        std::array<ਤਾਰਕਿਕ, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->ਅੰਕ().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->ਅੰਕ().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->ਅੰਕ().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        ਜੇ (m_graphing) {
            iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
            ਜੇ (!dropop[0]) iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
            ਜੇ (!dropop[1]) iterate_And_ਅਗਲਾ_Null(nodep->thsp());
            ਜੇ (!dropop[2]) iterate_And_ਅਗਲਾ_Null(nodep->fhsp());
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* nonXp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!dropop[0]) {
                nonXp = nodep->rhsp();
            } ਨਹੀਂਤਾਂ ਜੇ (!dropop[1]) {
                nonXp = nodep->thsp();
            } ਨਹੀਂਤਾਂ ਜੇ (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            ਜੇ (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ਅਚਲ AstVarRef* ਅਚਲ varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                ਜੇ (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    ਵਾਪਸ;
                }
                AstVar* ਅਚਲ envarp = getCreateEnVarp(varrefp->varp(), ਝੂਠ);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlink_Fr_ਪਿੱਠ(&relinkHandle);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਨਵਾਂ Ast_ਗਿਣਤੀ_Ones{
                    nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                ਜੇ (nonXp) {  // Need to still count '0 or '1 or 'x's
                    ਜੇ (dropop[0]) {
                        nodep->rhsp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                        nodep->rhsp(nonXp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
                    }
                    ਜੇ (dropop[1]) {
                        nodep->thsp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                        nodep->thsp(nonXp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
                    }
                    ਜੇ (dropop[2]) {
                        nodep->fhsp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                        nodep->fhsp(nonXp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
                    }
                    newp = ਨਵਾਂ Ast_ਜੋੜੋ{nodep->fileline(), nodep, newp};
                } ਨਹੀਂਤਾਂ {
                    ਧੱਕੋ_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        ਜੇ (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.ਸੈੱਟ_Tristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } ਨਹੀਂਤਾਂ {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(nodep->lhsp(), Sel);
                ਅਚਲ AstConst* ਅਚਲ lsbp = selp ? VN_CAST(selp->lsbp(), Const) : ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (lsbp) {
                    ਅਚਲ ਪੂਰਨ lsb = lsbp->ਮੰਜ਼ਿਲ_SInt();
                    ਲਈ (ਪੂਰਨ i = 0; i < selp->ਚੌੜਾਈ_Const(); ++i) {
                        ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pull_Direction(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } ਨਹੀਂਤਾਂ {
                    ਸੈੱਟ_Pull_Direction(varrefp->varp(), nodep);
                }
            }
        }
        ਜੇ (!m_graphing) {
            nodep->unlink_Fr_ਪਿੱਠ();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    ਖਾਲੀ iteratePinGuts(AstPin* nodep) {
        ਜੇ (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            ਜੇ (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            // All heavy lifting completed in graph visitor.
            ਜੇ (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {
        ਜੇ (m_graphing) {
            ਜੇ (nodep->user2() & U2_GRAPHING) ਵਾਪਸ;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ਅਚਲ enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            ਜੇ (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, ਝੂਠ);
                iteratePinGuts(nodep);
                ਵਾਪਸ;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Decl_Processing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::ਨਿਵੇਸ਼
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            ਜੇ (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ਅਚਲ ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(ਨਵਾਂ AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.ਸੈੱਟ_Tristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Decl_Processing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, ਸੱਚ, ਸੱਚ);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.ਸੈੱਟ_Tristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ਅਚਲ enVarp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvt_ਮੰਜ਼ਿਲ_Str(m_unique++),
                                              VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, enModVarp->ਚੌੜਾਈ()};
            ਜੇ (ਵਿੱਚ_Decl_Processing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->var_Type2_ਵਿੱਚ();
            } ਨਹੀਂਤਾਂ {
                enModVarp->var_Type2_ਬਾਹਰਲਾ();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ਅਚਲ enpinp
                = ਨਵਾਂ AstPin{nodep->fileline(), nodep->pin_ਅੰਕ(),
                             enModVarp->name(),  // should be {var}"__en"
                             ਨਵਾਂ AstVarRef{nodep->fileline(), enVarp,
                                           ਵਿੱਚ_Decl_Processing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(enpinp);
            m_modp->ਜੋੜੋ_Stmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ਅਚਲ Ast_ਸੌਂਪੋ_W* ਬਾਹਰਲਾ_Assignp = ਖਾਲੀ_ਸੂਚਕ;  // If reconnected, the related assignment
            AstPin* outpinp = ਖਾਲੀ_ਸੂਚਕ;
            AstVar* ਅਚਲ ਬਾਹਰਲਾ_Mod_Varp = m_varAux(nodep->modVarp()).ਬਾਹਰਲਾ_Varp;
            ਜੇ (!ਬਾਹਰਲਾ_Mod_Varp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                ਜੇ (!m_modp->is_ਸਿਖਰ()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } ਨਹੀਂਤਾਂ {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ outexprp = VN_AS(nodep->exprp(), ਗੰਢ_ਵਿਅੰਜਕ)
                                                  ->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);  // Note has lvalue() set
                outpinp = ਨਵਾਂ AstPin{nodep->fileline(), nodep->pin_ਅੰਕ(),
                                     ਬਾਹਰਲਾ_Mod_Varp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(ਬਾਹਰਲਾ_Mod_Varp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(outpinp);
                // Simplify
                ਜੇ (ਵਿੱਚ_Decl_Processing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, ਸੱਚ};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                ਬਾਹਰਲਾ_Assignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        ਸੱਚ);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, ਬਾਹਰਲਾ_Assignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ਅਚਲ TristatePinVisitor visitor{nodep->exprp(), m_tgraph, ਝੂਠ};
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ ਵਿੱਚ_Assignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, ਸੱਚ);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, ਵਿੱਚ_Assignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            ਜੇ (!ਬਾਹਰਲਾ_Assignp) {
                ਜੇ (!outpinp) {
                    exprrefp = ਖਾਲੀ_ਸੂਚਕ;  // Primary input only
                } ਨਹੀਂਤਾਂ {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    ਜੇ (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } ਨਹੀਂਤਾਂ {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(ਬਾਹਰਲਾ_Assignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                ਜੇ (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            ਜੇ (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(ਨਵਾਂ AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                ਜੇ (!ਬਾਹਰਲਾ_Assignp) {
                    map_ਪਾਓ_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            ਜੇ (exprrefp) {
                AstPull* ਅਚਲ pullp = m_varAux(nodep->modVarp()).pullp;
                ਅਚਲ ਸਵੈਚਾਲਿਤ& src_ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls = m_varAux(nodep->modVarp()).ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = ਖਾਲੀ_ਸੂਚਕ;
                ਪੂਰਨ selLsb = 0;
                ਪੂਰਨ sel_ਚੌੜਾਈ = 0;
                ਜੇ (ਬਾਹਰਲਾ_Assignp) {
                    ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(ਬਾਹਰਲਾ_Assignp->lhsp(), Sel)) {
                        AstVarRef* ਅਚਲ vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ਅਚਲ lsbp = VN_CAST(selp->lsbp(), Const);
                        ਜੇ (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->ਮੰਜ਼ਿਲ_SInt();
                            sel_ਚੌੜਾਈ = selp->ਚੌੜਾਈ_Const();
                        }
                    }
                }

                ਜੇ (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    ਸੈੱਟ_Pull_Direction(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    ਜੇ (selVarp) {
                        ਅਚਲ ਪੂਰਨ direction = pullp->direction();
                        ਲਈ (ਪੂਰਨ i = 0; i < sel_ਚੌੜਾਈ; ++i) {
                            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pull_Direction(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                ਜੇ (!src_ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : src_ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls) {
                        ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pull_Direction(exprrefp->varp(), pair.ਪਹਿਲਾ, pair.ਦੂਜਾ);
                    }  // LCOV_EXCL_LINE
                    ਜੇ (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : src_ਦੋਆਧਾਰੀ_ਅੰਕ_Pulls) {
                            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Pull_Direction(selVarp, selLsb + pair.ਪਹਿਲਾ, pair.ਦੂਜਾ);
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
        ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->user2() & U2_NONGRAPH) ਵਾਪਸ;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    ਖਾਲੀ handle_ਗੰਢ_Var_Ref(Ast_ਗੰਢ_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        ਜੇ (m_graphing) {
            ਜੇ (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            ਜੇ (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            ਜੇ (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.ਸੈੱਟ_Tristate(nodep->varp());
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->user2() & U2_NONGRAPH) ਵਾਪਸ;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            ਜੇ (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                ਜੇ (m_ਵਿੱਚ_Alias) {
                    ਜੇ (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } ਨਹੀਂਤਾਂ {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    ਵਾਪਸ;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_ਪਾਓ_Lhs_Var_Ref(nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                AstVar* ਅਚਲ enVarp = getCreateEnVarp(nodep->varp(), ਝੂਠ);
                ਜੇ (AstVarXRef* ਅਚਲ xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } ਨਹੀਂਤਾਂ {
                    nodep->user1p(ਨਵਾਂ AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override { handle_ਗੰਢ_Var_Ref(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override { handle_ਗੰਢ_Var_Ref(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        ਜੇ (m_graphing) {
            // If tri0/1 force a pullup
            ਜੇ (nodep->user2() & U2_GRAPHING) ਵਾਪਸ;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            ਜੇ (nodep->isPulldown() || nodep->isPullup()) {
                Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstPull{
                    nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newp);
                // We'll iterate on the new AstPull later
            }
            ਜੇ (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.ਸੈੱਟ_Tristate(nodep);
            }
        } ਨਹੀਂਤਾਂ {  // !graphing
            ਜੇ (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        ਜੇ (m_processedIfaces && VN_IS(nodep, Iface)) ਵਾਪਸ;
        UINFO(8, dbgState() << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_graphing);
        VL_RESTORER(m_unique);
        VL_RESTORER_CLEAR(m_lhsmap);
        VL_RESTORER_CLEAR(m_assigns);
        VL_RESTORER_CLEAR(m_ਸੌਂਪੋ_Enps);
        VL_RESTORER_CLEAR(m_proceduralEnables);
        // Not preserved, needs pointer instead: TristateGraph origTgraph = m_tgraph;
        UASSERT_OBJ(m_tgraph.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Unsupported: NodeModule under NodeModule");

        // Clear state
        m_graphing = ਝੂਠ;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = ਖਾਲੀ_ਸੂਚਕ;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = ਸੱਚ;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = ਝੂਠ;
        UINFO(9, dbgState() << "processing mod " << nodep);

        // Merge the assignments for very Wired net LHS : wor, trior, wand and triand
        ਮਿਲਾਓ_Wired_Nets_Assignments();
        // Remove all assignments not stronger than the strongest uniform constant
        remove_Assignments_Not_Stronger_Than_ਇਕਸਾਰ_Constant();
        // Use graph to find tristate signals
        m_tgraph.ਗ੍ਰਾਫ_Walk(nodep);

        // Remove all assignments not stronger than the strongest non-tristate RHS
        removeAssignmentsNotStrongerThanNonTristate();

        // Build the LHS drivers map for this module
        iterateChildren(nodep);
        // Insert new logic for all tristates
        ਪਾਓ_Tristates(nodep);
        ਪਾਓ_Procedural_Enable_Assignments(nodep);

        ਲਈ (ਸਵੈਚਾਲਿਤ& ਸੌਂਪੋ_Enp : m_ਸੌਂਪੋ_Enps) {
            VL_DO_DANGLING(ਸੌਂਪੋ_Enp.ਦੂਜਾ->delete_ਰੁੱਖ(), ਸੌਂਪੋ_Enp.ਦੂਜਾ);
        }
        m_ਸੌਂਪੋ_Enps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        // don't deal with classes
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        // don't deal with functions
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        // don't deal with casez compare '???? values
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = ਝੂਠ;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    ਖਾਲੀ combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        ਲਈ (ਅਚਲ std::pair<AstVar* ਅਚਲ, std::ਸਦਿਸ਼<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ਅਚਲ invarp = kv.ਪਹਿਲਾ;
            ਅਚਲ std::ਸਦਿਸ਼<std::pair<AstVar*, AstVar*>>& contribs = kv.ਦੂਜਾ;
            Ast_ਗੰਢ_Module* ਅਚਲ ifaceModp = ਲੱਭੋ_ਮਾਪਾ_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_ਸਤਰ* ਅਚਲ fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.ਮਾਪ() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* orp = ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* enp = ਖਾਲੀ_ਸੂਚਕ;

            ਲਈ (ਅਚਲ std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ਅਚਲ contribEnp = contrib.ਪਹਿਲਾ;
                AstVar* ਅਚਲ contribOutp = contrib.ਦੂਜਾ;

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਬਾਹਰਲਾ_Refp = ਨਵਾਂ AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enRefp = ਨਵਾਂ AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ andp = ਨਵਾਂ AstAnd{fl, ਬਾਹਰਲਾ_Refp, enRefp};

                orp = (!orp) ? andp : ਨਵਾਂ AstOr{fl, orp, andp};

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enRef2p = ਨਵਾਂ AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : ਨਵਾਂ AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            ਜੇ (!envarp) {
                envarp = ਨਵਾਂ AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->ਸੈੱਟ_Dfg_Tri_Lowered();
                ifaceModp->ਜੋੜੋ_Stmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            Ast_ਸੌਂਪੋ_W* ਅਚਲ enAssp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{fl, ਨਵਾਂ AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{enAssp});

            // Pull resolution
            ਅਚਲ AstPull* ਅਚਲ pullp = m_varAux(invarp).pullp;
            ਅਚਲ ਤਾਰਕਿਕ pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* undrivenp = ਨਵਾਂ AstNot{fl, ਨਵਾਂ AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = ਨਵਾਂ AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = ਨਵਾਂ AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            Ast_ਸੌਂਪੋ_W* ਅਚਲ assp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{fl, ਨਵਾਂ AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{assp});
        }
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_Module*> ifacesp;
        ਲਈ (Ast_ਗੰਢ* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            ਜੇ (VN_IS(modp, Iface)) ifacesp.ਧੱਕੋ_ਪਿੱਠ(VN_AS(modp, ਗੰਢ_Module));
        }
        ਲਈ (ਸਵੈਚਾਲਿਤ it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = ਸੱਚ;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::ਸਦਿਸ਼<Ast_ਗੰਢ*> ਅਣਵਰਤਿਆ_Rootps;
        netlistp->foreach([&](Ast_ਗੰਢ* nodep) {
            Ast_ਗੰਢ* ਅਚਲ enp = nodep->user1p();
            ਜੇ (!enp) ਵਾਪਸ;
            ਜੇ (enp->backp()) ਵਾਪਸ;
            ਅਣਵਰਤਿਆ_Rootps.emplace_ਪਿੱਠ(enp);
        });
        ਲਈ (Ast_ਗੰਢ* ਅਚਲ nodep : ਅਣਵਰਤਿਆ_Rootps) VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
#ਜੇ_ਅੰਤ
    }
    ~TristateVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

ਖਾਲੀ V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("tristate", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
