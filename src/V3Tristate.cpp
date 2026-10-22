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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Tristate.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Graph.h"
#ضمن "V3Inst.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف Tristate_بنياد_Visitor VL_NOT_FINAL : عام VNVisitor {
عام:
    // METHODS
};

//######################################################################
// Graph support classes

صنف Tristate_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Tristate_راس, V3_گراف_راس)
    Ast_ڳنڍ* ثابت m_nodep;
    منطقي m_isTristate = خطأ;  // Logic indicates a tristate
    منطقي m_feedsTri = خطأ;  // Propagates to a tristate node (on RHS)
    منطقي m_processed = خطأ;  // Tristating was cleaned up
عام:
    Tristate_راس(V3_گراف* graphp, Ast_ڳنڍ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Tristate_راس() override = افتراضي;
    // ACCESSORS
    Ast_ڳنڍ* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
    ثابت AstVar* varp() ثابت { أرجع VN_CAST(nodep(), Var); }
    string name() ثابت override {
        أرجع ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvt_منزل_Hex(nodep())));
    }
    string dotColor() ثابت override {
        أرجع (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_سٽ* fileline() ثابت override { أرجع nodep()->fileline(); }
    منطقي isTristate() ثابت VL_MT_محفوظ { أرجع m_isTristate; }
    فراغ isTristate(منطقي flag) { m_isTristate = flag; }
    منطقي feedsTri() ثابت VL_MT_محفوظ { أرجع m_feedsTri; }
    فراغ feedsTri(منطقي flag) { m_feedsTri = flag; }
    منطقي processed() ثابت { أرجع m_processed; }
    فراغ processed(منطقي flag) { m_processed = flag; }
};

//######################################################################

صنف Tristate_گراف final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
عام:
    استعمال VarVec = std::ويڪٽر<AstVar*>;

خاص:
    // MEMBERS
    V3_گراف m_گراف;  // Logic graph

عام:
    // CONSTRUCTORS
    Tristate_گراف() { clearAndCheck(); }
    مجازي ~Tristate_گراف() { clearAndCheck(); }
    VL_UNCOPYABLE(Tristate_گراف);

خاص:
    // METHODS

    Tristate_راس* make_راس(Ast_ڳنڍ* nodep) {
        Tristate_راس* vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p());
        إذا (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = جديد Tristate_راس{&m_گراف, nodep};
            nodep->user4p(vertexp);
        }
        أرجع vertexp;
    }

    // METHODS - Graph optimization
    فراغ گراف_Walk_Recurse_Fwd(Tristate_راس* vtxp, صحيح level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        إذا (!vtxp->isTristate()) أرجع;  // tristate involved
        إذا (vtxp->user() == 1) أرجع;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        إذا (!vtxp->varp()) {  // not a var where we stop the recursion
            لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->ٻاهريون_ڪنارا()) {
                Tristate_راس* ثابت vvertexp = static_cast<Tristate_راس*>(ڪنارو.چوٽي());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                إذا (!vvertexp->isTristate()) {
                    vvertexp->isTristate(سچ);
                    گراف_Walk_Recurse_Fwd(vvertexp, level + 1);
                }
            }
        } وإلا {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->اندريون_ڪنارا()) {
                Tristate_راس* ثابت vvertexp = static_cast<Tristate_راس*>(ڪنارو.fromp());
                إذا (ثابت AstVarRef* ثابت refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    إذا (refp->access().isWriteOrRW()
                        // Doesn't hurt to not check if already set, but by doing so when we
                        // print out the debug messages, we'll see this node at level 0 instead.
                        && !vvertexp->isTristate()) {
                        vvertexp->isTristate(سچ);
                        گراف_Walk_Recurse_Fwd(vvertexp, level + 1);
                    }
                }
            }
        }
    }
    فراغ گراف_Walk_Recurse_پٺ(Tristate_راس* vtxp, صحيح level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        إذا (!(vtxp->isTristate() || vtxp->feedsTri())) أرجع;  // tristate involved
        إذا (vtxp->user() == 3) أرجع;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        إذا (!vtxp->varp()) {  // not a var where we stop the recursion
            لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->اندريون_ڪنارا()) {
                Tristate_راس* ثابت vvertexp = static_cast<Tristate_راس*>(ڪنارو.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                إذا (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(سچ);
                    گراف_Walk_Recurse_پٺ(vvertexp, level + 1);
                }
            }
        }
    }

عام:
    // METHODS
    منطقي خالي_جايون() ثابت { أرجع m_گراف.خالي_جايون(); }
    فراغ clearAndCheck() {
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            ثابت Tristate_راس& vvertex = static_cast<Tristate_راس&>(vtx);
            إذا (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_گراف.clear();
        Ast_ڳنڍ::user4_Clear_وڻ();  // Wipe all node user4p's that point to vertexes
    }
    فراغ گراف_Walk(Ast_ڳنڍ_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            گراف_Walk_Recurse_Fwd(static_cast<Tristate_راس*>(&vtx), 0);
        }
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            گراف_Walk_Recurse_پٺ(static_cast<Tristate_راس*>(&vtx), 0);
        }
        إذا (dump_گراف_Level() >= 9) m_گراف.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    فراغ associate(Ast_ڳنڍ* fromp, Ast_ڳنڍ* چوٽي) {
        جديد V3_گراف_ڪنارو{&m_گراف, make_راس(fromp), make_راس(چوٽي), 1};
    }
    فراغ deleteVerticesFromSubtreeRecurse(Ast_ڳنڍ* nodep) {
        إذا (!nodep) أرجع;
        // Skip vars, because they may be connected to more than one varref
        إذا (!VN_IS(nodep, Var)) {
            Tristate_راس* ثابت vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p());
            إذا (vertexp) vertexp->unlinkDelete(&m_گراف);
        }
        إذا (Ast_ڳنڍ* ثابت refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        إذا (Ast_ڳنڍ* ثابت refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        إذا (Ast_ڳنڍ* ثابت refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        إذا (Ast_ڳنڍ* ثابت refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    فراغ سيٽ_Tristate(Ast_ڳنڍ* nodep) { make_راس(nodep)->isTristate(سچ); }
    منطقي isTristate(ثابت Ast_ڳنڍ* nodep) {
        ثابت Tristate_راس* ثابت vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p());
        أرجع vertexp && vertexp->isTristate();
    }
    منطقي feedsTri(ثابت Ast_ڳنڍ* nodep) {
        ثابت Tristate_راس* ثابت vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p());
        أرجع vertexp && vertexp->feedsTri();
    }
    فراغ didProcess(Ast_ڳنڍ* nodep, منطقي quiet = خطأ) {
        إذا (Tristate_راس* ثابت vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(سچ);
        } وإلا إذا (!quiet) {
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
        لكل (ثابت V3_گراف_راس& vtx : m_گراف.vertices()) {
            ثابت Tristate_راس& vvertex = static_cast<ثابت Tristate_راس&>(vtx);
            إذا (vvertex.isTristate()) {
                إذا (AstVar* ثابت nodep = VN_CAST(vvertex.nodep(), Var)) v.ڌڪو_پٺ(nodep);
            }
        }
        أرجع v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

صنف TristatePinVisitor final : عام Tristate_بنياد_Visitor {
    Tristate_گراف& m_tgraph;
    ثابت منطقي m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        إذا (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } وإلا إذا (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.سيٽ_Tristate(nodep->varp());
        }
    }
    فراغ گهمو(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ٽڪرو_Sel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_ڳنڍ* nodep, Tristate_گراف& tgraph, منطقي lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = افتراضي;
};

//######################################################################

صنف TristateVisitor final : عام Tristate_بنياد_Visitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;
    ثابت VNUser3_اندريون_Use m_inuser3;
    ثابت VNUser4_اندريون_Use m_inuser4;

    بنية AuxAstVar final {
        AstPull* pullp = خالي_اشارو;  // pullup/pulldown direction (whole variable)
        AstVar* ٻاهريون_Varp = خالي_اشارو;  // output __out var
        منطقي ifaceTristate = خطأ;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<صحيح, صحيح>
            ٻٽو_انگ_Pulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    بنية RefStrength final {
        Ast_ڳنڍ_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_ڳنڍ_سونپيو* m_assignp;  // Assignment containing m_varrefp
        ثابت Ast_ڳنڍ_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_ڳنڍ_اظهار* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_ڳنڍ_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_ڳنڍ_اظهار* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_ڳنڍ_Var_Ref* varrefp, Ast_ڳنڍ_سونپيو* assignp,
                    ثابت Ast_ڳنڍ_Procedure* procedurep, Ast_ڳنڍ_اظهار* enLhsp,
                    Ast_ڳنڍ_Var_Ref* enVarrefp, Ast_ڳنڍ_اظهار* enRhsp, VStrength strength,
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
    بنية LhsProjection final {
        Ast_ڳنڍ_اظهار* m_lhsp;  // LHS giving the enable assign
        Ast_ڳنڍ_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_ڳنڍ_اظهار* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    بنية ProceduralEnable final {
        Ast_ڳنڍ_اظهار* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_ڳنڍ_اظهار* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    استعمال RefStrengthVec = std::ويڪٽر<RefStrength>;
    استعمال VarMap = std::map<AstVar*, RefStrengthVec*>;
    استعمال Assigns = std::ويڪٽر<Ast_سونپيو_W*>;
    استعمال Var_منزل_Assigns_Map = std::map<AstVar*, Assigns>;
    استعمال سونپيو_Enable_Map = std::map<Ast_ڳنڍ_سونپيو*, Ast_ڳنڍ_اظهار*>;
    استعمال ProceduralEnableMap = std::map<Ast_ڳنڍ_سونپيو*, std::ويڪٽر<ProceduralEnable>>;
    تعداد : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    منطقي m_graphing = خطأ;  // Major mode - creating graph
    //
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    AstCell* m_cellp = خالي_اشارو;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    Var_منزل_Assigns_Map m_assigns;  // Assigns in current module
    سونپيو_Enable_Map m_سونپيو_Enps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    صحيح m_unique = 0;
    منطقي m_alhs = خطأ;  // On LHS of assignment
    منطقي m_اندريون_Alias = خطأ;  // Inside alias statement
    منطقي m_processedIfaces = خطأ;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = خالي_اشارو;  // Delay of assignment currently collecting LHS drivers
    ثابت Ast_ڳنڍ* m_logicp = خالي_اشارو;  // Current logic being built
    Ast_ڳنڍ_سونپيو* m_assignp = خالي_اشارو;  // Current assignment
    ثابت Ast_ڳنڍ_Procedure* m_procedurep = خالي_اشارو;  // Current procedural process
    Tristate_گراف m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::ويڪٽر<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ثابت {
        string o = (m_graphing ? " gr " : " ng ");
        إذا (m_alhs) o += "alhs ";
        أرجع o;
    }
    فراغ mod_جوڙيو_Stmtp(Ast_ڳنڍ* nodep, Ast_ڳنڍ* newp) {
        إذا (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } وإلا {
            m_modp->جوڙيو_Stmtsp(newp);
        }
    }
    فراغ associateLogic(Ast_ڳنڍ* fromp, Ast_ڳنڍ* چوٽي) {
        إذا (m_logicp) m_tgraph.associate(fromp, چوٽي);
    }
    AstConst* newAllZerosOrOnes(Ast_ڳنڍ* nodep, منطقي ones) {
        V3_عدد انگ{nodep, nodep->ويڪر()};
        إذا (ones) انگ.سيٽ_All_Bits1();
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), انگ};
        أرجع newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_سٽ* fl, AstVar* varp, ثابت string& dotted, VAccess access,
                           ثابت string& inlinedDots) {
        AstVarXRef* ثابت xrefp = جديد AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        أرجع xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* ڳوليو_Modport_For_Dotted(Ast_ڳنڍ_Module* modp, ثابت string& dotted_رستو) {
        إذا (dotted_رستو.خالي_جايون()) أرجع خالي_اشارو;
        ثابت string پهريون_Comp = dotted_رستو.substr(0, dotted_رستو.ڳوليو('.'));
        لكل (Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت ivarp = VN_CAST(stmtp, Var);
            إذا (!ivarp || ivarp->name() != پهريون_Comp) استمر;
            ثابت AstIfaceRefDType* ثابت ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            إذا (ifaceDtp && ifaceDtp->modportp()) أرجع ifaceDtp->modportp();
            اكسر;
        }
        أرجع خالي_اشارو;
    }
    Ast_ڳنڍ_اظهار* getEnp(Ast_ڳنڍ* nodep) {
        إذا (nodep->user1p()) {
            إذا (Ast_ڳنڍ_Var_Ref* ثابت refp = VN_CAST(nodep, ڳنڍ_Var_Ref)) {
                إذا (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    أرجع newAllZerosOrOnes(nodep, سچ);
                }
            }
        } وإلا {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, سچ));
        }
        // Otherwise return the previous output enable
        أرجع VN_AS(nodep->user1p(), ڳنڍ_اظهار);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    فراغ جوڙيو_Modport_Var_Ref_If_Missing(AstModport* modportp, AstVar* varp, VDirection dir) {
        لكل (Ast_ڳنڍ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ثابت mvrp = VN_CAST(mrp, ModportVarRef);
            إذا (mvrp && mvrp->varp() == varp) أرجع;  // Already present
        }
        AstModportVarRef* ثابت mvrp = جديد AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->جوڙيو_Varsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, منطقي is_چوٽي) {
        // Return the master __en for the specified input variable
        إذا (!invarp->user1p()) {
            AstVar* ثابت newp
                = جديد AstVar{invarp->fileline(), is_چوٽي ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_ڳنڍ_Module* ثابت ownerModp = ڳوليو_والد_Module(invarp);
            إذا (ownerModp && ownerModp != m_modp) {
                ownerModp->جوڙيو_Stmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                لكل (Ast_ڳنڍ* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ثابت modportp = VN_CAST(stmtp, Modport);
                    إذا (!modportp) استمر;
                    لكل (Ast_ڳنڍ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ثابت mvrp = VN_CAST(mrp, ModportVarRef);
                        إذا (mvrp && mvrp->varp() == invarp) {
                            جوڙيو_Modport_Var_Ref_If_Missing(modportp, newp, VDirection::داخلا);
                            اكسر;
                        }
                    }
                }
            } وإلا {
                mod_جوڙيو_Stmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        أرجع VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ثابت constp) {
        File_سٽ* ثابت fl = constp->fileline();
        V3_عدد numz{constp, constp->ويڪر()};
        numz.op_بٽ_Z(constp->انگ());  // Z->1, else 0
        V3_عدد numz0{constp, constp->ويڪر()};
        numz0.opNot(numz);  // Z->0, else 1
        أرجع جديد AstConst{fl, numz0};
    }
    Ast_ڳنڍ_اظهار* get_En_اظهار_Based_On_Originalp(Ast_ڳنڍ_اظهار* ثابت nodep) {
        إذا (AstVarRef* ثابت varrefp = VN_CAST(nodep, VarRef)) {
            أرجع جديد AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), خطأ),
                                 VAccess::READ};
        } وإلا إذا (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ثابت enVarp = getCreateEnVarp(xrefp->varp(), خطأ);
            أرجع newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } وإلا إذا (AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            أرجع getNonZConstp(constp);
        } وإلا إذا (AstExtend* ثابت extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_ڳنڍ_اظهار* ثابت lhsp = extendp->lhsp()->unlink_Fr_پٺ();
            AstExtend* ثابت enExtendp = extendp->clone_وڻ(خطأ);
            extendp->lhsp(lhsp);
            Ast_ڳنڍ_اظهار* ثابت enLhsp = get_En_اظهار_Based_On_Originalp(lhsp);
            enExtendp->lhsp(جديد AstNot{enLhsp->fileline(), enLhsp});
            أرجع جديد AstNot{enExtendp->fileline(), enExtendp};
        } وإلا إذا (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            Ast_ڳنڍ_اظهار* ثابت fromp = selp->fromp()->unlink_Fr_پٺ();
            AstSel* ثابت enSelp = selp->clone_وڻ(خطأ);
            selp->fromp(fromp);
            Ast_ڳنڍ_اظهار* ثابت enFromp = get_En_اظهار_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            أرجع enSelp;
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            أرجع خالي_اشارو;
        }
    }
    AstVar* get_Create_ٻاهريون_Varp(AstVar* invarp, منطقي is_چوٽي) {
        // Return the master __out for the specified input variable
        إذا (!m_varAux(invarp).ٻاهريون_Varp) {
            AstVar* ثابت newp
                = جديد AstVar{invarp->fileline(), is_چوٽي ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            mod_جوڙيو_Stmtp(invarp, newp);
            m_varAux(invarp).ٻاهريون_Varp = newp;  // find outvar given invarp
        }
        أرجع m_varAux(invarp).ٻاهريون_Varp;
    }
    AstVar* getCreateUnconnVarp(Ast_ڳنڍ* fromp, Ast_ڳنڍ_DType* dtypep) {
        AstVar* ثابت newp = جديد AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvt_منزل_Str(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        mod_جوڙيو_Stmtp(newp, newp);
        أرجع newp;
    }

    LhsProjection newLhsProjection(Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_Var_Ref* targetp, Ast_ڳنڍ_اظهار* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_ڳنڍ_اظهار* ثابت newLhsp = lhsp->clone_وڻ_خالص(خطأ);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_ڳنڍ* nodep) { nodep->user1p(خالي_اشارو); });
        Ast_ڳنڍ_Var_Ref* ثابت newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        أرجع LhsProjection{newLhsp, newVarrefp, enp};
    }

    فراغ map_وجهو_Lhs_Var_Ref(Ast_ڳنڍ_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ثابت ڪنجي = nodep->varp();
        ثابت خودڪار pair = m_lhsmap.emplace(ڪنجي, خالي_اشارو);
        إذا (pair.ٻيو) pair.پهريون->ٻيو = جديد RefStrengthVec;
        Ast_ڳنڍ_اظهار* enLhsp = خالي_اشارو;
        Ast_ڳنڍ_Var_Ref* enVarrefp = خالي_اشارو;
        Ast_ڳنڍ_اظهار* enRhsp = خالي_اشارو;
        ثابت خودڪار enIt = m_سونپيو_Enps.ڳوليو(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ثابت منطقي canCombineProcedural = !VN_IS(ڪنجي->dtypep()->skipRefp(), ڳنڍ_UOr_Struct_DType);
        ثابت Ast_ڳنڍ_Procedure* ثابت procedurep
            = enIt != m_سونپيو_Enps.پڇاڙي() && canCombineProcedural ? m_procedurep : خالي_اشارو;
        إذا (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->ٻيو->clone_وڻ_خالص(خطأ));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.پهريون->ٻيو->ڌڪو_پٺ(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_ڳنڍ_اظهار* newEnableDeposit(AstSel* selp, Ast_ڳنڍ_اظهار* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_ڳنڍ_اظهار* ثابت newp = جديد Ast_سرڪايو_L{
            selp->fileline(), جديد AstExtend{selp->fileline(), enp, selp->fromp()->ويڪر()},
            selp->lsbp()->clone_وڻ_خالص(خطأ), selp->fromp()->ويڪر()};
        أرجع newp;
    }

    فراغ سيٽ_Pull_Direction(AstVar* varp, AstPull* pullp) {
        ثابت AstPull* ثابت oldpullp = m_varAux(varp).pullp;
        إذا (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } وإلا {
            إذا (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    فراغ سيٽ_ٻٽو_انگ_Pull_Direction(AstVar* varp, صحيح ٻٽو_انگ_اشاريو, صحيح direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        خودڪار& ٻٽو_انگ_Pulls = m_varAux(varp).ٻٽو_انگ_Pulls;
        خودڪار it = ٻٽو_انگ_Pulls.ڳوليو(ٻٽو_انگ_اشاريو);
        إذا (it != ٻٽو_انگ_Pulls.پڇاڙي() && it->ٻيو != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << ٻٽو_انگ_اشاريو << " of " << varp->prettyNameQ());
        }
        ٻٽو_انگ_Pulls[ٻٽو_انگ_اشاريو] = direction;
    }

    AstConst* create_في_ٻٽو_انگ_Pull_Const(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ثابت خودڪار& ٻٽو_انگ_Pulls = m_varAux(varp).ٻٽو_انگ_Pulls;
        V3_عدد انگ{varp, varp->ويڪر(), 0};  // Start with all zeros
        لكل (ثابت خودڪار& pair : ٻٽو_انگ_Pulls) {
            إذا (pair.ٻيو == 1) انگ.سيٽ_ٻٽو_انگ(pair.پهريون, 1);
        }  // LCOV_EXCL_LINE
        أرجع جديد AstConst{varp->fileline(), انگ};
    }

    منطقي has_في_ٻٽو_انگ_Pulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        أرجع !m_varAux(varp).ٻٽو_انگ_Pulls.خالي_جايون();
    }

    فراغ checkUnhandled(Ast_ڳنڍ* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        إذا (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        إذا ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    فراغ وجهو_Tristates(Ast_ڳنڍ_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ثابت Tristate_گراف::VarVec vars = m_tgraph.tristateVars();
        لكل (خودڪار varp : vars) {
            إذا (!m_tgraph.isTristate(varp)) استمر;
            ثابت خودڪار it = m_lhsmap.ڳوليو(varp);
            إذا (it != m_lhsmap.پڇاڙي()) استمر;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ثابت AstPull* ثابت pullp = m_varAux(varp).pullp;
            ثابت منطقي pull_قدر = pullp && pullp->direction() == 1;
            AstConst* ثابت constp = newAllZerosOrOnes(varp, pull_قدر);
            AstVarRef* ثابت varrefp = جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            Ast_سونپيو_W* ثابت newp = جديد Ast_سونپيو_W{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, خطأ));  // en=0 for floating/pullup vars
            nodep->جوڙيو_Stmtsp(جديد AstAlways{newp});
            map_وجهو_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        لكل (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ثابت std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.ڳوليو(varp);
            إذا (it == m_lhsmap.پڇاڙي()) استمر;
            AstVar* ثابت invarp = it->پهريون;
            RefStrengthVec* refsp = it->ٻيو;
            // Figure out if this var needs tristate expanded.
            إذا (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_ڳنڍ_Module* ثابت ownerModp = ڳوليو_والد_Module(invarp);
                ثابت منطقي isCrossModule = ownerModp && ownerModp != nodep;
                ثابت منطقي isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                إذا (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    لكل (ثابت RefStrength& rs : *refsp) {
                        إذا (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            اكسر;
                        }
                    }
                } وإلا إذا (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = سچ;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    بنية حصا_ڪريو_Info final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, حصا_ڪريو_Info> partitions;
                    لكل (ثابت RefStrength& rs : *refsp) {
                        إذا (AstVarXRef* ثابت xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            حصا_ڪريو_Info& pi = partitions[xrefp->dotted()];
                            pi.refs.ڌڪو_پٺ(rs);
                            إذا (pi.inlinedDots.خالي_جايون()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } وإلا {
                            partitions[""].refs.ڌڪو_پٺ(rs);
                        }
                    }
                    لكل (خودڪار& kv : partitions) {
                        وجهو_Tristates_Signal(nodep, invarp, &kv.ٻيو.refs, سچ, kv.پهريون,
                                              kv.ٻيو.inlinedDots,
                                              ڳوليو_Modport_For_Dotted(nodep, kv.پهريون));
                    }
                } وإلا إذا (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = سچ;
                    وجهو_Tristates_Signal(nodep, invarp, refsp, سچ, "", "", خالي_اشارو);
                } وإلا {
                    وجهو_Tristates_Signal(nodep, invarp, refsp, خطأ, "", "", خالي_اشارو);
                }
            } وإلا {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(ميٽيو refsp, refsp);
        }
    }

    فراغ aggregateTriSameStrength(Ast_ڳنڍ_Module* nodep, AstVar* ثابت varp, AstVar* ثابت envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator پڇاڙي_Strength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        بنية Driver_گروهه final {
            ثابت Ast_ڳنڍ_Procedure* m_procedurep;  // Process owning these assignments
            std::ويڪٽر<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::ويڪٽر<Driver_گروهه> drivers;
        std::map<ثابت Ast_ڳنڍ_Procedure*, سائيز_t> procedure_منزل_Driver;
        لكل (خودڪار it = beginStrength; it != پڇاڙي_Strength; ++it) {
            إذا (it->m_procedurep) {
                ثابت خودڪار emplaced = procedure_منزل_Driver.emplace(it->m_procedurep, drivers.سائيز());
                إذا (!emplaced.ٻيو) {
                    drivers[emplaced.پهريون->ٻيو].m_refsp.ڌڪو_پٺ(&*it);
                    استمر;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ڌڪو_پٺ(Driver_گروهه{it->m_procedurep, {&*it}});
        }

        Ast_ڳنڍ_اظهار* orp = خالي_اشارو;
        Ast_ڳنڍ_اظهار* enp = خالي_اشارو;

        لكل (Driver_گروهه& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ثابت newLhsp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvt_منزل_Str(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->سيٽ_Dfg_Tri_Lowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->جوڙيو_Stmtsp(newLhsp);
            AstVar* ثابت newEnLhsp
                = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvt_منزل_Str(m_unique++), envarp};  // 2-state ok
            newEnLhsp->سيٽ_Dfg_Tri_Lowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->جوڙيو_Stmtsp(newEnLhsp);

            لكل (RefStrength* ثابت refStrengthp : driver.m_refsp) {
                Ast_ڳنڍ_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                إذا (VN_IS(refp, VarXRef)) {
                    AstVarRef* ثابت localRefp
                        = جديد AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(خالي_اشارو);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } وإلا {
                    refp->varp(newLhsp);
                }

                إذا (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_ڳنڍ_اظهار* enLhsp = refStrengthp->m_enLhsp;
                    Ast_ڳنڍ_Var_Ref* ثابت enVarrefp = refStrengthp->m_enVarrefp;
                    إذا (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ثابت localRefp
                            = جديد AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        إذا (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_وڻ(), enVarrefp);
                        } وإلا {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ڌڪو_Deletep(enVarrefp), enVarrefp);
                        }
                    } وإلا {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ڌڪو_پٺ(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = خالي_اشارو;
                    refStrengthp->m_enVarrefp = خالي_اشارو;
                    refStrengthp->m_enRhsp = خالي_اشارو;
                } وإلا {
                    Ast_سونپيو_W* ثابت enLhspAssignp = جديد Ast_سونپيو_W{
                        refp->fileline(),
                        جديد AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    إذا (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_وڻ(خطأ));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->جوڙيو_Stmtsp(جديد AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_سٽ* ثابت fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_ڳنڍ_اظهار* ثابت ref1p = جديد AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_ڳنڍ_اظهار* ثابت ref2p = جديد AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_ڳنڍ_اظهار* ثابت andp = جديد AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : جديد AstOr{fl, orp, andp};

            Ast_ڳنڍ_اظهار* ثابت ref3p = جديد AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : جديد AstOr{ref3p->fileline(), enp, ref3p};
        }
        Ast_سونپيو_W* ثابت assp = جديد Ast_سونپيو_W{
            varp->fileline(), جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->جوڙيو_Stmtsp(جديد AstAlways{assp});

        Ast_سونپيو_W* ثابت enAssp = جديد Ast_سونپيو_W{
            envarp->fileline(), جديد AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->جوڙيو_Stmtsp(جديد AstAlways{enAssp});
    }

    فراغ وجهو_Procedural_Enable_Assignments(Ast_ڳنڍ_Module* nodep) {
        لكل (خودڪار& سونپيو_Enables : m_proceduralEnables) {
            Ast_ڳنڍ_سونپيو* ثابت assignp = سونپيو_Enables.پهريون;
            Ast_ڳنڍ_اظهار* combinedLhsp = assignp->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* combinedRhsp = assignp->rhsp()->unlink_Fr_پٺ();
            لكل (ProceduralEnable& enable : سونپيو_Enables.ٻيو) {
                combinedLhsp = جديد AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = جديد AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            إذا (!VN_IS(assignp, سونپيو_Dly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ثابت sampledp
                    = جديد AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvt_منزل_Str(m_unique++), combinedRhsp->dtypep()};
                nodep->جوڙيو_Stmtsp(sampledp);
                assignp->lhsp(جديد AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                Ast_سونپيو* ثابت applyp
                    = جديد Ast_سونپيو{assignp->fileline(), combinedLhsp,
                                    جديد AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->جوڙيو_ايندڙ_هتي(applyp);
            } وإلا {
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
    فراغ وجهو_Tristates_Signal(Ast_ڳنڍ_Module* nodep, AstVar* ثابت invarp, RefStrengthVec* refsp,
                               منطقي isIfaceTri, ثابت string& iface_Dotted_رستو,
                               ثابت string& ifaceInlinedDots, AstModport* ifaceModportp) {
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
        AstVar* envarp = خالي_اشارو;
        AstVar* outvarp = خالي_اشارو;  // __out
        AstVar* lhsp = invarp;  // Variable to assign drive-value to (<in> or __out)
        ثابت منطقي is_چوٽي_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_چوٽي();
        إذا (!isIfaceTri) {
            إذا (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().خالي_جايون())
                 && is_چوٽي_Inout)
                || (!nodep->is_چوٽي() && invarp->isIO())) {
                // This var becomes an input
                invarp->var_Type2_اندريون();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = get_Create_ٻاهريون_Varp(invarp, is_چوٽي_Inout);
                outvarp->var_Type2_ٻاهريون();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_چوٽي_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).ٻٽو_انگ_Pulls
                    = m_varAux(invarp).ٻٽو_انگ_Pulls;  // Per-bit pull propagation
                إذا (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                إذا (!m_varAux(invarp).ٻٽو_انگ_Pulls.خالي_جايون()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } وإلا إذا (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_ڳنڍ_اظهار* orp = خالي_اشارو;
        Ast_ڳنڍ_اظهار* enp = خالي_اشارو;

        std::ترتيب_ڏيو(refsp->begin(), refsp->پڇاڙي(),
                  [](RefStrength a, RefStrength b) { أرجع a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        طالما (beginStrength != refsp->پڇاڙي()) {
            RefStrengthVec::iterator پڇاڙي_Strength = beginStrength + 1;
            طالما (پڇاڙي_Strength != refsp->پڇاڙي()
                   && پڇاڙي_Strength->m_strength == beginStrength->m_strength)
                پڇاڙي_Strength++;

            File_سٽ* ثابت fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strength_اڳياڙي;
            إذا (isIfaceTri && !iface_Dotted_رستو.خالي_جايون()) {
                strength_اڳياڙي = iface_Dotted_رستو;
                std::replace(strength_اڳياڙي.begin(), strength_اڳياڙي.پڇاڙي(), '.', '_');
                strength_اڳياڙي += "__";
            }
            ثابت string strengthVarName
                = strength_اڳياڙي + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ثابت varStrengthp = جديد AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->سيٽ_Dfg_Tri_Lowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->جوڙيو_Stmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ثابت enVarStrengthp = جديد AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->سيٽ_Dfg_Tri_Lowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->جوڙيو_Stmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     پڇاڙي_Strength);

            Ast_ڳنڍ_اظهار* اظهار_Current_Strengthp;
            إذا (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                اظهار_Current_Strengthp
                    = جديد AstAnd{fl, جديد AstVarRef{fl, varStrengthp, VAccess::READ},
                                 جديد AstNot{fl, enp->clone_وڻ_خالص(خطأ)}};
            } وإلا {
                اظهار_Current_Strengthp = جديد AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? اظهار_Current_Strengthp : جديد AstOr{fl, orp, اظهار_Current_Strengthp};

            Ast_ڳنڍ_اظهار* ثابت enVarStrengthRefp
                = جديد AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : جديد AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = پڇاڙي_Strength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        إذا (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->سيٽ_Dfg_Allow_Multidrive_Tri();
            Ast_ڳنڍ_Module* ثابت ifaceModp = ڳوليو_والد_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ثابت صحيح contribIdx = static_cast<صحيح>(m_ifaceContribs[invarp].سائيز());
            File_سٽ* ثابت fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ثابت contribOutp = جديد AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvt_منزل_Str(contribIdx), invarp};
            contribOutp->سيٽ_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->جوڙيو_Stmtsp(contribOutp);

            AstVar* ثابت contribEnp = جديد AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvt_منزل_Str(contribIdx), invarp};
            contribEnp->سيٽ_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->جوڙيو_Stmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            إذا (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                جوڙيو_Modport_Var_Ref_If_Missing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                جوڙيو_Modport_Var_Ref_If_Missing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_ڳنڍ_Var_Ref* ثابت lhsp
                    = iface_Dotted_رستو.خالي_جايون() ? static_cast<Ast_ڳنڍ_Var_Ref*>(
                                                    جديد AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_ڳنڍ_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, iface_Dotted_رستو,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_سونپيو_W* ثابت assp = جديد Ast_سونپيو_W{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->جوڙيو_Stmtsp(جديد AstAlways{assp});
            }
            {
                Ast_ڳنڍ_Var_Ref* ثابت lhsp
                    = iface_Dotted_رستو.خالي_جايون() ? static_cast<Ast_ڳنڍ_Var_Ref*>(
                                                    جديد AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_ڳنڍ_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, iface_Dotted_رستو,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_سونپيو_W* ثابت assp = جديد Ast_سونپيو_W{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->جوڙيو_Stmtsp(جديد AstAlways{assp});
            }

            m_ifaceContribs[invarp].ڌڪو_پٺ({contribEnp, contribOutp});
            أرجع;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ثابت منطقي hasPull = m_varAux(lhsp).pullp || has_في_ٻٽو_انگ_Pulls(lhsp);
        إذا (!outvarp || hasPull) {
            Ast_ڳنڍ_اظهار* undrivenp;
            إذا (envarp) {
                undrivenp = جديد AstNot{envarp->fileline(),
                                       جديد AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } وإلا إذا (enp) {
                undrivenp = جديد AstNot{enp->fileline(), enp};
                enp = خالي_اشارو;  // moved into undrivenp
            } وإلا {
                undrivenp = newAllZerosOrOnes(invarp, سچ);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            إذا (has_في_ٻٽو_انگ_Pulls(lhsp)) {
                pullConstp = create_في_ٻٽو_انگ_Pull_Const(lhsp);
            } وإلا {
                ثابت AstPull* ثابت pullp = m_varAux(lhsp).pullp;
                ثابت منطقي pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = جديد AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? جديد AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        إذا (envarp && !enp) enp = newAllZerosOrOnes(invarp, خطأ);

        إذا (envarp) {
            Ast_سونپيو_W* ثابت enAssp = جديد Ast_سونپيو_W{
                enp->fileline(), جديد AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->جوڙيو_Stmtsp(جديد AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        Ast_سونپيو_W* ثابت assp = جديد Ast_سونپيو_W{
            lhsp->fileline(), جديد AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->جوڙيو_Stmtsp(جديد AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        إذا ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().خالي_جايون()) && is_چوٽي_Inout) {
            إذا (envarp) {
                envarp->primaryIO(سچ);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            إذا (outvarp) {
                outvarp->primaryIO(سچ);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            إذا (!v3Global.opt.libCreate().خالي_جايون()) {
                ثابت صحيح id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(سچ);
            }
        }
    }

    منطقي is_Only_Assignment_Is_منزل_Lhs_Var(ثابت Ast_سونپيو_W* ثابت nodep) {
        إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            خودڪار مليو_It = m_assigns.ڳوليو(varRefp->varp());
            إذا (مليو_It != m_assigns.پڇاڙي()) {
                خودڪار ثابت& assigns_منزل_Var = مليو_It->ٻيو;
                إذا (assigns_منزل_Var.سائيز() == 1 && assigns_منزل_Var[0] == nodep) أرجع سچ;
            }
        }
        أرجع خطأ;
    }

    فراغ جوڙيو_منزل_Assignment_List(Ast_سونپيو_W* nodep) {
        إذا (AstVarRef* ثابت varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            إذا (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ڌڪو_پٺ(nodep);
            } وإلا إذا (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlink_Fr_پٺ()->delete_وڻ();
            }
        } وإلا إذا (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ثابت Ast_سونپيو_W* ثابت nodep, منطقي قدر) {
        إذا (AstStrengthSpec* ثابت strengthSpec = nodep->strengthSpecp()) {
            أرجع قدر ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        أرجع VStrength::STRONG;  // default strength is strong
    }

    منطقي assignment_جو_قدر_On_All_بٽ(ثابت Ast_سونپيو_W* ثابت nodep, منطقي قدر) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const)) {
            ثابت V3_عدد انگ = constp->انگ();
            أرجع قدر ? انگ.isEqAllOnes() : انگ.isEqZero();
        }
        أرجع خطأ;
    }

    Ast_سونپيو_W* get_Strongest_Assignment_جو_قدر(ثابت Assigns& assigns, منطقي قدر) {
        خودڪار maxIt = std::max_element(
            assigns.begin(), assigns.پڇاڙي(), [&](ثابت Ast_سونپيو_W* ap, ثابت Ast_سونپيو_W* bp) {
                ثابت منطقي قدرون_On_Rhs_A = assignment_جو_قدر_On_All_بٽ(ap, قدر);
                ثابت منطقي قدرون_On_Rhs_B = assignment_جو_قدر_On_All_بٽ(bp, قدر);
                إذا (!قدرون_On_Rhs_A) أرجع قدرون_On_Rhs_B;
                إذا (!قدرون_On_Rhs_B) أرجع خطأ;
                أرجع getStrength(ap, قدر) < getStrength(bp, قدر);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        أرجع assignment_جو_قدر_On_All_بٽ(*maxIt, قدر) ? *maxIt : خالي_اشارو;
    }

    منطقي isAssignmentNotStrongerThanStrength(ثابت Ast_سونپيو_W* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ثابت uint8_t strength0 = getStrength(assignp, 0);
        ثابت uint8_t strength1 = getStrength(assignp, 1);
        أرجع (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_جو_قدر_On_All_بٽ(assignp, 0))
               || (strength1 <= strength && assignment_جو_قدر_On_All_بٽ(assignp, 1));
    }

    Ast_ڳنڍ_اظهار* new_ملايو_اظهار(Ast_ڳنڍ_اظهار* ثابت lhsp, Ast_ڳنڍ_اظهار* ثابت rhsp, File_سٽ* ثابت fl,
                              منطقي isWor) {
        Ast_ڳنڍ_اظهار* اظهار = خالي_اشارو;
        إذا (isWor)
            اظهار = جديد AstOr{fl, lhsp, rhsp};
        وإلا
            اظهار = جديد AstAnd{fl, lhsp, rhsp};
        أرجع اظهار;
    }

    فراغ ملايو_Wired_Nets_Assignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        لكل (خودڪار& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ٻيو;
            إذا (assigns.سائيز() <= 1) استمر;
            AstVar* ثابت varp = varpAssigns.پهريون;
            إذا (!varp->isWiredNet()) استمر;
            خودڪار it = assigns.begin();
            Ast_سونپيو_W* ثابت سونپيو_Wp0 = *it;
            File_سٽ* ثابت fl = سونپيو_Wp0->fileline();
            Ast_ڳنڍ_اظهار* wExp = خالي_اشارو;
            طالما (++it != assigns.پڇاڙي()) {
                Ast_سونپيو_W* سونپيو_Wpi = *it;
                إذا (!wExp) {
                    wExp
                        = new_ملايو_اظهار(سونپيو_Wp0->rhsp()->clone_وڻ_خالص(خطأ),
                                       سونپيو_Wpi->rhsp()->clone_وڻ_خالص(خطأ), fl, varp->isWor());
                } وإلا {
                    wExp = new_ملايو_اظهار(wExp, سونپيو_Wpi->rhsp()->clone_وڻ_خالص(خطأ), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((سونپيو_Wpi->unlink_Fr_پٺ()->delete_وڻ()), سونپيو_Wpi);
            }
            AstVarRef* ثابت wVarRef = جديد AstVarRef{fl, varp, VAccess::WRITE};
            Ast_سونپيو_W* ثابت wAssignp = جديد Ast_سونپيو_W{fl, wVarRef, wExp};
            سونپيو_Wp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ڌڪو_Deletep(سونپيو_Wp0), سونپيو_Wp0);
            assigns.clear();
            assigns.ڌڪو_پٺ(wAssignp);
        }
    }

    فراغ removeNotStrongerAssignments(Assigns& assigns, ثابت Ast_سونپيو_W* strongestp,
                                      uint8_t greatest_معلوم_Strength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        خودڪار removedIt = std::remove_if(assigns.begin(), assigns.پڇاڙي(), [&](Ast_سونپيو_W* assignp) {
            إذا (assignp == strongestp) أرجع خطأ;
            إذا (isAssignmentNotStrongerThanStrength(assignp, greatest_معلوم_Strength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ڌڪو_Deletep(assignp->unlink_Fr_پٺ()), assignp);
                أرجع سچ;
            }
            أرجع خطأ;
        });
        assigns.erase(removedIt, assigns.پڇاڙي());
    }

    فراغ remove_Assignments_Not_Stronger_Than_هڪجهڙو_Constant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        لكل (خودڪار& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ٻيو;
            إذا (assigns.سائيز() <= 1) استمر;
            ثابت Ast_سونپيو_W* ثابت strongest0p = get_Strongest_Assignment_جو_قدر(assigns, 0);
            ثابت Ast_سونپيو_W* ثابت strongest1p = get_Strongest_Assignment_جو_قدر(assigns, 1);
            ثابت Ast_سونپيو_W* strongestp = خالي_اشارو;
            uint8_t greatest_معلوم_Strength = 0;
            ثابت خودڪار getIfStrongest
                = [&](ثابت Ast_سونپيو_W* ثابت strongestCandidatep, منطقي قدر) {
                      إذا (!strongestCandidatep) أرجع;
                      ثابت uint8_t strength = getStrength(strongestCandidatep, قدر);
                      إذا (strength >= greatest_معلوم_Strength) {
                          greatest_معلوم_Strength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            إذا (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatest_معلوم_Strength);
            }
        }
    }

    فراغ removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        لكل (خودڪار& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.ٻيو;
            إذا (assigns.سائيز() <= 1) استمر;
            خودڪار maxIt = std::max_element(
                assigns.begin(), assigns.پڇاڙي(), [&](ثابت Ast_سونپيو_W* ap, ثابت Ast_سونپيو_W* bp) {
                    إذا (m_tgraph.isTristate(ap)) أرجع !m_tgraph.isTristate(bp);
                    إذا (m_tgraph.isTristate(bp)) أرجع خطأ;
                    ثابت uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ثابت uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    أرجع minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ثابت Ast_سونپيو_W* ثابت strongestp = m_tgraph.isTristate(*maxIt) ? خالي_اشارو : *maxIt;
            إذا (strongestp) {
                ثابت uint8_t greatest_معلوم_Strength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatest_معلوم_Strength);
            }
        }
    }

    // VISITORS
    فراغ گهمو(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        إذا (m_graphing) {
            إذا (!m_alhs && nodep->انگ().hasZ()) m_tgraph.سيٽ_Tristate(nodep);
        } وإلا {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            إذا (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ثابت varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_ڳنڍ* ثابت newp = جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            } وإلا إذا (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_سٽ* ثابت fl = nodep->fileline();
                AstConst* ثابت enp = getNonZConstp(nodep);
                V3_عدد num1{nodep, nodep->ويڪر()};
                num1.opAnd(nodep->انگ(), enp->انگ());  // 01X->01X, Z->0
                AstConst* ثابت newconstp = جديد AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    فراغ گهمو(AstCond* nodep) override {
        إذا (m_graphing) {
            iterateChildren(nodep);
            إذا (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } وإلا {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } وإلا {
            إذا (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                أرجع;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_ڳنڍ_اظهار* ثابت condp = nodep->condp();
            إذا (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_ڳنڍ_اظهار* ثابت thenp = nodep->thenp();
            Ast_ڳنڍ_اظهار* ثابت elsep = nodep->elsep();
            إذا (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_ڳنڍ_اظهار* ثابت en1p = getEnp(thenp);
                Ast_ڳنڍ_اظهار* ثابت en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_ڳنڍ_اظهار* ثابت enp
                    = جديد AstCond{nodep->fileline(), condp->clone_وڻ(خطأ), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                إذا (thenp->user1p() && !thenp->user1p()->backp()) ڌڪو_Deletep(thenp->user1p());
                إذا (elsep->user1p() && !elsep->user1p()->backp()) ڌڪو_Deletep(elsep->user1p());
                thenp->user1p(خالي_اشارو);
                elsep->user1p(خالي_اشارو);
            }
        }
    }

    فراغ گهمو(Ast_اظهار_Stmt* nodep) override {
        iterateChildren(nodep);
        إذا (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } وإلا إذا (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(خالي_اشارو);
            m_tgraph.didProcess(nodep);
        }
    }

    فراغ گهمو(AstSel* nodep) override {
        إذا (m_graphing) {
            iterateChildren(nodep);
            إذا (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } وإلا {
                associateLogic(nodep->fromp(), nodep);
            }
        } وإلا {
            إذا (m_alhs) {
                UINFO(9, dbgState() << nodep);
                إذا (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_ڳنڍ_اظهار* ثابت newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), ڳنڍ_اظهار));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } وإلا {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                إذا (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                إذا (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_ڳنڍ_اظهار* ثابت en1p = getEnp(nodep->fromp());
                    Ast_ڳنڍ_اظهار* ثابت enp
                        = جديد AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_وڻ_خالص(سچ),
                                     nodep->ويڪر_Const()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    فراغ گهمو(AstConcat* nodep) override {
        إذا (m_graphing) {
            iterateChildren(nodep);
            إذا (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } وإلا {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } وإلا {
            إذا (m_alhs) {
                UINFO(9, dbgState() << nodep);
                إذا (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_ڳنڍ_اظهار* ثابت enp = VN_AS(nodep->user1p(), ڳنڍ_اظهار);
                    nodep->user1p(خالي_اشارو);
                    nodep->lhsp()->user1p(جديد AstSel{nodep->fileline(), enp->clone_وڻ_خالص(سچ),
                                                     nodep->rhsp()->ويڪر(),
                                                     nodep->lhsp()->ويڪر()});
                    nodep->rhsp()->user1p(
                        جديد AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->ويڪر()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } وإلا {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_ڳنڍ_اظهار* ثابت expr1p = nodep->lhsp();
                Ast_ڳنڍ_اظهار* ثابت expr2p = nodep->rhsp();
                إذا (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_ڳنڍ_اظهار* ثابت en1p = getEnp(expr1p);
                    Ast_ڳنڍ_اظهار* ثابت en2p = getEnp(expr2p);
                    Ast_ڳنڍ_اظهار* ثابت enp = جديد AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(خالي_اشارو);
                    expr2p->user1p(خالي_اشارو);
                }
            }
        }
    }

    فراغ گهمو(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        إذا (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.سيٽ_Tristate(nodep);
        } وإلا {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            إذا (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                أرجع;
            }
            m_tgraph.didProcess(nodep);
            Ast_ڳنڍ_اظهار* ثابت expr1p = nodep->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت expr2p = nodep->rhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* enp;
            إذا (Ast_ڳنڍ_اظهار* ثابت en2p = VN_AS(expr2p->user1p(), ڳنڍ_اظهار)) {
                enp = جديد AstAnd{nodep->fileline(), expr1p, en2p};
            } وإلا {
                enp = expr1p;
            }
            expr1p->user1p(خالي_اشارو);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }

    فراغ گهمو_And_Or(Ast_ڳنڍ_Biop* nodep, منطقي isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        إذا (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } وإلا {
            إذا (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                أرجع;
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
            Ast_ڳنڍ_اظهار* ثابت expr1p = nodep->lhsp();
            Ast_ڳنڍ_اظهار* ثابت expr2p = nodep->rhsp();
            إذا (!expr1p->user1p() && !expr2p->user1p()) {
                أرجع;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_ڳنڍ_اظهار* en1p = getEnp(expr1p);
            Ast_ڳنڍ_اظهار* en2p = getEnp(expr2p);
            Ast_ڳنڍ_اظهار* subexpr1p = expr1p->clone_وڻ_خالص(خطأ);
            Ast_ڳنڍ_اظهار* subexpr2p = expr2p->clone_وڻ_خالص(خطأ);
            إذا (isAnd) {
                subexpr1p = جديد AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = جديد AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            إذا (!en1p->op1p() && en2p->op1p()) {
                std::مٽا_سٽا(en1p, en2p);
                std::مٽا_سٽا(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_ڳنڍ_اظهار* ثابت enp = جديد AstOr{
                nodep->fileline(),
                جديد AstAnd{nodep->fileline(), en1p, جديد AstOr{nodep->fileline(), en2p, subexpr1p}},
                جديد AstAnd{nodep->fileline(), en2p->clone_وڻ(خطأ), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(خالي_اشارو);
            expr2p->user1p(خالي_اشارو);
        }
    }
    فراغ گهمو(AstAnd* nodep) override { گهمو_And_Or(nodep, سچ); }
    فراغ گهمو(AstOr* nodep) override { گهمو_And_Or(nodep, خطأ); }

    فراغ گهمو_سونپيو(Ast_ڳنڍ_سونپيو* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        إذا (VN_IS(nodep->rhsp(), CReset)) أرجع;
        إذا (Ast_سونپيو_W* ثابت سونپيو_Wp = VN_CAST(nodep, سونپيو_W)) {
            m_currentDelayp = VN_CAST(سونپيو_Wp->timingControlp(), Delay);
        }
        إذا (m_graphing) {
            إذا (Ast_سونپيو_W* سونپيو_Wp = VN_CAST(nodep, سونپيو_W)) {
                إذا (سونپيو_Wp->getLhsNetDelay()) أرجع;
                إذا (سونپيو_Wp->timingControlp() && !m_currentDelayp) أرجع;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                إذا (m_currentDelayp && m_currentDelayp->fallDelay()) أرجع;
                إذا (!سونپيو_Wp->timingControlp()) جوڙيو_منزل_Assignment_List(سونپيو_Wp);
            }

            إذا (nodep->user2() & U2_GRAPHING) أرجع;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterate_And_ايندڙ_Null(nodep->rhsp());
            m_alhs = سچ;
            iterate_And_ايندڙ_Null(nodep->lhsp());
            m_alhs = خطأ;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } وإلا {
            إذا (nodep->user2() & U2_NONGRAPH) {
                أرجع;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterate_And_ايندڙ_Null(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            إذا (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, سونپيو_W), nodep,
                            "Continuous assignment inside procedural process");
                Ast_ڳنڍ_اظهار* ثابت enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), ڳنڍ_اظهار)->clone_وڻ_خالص(خطأ)
                          : newAllZerosOrOnes(nodep->lhsp(), سچ);
                ثابت منطقي inserted = m_سونپيو_Enps.emplace(nodep, enp).ٻيو;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            إذا (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(خالي_اشارو);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } وإلا {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                إذا (AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel)) {
                    إذا (Ast_ڳنڍ_Var_Ref* ثابت varrefp = VN_CAST(selp->fromp(), ڳنڍ_Var_Ref)) {
                        إذا (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), سچ));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } وإلا {
                            m_tgraph.didProcess(nodep, سچ);
                        }
                    } وإلا {
                        m_tgraph.didProcess(nodep, سچ);  // LCOV_EXCL_LINE
                    }
                } وإلا {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, سچ);
                }
            }
            m_alhs = سچ;  // And user1p() will indicate tristate equation, if any
            إذا (Ast_سونپيو_W* ثابت سونپيو_Wp = VN_CAST(nodep, سونپيو_W)) {
                إذا (AstStrengthSpec* ثابت specp = سونپيو_Wp->strengthSpecp()) {
                    إذا (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        إذا (!is_Only_Assignment_Is_منزل_Lhs_Var(سونپيو_Wp)) {
                            سونپيو_Wp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } وإلا {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterate_And_ايندڙ_Null(nodep->lhsp());
        }
    }
    فراغ گهمو(Ast_سونپيو_W* nodep) override { گهمو_سونپيو(nodep); }
    فراغ گهمو(Ast_سونپيو* nodep) override { گهمو_سونپيو(nodep); }
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override { گهمو_سونپيو(nodep); }

    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ثابت AstAlways* ثابت alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_سونپيو ? خالي_اشارو : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    فراغ گهمو(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_اندريون_Alias);
        m_اندريون_Alias = سچ;
        إذا (m_graphing) {
            إذا (nodep->user2() & U2_GRAPHING) أرجع;
            m_alhs = سچ;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            لكل (Ast_ڳنڍ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } وإلا {
            iterateChildren(nodep);
        }
    }

    فراغ گهمو_Case_Eq(Ast_ڳنڍ_Biop* nodep, منطقي neq) {
        إذا (m_graphing) {
            iterateChildren(nodep);
        } وإلا {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
            إذا (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp();
                rhsp->unlink_Fr_پٺ();
                File_سٽ* ثابت fl = nodep->fileline();
                Ast_ڳنڍ_اظهار* enRhsp;
                إذا (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), ڳنڍ_اظهار);
                    rhsp->user1p(خالي_اشارو);
                } وإلا {
                    enRhsp = get_En_اظهار_Based_On_Originalp(rhsp);
                }
                Ast_ڳنڍ_اظهار* newp
                    = جديد AstLogAnd{fl, جديد AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    جديد AstEqCase{fl, جديد AstConst{fl, constp->انگ()}, rhsp}};
                constp->user1p(خالي_اشارو);
                إذا (neq) newp = جديد AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            } وإلا إذا (constp && nodep->rhsp()->user1p()) {
                File_سٽ* ثابت fl = nodep->fileline();
                constp->unlink_Fr_پٺ();
                Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
                Ast_ڳنڍ_اظهار* newp = جديد AstLogAnd{fl,
                                                  جديد AstEq{fl, newAllZerosOrOnes(constp, خطأ),
                                                            VN_AS(rhsp->user1p(), ڳنڍ_اظهار)},
                                                  // Keep the caseeq if there are X's present
                                                  جديد AstEqCase{fl, constp, rhsp}};
                إذا (neq) newp = جديد AstLogNot{fl, newp};
                rhsp->user1p(خالي_اشارو);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            } وإلا {
                checkUnhandled(nodep);
            }
        }
    }
    فراغ گهمو_Eq_Neq_Wild(Ast_ڳنڍ_Biop* nodep) {
        إذا (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterate_And_ايندڙ_Null(nodep->lhsp());
        إذا (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            أرجع;
        }
    }
    فراغ گهمو(AstEqCase* nodep) override { گهمو_Case_Eq(nodep, خطأ); }
    فراغ گهمو(AstNeqCase* nodep) override { گهمو_Case_Eq(nodep, سچ); }
    فراغ گهمو(AstEqWild* nodep) override { گهمو_Eq_Neq_Wild(nodep); }
    فراغ گهمو(AstNeqWild* nodep) override { گهمو_Eq_Neq_Wild(nodep); }

    فراغ گهمو(Ast_ڳڻپ_بٽ* nodep) override {
        std::array<منطقي, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->انگ().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->انگ().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->انگ().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        إذا (m_graphing) {
            iterate_And_ايندڙ_Null(nodep->lhsp());
            إذا (!dropop[0]) iterate_And_ايندڙ_Null(nodep->rhsp());
            إذا (!dropop[1]) iterate_And_ايندڙ_Null(nodep->thsp());
            إذا (!dropop[2]) iterate_And_ايندڙ_Null(nodep->fhsp());
        } وإلا {
            Ast_ڳنڍ_اظهار* nonXp = خالي_اشارو;
            إذا (!dropop[0]) {
                nonXp = nodep->rhsp();
            } وإلا إذا (!dropop[1]) {
                nonXp = nodep->thsp();
            } وإلا إذا (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            إذا (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                إذا (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    أرجع;
                }
                AstVar* ثابت envarp = getCreateEnVarp(varrefp->varp(), خطأ);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlink_Fr_پٺ(&relinkHandle);
                Ast_ڳنڍ_اظهار* newp = جديد Ast_ڳڻپ_Ones{
                    nodep->fileline(), جديد AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                إذا (nonXp) {  // Need to still count '0 or '1 or 'x's
                    إذا (dropop[0]) {
                        nodep->rhsp()->unlink_Fr_پٺ()->delete_وڻ();
                        nodep->rhsp(nonXp->clone_وڻ_خالص(سچ));
                    }
                    إذا (dropop[1]) {
                        nodep->thsp()->unlink_Fr_پٺ()->delete_وڻ();
                        nodep->thsp(nonXp->clone_وڻ_خالص(سچ));
                    }
                    إذا (dropop[2]) {
                        nodep->fhsp()->unlink_Fr_پٺ()->delete_وڻ();
                        nodep->fhsp(nonXp->clone_وڻ_خالص(سچ));
                    }
                    newp = جديد Ast_جوڙيو{nodep->fileline(), nodep, newp};
                } وإلا {
                    ڌڪو_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    فراغ گهمو(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = خالي_اشارو;
        إذا (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } وإلا إذا (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        إذا (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } وإلا {
            إذا (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.سيٽ_Tristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } وإلا {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ثابت AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel);
                ثابت AstConst* ثابت lsbp = selp ? VN_CAST(selp->lsbp(), Const) : خالي_اشارو;
                إذا (lsbp) {
                    ثابت صحيح lsb = lsbp->منزل_SInt();
                    لكل (صحيح i = 0; i < selp->ويڪر_Const(); ++i) {
                        سيٽ_ٻٽو_انگ_Pull_Direction(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } وإلا {
                    سيٽ_Pull_Direction(varrefp->varp(), nodep);
                }
            }
        }
        إذا (!m_graphing) {
            nodep->unlink_Fr_پٺ();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }

    فراغ iteratePinGuts(AstPin* nodep) {
        إذا (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            إذا (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } وإلا {
            // All heavy lifting completed in graph visitor.
            إذا (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    فراغ گهمو(AstPin* nodep) override {
        إذا (m_graphing) {
            إذا (nodep->user2() & U2_GRAPHING) أرجع;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ثابت enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            إذا (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, خطأ);
                iteratePinGuts(nodep);
                أرجع;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ثابت منطقي اندريون_Decl_Processing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::داخلا
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            إذا (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ثابت ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(جديد AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.سيٽ_Tristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } وإلا إذا (اندريون_Decl_Processing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ثابت Ast_سونپيو_W* ثابت reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, سچ, سچ);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.سيٽ_Tristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ثابت enVarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvt_منزل_Str(m_unique++),
                                              VFlag_ٻٽو_انگ_Packed{}, enModVarp->ويڪر()};
            إذا (اندريون_Decl_Processing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->var_Type2_اندريون();
            } وإلا {
                enModVarp->var_Type2_ٻاهريون();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ثابت enpinp
                = جديد AstPin{nodep->fileline(), nodep->pin_انگ(),
                             enModVarp->name(),  // should be {var}"__en"
                             جديد AstVarRef{nodep->fileline(), enVarp,
                                           اندريون_Decl_Processing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->جوڙيو_ايندڙ_هتي(enpinp);
            m_modp->جوڙيو_Stmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ثابت Ast_سونپيو_W* ٻاهريون_Assignp = خالي_اشارو;  // If reconnected, the related assignment
            AstPin* outpinp = خالي_اشارو;
            AstVar* ثابت ٻاهريون_Mod_Varp = m_varAux(nodep->modVarp()).ٻاهريون_Varp;
            إذا (!ٻاهريون_Mod_Varp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                إذا (!m_modp->is_چوٽي()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } وإلا {
                Ast_ڳنڍ_اظهار* ثابت outexprp = VN_AS(nodep->exprp(), ڳنڍ_اظهار)
                                                  ->clone_وڻ_خالص(خطأ);  // Note has lvalue() set
                outpinp = جديد AstPin{nodep->fileline(), nodep->pin_انگ(),
                                     ٻاهريون_Mod_Varp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(ٻاهريون_Mod_Varp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->جوڙيو_ايندڙ_هتي(outpinp);
                // Simplify
                إذا (اندريون_Decl_Processing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, سچ};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                ٻاهريون_Assignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        سچ);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, ٻاهريون_Assignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ثابت TristatePinVisitor visitor{nodep->exprp(), m_tgraph, خطأ};
            ثابت Ast_ڳنڍ* ثابت اندريون_Assignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, سچ);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, اندريون_Assignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            إذا (!ٻاهريون_Assignp) {
                إذا (!outpinp) {
                    exprrefp = خالي_اشارو;  // Primary input only
                } وإلا {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    إذا (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } وإلا {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(ٻاهريون_Assignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                إذا (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            إذا (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(جديد AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                إذا (!ٻاهريون_Assignp) {
                    map_وجهو_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            إذا (exprrefp) {
                AstPull* ثابت pullp = m_varAux(nodep->modVarp()).pullp;
                ثابت خودڪار& src_ٻٽو_انگ_Pulls = m_varAux(nodep->modVarp()).ٻٽو_انگ_Pulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = خالي_اشارو;
                صحيح selLsb = 0;
                صحيح sel_ويڪر = 0;
                إذا (ٻاهريون_Assignp) {
                    إذا (AstSel* ثابت selp = VN_CAST(ٻاهريون_Assignp->lhsp(), Sel)) {
                        AstVarRef* ثابت vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
                        إذا (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->منزل_SInt();
                            sel_ويڪر = selp->ويڪر_Const();
                        }
                    }
                }

                إذا (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    سيٽ_Pull_Direction(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    إذا (selVarp) {
                        ثابت صحيح direction = pullp->direction();
                        لكل (صحيح i = 0; i < sel_ويڪر; ++i) {
                            سيٽ_ٻٽو_انگ_Pull_Direction(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                إذا (!src_ٻٽو_انگ_Pulls.خالي_جايون()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    لكل (ثابت خودڪار& pair : src_ٻٽو_انگ_Pulls) {
                        سيٽ_ٻٽو_انگ_Pull_Direction(exprrefp->varp(), pair.پهريون, pair.ٻيو);
                    }  // LCOV_EXCL_LINE
                    إذا (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        لكل (ثابت خودڪار& pair : src_ٻٽو_انگ_Pulls) {
                            سيٽ_ٻٽو_انگ_Pull_Direction(selVarp, selLsb + pair.پهريون, pair.ٻيو);
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
        وإلا {
            إذا (nodep->user2() & U2_NONGRAPH) أرجع;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    فراغ handle_ڳنڍ_Var_Ref(Ast_ڳنڍ_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        إذا (m_graphing) {
            إذا (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            إذا (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            إذا (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.سيٽ_Tristate(nodep->varp());
            }
        } وإلا {
            إذا (nodep->user2() & U2_NONGRAPH) أرجع;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            إذا (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                إذا (m_اندريون_Alias) {
                    إذا (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } وإلا {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    أرجع;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_وجهو_Lhs_Var_Ref(nodep);
            } وإلا إذا (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_سٽ* ثابت fl = nodep->fileline();
                AstVar* ثابت enVarp = getCreateEnVarp(nodep->varp(), خطأ);
                إذا (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } وإلا {
                    nodep->user1p(جديد AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    فراغ گهمو(AstVarRef* nodep) override { handle_ڳنڍ_Var_Ref(nodep); }
    فراغ گهمو(AstVarXRef* nodep) override { handle_ڳنڍ_Var_Ref(nodep); }

    فراغ گهمو(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        إذا (m_graphing) {
            // If tri0/1 force a pullup
            إذا (nodep->user2() & U2_GRAPHING) أرجع;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            إذا (nodep->isPulldown() || nodep->isPullup()) {
                Ast_ڳنڍ* ثابت newp = جديد AstPull{
                    nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->جوڙيو_ايندڙ_هتي(newp);
                // We'll iterate on the new AstPull later
            }
            إذا (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.سيٽ_Tristate(nodep);
            }
        } وإلا {  // !graphing
            إذا (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        إذا (m_processedIfaces && VN_IS(nodep, Iface)) أرجع;
        UINFO(8, dbgState() << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_graphing);
        VL_RESTORER(m_unique);
        VL_RESTORER_CLEAR(m_lhsmap);
        VL_RESTORER_CLEAR(m_assigns);
        VL_RESTORER_CLEAR(m_سونپيو_Enps);
        VL_RESTORER_CLEAR(m_proceduralEnables);
        // Not preserved, needs pointer instead: TristateGraph origTgraph = m_tgraph;
        UASSERT_OBJ(m_tgraph.خالي_جايون(), nodep, "Unsupported: NodeModule under NodeModule");

        // Clear state
        m_graphing = خطأ;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = خالي_اشارو;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = سچ;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = خطأ;
        UINFO(9, dbgState() << "processing mod " << nodep);

        // Merge the assignments for very Wired net LHS : wor, trior, wand and triand
        ملايو_Wired_Nets_Assignments();
        // Remove all assignments not stronger than the strongest uniform constant
        remove_Assignments_Not_Stronger_Than_هڪجهڙو_Constant();
        // Use graph to find tristate signals
        m_tgraph.گراف_Walk(nodep);

        // Remove all assignments not stronger than the strongest non-tristate RHS
        removeAssignmentsNotStrongerThanNonTristate();

        // Build the LHS drivers map for this module
        iterateChildren(nodep);
        // Insert new logic for all tristates
        وجهو_Tristates(nodep);
        وجهو_Procedural_Enable_Assignments(nodep);

        لكل (خودڪار& سونپيو_Enp : m_سونپيو_Enps) {
            VL_DO_DANGLING(سونپيو_Enp.ٻيو->delete_وڻ(), سونپيو_Enp.ٻيو);
        }
        m_سونپيو_Enps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    فراغ گهمو(AstClass* nodep) override {
        // don't deal with classes
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        // don't deal with functions
    }

    فراغ گهمو(Ast_Case_شي* nodep) override {
        // don't deal with casez compare '???? values
        iterate_And_ايندڙ_Null(nodep->stmtsp());
    }

    فراغ گهمو(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = خطأ;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    فراغ combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        لكل (ثابت std::pair<AstVar* ثابت, std::ويڪٽر<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ثابت invarp = kv.پهريون;
            ثابت std::ويڪٽر<std::pair<AstVar*, AstVar*>>& contribs = kv.ٻيو;
            Ast_ڳنڍ_Module* ثابت ifaceModp = ڳوليو_والد_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_سٽ* ثابت fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.سائيز() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_ڳنڍ_اظهار* orp = خالي_اشارو;
            Ast_ڳنڍ_اظهار* enp = خالي_اشارو;

            لكل (ثابت std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ثابت contribEnp = contrib.پهريون;
                AstVar* ثابت contribOutp = contrib.ٻيو;

                Ast_ڳنڍ_اظهار* ثابت ٻاهريون_Refp = جديد AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_ڳنڍ_اظهار* ثابت enRefp = جديد AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_ڳنڍ_اظهار* ثابت andp = جديد AstAnd{fl, ٻاهريون_Refp, enRefp};

                orp = (!orp) ? andp : جديد AstOr{fl, orp, andp};

                Ast_ڳنڍ_اظهار* ثابت enRef2p = جديد AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : جديد AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            إذا (!envarp) {
                envarp = جديد AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->سيٽ_Dfg_Tri_Lowered();
                ifaceModp->جوڙيو_Stmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            Ast_سونپيو_W* ثابت enAssp
                = جديد Ast_سونپيو_W{fl, جديد AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->جوڙيو_Stmtsp(جديد AstAlways{enAssp});

            // Pull resolution
            ثابت AstPull* ثابت pullp = m_varAux(invarp).pullp;
            ثابت منطقي pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_ڳنڍ_اظهار* undrivenp = جديد AstNot{fl, جديد AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = جديد AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = جديد AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            Ast_سونپيو_W* ثابت assp
                = جديد Ast_سونپيو_W{fl, جديد AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->جوڙيو_Stmtsp(جديد AstAlways{assp});
        }
    }

عام:
    // CONSTRUCTORS
    واضح TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::ويڪٽر<Ast_ڳنڍ_Module*> ifacesp;
        لكل (Ast_ڳنڍ* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            إذا (VN_IS(modp, Iface)) ifacesp.ڌڪو_پٺ(VN_AS(modp, ڳنڍ_Module));
        }
        لكل (خودڪار it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = سچ;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#جيڪڏهن_وصف_ٿيل VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::ويڪٽر<Ast_ڳنڍ*> اڻ_استعمال_ٿيل_Rootps;
        netlistp->foreach([&](Ast_ڳنڍ* nodep) {
            Ast_ڳنڍ* ثابت enp = nodep->user1p();
            إذا (!enp) أرجع;
            إذا (enp->backp()) أرجع;
            اڻ_استعمال_ٿيل_Rootps.emplace_پٺ(enp);
        });
        لكل (Ast_ڳنڍ* ثابت nodep : اڻ_استعمال_ٿيل_Rootps) VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
#جيڪڏهن_پورو
    }
    ~TristateVisitor() override {
        V3Stats::جوڙيو_Stat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

فراغ V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("tristate", 0, dump_وڻ_Either_Level() >= 3);
}
