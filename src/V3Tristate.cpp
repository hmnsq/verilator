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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Tristate.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3Graph.h"
#شامل "V3Inst.h"
#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت Tristate_بنیاد_Visitor VL_NOT_FINAL : عوامی VNVisitor {
عوامی:
    // METHODS
};

//######################################################################
// Graph support classes

جماعت Tristate_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Tristate_راس, V3_گراف_راس)
    Ast_گرہ* ثابت m_nodep;
    منطقی m_isTristate = جھوٹ;  // Logic indicates a tristate
    منطقی m_feedsTri = جھوٹ;  // Propagates to a tristate node (on RHS)
    منطقی m_processed = جھوٹ;  // Tristating was cleaned up
عوامی:
    Tristate_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Tristate_راس() override = طےشدہ;
    // ACCESSORS
    Ast_گرہ* nodep() ثابت VL_MT_STABLE { واپس m_nodep; }
    ثابت AstVar* varp() ثابت { واپس VN_CAST(nodep(), Var); }
    string name() ثابت override {
        واپس ((isTristate() ? "tri\\n"
                 : feedsTri() ? "feed\\n"
                              : "-\\n")
                + (nodep()->prettyTypeName() + " " + cvt_مقصد_Hex(nodep())));
    }
    string dotColor() ثابت override {
        واپس (varp() ? (isTristate() ? "darkblue"
                          : feedsTri() ? "blue"
                                       : "lightblue")
                       : (isTristate() ? "darkgreen"
                          : feedsTri() ? "green"
                                       : "lightgreen"));
    }
    File_سطر* fileline() ثابت override { واپس nodep()->fileline(); }
    منطقی isTristate() ثابت VL_MT_محفوظ { واپس m_isTristate; }
    خالی isTristate(منطقی flag) { m_isTristate = flag; }
    منطقی feedsTri() ثابت VL_MT_محفوظ { واپس m_feedsTri; }
    خالی feedsTri(منطقی flag) { m_feedsTri = flag; }
    منطقی processed() ثابت { واپس m_processed; }
    خالی processed(منطقی flag) { m_processed = flag; }
};

//######################################################################

جماعت Tristate_گراف final {
    // NODE STATE
    //   AstVar::user4p         -> TristateVertex* for variable being built
    // VNUser4InUse     m_inuser4;   // In visitor below

    // TYPES
عوامی:
    استعمال VarVec = std::سمتیہ<AstVar*>;

نجی:
    // MEMBERS
    V3_گراف m_گراف;  // Logic graph

عوامی:
    // CONSTRUCTORS
    Tristate_گراف() { clearAndCheck(); }
    مجازی ~Tristate_گراف() { clearAndCheck(); }
    VL_UNCOPYABLE(Tristate_گراف);

نجی:
    // METHODS

    Tristate_راس* make_راس(Ast_گرہ* nodep) {
        Tristate_راس* vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p());
        اگر (!vertexp) {
            UINFO(6, "         New vertex " << nodep);
            vertexp = نیا Tristate_راس{&m_گراف, nodep};
            nodep->user4p(vertexp);
        }
        واپس vertexp;
    }

    // METHODS - Graph optimization
    خالی گراف_Walk_Recurse_Fwd(Tristate_راس* vtxp, صحیح level) {
        // Propagate tristate forward to all sinks
        // For example if on a CONST, propagate through CONCATS to ASSIGN
        // to LHS VARREF of signal to tristate
        اگر (!vtxp->isTristate()) واپس;  // tristate involved
        اگر (vtxp->user() == 1) واپس;
        vtxp->user(1);  // Recursed
        UINFO(9, "  Mark tri " << level << "  " << vtxp);
        اگر (!vtxp->varp()) {  // not a var where we stop the recursion
            برائے (V3_گراف_کنارہ& کنارہ : vtxp->برآمد_کنارے()) {
                Tristate_راس* ثابت vvertexp = static_cast<Tristate_راس*>(کنارہ.چوٹی());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                اگر (!vvertexp->isTristate()) {
                    vvertexp->isTristate(سچ);
                    گراف_Walk_Recurse_Fwd(vvertexp, level + 1);
                }
            }
        } ورنہ {
            // A variable is tristated.  Find all of the LHS VARREFs that
            // drive this signal; they now need tristate drivers
            برائے (V3_گراف_کنارہ& کنارہ : vtxp->اندر_کنارے()) {
                Tristate_راس* ثابت vvertexp = static_cast<Tristate_راس*>(کنارہ.fromp());
                اگر (ثابت AstVarRef* ثابت refp = VN_CAST(vvertexp->nodep(), VarRef)) {
                    اگر (refp->access().isWriteOrRW()
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
    خالی گراف_Walk_Recurse_عقب(Tristate_راس* vtxp, صحیح level) {
        // Called only on a tristate node; propagate a feedsTri attribute "backwards"
        // towards any driving nodes, i.e. from a LHS VARREF back to a driving RHS VARREF
        // This way if the RHS VARREF is also tristated we'll connect the
        // enables up to the LHS VARREF. Otherwise if not marked
        // feedsTri() we'll drop the LHS' enables, if any
        اگر (!(vtxp->isTristate() || vtxp->feedsTri())) واپس;  // tristate involved
        اگر (vtxp->user() == 3) واپس;
        vtxp->user(3);  // Recursed
        UINFO(9, "  Mark feedstri " << level << "  " << vtxp);
        اگر (!vtxp->varp()) {  // not a var where we stop the recursion
            برائے (V3_گراف_کنارہ& کنارہ : vtxp->اندر_کنارے()) {
                Tristate_راس* ثابت vvertexp = static_cast<Tristate_راس*>(کنارہ.fromp());
                // Doesn't hurt to not check if already set, but by doing so when we
                // print out the debug messages, we'll see this node at level 0 instead.
                اگر (!vvertexp->feedsTri()) {
                    vvertexp->feedsTri(سچ);
                    گراف_Walk_Recurse_عقب(vvertexp, level + 1);
                }
            }
        }
    }

عوامی:
    // METHODS
    منطقی خالی_خانے() ثابت { واپس m_گراف.خالی_خانے(); }
    خالی clearAndCheck() {
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            ثابت Tristate_راس& vvertex = static_cast<Tristate_راس&>(vtx);
            اگر (vvertex.isTristate() && !vvertex.processed()) {
                // Not v3errorSrc as no reason to stop the world
                vvertex.nodep()->v3warn(E_UNSUPPORTED, "Unsupported tristate construct"
                                                       " (in graph; not converted): "
                                                           << vvertex.nodep()->prettyTypeName());
            }
        }
        m_گراف.clear();
        Ast_گرہ::user4_Clear_درخت();  // Wipe all node user4p's that point to vertexes
    }
    خالی گراف_Walk(Ast_گرہ_Module* nodep) {
        UINFO(9, " Walking " << nodep);
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            گراف_Walk_Recurse_Fwd(static_cast<Tristate_راس*>(&vtx), 0);
        }
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            گراف_Walk_Recurse_عقب(static_cast<Tristate_راس*>(&vtx), 0);
        }
        اگر (dump_گراف_Level() >= 9) m_گراف.dumpDotFilePrefixed("tri_pos__" + nodep->name());
    }
    خالی associate(Ast_گرہ* fromp, Ast_گرہ* چوٹی) {
        نیا V3_گراف_کنارہ{&m_گراف, make_راس(fromp), make_راس(چوٹی), 1};
    }
    خالی deleteVerticesFromSubtreeRecurse(Ast_گرہ* nodep) {
        اگر (!nodep) واپس;
        // Skip vars, because they may be connected to more than one varref
        اگر (!VN_IS(nodep, Var)) {
            Tristate_راس* ثابت vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p());
            اگر (vertexp) vertexp->unlinkDelete(&m_گراف);
        }
        اگر (Ast_گرہ* ثابت refp = nodep->op1p()) deleteVerticesFromSubtreeRecurse(refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op2p()) deleteVerticesFromSubtreeRecurse(refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op3p()) deleteVerticesFromSubtreeRecurse(refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op4p()) deleteVerticesFromSubtreeRecurse(refp);
    }
    خالی سیٹ_Tristate(Ast_گرہ* nodep) { make_راس(nodep)->isTristate(سچ); }
    منطقی isTristate(ثابت Ast_گرہ* nodep) {
        ثابت Tristate_راس* ثابت vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p());
        واپس vertexp && vertexp->isTristate();
    }
    منطقی feedsTri(ثابت Ast_گرہ* nodep) {
        ثابت Tristate_راس* ثابت vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p());
        واپس vertexp && vertexp->feedsTri();
    }
    خالی didProcess(Ast_گرہ* nodep, منطقی quiet = جھوٹ) {
        اگر (Tristate_راس* ثابت vertexp = reinterpret_cast<Tristate_راس*>(nodep->user4p())) {
            // We don't warn if no vertexp->isTristate() as the creation
            // process makes midling nodes that don't have it set
            vertexp->processed(سچ);
        } ورنہ اگر (!quiet) {
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
        برائے (ثابت V3_گراف_راس& vtx : m_گراف.vertices()) {
            ثابت Tristate_راس& vvertex = static_cast<ثابت Tristate_راس&>(vtx);
            اگر (vvertex.isTristate()) {
                اگر (AstVar* ثابت nodep = VN_CAST(vvertex.nodep(), Var)) v.ڈالو_عقب(nodep);
            }
        }
        واپس v;
    }
};

//######################################################################
// Given a node, flip any VarRef from LValue to RValue (i.e. make it an input)
// See also V3LinkLValue::linkLValueSet

جماعت TristatePinVisitor final : عوامی Tristate_بنیاد_Visitor {
    Tristate_گراف& m_tgraph;
    ثابت منطقی m_lvalue;  // Flip to be an LVALUE
    // VISITORS
    // AstNodeVarRef, not just AstVarRef: a cross-hierarchy pin expression into an
    // interface (.pin(iface.net)) is an AstVarXRef and needs the same access flip.
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access flip");
        اگر (m_lvalue && !nodep->access().isWriteOrRW()) {
            UINFO(9, "  Flip-to-LValue " << nodep);
            nodep->access(VAccess::WRITE);
        } ورنہ اگر (!m_lvalue && !nodep->access().isReadOnly()) {
            UINFO(9, "  Flip-to-RValue " << nodep);
            nodep->access(VAccess::READ);
            // Mark the ex-output as tristated
            UINFO(9, "  setTristate-subpin " << nodep->varp());
            m_tgraph.سیٹ_Tristate(nodep->varp());
        }
    }
    خالی دورہ(AstArraySel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "ArraySel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_قاش_Sel* nodep) override {
        // Doesn't work because we'd set lvalue on the array index's var
        UASSERT_OBJ(!m_lvalue, nodep, "SliceSel conversion to output, under tristate node");
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    TristatePinVisitor(Ast_گرہ* nodep, Tristate_گراف& tgraph, منطقی lvalue)
        : m_tgraph(tgraph)  // Need () or GCC 4.8 false warning
        , m_lvalue{lvalue} {
        iterate(nodep);
    }
    ~TristatePinVisitor() override = طےشدہ;
};

//######################################################################

جماعت TristateVisitor final : عوامی Tristate_بنیاد_Visitor {
    // NODE STATE
    //   *::user1p              -> pointer to output enable __en expressions
    //   *::user2               -> int - already visited, see U2_ enum
    //   AstVar::user3p         -> See AuxAstVar via m_varAux
    // See TristateGraph:
    //   AstVar::user4p         -> TristateVertex* for variable being built
    //   AstStmt::user4p        -> TristateVertex* for this statement
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    ثابت VNUser3_اندر_Use m_inuser3;
    ثابت VNUser4_اندر_Use m_inuser4;

    ساخت AuxAstVar final {
        AstPull* pullp = خالی_اشاریہ;  // pullup/pulldown direction (whole variable)
        AstVar* برآمد_Varp = خالی_اشاریہ;  // output __out var
        منطقی ifaceTristate = جھوٹ;  // Interface var known to be tristate (set when the
                                     // interface module is processed). Lets a module that
                                     // drives this var across hierarchy with a plain (non-Z)
                                     // assign be recognised as a tristate contributor even
                                     // though that module's own graph has no Z on the net.
        std::unordered_map<صحیح, صحیح>
            ثنائی_ہندسہ_Pulls;  // Per-bit pull: bit_index -> direction (1=up, 0=down)
    };

    AstUser3Allocator<AstVar, AuxAstVar> m_varAux;

    // TYPES
    ساخت RefStrength final {
        Ast_گرہ_Var_Ref* m_varrefp;  // Variable reference this strength/enable info was found on
        Ast_گرہ_تفویض* m_assignp;  // Assignment containing m_varrefp
        ثابت Ast_گرہ_Procedure* m_procedurep;  // Process containing m_assignp, if procedural
        Ast_گرہ_اظہاریہ* m_enLhsp;  // Procedural enable LHS with same footprint as m_varrefp
        Ast_گرہ_Var_Ref* m_enVarrefp;  // Variable reference within m_enLhsp to retarget
        Ast_گرہ_اظہاریہ* m_enRhsp;  // Procedural enable value matching m_enLhsp
        VStrength m_strength;  // Drive strength of the assignment/procedural write
        AstDelay* m_delayp;  // Explicit delay on the continuous assignment
        RefStrength(Ast_گرہ_Var_Ref* varrefp, Ast_گرہ_تفویض* assignp,
                    ثابت Ast_گرہ_Procedure* procedurep, Ast_گرہ_اظہاریہ* enLhsp,
                    Ast_گرہ_Var_Ref* enVarrefp, Ast_گرہ_اظہاریہ* enRhsp, VStrength strength,
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
    ساخت LhsProjection final {
        Ast_گرہ_اظہاریہ* m_lhsp;  // LHS giving the enable assign
        Ast_گرہ_Var_Ref* m_varrefp;  // Variable reference within m_lhsp to retarget to enable var
        Ast_گرہ_اظہاریہ* m_enp;  // Procedural enable value to assign through m_lhsp
    };
    ساخت ProceduralEnable final {
        Ast_گرہ_اظہاریہ* m_lhsp;  // Enable LHS piece to concat onto the combined assignment LHS
        Ast_گرہ_اظہاریہ* m_rhsp;  // Enable value piece to concat onto the combined assignment RHS
    };
    استعمال RefStrengthVec = std::سمتیہ<RefStrength>;
    استعمال VarMap = std::map<AstVar*, RefStrengthVec*>;
    استعمال Assigns = std::سمتیہ<Ast_تفویض_W*>;
    استعمال Var_مقصد_Assigns_Map = std::map<AstVar*, Assigns>;
    استعمال تفویض_Enable_Map = std::map<Ast_گرہ_تفویض*, Ast_گرہ_اظہاریہ*>;
    استعمال ProceduralEnableMap = std::map<Ast_گرہ_تفویض*, std::سمتیہ<ProceduralEnable>>;
    شمار : uint8_t {
        U2_GRAPHING = 1,  // bit[0] if did m_graphing visit
        U2_NONGRAPH = 2,  // bit[1] if did !m_graphing visit
        U2_BOTH = 3  // Both bits set
    };

    // MEMBERS
    منطقی m_graphing = جھوٹ;  // Major mode - creating graph
    //
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    AstCell* m_cellp = خالی_اشاریہ;  // current cell
    VarMap m_lhsmap;  // Tristate left-hand-side driver map
    Var_مقصد_Assigns_Map m_assigns;  // Assigns in current module
    تفویض_Enable_Map m_تفویض_Enps;  // Raw enable expression for each procedural assignment
    ProceduralEnableMap m_proceduralEnables;  // Enable updates to merge into assignments
    صحیح m_unique = 0;
    منطقی m_alhs = جھوٹ;  // On LHS of assignment
    منطقی m_اندر_Alias = جھوٹ;  // Inside alias statement
    منطقی m_processedIfaces = جھوٹ;  // Interface modules already processed (interfaces-first pass)
    VStrength m_currentStrength = VStrength::STRONG;  // Current strength of assignment,
                                                      // Used only on LHS of assignment
    AstDelay* m_currentDelayp = خالی_اشاریہ;  // Delay of assignment currently collecting LHS drivers
    ثابت Ast_گرہ* m_logicp = خالی_اشاریہ;  // Current logic being built
    Ast_گرہ_تفویض* m_assignp = خالی_اشاریہ;  // Current assignment
    ثابت Ast_گرہ_Procedure* m_procedurep = خالی_اشاریہ;  // Current procedural process
    Tristate_گراف m_tgraph;  // Logic graph
    // Map: interface AstVar* -> list of per-module (enVarp, outVarp) contribution pairs
    std::map<AstVar*, std::سمتیہ<std::pair<AstVar*, AstVar*>>> m_ifaceContribs;

    // STATS
    VDouble0 m_statTriSigs;  // stat tracking

    // METHODS
    string dbgState() ثابت {
        string o = (m_graphing ? " gr " : " ng ");
        اگر (m_alhs) o += "alhs ";
        واپس o;
    }
    خالی mod_جوڑو_Stmtp(Ast_گرہ* nodep, Ast_گرہ* newp) {
        اگر (!m_modp) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Creating tristate signal not underneath a module: "
                              << nodep->prettyNameQ());
        } ورنہ {
            m_modp->جوڑو_Stmtsp(newp);
        }
    }
    خالی associateLogic(Ast_گرہ* fromp, Ast_گرہ* چوٹی) {
        اگر (m_logicp) m_tgraph.associate(fromp, چوٹی);
    }
    AstConst* newAllZerosOrOnes(Ast_گرہ* nodep, منطقی ones) {
        V3_نمبر عدد{nodep, nodep->چوڑائی()};
        اگر (ones) عدد.سیٹ_All_Bits1();
        AstConst* ثابت newp = نیا AstConst{nodep->fileline(), عدد};
        واپس newp;
    }
    // Create AstVarXRef with inlinedDots copied from a source xref
    AstVarXRef* newVarXRef(File_سطر* fl, AstVar* varp, ثابت string& dotted, VAccess access,
                           ثابت string& inlinedDots) {
        AstVarXRef* ثابت xrefp = نیا AstVarXRef{fl, varp, dotted, access};
        xrefp->inlinedDots(inlinedDots);
        واپس xrefp;
    }
    // Check if a module accesses an interface through a modport, by looking up
    // the first component of the dotted path among the module's interface port vars.
    // No symbol table is available at V3Tristate time, so linear scan is required.
    AstModport* ڈھونڈو_Modport_For_Dotted(Ast_گرہ_Module* modp, ثابت string& dotted_راستہ) {
        اگر (dotted_راستہ.خالی_خانے()) واپس خالی_اشاریہ;
        ثابت string پہلا_Comp = dotted_راستہ.substr(0, dotted_راستہ.ڈھونڈو('.'));
        برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت ivarp = VN_CAST(stmtp, Var);
            اگر (!ivarp || ivarp->name() != پہلا_Comp) جاری;
            ثابت AstIfaceRefDType* ثابت ifaceDtp
                = VN_CAST(ivarp->dtypep()->skipRefp(), IfaceRefDType);
            اگر (ifaceDtp && ifaceDtp->modportp()) واپس ifaceDtp->modportp();
            توڑو;
        }
        واپس خالی_اشاریہ;
    }
    Ast_گرہ_اظہاریہ* getEnp(Ast_گرہ* nodep) {
        اگر (nodep->user1p()) {
            اگر (Ast_گرہ_Var_Ref* ثابت refp = VN_CAST(nodep, گرہ_Var_Ref)) {
                اگر (refp->varp()->isIO()) {
                    // When reading a tri-state port, we can always use the value
                    // because such port will have resolution logic in upper module.
                    واپس newAllZerosOrOnes(nodep, سچ);
                }
            }
        } ورنہ {
            // There's no select being built yet, so add what will become a
            // constant output enable driver of all 1's
            nodep->user1p(newAllZerosOrOnes(nodep, سچ));
        }
        // Otherwise return the previous output enable
        واپس VN_AS(nodep->user1p(), گرہ_اظہاریہ);
    }
    // Add a ModportVarRef for varp to modportp if not already present
    خالی جوڑو_Modport_Var_Ref_If_Missing(AstModport* modportp, AstVar* varp, VDirection dir) {
        برائے (Ast_گرہ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
            AstModportVarRef* ثابت mvrp = VN_CAST(mrp, ModportVarRef);
            اگر (mvrp && mvrp->varp() == varp) واپس;  // Already present
        }
        AstModportVarRef* ثابت mvrp = نیا AstModportVarRef{varp->fileline(), varp->name(), dir};
        mvrp->varp(varp);
        modportp->جوڑو_Varsp(mvrp);
    }
    AstVar* getCreateEnVarp(AstVar* invarp, منطقی is_چوٹی) {
        // Return the master __en for the specified input variable
        اگر (!invarp->user1p()) {
            AstVar* ثابت newp
                = نیا AstVar{invarp->fileline(), is_چوٹی ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__en", invarp};
            // Inherited VDirection::INPUT
            UINFO(9, "       newenv " << newp);
            // If the variable belongs to a different module (e.g. interface),
            // create __en in that module so VarXRefs with the same dotted path can reach it.
            Ast_گرہ_Module* ثابت ownerModp = ڈھونڈو_والد_Module(invarp);
            اگر (ownerModp && ownerModp != m_modp) {
                ownerModp->جوڑو_Stmtsp(newp);
                // Add __en to any modports that reference the original var,
                // so VarXRefs through modport paths can resolve it.
                برائے (Ast_گرہ* stmtp = ownerModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    AstModport* ثابت modportp = VN_CAST(stmtp, Modport);
                    اگر (!modportp) جاری;
                    برائے (Ast_گرہ* mrp = modportp->varsp(); mrp; mrp = mrp->nextp()) {
                        AstModportVarRef* ثابت mvrp = VN_CAST(mrp, ModportVarRef);
                        اگر (mvrp && mvrp->varp() == invarp) {
                            جوڑو_Modport_Var_Ref_If_Missing(modportp, newp, VDirection::داخلہ);
                            توڑو;
                        }
                    }
                }
            } ورنہ {
                mod_جوڑو_Stmtp(invarp, newp);
            }
            invarp->user1p(newp);  // find envar given invarp
        }
        واپس VN_AS(invarp->user1p(), Var);
    }
    AstConst* getNonZConstp(AstConst* ثابت constp) {
        File_سطر* ثابت fl = constp->fileline();
        V3_نمبر numz{constp, constp->چوڑائی()};
        numz.op_بٹ_Z(constp->عدد());  // Z->1, else 0
        V3_نمبر numz0{constp, constp->چوڑائی()};
        numz0.opNot(numz);  // Z->0, else 1
        واپس نیا AstConst{fl, numz0};
    }
    Ast_گرہ_اظہاریہ* get_En_اظہاریہ_Based_On_Originalp(Ast_گرہ_اظہاریہ* ثابت nodep) {
        اگر (AstVarRef* ثابت varrefp = VN_CAST(nodep, VarRef)) {
            واپس نیا AstVarRef{varrefp->fileline(), getCreateEnVarp(varrefp->varp(), جھوٹ),
                                 VAccess::READ};
        } ورنہ اگر (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
            AstVar* ثابت enVarp = getCreateEnVarp(xrefp->varp(), جھوٹ);
            واپس newVarXRef(xrefp->fileline(), enVarp, xrefp->dotted(), VAccess::READ,
                              xrefp->inlinedDots());
        } ورنہ اگر (AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            واپس getNonZConstp(constp);
        } ورنہ اگر (AstExtend* ثابت extendp = VN_CAST(nodep, Extend)) {
            // Extend inserts 0 at the beginning. 0 in __en variable means that this bit equals z,
            // so in order to preserve the value of the original AstExtend node we should insert 1
            // instead of 0. To extend __en expression we have to negate its lhsp() and then negate
            // whole extend.

            // Unlink lhsp before copying to save unnecessary copy of lhsp
            Ast_گرہ_اظہاریہ* ثابت lhsp = extendp->lhsp()->unlink_Fr_عقب();
            AstExtend* ثابت enExtendp = extendp->clone_درخت(جھوٹ);
            extendp->lhsp(lhsp);
            Ast_گرہ_اظہاریہ* ثابت enLhsp = get_En_اظہاریہ_Based_On_Originalp(lhsp);
            enExtendp->lhsp(نیا AstNot{enLhsp->fileline(), enLhsp});
            واپس نیا AstNot{enExtendp->fileline(), enExtendp};
        } ورنہ اگر (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            Ast_گرہ_اظہاریہ* ثابت fromp = selp->fromp()->unlink_Fr_عقب();
            AstSel* ثابت enSelp = selp->clone_درخت(جھوٹ);
            selp->fromp(fromp);
            Ast_گرہ_اظہاریہ* ثابت enFromp = get_En_اظہاریہ_Based_On_Originalp(fromp);
            enSelp->fromp(enFromp);
            واپس enSelp;
        } ورنہ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName()
                                                             << " in function " << __func__);
            واپس خالی_اشاریہ;
        }
    }
    AstVar* get_Create_برآمد_Varp(AstVar* invarp, منطقی is_چوٹی) {
        // Return the master __out for the specified input variable
        اگر (!m_varAux(invarp).برآمد_Varp) {
            AstVar* ثابت newp
                = نیا AstVar{invarp->fileline(), is_چوٹی ? VVarType::VAR : VVarType::MODULETEMP,
                             invarp->name() + "__out", invarp};
            // Inherited VDirection::OUTPUT
            UINFO(9, "       newout " << newp);
            mod_جوڑو_Stmtp(invarp, newp);
            m_varAux(invarp).برآمد_Varp = newp;  // find outvar given invarp
        }
        واپس m_varAux(invarp).برآمد_Varp;
    }
    AstVar* getCreateUnconnVarp(Ast_گرہ* fromp, Ast_گرہ_DType* dtypep) {
        AstVar* ثابت newp = نیا AstVar{fromp->fileline(), VVarType::MODULETEMP,
                                        "__Vtriunconn" + cvt_مقصد_Str(m_unique++), dtypep};
        UINFO(9, "       newunc " << newp);
        mod_جوڑو_Stmtp(newp, newp);
        واپس newp;
    }

    LhsProjection newLhsProjection(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_Var_Ref* targetp, Ast_گرہ_اظہاریہ* enp) {
        // LHS concatenations are split before V3Tristate. Clone the remaining LHS expression so
        // the data and enable assignments update the same footprint.
        Ast_گرہ_اظہاریہ* ثابت newLhsp = lhsp->clone_درخت_خالص(جھوٹ);
        // user1p carries pass-local enable ownership, not semantic AST state. Sharing those
        // pointers with the projected LHS would leave the clone referring to enable nodes owned
        // by the original assignment.
        newLhsp->foreach([](Ast_گرہ* nodep) { nodep->user1p(خالی_اشاریہ); });
        Ast_گرہ_Var_Ref* ثابت newVarrefp = targetp->clonep();
        UASSERT_OBJ(newVarrefp, targetp, "Procedural tristate LHS clone lost variable reference");
        واپس LhsProjection{newLhsp, newVarrefp, enp};
    }

    خالی map_داخل_کرو_Lhs_Var_Ref(Ast_گرہ_Var_Ref* nodep) {
        UINFO(9, "    mapInsertLhsVarRef " << nodep);
        AstVar* ثابت کلید = nodep->varp();
        ثابت خودکار pair = m_lhsmap.emplace(کلید, خالی_اشاریہ);
        اگر (pair.دوسرا) pair.پہلا->دوسرا = نیا RefStrengthVec;
        Ast_گرہ_اظہاریہ* enLhsp = خالی_اشاریہ;
        Ast_گرہ_Var_Ref* enVarrefp = خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* enRhsp = خالی_اشاریہ;
        ثابت خودکار enIt = m_تفویض_Enps.ڈھونڈو(m_assignp);
        // Struct and union resolver expressions require member-wise lowering. Keep their
        // established per-assignment path until procedural coalescing provides that
        // representation.
        ثابت منطقی canCombineProcedural = !VN_IS(کلید->dtypep()->skipRefp(), گرہ_UOr_Struct_DType);
        ثابت Ast_گرہ_Procedure* ثابت procedurep
            = enIt != m_تفویض_Enps.انتہا() && canCombineProcedural ? m_procedurep : خالی_اشاریہ;
        اگر (procedurep) {
            LhsProjection projection
                = newLhsProjection(m_assignp->lhsp(), nodep, enIt->دوسرا->clone_درخت_خالص(جھوٹ));
            enLhsp = projection.m_lhsp;
            enVarrefp = projection.m_varrefp;
            enRhsp = projection.m_enp;
        }
        pair.پہلا->دوسرا->ڈالو_عقب(RefStrength{nodep, m_assignp, procedurep, enLhsp, enVarrefp,
                                                  enRhsp, m_currentStrength, m_currentDelayp});
    }

    Ast_گرہ_اظہاریہ* newEnableDeposit(AstSel* selp, Ast_گرہ_اظہاریہ* enp) {
        // Form a "deposit" instruction for given enable, using existing select as a template.
        // Would be nicer if we made this a new AST type
        Ast_گرہ_اظہاریہ* ثابت newp = نیا Ast_سرکاؤ_L{
            selp->fileline(), نیا AstExtend{selp->fileline(), enp, selp->fromp()->چوڑائی()},
            selp->lsbp()->clone_درخت_خالص(جھوٹ), selp->fromp()->چوڑائی()};
        واپس newp;
    }

    خالی سیٹ_Pull_Direction(AstVar* varp, AstPull* pullp) {
        ثابت AstPull* ثابت oldpullp = m_varAux(varp).pullp;
        اگر (!oldpullp) {
            m_varAux(varp).pullp = pullp;  // save off to indicate the pull direction
        } ورنہ {
            اگر (oldpullp->direction() != pullp->direction()) {
                pullp->v3warn(E_UNSUPPORTED, "Unsupported: Conflicting pull directions.\n"
                                                 << pullp->warnContextPrimary() << '\n'
                                                 << oldpullp->warnOther()
                                                 << "... Location of conflicting pull.\n"
                                                 << oldpullp->warnContextSecondary());
            }
        }
    }

    خالی سیٹ_ثنائی_ہندسہ_Pull_Direction(AstVar* varp, صحیح ثنائی_ہندسہ_اشاریہ, صحیح direction) {
        // Set pull direction for a specific bit of a bus.
        // direction: 1 = pullup, 0 = pulldown
        خودکار& ثنائی_ہندسہ_Pulls = m_varAux(varp).ثنائی_ہندسہ_Pulls;
        خودکار it = ثنائی_ہندسہ_Pulls.ڈھونڈو(ثنائی_ہندسہ_اشاریہ);
        اگر (it != ثنائی_ہندسہ_Pulls.انتہا() && it->دوسرا != direction) {
            varp->v3warn(E_UNSUPPORTED, "Conflicting pullup/pulldown direction on bit "
                                            << ثنائی_ہندسہ_اشاریہ << " of " << varp->prettyNameQ());
        }
        ثنائی_ہندسہ_Pulls[ثنائی_ہندسہ_اشاریہ] = direction;
    }

    AstConst* create_فی_ثنائی_ہندسہ_Pull_Const(AstVar* varp) {
        // Create a constant with per-bit pull values.
        // Bits without explicit pull default to pulldown (0).
        // V3Number::setBit silently no-ops if bitIndex exceeds the variable width.
        ثابت خودکار& ثنائی_ہندسہ_Pulls = m_varAux(varp).ثنائی_ہندسہ_Pulls;
        V3_نمبر عدد{varp, varp->چوڑائی(), 0};  // Start with all zeros
        برائے (ثابت خودکار& pair : ثنائی_ہندسہ_Pulls) {
            اگر (pair.دوسرا == 1) عدد.سیٹ_ثنائی_ہندسہ(pair.پہلا, 1);
        }  // LCOV_EXCL_LINE
        واپس نیا AstConst{varp->fileline(), عدد};
    }

    منطقی has_فی_ثنائی_ہندسہ_Pulls(AstVar* varp) {
        // Note: Not const - m_varAux allocates on access
        واپس !m_varAux(varp).ثنائی_ہندسہ_Pulls.خالی_خانے();
    }

    خالی checkUnhandled(Ast_گرہ* nodep) {
        // Check for unsupported tristate constructs. This is not a 100% check.
        // The best way would be to visit the tree again and find any user1p()
        // pointers that did not get picked up and expanded.
        اگر (m_alhs && nodep->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
        }
        // Ignore Var because they end up adjacent to statements
        اگر ((nodep->op1p() && nodep->op1p()->user1p() && !VN_IS(nodep->op1p(), Var))
            || (nodep->op2p() && nodep->op2p()->user1p() && !VN_IS(nodep->op2p(), Var))
            || (nodep->op3p() && nodep->op3p()->user1p() && !VN_IS(nodep->op3p(), Var))
            || (nodep->op4p() && nodep->op4p()->user1p() && !VN_IS(nodep->op4p(), Var))) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported tristate construct: " << nodep->prettyTypeName());
        }
    }

    خالی داخل_کرو_Tristates(Ast_گرہ_Module* nodep) {
        // Go through all the vars and find any that are outputs without drivers
        // or inouts without high-Z logic and put a 1'bz driver on them and add
        // them to the lhs map so they get expanded correctly.
        ثابت Tristate_گراف::VarVec vars = m_tgraph.tristateVars();
        برائے (خودکار varp : vars) {
            اگر (!m_tgraph.isTristate(varp)) جاری;
            ثابت خودکار it = m_lhsmap.ڈھونڈو(varp);
            اگر (it != m_lhsmap.انتہا()) جاری;
            // This variable is floating, set output enable to always be off on this assign.
            // For pullup/pulldown vars, use pull value with en=0 - the final resolution will
            // apply the pull formula: final = (driven_value & en) | (~en & pull_value)
            UINFO(8, "  Adding driver to var " << varp);
            ثابت AstPull* ثابت pullp = m_varAux(varp).pullp;
            ثابت منطقی pull_قدر = pullp && pullp->direction() == 1;
            AstConst* ثابت constp = newAllZerosOrOnes(varp, pull_قدر);
            AstVarRef* ثابت varrefp = نیا AstVarRef{varp->fileline(), varp, VAccess::WRITE};
            Ast_تفویض_W* ثابت newp = نیا Ast_تفویض_W{varp->fileline(), varrefp, constp};
            UINFO(9, "       newoev " << newp);
            varrefp->user1p(newAllZerosOrOnes(varp, جھوٹ));  // en=0 for floating/pullup vars
            nodep->جوڑو_Stmtsp(نیا AstAlways{newp});
            map_داخل_کرو_Lhs_Var_Ref(varrefp);  // insertTristates will convert
            //                               // to a varref to the __out# variable
        }

        // Now go through the lhs driver map and generate the output
        // enable logic for any tristates.
        // Note there might not be any drivers.
        برائے (AstVar* varp : vars) {  // Use vector instead of m_lhsmap iteration for stability
            ثابت std::map<AstVar*, RefStrengthVec*>::iterator it = m_lhsmap.ڈھونڈو(varp);
            اگر (it == m_lhsmap.انتہا()) جاری;
            AstVar* ثابت invarp = it->پہلا;
            RefStrengthVec* refsp = it->دوسرا;
            // Figure out if this var needs tristate expanded.
            اگر (m_tgraph.isTristate(invarp)) {
                // Check if the var is owned by a different module (cross-module reference).
                // For interface vars this is expected; for regular modules it's unsupported.
                Ast_گرہ_Module* ثابت ownerModp = ڈھونڈو_والد_Module(invarp);
                ثابت منطقی isCrossModule = ownerModp && ownerModp != nodep;
                ثابت منطقی isIfaceTri = isCrossModule && VN_IS(ownerModp, Iface);

                اگر (isCrossModule && !isIfaceTri) {
                    // Hierarchical writes to non-interface module tri wires are unsupported
                    برائے (ثابت RefStrength& rs : *refsp) {
                        اگر (VN_IS(rs.m_varrefp, VarXRef)) {
                            rs.m_varrefp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported tristate construct: hierarchical driver"
                                " of non-interface tri signal: "
                                    << invarp->prettyNameQ());
                            توڑو;
                        }
                    }
                } ورنہ اگر (isIfaceTri) {
                    // Mark here too, so a net made tristate only by an external 'z driver
                    // still captures a plain cross-hierarchy driver processed later.
                    m_varAux(invarp).ifaceTristate = سچ;
                    // Interface tristate vars: drivers from different interface instances
                    // (different VarXRef dotted paths) must be processed separately.
                    // E.g. io_ifc.d and io_ifc_local.d both target the same AstVar d in
                    // the ifc interface, but each instance needs its own contribution slot.
                    ساخت تقسیم_Info final {
                        RefStrengthVec refs;
                        string inlinedDots;
                    };
                    std::map<string, تقسیم_Info> partitions;
                    برائے (ثابت RefStrength& rs : *refsp) {
                        اگر (AstVarXRef* ثابت xrefp = VN_CAST(rs.m_varrefp, VarXRef)) {
                            تقسیم_Info& pi = partitions[xrefp->dotted()];
                            pi.refs.ڈالو_عقب(rs);
                            اگر (pi.inlinedDots.خالی_خانے()) { pi.inlinedDots = xrefp->inlinedDots(); }
                        } ورنہ {
                            partitions[""].refs.ڈالو_عقب(rs);
                        }
                    }
                    برائے (خودکار& kv : partitions) {
                        داخل_کرو_Tristates_Signal(nodep, invarp, &kv.دوسرا.refs, سچ, kv.پہلا,
                                              kv.دوسرا.inlinedDots,
                                              ڈھونڈو_Modport_For_Dotted(nodep, kv.پہلا));
                    }
                } ورنہ اگر (VN_IS(nodep, Iface) && !invarp->isIO()) {
                    // Local driver in an interface module - use contribution mechanism
                    // so it can be combined with any external drivers later. Record that
                    // this interface net is tristate, so a module that drives it across
                    // hierarchy with a plain (non-Z) assign is also routed through the
                    // contribution mechanism (its own graph has no Z to mark it tristate).
                    m_varAux(invarp).ifaceTristate = سچ;
                    داخل_کرو_Tristates_Signal(nodep, invarp, refsp, سچ, "", "", خالی_اشاریہ);
                } ورنہ {
                    داخل_کرو_Tristates_Signal(nodep, invarp, refsp, جھوٹ, "", "", خالی_اشاریہ);
                }
            } ورنہ {
                UINFO(8, "  NO TRISTATE ON:" << invarp);
            }
            // Delete the map and vector list now that we have expanded it.
            m_lhsmap.erase(invarp);
            VL_DO_DANGLING(حذف refsp, refsp);
        }
    }

    خالی aggregateTriSameStrength(Ast_گرہ_Module* nodep, AstVar* ثابت varp, AstVar* ثابت envarp,
                                  RefStrengthVec::iterator beginStrength,
                                  RefStrengthVec::iterator انتہا_Strength) {
        // Separate procedural processes are independent drivers, but ordered assignments within
        // one process form a single driver. Create data and enable variables for each driver, then
        // aggregate independent drivers using | to form assignments to varp and envarp.
        ساخت Driver_گروہ final {
            ثابت Ast_گرہ_Procedure* m_procedurep;  // Process owning these assignments
            std::سمتیہ<RefStrength*> m_refsp;  // Ordered assignments in this driver
        };
        std::سمتیہ<Driver_گروہ> drivers;
        std::map<ثابت Ast_گرہ_Procedure*, سائز_t> procedure_مقصد_Driver;
        برائے (خودکار it = beginStrength; it != انتہا_Strength; ++it) {
            اگر (it->m_procedurep) {
                ثابت خودکار emplaced = procedure_مقصد_Driver.emplace(it->m_procedurep, drivers.سائز());
                اگر (!emplaced.دوسرا) {
                    drivers[emplaced.پہلا->دوسرا].m_refsp.ڈالو_عقب(&*it);
                    جاری;
                }
            }  // LCOV_EXCL_LINE -- GCC exception cleanup branch
            drivers.ڈالو_عقب(Driver_گروہ{it->m_procedurep, {&*it}});
        }

        Ast_گرہ_اظہاریہ* orp = خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* enp = خالی_اشاریہ;

        برائے (Driver_گروہ& driver : drivers) {
            // Create the new LHS data and enable variables for this driver.
            AstVar* ثابت newLhsp = نیا AstVar{varp->fileline(), VVarType::MODULETEMP,
                                               varp->name() + "__out" + cvt_مقصد_Str(m_unique),
                                               varp};  // 2-state ok; sep enable
            newLhsp->سیٹ_Dfg_Tri_Lowered();
            UINFO(9, "       newout " << newLhsp);
            nodep->جوڑو_Stmtsp(newLhsp);
            AstVar* ثابت newEnLhsp
                = نیا AstVar{varp->fileline(), VVarType::MODULETEMP,
                             varp->name() + "__en" + cvt_مقصد_Str(m_unique++), envarp};  // 2-state ok
            newEnLhsp->سیٹ_Dfg_Tri_Lowered();
            UINFO(9, "       newenlhsp " << newEnLhsp);
            nodep->جوڑو_Stmtsp(newEnLhsp);

            برائے (RefStrength* ثابت refStrengthp : driver.m_refsp) {
                Ast_گرہ_Var_Ref* refp = refStrengthp->m_varrefp;

                // When retargeting a VarXRef to a local __out var, the dotted path becomes
                // inconsistent. Replace the VarXRef with a local VarRef.
                اگر (VN_IS(refp, VarXRef)) {
                    AstVarRef* ثابت localRefp
                        = نیا AstVarRef{refp->fileline(), newLhsp, VAccess::WRITE};
                    // Preserve ownership of any detached enable tree for the existing end-of-pass
                    // cleanup, regardless of whether this is a procedural driver.
                    localRefp->user1p(refp->user1p());
                    refp->user1p(خالی_اشاریہ);
                    refp->replaceWith(localRefp);
                    VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                    refp = localRefp;
                    refStrengthp->m_varrefp = localRefp;
                } ورنہ {
                    refp->varp(newLhsp);
                }

                اگر (driver.m_procedurep) {
                    UASSERT_OBJ(refStrengthp->m_assignp, refp,
                                "Procedural tristate driver without assignment");
                    UASSERT_OBJ(refStrengthp->m_enLhsp && refStrengthp->m_enVarrefp
                                    && refStrengthp->m_enRhsp,
                                refp, "Procedural tristate driver without projected enable");

                    Ast_گرہ_اظہاریہ* enLhsp = refStrengthp->m_enLhsp;
                    Ast_گرہ_Var_Ref* ثابت enVarrefp = refStrengthp->m_enVarrefp;
                    اگر (VN_IS(enVarrefp, VarXRef)) {
                        AstVarRef* ثابت localRefp
                            = نیا AstVarRef{enVarrefp->fileline(), newEnLhsp, VAccess::WRITE};
                        اگر (enLhsp == enVarrefp) {
                            enLhsp = localRefp;
                            VL_DO_DANGLING(enVarrefp->delete_درخت(), enVarrefp);
                        } ورنہ {
                            enVarrefp->replaceWith(localRefp);
                            VL_DO_DANGLING(ڈالو_Deletep(enVarrefp), enVarrefp);
                        }
                    } ورنہ {
                        enVarrefp->varp(newEnLhsp);
                    }
                    UINFO(9, "       new procedural enable LHS " << enLhsp);
                    m_proceduralEnables[refStrengthp->m_assignp].ڈالو_عقب(
                        ProceduralEnable{enLhsp, refStrengthp->m_enRhsp});
                    refStrengthp->m_enLhsp = خالی_اشاریہ;
                    refStrengthp->m_enVarrefp = خالی_اشاریہ;
                    refStrengthp->m_enRhsp = خالی_اشاریہ;
                } ورنہ {
                    Ast_تفویض_W* ثابت enLhspAssignp = نیا Ast_تفویض_W{
                        refp->fileline(),
                        نیا AstVarRef{refp->fileline(), newEnLhsp, VAccess::WRITE}, getEnp(refp)};
                    اگر (refStrengthp->m_delayp) {
                        enLhspAssignp->timingControlp(refStrengthp->m_delayp->clone_درخت(جھوٹ));
                    }
                    UINFO(9, "       newenlhspAssignp " << enLhspAssignp);
                    nodep->جوڑو_Stmtsp(نیا AstAlways{enLhspAssignp});
                }
            }

            // Append this driver to the resolution logic.
            File_سطر* ثابت fl = driver.m_refsp.front()->m_varrefp->fileline();
            Ast_گرہ_اظہاریہ* ثابت ref1p = نیا AstVarRef{fl, newLhsp, VAccess::READ};
            Ast_گرہ_اظہاریہ* ثابت ref2p = نیا AstVarRef{fl, newEnLhsp, VAccess::READ};
            Ast_گرہ_اظہاریہ* ثابت andp = نیا AstAnd{fl, ref1p, ref2p};

            // or this to the others
            orp = (!orp) ? andp : نیا AstOr{fl, orp, andp};

            Ast_گرہ_اظہاریہ* ثابت ref3p = نیا AstVarRef{fl, newEnLhsp, VAccess::READ};
            enp = (!enp) ? ref3p : نیا AstOr{ref3p->fileline(), enp, ref3p};
        }
        Ast_تفویض_W* ثابت assp = نیا Ast_تفویض_W{
            varp->fileline(), نیا AstVarRef{varp->fileline(), varp, VAccess::WRITE}, orp};
        UINFO(9, "       newassp " << assp);
        nodep->جوڑو_Stmtsp(نیا AstAlways{assp});

        Ast_تفویض_W* ثابت enAssp = نیا Ast_تفویض_W{
            envarp->fileline(), نیا AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
        UINFO(9, "       newenassp " << enAssp);
        nodep->جوڑو_Stmtsp(نیا AstAlways{enAssp});
    }

    خالی داخل_کرو_Procedural_Enable_Assignments(Ast_گرہ_Module* nodep) {
        برائے (خودکار& تفویض_Enables : m_proceduralEnables) {
            Ast_گرہ_تفویض* ثابت assignp = تفویض_Enables.پہلا;
            Ast_گرہ_اظہاریہ* combinedLhsp = assignp->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* combinedRhsp = assignp->rhsp()->unlink_Fr_عقب();
            برائے (ProceduralEnable& enable : تفویض_Enables.دوسرا) {
                combinedLhsp = نیا AstConcat{assignp->fileline(), enable.m_lhsp, combinedLhsp};
                combinedRhsp = نیا AstConcat{assignp->fileline(), enable.m_rhsp, combinedRhsp};
            }

            اگر (!VN_IS(assignp, تفویض_Dly) && assignp->timingControlp()) {
                // A blocking intra-assignment timing control must suspend only once. Sample data
                // and enables together, then apply both after the timing control completes.
                AstVar* ثابت sampledp
                    = نیا AstVar{assignp->fileline(), VVarType::MODULETEMP,
                                 "__VtriSample" + cvt_مقصد_Str(m_unique++), combinedRhsp->dtypep()};
                nodep->جوڑو_Stmtsp(sampledp);
                assignp->lhsp(نیا AstVarRef{assignp->fileline(), sampledp, VAccess::WRITE});
                assignp->rhsp(combinedRhsp);
                assignp->dtypeFrom(combinedRhsp);
                Ast_تفویض* ثابت applyp
                    = نیا Ast_تفویض{assignp->fileline(), combinedLhsp,
                                    نیا AstVarRef{assignp->fileline(), sampledp, VAccess::READ}};
                UINFO(9, "       new timed procedural tristate apply " << applyp);
                assignp->جوڑو_اگلا_یہاں(applyp);
            } ورنہ {
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
    خالی داخل_کرو_Tristates_Signal(Ast_گرہ_Module* nodep, AstVar* ثابت invarp, RefStrengthVec* refsp,
                               منطقی isIfaceTri, ثابت string& iface_Dotted_راستہ,
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
        AstVar* envarp = خالی_اشاریہ;
        AstVar* outvarp = خالی_اشاریہ;  // __out
        AstVar* lhsp = invarp;  // Variable to assign drive-value to (<in> or __out)
        ثابت منطقی is_چوٹی_Inout = !isIfaceTri && (invarp->direction() == VDirection::INOUT)
                                && invarp->isIO() && nodep->is_چوٹی();
        اگر (!isIfaceTri) {
            اگر (((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().خالی_خانے())
                 && is_چوٹی_Inout)
                || (!nodep->is_چوٹی() && invarp->isIO())) {
                // This var becomes an input
                invarp->var_Type2_اندر();  // convert existing port to type input
                // Create an output port (__out)
                outvarp = get_Create_برآمد_Varp(invarp, is_چوٹی_Inout);
                outvarp->var_Type2_برآمد();
                lhsp = outvarp;  // Must assign to __out, not to normal input signal
                UINFO(9, "     TRISTATE propagates up with " << lhsp);
                // Create an output enable port (__en)
                // May already be created if have foo === 1'bz somewhere
                envarp = getCreateEnVarp(invarp, is_چوٹی_Inout);  // dir set in visit(AstPin*)
                outvarp->user1p(envarp);
                m_varAux(outvarp).pullp = m_varAux(invarp).pullp;  // AstPull* propagation
                m_varAux(outvarp).ثنائی_ہندسہ_Pulls
                    = m_varAux(invarp).ثنائی_ہندسہ_Pulls;  // Per-bit pull propagation
                اگر (m_varAux(invarp).pullp) UINFO(9, "propagate pull to " << outvarp);
                اگر (!m_varAux(invarp).ثنائی_ہندسہ_Pulls.خالی_خانے()) {
                    UINFO(9, "propagate per-bit pulls to " << outvarp);
                }
            } ورنہ اگر (invarp->user1p()) {
                envarp = VN_AS(invarp->user1p(), Var);  // From CASEEQ, foo === 1'bz
            }
        }

        Ast_گرہ_اظہاریہ* orp = خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* enp = خالی_اشاریہ;

        std::ترتیب_دو(refsp->begin(), refsp->انتہا(),
                  [](RefStrength a, RefStrength b) { واپس a.m_strength > b.m_strength; });

        RefStrengthVec::iterator beginStrength = refsp->begin();
        جب (beginStrength != refsp->انتہا()) {
            RefStrengthVec::iterator انتہا_Strength = beginStrength + 1;
            جب (انتہا_Strength != refsp->انتہا()
                   && انتہا_Strength->m_strength == beginStrength->m_strength)
                انتہا_Strength++;

            File_سطر* ثابت fl = beginStrength->m_varrefp->fileline();
            // For interface tristate vars, uniquify strength var names by including the
            // interface instance path, since multiple interface instances may have
            // identically-named tristate signals (e.g. io_ifc.d and io_ifc_mc.d both
            // create d__strong in the driving module)
            string strength_سابقہ;
            اگر (isIfaceTri && !iface_Dotted_راستہ.خالی_خانے()) {
                strength_سابقہ = iface_Dotted_راستہ;
                std::replace(strength_سابقہ.begin(), strength_سابقہ.انتہا(), '.', '_');
                strength_سابقہ += "__";
            }
            ثابت string strengthVarName
                = strength_سابقہ + lhsp->name() + "__" + beginStrength->m_strength.ascii();

            // var__strength variable
            AstVar* ثابت varStrengthp = نیا AstVar{fl, VVarType::MODULETEMP, strengthVarName,
                                                    invarp};  // 2-state ok; sep enable;
            varStrengthp->سیٹ_Dfg_Tri_Lowered();
            UINFO(9, "       newstrength " << varStrengthp);
            nodep->جوڑو_Stmtsp(varStrengthp);

            // var__strength__en variable
            AstVar* ثابت enVarStrengthp = نیا AstVar{
                fl, VVarType::MODULETEMP, strengthVarName + "__en", invarp};  // 2-state ok;
            enVarStrengthp->سیٹ_Dfg_Tri_Lowered();
            UINFO(9, "       newenstrength " << enVarStrengthp);
            nodep->جوڑو_Stmtsp(enVarStrengthp);

            aggregateTriSameStrength(nodep, varStrengthp, enVarStrengthp, beginStrength,
                                     انتہا_Strength);

            Ast_گرہ_اظہاریہ* اظہاریہ_Current_Strengthp;
            اگر (enp) {
                // If weaker driver should be overwritten by a stronger, replace its value with z
                اظہاریہ_Current_Strengthp
                    = نیا AstAnd{fl, نیا AstVarRef{fl, varStrengthp, VAccess::READ},
                                 نیا AstNot{fl, enp->clone_درخت_خالص(جھوٹ)}};
            } ورنہ {
                اظہاریہ_Current_Strengthp = نیا AstVarRef{fl, varStrengthp, VAccess::READ};
            }
            orp = (!orp) ? اظہاریہ_Current_Strengthp : نیا AstOr{fl, orp, اظہاریہ_Current_Strengthp};

            Ast_گرہ_اظہاریہ* ثابت enVarStrengthRefp
                = نیا AstVarRef{fl, enVarStrengthp, VAccess::READ};

            enp = (!enp) ? enVarStrengthRefp : نیا AstOr{fl, enp, enVarStrengthRefp};

            beginStrength = انتہا_Strength;
        }

        // For interface tristate vars, store per-module contributions for later combining.
        // The interface module owns the contribution vars; resolution happens in
        // combineIfaceContribs() after all modules are processed.
        اگر (isIfaceTri) {
            // This net intentionally has multiple contributors; DFG should not emit
            // MULTIDRIVEN warnings for this lowered tristate pattern.
            invarp->سیٹ_Dfg_Allow_Multidrive_Tri();
            Ast_گرہ_Module* ثابت ifaceModp = ڈھونڈو_والد_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            ثابت صحیح contribIdx = static_cast<صحیح>(m_ifaceContribs[invarp].سائز());
            File_سطر* ثابت fl = invarp->fileline();

            // Create contribution vars in the interface module
            AstVar* ثابت contribOutp = نیا AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__out" + cvt_مقصد_Str(contribIdx), invarp};
            contribOutp->سیٹ_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribOut " << contribOutp);
            ifaceModp->جوڑو_Stmtsp(contribOutp);

            AstVar* ثابت contribEnp = نیا AstVar{
                fl, VVarType::MODULETEMP, invarp->name() + "__en" + cvt_مقصد_Str(contribIdx), invarp};
            contribEnp->سیٹ_Dfg_Tri_Lowered();
            UINFO(9, "       iface contribEn " << contribEnp);
            ifaceModp->جوڑو_Stmtsp(contribEnp);

            // If the driving module accesses the interface through a modport,
            // add the new contribution vars to the modport so VarXRefs
            // can be resolved by V3LinkDot through the modport path.
            اگر (ifaceModportp) {
                UINFO(9, "       adding to modport " << ifaceModportp->name());
                جوڑو_Modport_Var_Ref_If_Missing(ifaceModportp, contribOutp, VDirection::OUTPUT);
                جوڑو_Modport_Var_Ref_If_Missing(ifaceModportp, contribEnp, VDirection::OUTPUT);
            }

            // Assign drive value and enable to contribution vars.
            // External drivers use VarXRef; local drivers use VarRef.
            {
                Ast_گرہ_Var_Ref* ثابت lhsp
                    = iface_Dotted_راستہ.خالی_خانے() ? static_cast<Ast_گرہ_Var_Ref*>(
                                                    نیا AstVarRef{fl, contribOutp, VAccess::WRITE})
                                              : static_cast<Ast_گرہ_Var_Ref*>(
                                                    newVarXRef(fl, contribOutp, iface_Dotted_راستہ,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_تفویض_W* ثابت assp = نیا Ast_تفویض_W{fl, lhsp, orp};
                assp->user2Or(U2_BOTH);
                nodep->جوڑو_Stmtsp(نیا AstAlways{assp});
            }
            {
                Ast_گرہ_Var_Ref* ثابت lhsp
                    = iface_Dotted_راستہ.خالی_خانے() ? static_cast<Ast_گرہ_Var_Ref*>(
                                                    نیا AstVarRef{fl, contribEnp, VAccess::WRITE})
                                              : static_cast<Ast_گرہ_Var_Ref*>(
                                                    newVarXRef(fl, contribEnp, iface_Dotted_راستہ,
                                                               VAccess::WRITE, ifaceInlinedDots));
                Ast_تفویض_W* ثابت assp = نیا Ast_تفویض_W{fl, lhsp, enp};
                assp->user2Or(U2_BOTH);
                nodep->جوڑو_Stmtsp(نیا AstAlways{assp});
            }

            m_ifaceContribs[invarp].ڈالو_عقب({contribEnp, contribOutp});
            واپس;
        }

        // Apply pull formula: final = orp | (~enp & pull_value)
        // Always run at top level (!outvarp) so that the strength-enable expression (enp)
        // is consumed into orp even when no pull is present; pull_value is then all-zeros
        // so this is semantically a no-op but it links enp into the tree.
        // Also run at intermediate levels when there's a pull, so the pull is baked into
        // the contribution flowing up the hierarchy.
        ثابت منطقی hasPull = m_varAux(lhsp).pullp || has_فی_ثنائی_ہندسہ_Pulls(lhsp);
        اگر (!outvarp || hasPull) {
            Ast_گرہ_اظہاریہ* undrivenp;
            اگر (envarp) {
                undrivenp = نیا AstNot{envarp->fileline(),
                                       نیا AstVarRef{envarp->fileline(), envarp, VAccess::READ}};
            } ورنہ اگر (enp) {
                undrivenp = نیا AstNot{enp->fileline(), enp};
                enp = خالی_اشاریہ;  // moved into undrivenp
            } ورنہ {
                undrivenp = newAllZerosOrOnes(invarp, سچ);  // LCOV_EXCL_LINE
            }

            AstConst* pullConstp;
            اگر (has_فی_ثنائی_ہندسہ_Pulls(lhsp)) {
                pullConstp = create_فی_ثنائی_ہندسہ_Pull_Const(lhsp);
            } ورنہ {
                ثابت AstPull* ثابت pullp = m_varAux(lhsp).pullp;
                ثابت منطقی pull1 = pullp && pullp->direction() == 1;
                pullConstp = newAllZerosOrOnes(invarp, pull1);
            }

            undrivenp = نیا AstAnd{invarp->fileline(), undrivenp, pullConstp};
            orp = orp ? نیا AstOr{invarp->fileline(), orp, undrivenp} : undrivenp;
        }

        // Ensure enp is valid when envarp exists (pullup/pulldown only = no active driver)
        اگر (envarp && !enp) enp = newAllZerosOrOnes(invarp, جھوٹ);

        اگر (envarp) {
            Ast_تفویض_W* ثابت enAssp = نیا Ast_تفویض_W{
                enp->fileline(), نیا AstVarRef{envarp->fileline(), envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "enAssp");
            nodep->جوڑو_Stmtsp(نیا AstAlways{enAssp});
        }

        // __out (child) or <in> (parent) = drive-value expression
        Ast_تفویض_W* ثابت assp = نیا Ast_تفویض_W{
            lhsp->fileline(), نیا AstVarRef{lhsp->fileline(), lhsp, VAccess::WRITE}, orp};
        assp->user2Or(U2_BOTH);  // Don't process further; already resolved
        UINFOTREE(9, assp, "", "lhsp-eqn");
        nodep->جوڑو_Stmtsp(نیا AstAlways{assp});

        // If this is a top-level inout, make sure that the INOUT pins get __en and __out
        اگر ((v3Global.opt.pinsInoutEnables() || !v3Global.opt.libCreate().خالی_خانے()) && is_چوٹی_Inout) {
            اگر (envarp) {
                envarp->primaryIO(سچ);
                envarp->direction(VDirection::OUTPUT);
                envarp->protect(invarp->protect());
            }
            اگر (outvarp) {
                outvarp->primaryIO(سچ);
                outvarp->direction(VDirection::OUTPUT);
                outvarp->protect(invarp->protect());
            }
            اگر (!v3Global.opt.libCreate().خالی_خانے()) {
                ثابت صحیح id = ++m_unique;
                invarp->libInoutId(id);
                outvarp->libInoutId(id);
                envarp->libInoutId(id);
                envarp->libInoutEnable(سچ);
            }
        }
    }

    منطقی is_Only_Assignment_Is_مقصد_Lhs_Var(ثابت Ast_تفویض_W* ثابت nodep) {
        اگر (ثابت AstVarRef* ثابت varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            خودکار ملا_It = m_assigns.ڈھونڈو(varRefp->varp());
            اگر (ملا_It != m_assigns.انتہا()) {
                خودکار ثابت& assigns_مقصد_Var = ملا_It->دوسرا;
                اگر (assigns_مقصد_Var.سائز() == 1 && assigns_مقصد_Var[0] == nodep) واپس سچ;
            }
        }
        واپس جھوٹ;
    }

    خالی جوڑو_مقصد_Assignment_List(Ast_تفویض_W* nodep) {
        اگر (AstVarRef* ثابت varRefp = VN_CAST(nodep->lhsp(), VarRef)) {
            اگر (varRefp->varp()->isNet()) {
                m_assigns[varRefp->varp()].ڈالو_عقب(nodep);
            } ورنہ اگر (nodep->strengthSpecp()) {
                nodep->strengthSpecp()->unlink_Fr_عقب()->delete_درخت();
            }
        } ورنہ اگر (nodep->strengthSpecp()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Assignments with signal strength with LHS of type: "
                              << nodep->lhsp()->prettyTypeName());
        }
    }

    uint8_t getStrength(ثابت Ast_تفویض_W* ثابت nodep, منطقی قدر) {
        اگر (AstStrengthSpec* ثابت strengthSpec = nodep->strengthSpecp()) {
            واپس قدر ? strengthSpec->strength1() : strengthSpec->strength0();
        }
        واپس VStrength::STRONG;  // default strength is strong
    }

    منطقی assignment_کا_قدر_On_All_بٹ(ثابت Ast_تفویض_W* ثابت nodep, منطقی قدر) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const)) {
            ثابت V3_نمبر عدد = constp->عدد();
            واپس قدر ? عدد.isEqAllOnes() : عدد.isEqZero();
        }
        واپس جھوٹ;
    }

    Ast_تفویض_W* get_Strongest_Assignment_کا_قدر(ثابت Assigns& assigns, منطقی قدر) {
        خودکار maxIt = std::max_element(
            assigns.begin(), assigns.انتہا(), [&](ثابت Ast_تفویض_W* ap, ثابت Ast_تفویض_W* bp) {
                ثابت منطقی اقدار_On_Rhs_A = assignment_کا_قدر_On_All_بٹ(ap, قدر);
                ثابت منطقی اقدار_On_Rhs_B = assignment_کا_قدر_On_All_بٹ(bp, قدر);
                اگر (!اقدار_On_Rhs_A) واپس اقدار_On_Rhs_B;
                اگر (!اقدار_On_Rhs_B) واپس جھوٹ;
                واپس getStrength(ap, قدر) < getStrength(bp, قدر);
            });
        // If not all assignments have const with all bits equal to value on the RHS,
        // std::max_element will return one of them anyway, so it has to be checked before
        // returning
        واپس assignment_کا_قدر_On_All_بٹ(*maxIt, قدر) ? *maxIt : خالی_اشاریہ;
    }

    منطقی isAssignmentNotStrongerThanStrength(ثابت Ast_تفویض_W* assignp, uint8_t strength) {
        // If the value of the RHS is known and has all bits equal, only strength corresponding to
        // its value is taken into account. In opposite case, both strengths are compared.
        ثابت uint8_t strength0 = getStrength(assignp, 0);
        ثابت uint8_t strength1 = getStrength(assignp, 1);
        واپس (strength0 <= strength && strength1 <= strength)
               || (strength0 <= strength && assignment_کا_قدر_On_All_بٹ(assignp, 0))
               || (strength1 <= strength && assignment_کا_قدر_On_All_بٹ(assignp, 1));
    }

    Ast_گرہ_اظہاریہ* new_ملاؤ_اظہاریہ(Ast_گرہ_اظہاریہ* ثابت lhsp, Ast_گرہ_اظہاریہ* ثابت rhsp, File_سطر* ثابت fl,
                              منطقی isWor) {
        Ast_گرہ_اظہاریہ* اظہاریہ = خالی_اشاریہ;
        اگر (isWor)
            اظہاریہ = نیا AstOr{fl, lhsp, rhsp};
        ورنہ
            اظہاریہ = نیا AstAnd{fl, lhsp, rhsp};
        واپس اظہاریہ;
    }

    خالی ملاؤ_Wired_Nets_Assignments() {
        // Support for WOR/TRIOR/WAND/TRIAND, by merging the Assignments for the
        // same Net (merge by or for WOR/TIOR and merge by and for WAND/TRIAND).
        برائے (خودکار& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.دوسرا;
            اگر (assigns.سائز() <= 1) جاری;
            AstVar* ثابت varp = varpAssigns.پہلا;
            اگر (!varp->isWiredNet()) جاری;
            خودکار it = assigns.begin();
            Ast_تفویض_W* ثابت تفویض_Wp0 = *it;
            File_سطر* ثابت fl = تفویض_Wp0->fileline();
            Ast_گرہ_اظہاریہ* wExp = خالی_اشاریہ;
            جب (++it != assigns.انتہا()) {
                Ast_تفویض_W* تفویض_Wpi = *it;
                اگر (!wExp) {
                    wExp
                        = new_ملاؤ_اظہاریہ(تفویض_Wp0->rhsp()->clone_درخت_خالص(جھوٹ),
                                       تفویض_Wpi->rhsp()->clone_درخت_خالص(جھوٹ), fl, varp->isWor());
                } ورنہ {
                    wExp = new_ملاؤ_اظہاریہ(wExp, تفویض_Wpi->rhsp()->clone_درخت_خالص(جھوٹ), fl,
                                        varp->isWor());
                }
                VL_DO_DANGLING((تفویض_Wpi->unlink_Fr_عقب()->delete_درخت()), تفویض_Wpi);
            }
            AstVarRef* ثابت wVarRef = نیا AstVarRef{fl, varp, VAccess::WRITE};
            Ast_تفویض_W* ثابت wAssignp = نیا Ast_تفویض_W{fl, wVarRef, wExp};
            تفویض_Wp0->replaceWith(wAssignp);
            VL_DO_DANGLING(ڈالو_Deletep(تفویض_Wp0), تفویض_Wp0);
            assigns.clear();
            assigns.ڈالو_عقب(wAssignp);
        }
    }

    خالی removeNotStrongerAssignments(Assigns& assigns, ثابت Ast_تفویض_W* strongestp,
                                      uint8_t greatest_معلوم_Strength) {
        // Weaker assignments are these assignments that can't change the final value of the net.
        // They can be safely removed. Assignments of the same strength are also removed, because
        // duplicates aren't needed. One problem is with 2 assignments of different values and
        // equal strengths. It should result in assignment of x value, but these values aren't
        // supported now.
        خودکار removedIt = std::remove_if(assigns.begin(), assigns.انتہا(), [&](Ast_تفویض_W* assignp) {
            اگر (assignp == strongestp) واپس جھوٹ;
            اگر (isAssignmentNotStrongerThanStrength(assignp, greatest_معلوم_Strength)) {
                // Vertices corresponding to nodes from removed assignment's subtree have to be
                // removed.
                m_tgraph.deleteVerticesFromSubtreeRecurse(assignp);
                VL_DO_DANGLING(ڈالو_Deletep(assignp->unlink_Fr_عقب()), assignp);
                واپس سچ;
            }
            واپس جھوٹ;
        });
        assigns.erase(removedIt, assigns.انتہا());
    }

    خالی remove_Assignments_Not_Stronger_Than_یکساں_Constant() {
        // If a stronger assignment of a constant with all bits equal to the same
        // value (0 or 1), is found, all weaker assignments can be safely removed.
        برائے (خودکار& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.دوسرا;
            اگر (assigns.سائز() <= 1) جاری;
            ثابت Ast_تفویض_W* ثابت strongest0p = get_Strongest_Assignment_کا_قدر(assigns, 0);
            ثابت Ast_تفویض_W* ثابت strongest1p = get_Strongest_Assignment_کا_قدر(assigns, 1);
            ثابت Ast_تفویض_W* strongestp = خالی_اشاریہ;
            uint8_t greatest_معلوم_Strength = 0;
            ثابت خودکار getIfStrongest
                = [&](ثابت Ast_تفویض_W* ثابت strongestCandidatep, منطقی قدر) {
                      اگر (!strongestCandidatep) واپس;
                      ثابت uint8_t strength = getStrength(strongestCandidatep, قدر);
                      اگر (strength >= greatest_معلوم_Strength) {
                          greatest_معلوم_Strength = strength;
                          strongestp = strongestCandidatep;
                      }
                  };
            getIfStrongest(strongest0p, 0);
            getIfStrongest(strongest1p, 1);

            اگر (strongestp) {
                removeNotStrongerAssignments(assigns, strongestp, greatest_معلوم_Strength);
            }
        }
    }

    خالی removeAssignmentsNotStrongerThanNonTristate() {
        // Similar function as removeAssignmentsNotStrongerThanUniformConstant, but here the
        // assignments that have strength not stronger than the strongest assignment with
        // non-tristate RHS are removed. Strengths are compared according to their smaller values,
        // because the values of RHSs are unknown. (Assignments not stronger than strongest
        // constant are already removed.)
        برائے (خودکار& varpAssigns : m_assigns) {
            Assigns& assigns = varpAssigns.دوسرا;
            اگر (assigns.سائز() <= 1) جاری;
            خودکار maxIt = std::max_element(
                assigns.begin(), assigns.انتہا(), [&](ثابت Ast_تفویض_W* ap, ثابت Ast_تفویض_W* bp) {
                    اگر (m_tgraph.isTristate(ap)) واپس !m_tgraph.isTristate(bp);
                    اگر (m_tgraph.isTristate(bp)) واپس جھوٹ;
                    ثابت uint8_t minStrengthA = std::min(getStrength(ap, 0), getStrength(ap, 1));
                    ثابت uint8_t minStrengthB = std::min(getStrength(bp, 0), getStrength(bp, 1));
                    واپس minStrengthA < minStrengthB;
                });
            // If RHSs of all assignments are tristate, 1st element is returned, so it is
            // needed to check if it is non-tristate.
            ثابت Ast_تفویض_W* ثابت strongestp = m_tgraph.isTristate(*maxIt) ? خالی_اشاریہ : *maxIt;
            اگر (strongestp) {
                ثابت uint8_t greatest_معلوم_Strength
                    = std::min(getStrength(strongestp, 0), getStrength(strongestp, 1));
                removeNotStrongerAssignments(assigns, strongestp, greatest_معلوم_Strength);
            }
        }
    }

    // VISITORS
    خالی دورہ(AstConst* nodep) override {
        UINFO(9, dbgState() << nodep);
        اگر (m_graphing) {
            اگر (!m_alhs && nodep->عدد().hasZ()) m_tgraph.سیٹ_Tristate(nodep);
        } ورنہ {
            // Detect any Z consts and convert them to 0's with an enable that is also 0.
            اگر (m_alhs && nodep->user1p()) {
                // A pin with 1'b0 or similar connection results in an assign with constant on LHS
                // due to the pinReconnectSimple call in visit AstPin.
                // We can ignore the output override by making a temporary
                AstVar* ثابت varp = getCreateUnconnVarp(nodep, nodep->dtypep());
                Ast_گرہ* ثابت newp = نیا AstVarRef{nodep->fileline(), varp, VAccess::WRITE};
                UINFO(9, " const->" << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            } ورنہ اگر (m_tgraph.isTristate(nodep)) {
                m_tgraph.didProcess(nodep);
                File_سطر* ثابت fl = nodep->fileline();
                AstConst* ثابت enp = getNonZConstp(nodep);
                V3_نمبر num1{nodep, nodep->چوڑائی()};
                num1.opAnd(nodep->عدد(), enp->عدد());  // 01X->01X, Z->0
                AstConst* ثابت newconstp = نیا AstConst{fl, num1};
                nodep->replaceWith(newconstp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                newconstp->user1p(enp);  // Propagate up constant with non-Z bits as 1
            }
        }
    }

    خالی دورہ(AstCond* nodep) override {
        اگر (m_graphing) {
            iterateChildren(nodep);
            اگر (m_alhs) {
                associateLogic(nodep, nodep->thenp());
                associateLogic(nodep, nodep->elsep());
            } ورنہ {
                associateLogic(nodep->thenp(), nodep);
                associateLogic(nodep->elsep(), nodep);
            }
        } ورنہ {
            اگر (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                واپس;
            }
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Generate the new output enable signal for this cond if either
            // expression 1 or 2 have an output enable '__en' signal. If the
            // condition has an enable, not sure what to do, so generate an
            // error.
            Ast_گرہ_اظہاریہ* ثابت condp = nodep->condp();
            اگر (condp->user1p()) {
                condp->v3warn(E_UNSUPPORTED, "Unsupported: don't know how to deal with "
                                             "tristate logic in the conditional expression");
            }
            Ast_گرہ_اظہاریہ* ثابت thenp = nodep->thenp();
            Ast_گرہ_اظہاریہ* ثابت elsep = nodep->elsep();
            اگر (thenp->user1p() || elsep->user1p()) {  // else no tristates
                m_tgraph.didProcess(nodep);
                Ast_گرہ_اظہاریہ* ثابت en1p = getEnp(thenp);
                Ast_گرہ_اظہاریہ* ثابت en2p = getEnp(elsep);
                // The output enable of a cond is a cond of the output enable of the
                // two expressions with the same conditional.
                Ast_گرہ_اظہاریہ* ثابت enp
                    = نیا AstCond{nodep->fileline(), condp->clone_درخت(جھوٹ), en1p, en2p};
                UINFO(9, "       newcond " << enp);
                nodep->user1p(enp);  // propagate up COND(lhsp->enable, rhsp->enable)
                اگر (thenp->user1p() && !thenp->user1p()->backp()) ڈالو_Deletep(thenp->user1p());
                اگر (elsep->user1p() && !elsep->user1p()->backp()) ڈالو_Deletep(elsep->user1p());
                thenp->user1p(خالی_اشاریہ);
                elsep->user1p(خالی_اشاریہ);
            }
        }
    }

    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        iterateChildren(nodep);
        اگر (m_graphing) {
            UASSERT_OBJ(!m_alhs, nodep, "AstExprStmt node on the LHS of assignment");
            associateLogic(nodep->resultp(), nodep);
        } ورنہ اگر (nodep->resultp()->user1p()) {
            nodep->user1p(getEnp(nodep->resultp()));
            nodep->resultp()->user1p(خالی_اشاریہ);
            m_tgraph.didProcess(nodep);
        }
    }

    خالی دورہ(AstSel* nodep) override {
        اگر (m_graphing) {
            iterateChildren(nodep);
            اگر (m_alhs) {
                associateLogic(nodep, nodep->fromp());
            } ورنہ {
                associateLogic(nodep->fromp(), nodep);
            }
        } ورنہ {
            اگر (m_alhs) {
                UINFO(9, dbgState() << nodep);
                اگر (nodep->user1p()) {
                    // Form a "deposit" instruction.  Would be nicer if we made this a new AST type
                    Ast_گرہ_اظہاریہ* ثابت newp
                        = newEnableDeposit(nodep, VN_AS(nodep->user1p(), گرہ_اظہاریہ));
                    nodep->fromp()->user1p(newp);  // Push to varref (etc)
                    UINFOTREE(9, newp, "", "assign-sel");
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ورنہ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                اگر (nodep->lsbp()->user1p()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported RHS tristate construct: "
                                                     << nodep->prettyTypeName());
                }
                اگر (nodep->fromp()->user1p()) {  // SEL(VARREF, lsb)
                    Ast_گرہ_اظہاریہ* ثابت en1p = getEnp(nodep->fromp());
                    Ast_گرہ_اظہاریہ* ثابت enp
                        = نیا AstSel{nodep->fileline(), en1p, nodep->lsbp()->clone_درخت_خالص(سچ),
                                     nodep->چوڑائی_Const()};
                    UINFO(9, "       newsel " << enp);
                    nodep->user1p(enp);  // propagate up SEL(fromp->enable, value)
                    m_tgraph.didProcess(nodep);
                }
            }
        }
    }

    خالی دورہ(AstConcat* nodep) override {
        اگر (m_graphing) {
            iterateChildren(nodep);
            اگر (m_alhs) {
                associateLogic(nodep, nodep->lhsp());
                associateLogic(nodep, nodep->rhsp());
            } ورنہ {
                associateLogic(nodep->lhsp(), nodep);
                associateLogic(nodep->rhsp(), nodep);
            }
        } ورنہ {
            اگر (m_alhs) {
                UINFO(9, dbgState() << nodep);
                اگر (nodep->user1p()) {
                    // Each half of the concat gets a select of the enable expression
                    Ast_گرہ_اظہاریہ* ثابت enp = VN_AS(nodep->user1p(), گرہ_اظہاریہ);
                    nodep->user1p(خالی_اشاریہ);
                    nodep->lhsp()->user1p(نیا AstSel{nodep->fileline(), enp->clone_درخت_خالص(سچ),
                                                     nodep->rhsp()->چوڑائی(),
                                                     nodep->lhsp()->چوڑائی()});
                    nodep->rhsp()->user1p(
                        نیا AstSel{nodep->fileline(), enp, 0, nodep->rhsp()->چوڑائی()});
                    m_tgraph.didProcess(nodep);
                }
                iterateChildren(nodep);
            } ورنہ {
                iterateChildren(nodep);
                UINFO(9, dbgState() << nodep);
                // Generate the new output enable signal, just as a concat
                // identical to the data concat
                Ast_گرہ_اظہاریہ* ثابت expr1p = nodep->lhsp();
                Ast_گرہ_اظہاریہ* ثابت expr2p = nodep->rhsp();
                اگر (expr1p->user1p() || expr2p->user1p()) {  // else no tristates
                    m_tgraph.didProcess(nodep);
                    Ast_گرہ_اظہاریہ* ثابت en1p = getEnp(expr1p);
                    Ast_گرہ_اظہاریہ* ثابت en2p = getEnp(expr2p);
                    Ast_گرہ_اظہاریہ* ثابت enp = نیا AstConcat{nodep->fileline(), en1p, en2p};
                    UINFO(9, "       newconc " << enp);
                    nodep->user1p(enp);  // propagate up CONCAT(lhsp->enable, rhsp->enable)
                    expr1p->user1p(خالی_اشاریہ);
                    expr2p->user1p(خالی_اشاریہ);
                }
            }
        }
    }

    خالی دورہ(AstBufIf1* nodep) override {
        // For BufIf1, the enable is the LHS expression
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        اگر (m_graphing) {
            associateLogic(nodep->rhsp(), nodep);
            m_tgraph.سیٹ_Tristate(nodep);
        } ورنہ {
            UINFOTREE(9, nodep->backp(), "", "bufif");
            اگر (m_alhs) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                واپس;
            }
            m_tgraph.didProcess(nodep);
            Ast_گرہ_اظہاریہ* ثابت expr1p = nodep->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت expr2p = nodep->rhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* enp;
            اگر (Ast_گرہ_اظہاریہ* ثابت en2p = VN_AS(expr2p->user1p(), گرہ_اظہاریہ)) {
                enp = نیا AstAnd{nodep->fileline(), expr1p, en2p};
            } ورنہ {
                enp = expr1p;
            }
            expr1p->user1p(خالی_اشاریہ);
            expr2p->user1p(enp);  // Becomes new node
            // Don't need the BufIf any more, can just have the data direct
            nodep->replaceWith(expr2p);
            UINFO(9, "   bufif  datap=" << expr2p);
            UINFO(9, "   bufif  enp=" << enp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }

    خالی دورہ_And_Or(Ast_گرہ_Biop* nodep, منطقی isAnd) {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        اگر (m_graphing) {
            associateLogic(nodep->lhsp(), nodep);
            associateLogic(nodep->rhsp(), nodep);
        } ورنہ {
            اگر (m_alhs && nodep->user1p()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
                واپس;
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
            Ast_گرہ_اظہاریہ* ثابت expr1p = nodep->lhsp();
            Ast_گرہ_اظہاریہ* ثابت expr2p = nodep->rhsp();
            اگر (!expr1p->user1p() && !expr2p->user1p()) {
                واپس;  // no tristates in either expression, so nothing to do
            }
            m_tgraph.didProcess(nodep);
            Ast_گرہ_اظہاریہ* en1p = getEnp(expr1p);
            Ast_گرہ_اظہاریہ* en2p = getEnp(expr2p);
            Ast_گرہ_اظہاریہ* subexpr1p = expr1p->clone_درخت_خالص(جھوٹ);
            Ast_گرہ_اظہاریہ* subexpr2p = expr2p->clone_درخت_خالص(جھوٹ);
            اگر (isAnd) {
                subexpr1p = نیا AstNot{nodep->fileline(), subexpr1p};
                subexpr2p = نیا AstNot{nodep->fileline(), subexpr2p};
            }
            // The new output enable is en1&en2 | en1&sub1 | en2&sub2, which factors to
            // en1&(en2|sub1) | en2&sub2. That references en1 once and en2 twice. Chained
            // gates (e.g. a many-input primitive) nest on one side, so make the nested
            // (non-leaf) side en1 to keep the enable linear in size.
            اگر (!en1p->op1p() && en2p->op1p()) {
                std::تبادلہ(en1p, en2p);
                std::تبادلہ(subexpr1p, subexpr2p);
            }
            // calc new output enable
            Ast_گرہ_اظہاریہ* ثابت enp = نیا AstOr{
                nodep->fileline(),
                نیا AstAnd{nodep->fileline(), en1p, نیا AstOr{nodep->fileline(), en2p, subexpr1p}},
                نیا AstAnd{nodep->fileline(), en2p->clone_درخت(جھوٹ), subexpr2p}};
            UINFO(9, "       neweqn " << enp);
            nodep->user1p(enp);
            expr1p->user1p(خالی_اشاریہ);
            expr2p->user1p(خالی_اشاریہ);
        }
    }
    خالی دورہ(AstAnd* nodep) override { دورہ_And_Or(nodep, سچ); }
    خالی دورہ(AstOr* nodep) override { دورہ_And_Or(nodep, جھوٹ); }

    خالی دورہ_تفویض(Ast_گرہ_تفویض* nodep) {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_currentStrength);
        VL_RESTORER(m_currentDelayp);
        m_assignp = nodep;
        اگر (VN_IS(nodep->rhsp(), CReset)) واپس;
        اگر (Ast_تفویض_W* ثابت تفویض_Wp = VN_CAST(nodep, تفویض_W)) {
            m_currentDelayp = VN_CAST(تفویض_Wp->timingControlp(), Delay);
        }
        اگر (m_graphing) {
            اگر (Ast_تفویض_W* تفویض_Wp = VN_CAST(nodep, تفویض_W)) {
                اگر (تفویض_Wp->getLhsNetDelay()) واپس;
                اگر (تفویض_Wp->timingControlp() && !m_currentDelayp) واپس;
                // Separate data and enable assignments cannot preserve distinct rise/fall delays.
                اگر (m_currentDelayp && m_currentDelayp->fallDelay()) واپس;
                اگر (!تفویض_Wp->timingControlp()) جوڑو_مقصد_Assignment_List(تفویض_Wp);
            }

            اگر (nodep->user2() & U2_GRAPHING) واپس;
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            nodep->user2Or(U2_GRAPHING);
            iterate_And_اگلا_Null(nodep->rhsp());
            m_alhs = سچ;
            iterate_And_اگلا_Null(nodep->lhsp());
            m_alhs = جھوٹ;
            associateLogic(nodep->rhsp(), nodep);
            associateLogic(nodep, nodep->lhsp());
        } ورنہ {
            اگر (nodep->user2() & U2_NONGRAPH) {
                واپس;  // Iterated here, or created assignment to ignore
            }
            nodep->user2Or(U2_NONGRAPH);
            iterate_And_اگلا_Null(nodep->rhsp());
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "assign");
            اگر (m_procedurep) {
                UASSERT_OBJ(!VN_IS(nodep, تفویض_W), nodep,
                            "Continuous assignment inside procedural process");
                Ast_گرہ_اظہاریہ* ثابت enp
                    = nodep->rhsp()->user1p()
                          ? VN_AS(nodep->rhsp()->user1p(), گرہ_اظہاریہ)->clone_درخت_خالص(جھوٹ)
                          : newAllZerosOrOnes(nodep->lhsp(), سچ);
                ثابت منطقی inserted = m_تفویض_Enps.emplace(nodep, enp).دوسرا;
                UASSERT_OBJ(inserted, nodep, "Procedural assignment enable recorded twice");
            }
            // if the rhsp of this assign statement has an output enable driver,
            // then propagate the corresponding output enable assign statement.
            // down the lvalue tree by recursion for eventual attachment to
            // the appropriate output signal's VarRef.
            اگر (nodep->rhsp()->user1p()) {
                nodep->lhsp()->user1p(nodep->rhsp()->user1p());
                nodep->rhsp()->user1p(خالی_اشاریہ);
                UINFO(9, "   enp<-rhs " << nodep->lhsp()->user1p());
                m_tgraph.didProcess(nodep);
            } ورنہ {
                // Non-tristate RHS. For SEL assigns to tristate variables, we still
                // need to track which bits are driven by creating a proper enable.
                اگر (AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel)) {
                    اگر (Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(selp->fromp(), گرہ_Var_Ref)) {
                        اگر (m_tgraph.isTristate(varrefp->varp())) {
                            // Create an all-1s enable for the SEL width - this will be
                            // deposited into the correct bit positions by newEnableDeposit
                            nodep->lhsp()->user1p(newAllZerosOrOnes(nodep->lhsp(), سچ));
                            UINFO(9, "   enp<-nonTri SEL " << nodep->lhsp()->user1p());
                            m_tgraph.didProcess(nodep);
                        } ورنہ {
                            m_tgraph.didProcess(nodep, سچ);
                        }
                    } ورنہ {
                        m_tgraph.didProcess(nodep, سچ);  // LCOV_EXCL_LINE
                    }
                } ورنہ {
                    // Don't set user1p here as there are no handlers for many LHS node types
                    // (ArraySel, MemberSel, StructSel, etc.) and checkUnhandled() would error.
                    m_tgraph.didProcess(nodep, سچ);
                }
            }
            m_alhs = سچ;  // And user1p() will indicate tristate equation, if any
            اگر (Ast_تفویض_W* ثابت تفویض_Wp = VN_CAST(nodep, تفویض_W)) {
                اگر (AstStrengthSpec* ثابت specp = تفویض_Wp->strengthSpecp()) {
                    اگر (specp->strength0() != specp->strength1()) {
                        // Unequal strengths are not a problem if the assignment is the only
                        // assignment to its variable. Unfortunately, m_assigns map stores only
                        // assignments to var. Selects are not inserted, so they may be handled
                        // improperly
                        اگر (!is_Only_Assignment_Is_مقصد_Lhs_Var(تفویض_Wp)) {
                            تفویض_Wp->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Unable to resolve unequal strength specifier");
                        }
                    } ورنہ {
                        m_currentStrength = specp->strength0();
                    }
                }
            }
            iterate_And_اگلا_Null(nodep->lhsp());
        }
    }
    خالی دورہ(Ast_تفویض_W* nodep) override { دورہ_تفویض(nodep); }
    خالی دورہ(Ast_تفویض* nodep) override { دورہ_تفویض(nodep); }
    خالی دورہ(Ast_تفویض_Dly* nodep) override { دورہ_تفویض(nodep); }

    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        ثابت AstAlways* ثابت alwaysp = VN_CAST(nodep, Always);
        m_procedurep = alwaysp && alwaysp->keyword() == VAlwaysKwd::CONT_تفویض ? خالی_اشاریہ : nodep;
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }
    خالی دورہ(AstAlias* nodep) override {
        VL_RESTORER(m_alhs);
        VL_RESTORER(m_اندر_Alias);
        m_اندر_Alias = سچ;
        اگر (m_graphing) {
            اگر (nodep->user2() & U2_GRAPHING) واپس;
            m_alhs = سچ;  // In AstAlias all operands should be considered as lhs
            iterateChildren(nodep);
            برائے (Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                associateLogic(itemp, nodep);
                associateLogic(nodep, itemp);
            }
        } ورنہ {
            iterateChildren(nodep);
        }
    }

    خالی دورہ_Case_Eq(Ast_گرہ_Biop* nodep, منطقی neq) {
        اگر (m_graphing) {
            iterateChildren(nodep);
        } ورنہ {
            checkUnhandled(nodep);
            // Unsupported: A === 3'b000 should compare with the enables, but we don't do
            // so at present, we only compare if there is a z in the equation.
            // Otherwise we'd need to attach an enable to every signal, then optimize them
            // away later when we determine the signal has no tristate
            iterateChildren(nodep);
            UINFO(9, dbgState() << nodep);
            // Constification always moves const to LHS
            AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
            اگر (constp && constp->user1p()) {
                // 3'b1z0 -> ((3'b101 == in__en) && (3'b100 == in))
                Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();
                rhsp->unlink_Fr_عقب();
                File_سطر* ثابت fl = nodep->fileline();
                Ast_گرہ_اظہاریہ* enRhsp;
                اگر (rhsp->user1p()) {
                    enRhsp = VN_AS(rhsp->user1p(), گرہ_اظہاریہ);
                    rhsp->user1p(خالی_اشاریہ);
                } ورنہ {
                    enRhsp = get_En_اظہاریہ_Based_On_Originalp(rhsp);
                }
                Ast_گرہ_اظہاریہ* newp
                    = نیا AstLogAnd{fl, نیا AstEq{fl, VN_AS(constp->user1p(), Const), enRhsp},
                                    // Keep the caseeq if there are X's present
                                    نیا AstEqCase{fl, نیا AstConst{fl, constp->عدد()}, rhsp}};
                constp->user1p(خالی_اشاریہ);
                اگر (neq) newp = نیا AstLogNot{fl, newp};
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            } ورنہ اگر (constp && nodep->rhsp()->user1p()) {
                File_سطر* ثابت fl = nodep->fileline();
                constp->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* newp = نیا AstLogAnd{fl,
                                                  نیا AstEq{fl, newAllZerosOrOnes(constp, جھوٹ),
                                                            VN_AS(rhsp->user1p(), گرہ_اظہاریہ)},
                                                  // Keep the caseeq if there are X's present
                                                  نیا AstEqCase{fl, constp, rhsp}};
                اگر (neq) newp = نیا AstLogNot{fl, newp};
                rhsp->user1p(خالی_اشاریہ);
                UINFO(9, "       newceq " << newp);
                UINFOTREE(9, nodep, "", "caseeq-old");
                UINFOTREE(9, newp, "", "caseeq-new");
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            } ورنہ {
                checkUnhandled(nodep);
            }
        }
    }
    خالی دورہ_Eq_Neq_Wild(Ast_گرہ_Biop* nodep) {
        اگر (!VN_IS(nodep->rhsp(), Const) && nodep->rhsp()->dtypep()->isFourstate()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
            // rhs we want to keep X/Z intact, so otherwise ignore
        }
        iterate_And_اگلا_Null(nodep->lhsp());
        اگر (nodep->lhsp()->user1p()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported LHS tristate construct: " << nodep->prettyTypeName());
            واپس;
        }
    }
    خالی دورہ(AstEqCase* nodep) override { دورہ_Case_Eq(nodep, جھوٹ); }
    خالی دورہ(AstNeqCase* nodep) override { دورہ_Case_Eq(nodep, سچ); }
    خالی دورہ(AstEqWild* nodep) override { دورہ_Eq_Neq_Wild(nodep); }
    خالی دورہ(AstNeqWild* nodep) override { دورہ_Eq_Neq_Wild(nodep); }

    خالی دورہ(Ast_گنتی_بٹ* nodep) override {
        std::array<منطقی, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->عدد().isAnyZ();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->عدد().isAnyZ();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->عدد().isAnyZ();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);
        اگر (m_graphing) {
            iterate_And_اگلا_Null(nodep->lhsp());
            اگر (!dropop[0]) iterate_And_اگلا_Null(nodep->rhsp());
            اگر (!dropop[1]) iterate_And_اگلا_Null(nodep->thsp());
            اگر (!dropop[2]) iterate_And_اگلا_Null(nodep->fhsp());
        } ورنہ {
            Ast_گرہ_اظہاریہ* nonXp = خالی_اشاریہ;
            اگر (!dropop[0]) {
                nonXp = nodep->rhsp();
            } ورنہ اگر (!dropop[1]) {
                nonXp = nodep->thsp();
            } ورنہ اگر (!dropop[2]) {
                nonXp = nodep->fhsp();
            }
            // Replace 'z with non-Z
            اگر (dropop[0] || dropop[1] || dropop[2]) {
                // Unsupported: A $countones('0) should compare with the enables, but we don't
                // do so at present, we only compare if there is a z in the equation. Otherwise
                // we'd need to attach an enable to every signal, then optimize them away later
                // when we determine the signal has no tristate
                ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);  // Input variable
                اگر (!varrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported LHS tristate construct: "
                                                     << nodep->prettyTypeName());
                    واپس;
                }
                AstVar* ثابت envarp = getCreateEnVarp(varrefp->varp(), جھوٹ);
                // If any drops, we need to add in the count of Zs (from __en)
                UINFO(4, " COUNTBITS('z)-> " << nodep);
                VNRelinker relinkHandle;
                nodep->unlink_Fr_عقب(&relinkHandle);
                Ast_گرہ_اظہاریہ* newp = نیا Ast_گنتی_Ones{
                    nodep->fileline(), نیا AstVarRef{nodep->fileline(), envarp, VAccess::READ}};
                اگر (nonXp) {  // Need to still count '0 or '1 or 'x's
                    اگر (dropop[0]) {
                        nodep->rhsp()->unlink_Fr_عقب()->delete_درخت();
                        nodep->rhsp(nonXp->clone_درخت_خالص(سچ));
                    }
                    اگر (dropop[1]) {
                        nodep->thsp()->unlink_Fr_عقب()->delete_درخت();
                        nodep->thsp(nonXp->clone_درخت_خالص(سچ));
                    }
                    اگر (dropop[2]) {
                        nodep->fhsp()->unlink_Fr_عقب()->delete_درخت();
                        nodep->fhsp(nonXp->clone_درخت_خالص(سچ));
                    }
                    newp = نیا Ast_جوڑو{nodep->fileline(), nodep, newp};
                } ورنہ {
                    ڈالو_Deletep(nodep);  // TODO: looks dubious that we still iterate this below...
                }
                UINFOTREE(9, newp, "", "countout");
                relinkHandle.relink(newp);
            }
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstPull* nodep) override {
        UINFO(9, dbgState() << nodep);
        AstVarRef* varrefp = خالی_اشاریہ;
        اگر (VN_IS(nodep->lhsp(), VarRef)) {
            varrefp = VN_AS(nodep->lhsp(), VarRef);
        } ورنہ اگر (VN_IS(nodep->lhsp(), Sel)
                   && VN_IS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef)) {
            varrefp = VN_AS(VN_AS(nodep->lhsp(), Sel)->fromp(), VarRef);
        }
        اگر (!varrefp) {
            UINFOTREE(4, nodep, "", "");
            nodep->v3warn(E_UNSUPPORTED, "Unsupported pullup/down (weak driver) construct.");
        } ورنہ {
            اگر (m_graphing) {
                VL_RESTORER(m_logicp);
                m_logicp = nodep;
                varrefp->access(VAccess::WRITE);
                m_tgraph.سیٹ_Tristate(nodep);
                associateLogic(nodep, varrefp->varp());
            } ورنہ {
                // Replace any pullup/pulldowns with assignw logic and set the
                // direction of the pull in the var aux data.  A pull on a
                // constant part select is recorded per bit, so different bits
                // of a bus may be pulled in opposite directions.  Otherwise
                // the pull applies to all bits of the variable.
                varrefp->access(VAccess::WRITE);
                m_tgraph.didProcess(nodep);
                m_tgraph.didProcess(varrefp->varp());
                ثابت AstSel* ثابت selp = VN_CAST(nodep->lhsp(), Sel);
                ثابت AstConst* ثابت lsbp = selp ? VN_CAST(selp->lsbp(), Const) : خالی_اشاریہ;
                اگر (lsbp) {
                    ثابت صحیح lsb = lsbp->مقصد_SInt();
                    برائے (صحیح i = 0; i < selp->چوڑائی_Const(); ++i) {
                        سیٹ_ثنائی_ہندسہ_Pull_Direction(varrefp->varp(), lsb + i, nodep->direction());
                    }
                } ورنہ {
                    سیٹ_Pull_Direction(varrefp->varp(), nodep);
                }
            }
        }
        اگر (!m_graphing) {
            nodep->unlink_Fr_عقب();
            // Node must persist as var aux data points to it
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }

    خالی iteratePinGuts(AstPin* nodep) {
        اگر (m_graphing) {
            VL_RESTORER(m_logicp);
            m_logicp = nodep;
            اگر (nodep->exprp()) {
                associateLogic(nodep->exprp(), nodep);
                associateLogic(nodep, nodep->exprp());
            }
            iterateChildren(nodep);
        } ورنہ {
            // All heavy lifting completed in graph visitor.
            اگر (nodep->exprp()) m_tgraph.didProcess(nodep);
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
    خالی دورہ(AstPin* nodep) override {
        اگر (m_graphing) {
            اگر (nodep->user2() & U2_GRAPHING) واپس;  // This pin is already expanded
            nodep->user2Or(U2_GRAPHING);
            // Find child module's new variables.
            AstVar* ثابت enModVarp = static_cast<AstVar*>(nodep->modVarp()->user1p());
            اگر (!enModVarp) {
                // May have an output only that later connects to a tristate, so simplify now.
                V3Inst::pinReconnectSimple(nodep, m_cellp, جھوٹ);
                iteratePinGuts(nodep);
                واپس;  // No __en signals on this pin
            }
            // Tristate exists:
            UINFO(9, dbgState() << nodep);
            UINFOTREE(9, nodep, "", "pin-pre");

            // Empty/in-only; need Z to propagate
            ثابت منطقی اندر_Decl_Processing = (nodep->exprp()
                                           && nodep->modVarp()->direction() == VDirection::داخلہ
                                           // Need to consider the original state
                                           // instead of current state as we converted
                                           // tristates to inputs, which do not want
                                           // to have this.
                                           && !nodep->modVarp()->declDirection().isWritable());
            اگر (!nodep->exprp()) {  // No-connect; covert to empty connection
                UINFO(5, "Unconnected pin terminate " << nodep);
                AstVar* ثابت ucVarp = getCreateUnconnVarp(nodep, nodep->modVarp()->dtypep());
                nodep->exprp(نیا AstVarRef{nodep->fileline(), ucVarp,
                                           // We converted, so use declaration output state
                                           nodep->modVarp()->declDirection().isWritable()
                                               ? VAccess::WRITE
                                               : VAccess::READ});
                m_tgraph.سیٹ_Tristate(ucVarp);
                // We don't need a driver on the wire; the lack of one will default to tristate
            } ورنہ اگر (اندر_Decl_Processing) {  // Not an input that was a converted tristate
                // Input only may have driver in underneath module which would stomp
                // the input value.  So make a temporary connection.
                ثابت Ast_تفویض_W* ثابت reAssignp
                    = V3Inst::pinReconnectSimple(nodep, m_cellp, سچ, سچ);
                UINFO(5, "Input pin buffering: " << reAssignp);
                m_tgraph.سیٹ_Tristate(reAssignp->lhsp());
            }

            // pinReconnectSimple needs to presume input or output behavior; we need both
            // Therefore, create the enable, output and separate input pin,
            // then pinReconnectSimple all
            // Create the output enable pin, connect to new signal
            AstVar* ثابت enVarp = نیا AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                              nodep->name() + "__en" + cvt_مقصد_Str(m_unique++),
                                              VFlag_ثنائی_ہندسہ_Packed{}, enModVarp->چوڑائی()};
            اگر (اندر_Decl_Processing) {  // __en(from-resolver-const) or __en(from-resolver-wire)
                enModVarp->var_Type2_اندر();
            } ورنہ {
                enModVarp->var_Type2_برآمد();
            }
            UINFO(9, "       newenv " << enVarp);
            AstPin* ثابت enpinp
                = نیا AstPin{nodep->fileline(), nodep->pin_عدد(),
                             enModVarp->name(),  // should be {var}"__en"
                             نیا AstVarRef{nodep->fileline(), enVarp,
                                           اندر_Decl_Processing ? VAccess::READ : VAccess::WRITE}};
            enpinp->modVarp(enModVarp);
            UINFO(9, "       newpin " << enpinp);
            enpinp->user2Or(U2_BOTH);  // don't iterate the pin later
            nodep->جوڑو_اگلا_یہاں(enpinp);
            m_modp->جوڑو_Stmtsp(enVarp);
            UINFOTREE(9, enpinp, "", "pin-ena");

            // Create new output pin
            ثابت Ast_تفویض_W* برآمد_Assignp = خالی_اشاریہ;  // If reconnected, the related assignment
            AstPin* outpinp = خالی_اشاریہ;
            AstVar* ثابت برآمد_Mod_Varp = m_varAux(nodep->modVarp()).برآمد_Varp;
            اگر (!برآمد_Mod_Varp) {
                // At top, no need for __out as might be input only. Otherwise resolvable.
                اگر (!m_modp->is_چوٹی()) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: tristate in top-level IO: "
                                                     << nodep->prettyNameQ());
                }
            } ورنہ {
                Ast_گرہ_اظہاریہ* ثابت outexprp = VN_AS(nodep->exprp(), گرہ_اظہاریہ)
                                                  ->clone_درخت_خالص(جھوٹ);  // Note has lvalue() set
                outpinp = نیا AstPin{nodep->fileline(), nodep->pin_عدد(),
                                     برآمد_Mod_Varp->name(),  // should be {var}"__out"
                                     outexprp};
                outpinp->modVarp(برآمد_Mod_Varp);
                UINFO(9, "       newpin " << outpinp);
                outpinp->user2Or(U2_BOTH);  // don't iterate the pin later
                nodep->جوڑو_اگلا_یہاں(outpinp);
                // Simplify
                اگر (اندر_Decl_Processing) {  // Not an input that was a converted tristate
                    // The pin is an input, but we need an output
                    // The if() above is needed because the Visitor is
                    // simple, it will flip ArraySel's and such, but if the
                    // pin is an input the earlier reconnectSimple made it
                    // a VarRef without any ArraySel, etc
                    TristatePinVisitor{outexprp, m_tgraph, سچ};
                }
                UINFOTREE(9, outpinp, "", "pin-opr");
                برآمد_Assignp = V3Inst::pinReconnectSimple(outpinp, m_cellp,
                                                        سچ);  // Note may change outpinp->exprp()
                UINFOTREE(9, outpinp, "", "pin-out");
                UINFOTREE(9, برآمد_Assignp, "", "pin-oas");
                // Must still iterate the outAssignp, as need to build output equation
            }

            // Existing pin becomes an input, and we mark each resulting signal as tristate
            ثابت TristatePinVisitor visitor{nodep->exprp(), m_tgraph, جھوٹ};
            ثابت Ast_گرہ* ثابت اندر_Assignp = V3Inst::pinReconnectSimple(
                nodep, m_cellp, سچ);  // Note may change nodep->exprp()
            UINFOTREE(9, nodep, "", "pin-in:");
            UINFOTREE(9, اندر_Assignp, "", "pin-as:");

            // Connect enable to output signal
            AstVarRef* exprrefp;  // Tristate variable that the Pin's expression refers to
            اگر (!برآمد_Assignp) {
                اگر (!outpinp) {
                    exprrefp = خالی_اشاریہ;  // Primary input only
                } ورنہ {
                    // pinReconnect should have converted this
                    exprrefp = VN_CAST(outpinp->exprp(), VarRef);
                    اگر (!exprrefp) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                         << nodep->exprp()->prettyTypeName());
                    }
                }
            } ورنہ {
                // pinReconnect should have converted this
                exprrefp = VN_CAST(برآمد_Assignp->rhsp(),
                                   VarRef);  // This should be the same var as the output pin
                اگر (!exprrefp) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported tristate port expression: "
                                                     << nodep->exprp()->prettyTypeName());
                }
            }
            اگر (exprrefp) {
                UINFO(9, "outref " << exprrefp);
                // Mark as now tristated; iteration will pick it up from there
                exprrefp->user1p(نیا AstVarRef{nodep->fileline(), enVarp, VAccess::READ});
                اگر (!برآمد_Assignp) {
                    map_داخل_کرو_Lhs_Var_Ref(exprrefp);  // insertTristates will convert
                    //                     // to a varref to the __out# variable
                }  // else the assignment deals with the connection
            }

            // Propagate any pullups/pulldowns upwards if necessary
            اگر (exprrefp) {
                AstPull* ثابت pullp = m_varAux(nodep->modVarp()).pullp;
                ثابت خودکار& src_ثنائی_ہندسہ_Pulls = m_varAux(nodep->modVarp()).ثنائی_ہندسہ_Pulls;

                // For part-select port connections (e.g. .out(bus[23:16])) extract
                // the parent var, LSB, and width once; reused by both propagations.
                AstVar* selVarp = خالی_اشاریہ;
                صحیح selLsb = 0;
                صحیح sel_چوڑائی = 0;
                اگر (برآمد_Assignp) {
                    اگر (AstSel* ثابت selp = VN_CAST(برآمد_Assignp->lhsp(), Sel)) {
                        AstVarRef* ثابت vrefp = VN_CAST(selp->fromp(), VarRef);
                        AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
                        اگر (vrefp && lsbp) {  // LCOV_EXCL_BR_LINE
                            selVarp = vrefp->varp();
                            selLsb = lsbp->مقصد_SInt();
                            sel_چوڑائی = selp->چوڑائی_Const();
                        }
                    }
                }

                اگر (pullp) {
                    UINFO(9, "propagate pull on " << exprrefp);
                    سیٹ_Pull_Direction(exprrefp->varp(), pullp);
                    // For a part-select target, record per-bit pull direction across
                    // the SEL range instead of setting pull on the whole variable.
                    اگر (selVarp) {
                        ثابت صحیح direction = pullp->direction();
                        برائے (صحیح i = 0; i < sel_چوڑائی; ++i) {
                            سیٹ_ثنائی_ہندسہ_Pull_Direction(selVarp, selLsb + i, direction);
                        }
                    }
                }

                // Per-bit pulls: the source bit indices must be offset by the SEL's
                // LSB so they land on the correct bits of the parent variable.
                اگر (!src_ثنائی_ہندسہ_Pulls.خالی_خانے()) {
                    UINFO(9, "propagate per-bit pulls from " << nodep->modVarp());
                    برائے (ثابت خودکار& pair : src_ثنائی_ہندسہ_Pulls) {
                        سیٹ_ثنائی_ہندسہ_Pull_Direction(exprrefp->varp(), pair.پہلا, pair.دوسرا);
                    }  // LCOV_EXCL_LINE
                    اگر (selVarp) {
                        UINFO(9, "propagate per-bit pulls to SEL target " << selVarp
                                                                          << " offset=" << selLsb);
                        برائے (ثابت خودکار& pair : src_ثنائی_ہندسہ_Pulls) {
                            سیٹ_ثنائی_ہندسہ_Pull_Direction(selVarp, selLsb + pair.پہلا, pair.دوسرا);
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
        ورنہ {
            اگر (nodep->user2() & U2_NONGRAPH) واپس;  // This pin is already expanded
            nodep->user2Or(U2_NONGRAPH);
            UINFO(9, " " << nodep);
            iteratePinGuts(nodep);
        }
    }

    خالی handle_گرہ_Var_Ref(Ast_گرہ_Var_Ref* nodep) {
        UINFO(9, dbgState() << nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        اگر (m_graphing) {
            اگر (nodep->access().isWriteOrRW()) associateLogic(nodep, nodep->varp());
            اگر (nodep->access().isReadOrRW()) associateLogic(nodep->varp(), nodep);
            // Only interface tristate nets need this: a plain (non-Z) cross-hierarchy
            // driver has no Z in its own module's graph, so mark the net tristate here to
            // collect the driver as a contribution. The ifaceTristate flag is only set for
            // interface nets; a non-interface cross-module tri driver does nothing here and
            // is instead rejected (E_UNSUPPORTED) later in insertTristates.
            اگر (nodep->access().isWriteOrRW() && VN_IS(nodep, VarXRef)
                && m_varAux(nodep->varp()).ifaceTristate) {
                m_tgraph.سیٹ_Tristate(nodep->varp());
            }
        } ورنہ {
            اگر (nodep->user2() & U2_NONGRAPH) واپس;  // Processed
            nodep->user2Or(U2_NONGRAPH);
            // Detect all var lhs drivers and adds them to the
            // VarMap so that after the walk through the module we can expand
            // any tristate logic on the driver.
            اگر (nodep->access().isWriteOrRW() && m_tgraph.isTristate(nodep->varp())) {
                UINFO(9, "     Ref-to-lvalue " << nodep);
                اگر (m_اندر_Alias) {
                    اگر (nodep->varp()->direction().isAny()) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Port as alias argument: "
                                                         << nodep->prettyNameQ());
                    } ورنہ {
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: Tristate variable referenced in alias: "
                                          << nodep->prettyNameQ());
                    }
                    واپس;
                }
                UASSERT_OBJ(!nodep->access().isRW(), nodep, "Tristate unexpected on R/W access");
                m_tgraph.didProcess(nodep);
                map_داخل_کرو_Lhs_Var_Ref(nodep);
            } ورنہ اگر (nodep->access().isReadOnly()
                       // Not already processed, nor varref from visit(AstPin) creation
                       && !nodep->user1p()
                       // Reference to another tristate variable
                       && m_tgraph.isTristate(nodep->varp())
                       // and in a position where it feeds upstream to another tristate
                       && m_tgraph.feedsTri(nodep)) {
                // Then propagate the enable from the original variable
                UINFO(9, "     Ref-to-tri " << nodep);
                File_سطر* ثابت fl = nodep->fileline();
                AstVar* ثابت enVarp = getCreateEnVarp(nodep->varp(), جھوٹ);
                اگر (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
                    nodep->user1p(newVarXRef(fl, enVarp, xrefp->dotted(), VAccess::READ,
                                             xrefp->inlinedDots()));
                } ورنہ {
                    nodep->user1p(نیا AstVarRef{fl, enVarp, VAccess::READ});
                }
            }
        }
    }

    خالی دورہ(AstVarRef* nodep) override { handle_گرہ_Var_Ref(nodep); }
    خالی دورہ(AstVarXRef* nodep) override { handle_گرہ_Var_Ref(nodep); }

    خالی دورہ(AstVar* nodep) override {
        iterateChildren(nodep);
        UINFO(9, dbgState() << nodep);
        اگر (m_graphing) {
            // If tri0/1 force a pullup
            اگر (nodep->user2() & U2_GRAPHING) واپس;  // Already processed
            nodep->user2Or(U2_GRAPHING);

            اگر (nodep->isPulldown() || nodep->isPullup()) {
                Ast_گرہ* ثابت newp = نیا AstPull{
                    nodep->fileline(), نیا AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                    nodep->isPullup()};
                UINFO(9, "       newpul " << newp);
                nodep->جوڑو_اگلا_یہاں(newp);
                // We'll iterate on the new AstPull later
            }
            اگر (nodep->isInout()) {
                //|| varp->isOutput()
                // Note unconnected output only changes behavior vs. previous
                // versions and causes outputs that don't come from anywhere to
                // possibly create connection errors.
                // One example of problems is this:  "output z;  task t; z <= {something}; endtask"
                UINFO(9, "  setTristate-inout " << nodep);
                m_tgraph.سیٹ_Tristate(nodep);
            }
        } ورنہ {  // !graphing
            اگر (m_tgraph.isTristate(nodep)) {
                // nodep->isPulldown() || nodep->isPullup() handled in TristateGraphVisitor
                m_tgraph.didProcess(nodep);
            }
        }
    }

    خالی دورہ(Ast_گرہ_Module* nodep) override {
        // Interfaces are processed first in the constructor; skip the duplicate visit
        // during the later iterateChildrenBackwardsConst() pass over all modules.
        اگر (m_processedIfaces && VN_IS(nodep, Iface)) واپس;
        UINFO(8, dbgState() << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_graphing);
        VL_RESTORER(m_unique);
        VL_RESTORER_CLEAR(m_lhsmap);
        VL_RESTORER_CLEAR(m_assigns);
        VL_RESTORER_CLEAR(m_تفویض_Enps);
        VL_RESTORER_CLEAR(m_proceduralEnables);
        // Not preserved, needs pointer instead: TristateGraph origTgraph = m_tgraph;
        UASSERT_OBJ(m_tgraph.خالی_خانے(), nodep, "Unsupported: NodeModule under NodeModule");

        // Clear state
        m_graphing = جھوٹ;
        m_tgraph.clearAndCheck();
        m_unique = 0;
        m_logicp = خالی_اشاریہ;
        m_modp = nodep;
        // Walk the graph, finding all variables and tristate constructs
        m_graphing = سچ;
        UINFO(9, dbgState() << "graphing mod " << nodep);
        iterateChildren(nodep);
        m_graphing = جھوٹ;
        UINFO(9, dbgState() << "processing mod " << nodep);

        // Merge the assignments for very Wired net LHS : wor, trior, wand and triand
        ملاؤ_Wired_Nets_Assignments();
        // Remove all assignments not stronger than the strongest uniform constant
        remove_Assignments_Not_Stronger_Than_یکساں_Constant();
        // Use graph to find tristate signals
        m_tgraph.گراف_Walk(nodep);

        // Remove all assignments not stronger than the strongest non-tristate RHS
        removeAssignmentsNotStrongerThanNonTristate();

        // Build the LHS drivers map for this module
        iterateChildren(nodep);
        // Insert new logic for all tristates
        داخل_کرو_Tristates(nodep);
        داخل_کرو_Procedural_Enable_Assignments(nodep);

        برائے (خودکار& تفویض_Enp : m_تفویض_Enps) {
            VL_DO_DANGLING(تفویض_Enp.دوسرا->delete_درخت(), تفویض_Enp.دوسرا);
        }
        m_تفویض_Enps.clear();

        m_tgraph.clearAndCheck();  // Recursion not supported
    }

    خالی دورہ(AstClass* nodep) override {
        // don't deal with classes
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        // don't deal with functions
    }

    خالی دورہ(Ast_Case_شے* nodep) override {
        // don't deal with casez compare '???? values
        iterate_And_اگلا_Null(nodep->stmtsp());
    }

    خالی دورہ(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_alhs);
        m_cellp = nodep;
        m_alhs = جھوٹ;
        iterateChildren(nodep);
    }

    // Default: Just iterate
    خالی دورہ(Ast_گرہ* nodep) override {
        iterateChildren(nodep);
        checkUnhandled(nodep);
    }

    خالی combineIfaceContribs() {
        // After all modules have been processed, combine per-module contributions
        // for each interface tristate signal into final resolution logic.
        // Key is the canonical AstVar in the interface module (shared across instances).
        برائے (ثابت std::pair<AstVar* ثابت, std::سمتیہ<std::pair<AstVar*, AstVar*>>>& kv :
             m_ifaceContribs) {
            AstVar* ثابت invarp = kv.پہلا;
            ثابت std::سمتیہ<std::pair<AstVar*, AstVar*>>& contribs = kv.دوسرا;
            Ast_گرہ_Module* ثابت ifaceModp = ڈھونڈو_والد_Module(invarp);
            UASSERT_OBJ(ifaceModp, invarp, "Interface tristate var has no parent module");
            File_سطر* ثابت fl = invarp->fileline();

            UINFO(8, "  IFACE COMBINE " << contribs.سائز() << " contribs for " << invarp);

            // Build resolution: d = (out0 & en0) | (out1 & en1) | ... | (~combined_en & pull)
            Ast_گرہ_اظہاریہ* orp = خالی_اشاریہ;
            Ast_گرہ_اظہاریہ* enp = خالی_اشاریہ;

            برائے (ثابت std::pair<AstVar*, AstVar*>& contrib : contribs) {
                AstVar* ثابت contribEnp = contrib.پہلا;
                AstVar* ثابت contribOutp = contrib.دوسرا;

                Ast_گرہ_اظہاریہ* ثابت برآمد_Refp = نیا AstVarRef{fl, contribOutp, VAccess::READ};
                Ast_گرہ_اظہاریہ* ثابت enRefp = نیا AstVarRef{fl, contribEnp, VAccess::READ};
                Ast_گرہ_اظہاریہ* ثابت andp = نیا AstAnd{fl, برآمد_Refp, enRefp};

                orp = (!orp) ? andp : نیا AstOr{fl, orp, andp};

                Ast_گرہ_اظہاریہ* ثابت enRef2p = نیا AstVarRef{fl, contribEnp, VAccess::READ};
                enp = (!enp) ? enRef2p : نیا AstOr{fl, enp, enRef2p};
            }

            // Create or get __en var for this interface signal
            AstVar* envarp = VN_CAST(invarp->user1p(), Var);
            اگر (!envarp) {
                envarp = نیا AstVar{fl, VVarType::MODULETEMP, invarp->name() + "__en", invarp};
                envarp->سیٹ_Dfg_Tri_Lowered();
                ifaceModp->جوڑو_Stmtsp(envarp);
                invarp->user1p(envarp);
            }

            // Assign combined enable
            Ast_تفویض_W* ثابت enAssp
                = نیا Ast_تفویض_W{fl, نیا AstVarRef{fl, envarp, VAccess::WRITE}, enp};
            UINFOTREE(9, enAssp, "", "iface-enAssp");
            ifaceModp->جوڑو_Stmtsp(نیا AstAlways{enAssp});

            // Pull resolution
            ثابت AstPull* ثابت pullp = m_varAux(invarp).pullp;
            ثابت منطقی pull1 = pullp && pullp->direction() == 1;  // Else default is down

            Ast_گرہ_اظہاریہ* undrivenp = نیا AstNot{fl, نیا AstVarRef{fl, envarp, VAccess::READ}};
            undrivenp = نیا AstAnd{fl, undrivenp, newAllZerosOrOnes(invarp, pull1)};
            orp = نیا AstOr{fl, orp, undrivenp};

            // Assign final value to invarp
            Ast_تفویض_W* ثابت assp
                = نیا Ast_تفویض_W{fl, نیا AstVarRef{fl, invarp, VAccess::WRITE}, orp};
            assp->user2Or(U2_BOTH);
            UINFOTREE(9, assp, "", "iface-lhsp-eqn");
            ifaceModp->جوڑو_Stmtsp(نیا AstAlways{assp});
        }
    }

عوامی:
    // CONSTRUCTORS
    صریح TristateVisitor(AstNetlist* netlistp) {
        m_tgraph.clearAndCheck();
        // Process interface modules first, so an interface tristate net is recorded
        // (AuxAstVar::ifaceTristate) before any module that drives it across hierarchy is
        // graphed. A module driving such a net with a plain (non-Z) assign has no Z in its
        // own graph to mark the net tristate, so without this its driver would be dropped.
        // Collect only the interfaces (reverse declaration order to match the pass below).
        std::سمتیہ<Ast_گرہ_Module*> ifacesp;
        برائے (Ast_گرہ* modp = netlistp->modulesp(); modp; modp = modp->nextp()) {
            اگر (VN_IS(modp, Iface)) ifacesp.ڈالو_عقب(VN_AS(modp, گرہ_Module));
        }
        برائے (خودکار it = ifacesp.rbegin(); it != ifacesp.rend(); ++it) iterate(*it);
        m_processedIfaces = سچ;
        // Then the rest in the historical order; interfaces are skipped (already done).
        iterateChildrenBackwardsConst(netlistp);

        // Combine interface tristate contributions after all modules processed
        combineIfaceContribs();

#اگر_تعریف_شدہ VL_LEAK_CHECKS
        // It's a bit chaotic up there
        std::سمتیہ<Ast_گرہ*> غیر_مستعمل_Rootps;
        netlistp->foreach([&](Ast_گرہ* nodep) {
            Ast_گرہ* ثابت enp = nodep->user1p();
            اگر (!enp) واپس;
            اگر (enp->backp()) واپس;
            غیر_مستعمل_Rootps.emplace_عقب(enp);
        });
        برائے (Ast_گرہ* ثابت nodep : غیر_مستعمل_Rootps) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
#اگر_ختم
    }
    ~TristateVisitor() override {
        V3Stats::جوڑو_Stat("Tristate, Tristate resolved nets", m_statTriSigs);
    }
};

//######################################################################
// Tristate class functions

خالی V3Tristate::tristateAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TristateVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("tristate", 0, dump_درخت_Either_Level() >= 3);
}
