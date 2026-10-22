// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Scheduling - partitioning
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
//
// V3SchedPartition (and in particular V3Sched::partition) partitions all
// logic into two regions, the 'act' region contains all logic that might
// compute a clock via an update even that falls into the SystemVerilog Active
// scheduling region (that is: blocking and continuous assignments in
// particular). All other logic is assigned to the 'nba' region.
//
// To achieve this, we build a dependency graph of all logic in the design,
// and trace back from every AstSenItem through all logic that might (via an
// Active region update) feed into triggering that AstSenItem. Any such logic
// is then assigned to the 'act' region, and all other logic is assigned to
// the 'nba' region.
//
// For later practical purposes, AstAlwaysPre logic that would be assigned to
// the 'act' region is returned separately. Nevertheless, this logic is part of
// the 'act' region.
//
// For more details, please see the internals documentation.
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <tuple>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {

ਨਾਮਾਕਾਸ਼ {

ਵਰਗ Sched_Sen_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sched_Sen_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    ਅਚਲ Ast_Sen_ਵਸਤੂ* ਅਚਲ m_senItemp;

ਜਨਤਕ:
    Sched_Sen_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, ਅਚਲ Ast_Sen_ਵਸਤੂ* senItemp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ਅਚਲ override {
        std::ostringstream os;
        V3EmitV::verilog_For_ਰੁੱਖ(const_cast<Ast_Sen_ਵਸਤੂ*>(m_senItemp), os);
        ਵਾਪਸ os.str();
    }
    string dotShape() ਅਚਲ override { ਵਾਪਸ "doubleoctagon"; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "red"; }
    // LCOV_EXCL_STOP
};

ਵਰਗ Sched_Logic_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sched_Logic_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    AstScope* ਅਚਲ m_scopep;
    Ast_Sen_ਰੁੱਖ* ਅਚਲ m_senTreep;
    Ast_ਗੰਢ* ਅਚਲ m_logicp;

ਜਨਤਕ:
    Sched_Logic_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstScope* scopep, Ast_Sen_ਰੁੱਖ* senTreep, Ast_ਗੰਢ* logicp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_scopep; }
    Ast_Sen_ਰੁੱਖ* senTreep() ਅਚਲ { ਵਾਪਸ m_senTreep; }
    Ast_ਗੰਢ* logicp() ਅਚਲ { ਵਾਪਸ m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ਅਚਲ override { ਵਾਪਸ "rectangle"; }
    // LCOV_EXCL_STOP
};

ਵਰਗ Sched_Var_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sched_Var_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    ਅਚਲ AstVarScope* ਅਚਲ m_vscp;

ਜਨਤਕ:
    Sched_Var_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ਅਚਲ { ਵਾਪਸ m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_vscp->name(); }
    string dotShape() ਅਚਲ override {
        ਵਾਪਸ m_vscp->scopep()->is_ਸਿਖਰ() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ਅਚਲ override {
        ਵਾਪਸ m_vscp->scopep()->is_ਸਿਖਰ() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

ਵਰਗ Sched_ਗ੍ਰਾਫ_Builder final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;

    // STATE
    V3_ਗ੍ਰਾਫ* ਅਚਲ m_graphp = ਨਵਾਂ V3_ਗ੍ਰਾਫ;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<Ast_Sen_ਵਸਤੂ>, Sched_Sen_ਸ਼ੀਰਸ਼*> m_senVertices;
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // AstScope of the current AstActive
    Ast_Sen_ਰੁੱਖ* m_senTreep = ਖਾਲੀ_ਸੂਚਕ;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<ਤਾਰਕਿਕ(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ਅਚਲ m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    Sched_Var_ਸ਼ੀਰਸ਼* get_Var_ਸ਼ੀਰਸ਼(AstVarScope* vscp) ਅਚਲ {
        ਜੇ (!vscp->user1p()) {
            Sched_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਨਵਾਂ Sched_Var_ਸ਼ੀਰਸ਼{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            ਜੇ (vscp->varp()->isWrittenByDpi()) {
                ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp, get_Var_ਸ਼ੀਰਸ਼(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        ਵਾਪਸ vscp->user1u().ਮੰਜ਼ਿਲ<Sched_Var_ਸ਼ੀਰਸ਼*>();
    }

    Sched_Sen_ਸ਼ੀਰਸ਼* get_Sen_ਸ਼ੀਰਸ਼(Ast_Sen_ਵਸਤੂ* senItemp) {
        ਜੇ (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_senVertices.emplace(*senItemp, ਖਾਲੀ_ਸੂਚਕ);
            // If it does not exist, create it
            ਜੇ (pair.ਦੂਜਾ) {
                // Create the vertex
                Sched_Sen_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਨਵਾਂ Sched_Sen_ਸ਼ੀਰਸ਼{m_graphp, senItemp};

                // Connect up the variable references
                ਜੇ (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp, get_Var_ਸ਼ੀਰਸ਼(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.ਪਹਿਲਾ->ਦੂਜਾ = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.ਪਹਿਲਾ->ਦੂਜਾ);
        }
        ਵਾਪਸ senItemp->user1u().ਮੰਜ਼ਿਲ<Sched_Sen_ਸ਼ੀਰਸ਼*>();
    }

    ਖਾਲੀ ਦੇਖੋ_Logic(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        Sched_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ logicVtxp
            = ਨਵਾਂ Sched_Logic_ਸ਼ੀਰਸ਼{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        ਜੇ (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([ਇਹ, nodep, logicVtxp](Ast_Sen_ਵਸਤੂ* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ eventVtxp = get_Sen_ਸ਼ੀਰਸ਼(senItemp);
                ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::Var_Scope_ਸੈੱਟ force_Read_ਕਿਨਾਰਾ_Ignores;
        V3Sched::util::collect_Force_Read_ਕਿਨਾਰਾ_Ignores(nodep, force_Read_ਕਿਨਾਰਾ_Ignores);
        nodep->foreach([ਇਹ, logicVtxp, &force_Read_ਕਿਨਾਰਾ_Ignores](ਅਚਲ AstVarRef* vrefp) {
            AstVarScope* ਅਚਲ vscp = vrefp->varScopep();
            ਜੇ (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !force_Read_ਕਿਨਾਰਾ_Ignores.ਗਿਣਤੀ_2(vscp)) {
                ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp, get_Var_ਸ਼ੀਰਸ਼(vscp), logicVtxp, 10};
            }
            ਜੇ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp, logicVtxp, get_Var_ਸ਼ੀਰਸ਼(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        ਜੇ (m_dpiExportTriggerp) {
            nodep->foreach([ਇਹ, logicVtxp](ਅਚਲ AstCCall* callp) {
                ਜੇ (!callp->funcp()->dpiImportWrapper()) ਵਾਪਸ;
                ਜੇ (!callp->funcp()->dpiContext()) ਵਾਪਸ;
                ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp, logicVtxp, get_Var_ਸ਼ੀਰਸ਼(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        ਜੇ (senTreep->hasHybrid()) {
            Ast_ਗੰਢ::user2_Clear_ਰੁੱਖ();
            senTreep->foreach([](ਅਚਲ AstVarRef* refp) {  //
                refp->varScopep()->user2(ਸੱਚ);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override { ਦੇਖੋ_Logic(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override { ਦੇਖੋ_Logic(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override { ਦੇਖੋ_Logic(nodep); }

    // Pre and Post logic are handled separately
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPre* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    ਖਾਲੀ ਦੇਖੋ(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    ਖਾਲੀ ਦੇਖੋ(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    Sched_ਗ੍ਰਾਫ_Builder(ਅਚਲ LogicByScope& clockedLogic, ਅਚਲ LogicByScope& combinationalLogic,
                      ਅਚਲ LogicByScope& hybridLogic) {
        // Build the data flow graph
        ਅਚਲ ਸਵੈਚਾਲਿਤ iter = [ਇਹ](ਅਚਲ LogicByScope& lbs) {
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : lbs) {
                m_scopep = ਇੰਦਰਾਜ.m_scopep;
                iterate(ਇੰਦਰਾਜ.m_activep);
                m_scopep = ਖਾਲੀ_ਸੂਚਕ;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ਵਾਪਸ ਝੂਠ; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ਵਾਪਸ ਸੱਚ; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { ਵਾਪਸ !vscp->user2(); };
        iter(hybridLogic);
    }

ਜਨਤਕ:
    // Build the dataflow graph for partitioning
    ਸਥਿਰ std::unique_ptr<V3_ਗ੍ਰਾਫ> build(ਅਚਲ LogicByScope& clockedLogic,
                                          ਅਚਲ LogicByScope& combinationalLogic,
                                          ਅਚਲ LogicByScope& hybridLogic) {
        ਅਚਲ Sched_ਗ੍ਰਾਫ_Builder visitor{clockedLogic, combinationalLogic, hybridLogic};
        ਵਾਪਸ std::unique_ptr<V3_ਗ੍ਰਾਫ>{visitor.m_graphp};
    }
};

ਖਾਲੀ color_ਸਰਗਰਮ_Region(V3_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) {
    // Work queue for depth first traversal
    std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> ਕਤਾਰ{};

    // Trace from all SchedSenVertex
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : ਗ੍ਰਾਫ.vertices()) {
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ ਸਰਗਰਮ_Event_Vtxp = vtx.cast<Sched_Sen_ਸ਼ੀਰਸ਼>()) {
            ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(ਸਰਗਰਮ_Event_Vtxp);
        }
    }

    // Depth first traversal
    ਜਦੋਂ (!ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        // Pop next work item
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx = *ਕਤਾਰ.ਪਿੱਠ();
        ਕਤਾਰ.ਕੱਢੋ_ਪਿੱਠ();
        // If not first encounter, move on
        ਜੇ (vtx.color() != 0) ਜਾਰੀ;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtx.ਵਿੱਚ_ਕਿਨਾਰੇ()) ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(ਕਿਨਾਰਾ.fromp());

        // Mark top level ports that drive a sensitivity list
        ਜੇ (Sched_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp = vtx.cast<Sched_Var_ਸ਼ੀਰਸ਼>()) {
            AstVar* ਅਚਲ varp = vvtxp->varp();
            ਜੇ (varp->isPrimaryIO()) varp->ਸੈੱਟ_Primary_Clock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        ਜੇ (vtx.is<Sched_Logic_ਸ਼ੀਰਸ਼>()) {
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtx.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                UASSERT(ਕਿਨਾਰਾ.ਸਿਖਰ()->is<Sched_Var_ਸ਼ੀਰਸ਼>(), "Should be var vertex");
                ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(ਕਿਨਾਰਾ.ਸਿਖਰ());
            }
        }
    }
}

}  // namespace

LogicRegions ਹਿੱਸੇ_ਕਰੋ(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ਅਚਲ std::unique_ptr<V3_ਗ੍ਰਾਫ> graphp
        = Sched_ਗ੍ਰਾਫ_Builder::build(clockedLogic, combinationalLogic, hybridLogic);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    color_ਸਰਗਰਮ_Region(*(graphp.get()));
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", ਸੱਚ);

    LogicRegions ਨਤੀਜਾ;

    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ lvtxp = vtx.cast<Sched_Logic_ਸ਼ੀਰਸ਼>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            ਤਾਰਕਿਕ ਮੰਜ਼ਿਲ_Act = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            ਜੇ (v3Global.ਵਰਤਦਾ_Zero_Delay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) ਮੰਜ਼ਿਲ_Act = ਸੱਚ;
            LogicByScope& lbs = ਮੰਜ਼ਿਲ_Act ? ਨਤੀਜਾ.m_act : ਨਤੀਜਾ.m_nba;
            Ast_ਗੰਢ* ਅਚਲ logicp = lvtxp->logicp();
            logicp->unlink_Fr_ਪਿੱਠ();
            lbs.ਜੋੜੋ(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;  // AstVarScope::user1() -> bool: read in Active region
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;  // AstVarScope::user2() -> bool: written in Active region

        ਅਚਲ ਸਵੈਚਾਲਿਤ markVars = [](Ast_ਗੰਢ* nodep) {
            nodep->foreach([](ਅਚਲ Ast_ਗੰਢ_Var_Ref* vrefp) {
                AstVarScope* ਅਚਲ vscp = vrefp->varScopep();
                ਜੇ (vrefp->access().isReadOrRW()) vscp->user1(ਸੱਚ);
                ਜੇ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(ਸੱਚ);
            });
        };

        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : ਨਤੀਜਾ.m_act) {
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : clockedLogic) {
            AstScope* ਅਚਲ scopep = ਇੰਦਰਾਜ.m_scopep;
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
            ਲਈ (Ast_ਗੰਢ *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ਜੇ (AstAlwaysPre* ਅਚਲ logicp = VN_CAST(nodep, AlwaysPre)) {
                    ਤਾਰਕਿਕ ਮੰਜ਼ਿਲ_ਸਰਗਰਮ_Region = ਝੂਠ;
                    logicp->foreach([&](ਅਚਲ Ast_ਗੰਢ_Var_Ref* vrefp) {
                        AstVarScope* ਅਚਲ vscp = vrefp->varScopep();
                        ਜੇ (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            ਜੇ (vscp->user2()) ਮੰਜ਼ਿਲ_ਸਰਗਰਮ_Region = ਸੱਚ;
                        } ਨਹੀਂਤਾਂ {
                            // Variable written in Pre, and referenced in active region
                            ਜੇ (vscp->user1() || vscp->user2()) ਮੰਜ਼ਿਲ_ਸਰਗਰਮ_Region = ਸੱਚ;
                        }
                    });
                    LogicByScope& lbs = ਮੰਜ਼ਿਲ_ਸਰਗਰਮ_Region ? ਨਤੀਜਾ.m_pre : ਨਤੀਜਾ.m_nba;
                    logicp->unlink_Fr_ਪਿੱਠ();
                    lbs.ਜੋੜੋ(scopep, activep->sentreep(), logicp);
                } ਨਹੀਂਤਾਂ {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlink_Fr_ਪਿੱਠ();
                    ਨਤੀਜਾ.m_nba.ਜੋੜੋ(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    ਵਾਪਸ ਨਤੀਜਾ;
}

}  // namespace V3Sched
