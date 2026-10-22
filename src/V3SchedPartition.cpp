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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ <tuple>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ನಾಮಾಕಾಶ {

ವರ್ಗ Sched_Sen_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Sched_Sen_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ m_senItemp;

ಸಾರ್ವಜನಿಕ:
    Sched_Sen_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* senItemp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ಸ್ಥಿರಾಂಕ override {
        std::ostringstream os;
        V3EmitV::verilog_For_ವೃಕ್ಷ(const_cast<Ast_Sen_ಅಂಶ*>(m_senItemp), os);
        ಹಿಂತಿರುಗಿ os.str();
    }
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "doubleoctagon"; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "red"; }
    // LCOV_EXCL_STOP
};

ವರ್ಗ Sched_Logic_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Sched_Logic_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    AstScope* ಸ್ಥಿರಾಂಕ m_scopep;
    Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ m_senTreep;
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_logicp;

ಸಾರ್ವಜನಿಕ:
    Sched_Logic_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstScope* scopep, Ast_Sen_ವೃಕ್ಷ* senTreep, Ast_ಗಂಟು* logicp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopep; }
    Ast_Sen_ವೃಕ್ಷ* senTreep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_senTreep; }
    Ast_ಗಂಟು* logicp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "rectangle"; }
    // LCOV_EXCL_STOP
};

ವರ್ಗ Sched_Var_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Sched_Var_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ m_vscp;

ಸಾರ್ವಜನಿಕ:
    Sched_Var_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* vscp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_vscp->name(); }
    string dotShape() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_vscp->scopep()->is_ತುದಿ() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_vscp->scopep()->is_ತುದಿ() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

ವರ್ಗ Sched_ನಕ್ಷೆ_Builder final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;

    // STATE
    V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_graphp = ಹೊಸ V3_ನಕ್ಷೆ;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<Ast_Sen_ಅಂಶ>, Sched_Sen_ಶೃಂಗ*> m_senVertices;
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // AstScope of the current AstActive
    Ast_Sen_ವೃಕ್ಷ* m_senTreep = ಶೂನ್ಯಸೂಚಕ;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<ತಾರ್ಕಿಕ(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ಸ್ಥಿರಾಂಕ m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    Sched_Var_ಶೃಂಗ* get_Var_ಶೃಂಗ(AstVarScope* vscp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!vscp->user1p()) {
            Sched_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ಹೊಸ Sched_Var_ಶೃಂಗ{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            ಆದರೆ (vscp->varp()->isWrittenByDpi()) {
                ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp, get_Var_ಶೃಂಗ(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        ಹಿಂತಿರುಗಿ vscp->user1u().ಗಮ್ಯ<Sched_Var_ಶೃಂಗ*>();
    }

    Sched_Sen_ಶೃಂಗ* get_Sen_ಶೃಂಗ(Ast_Sen_ಅಂಶ* senItemp) {
        ಆದರೆ (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_senVertices.emplace(*senItemp, ಶೂನ್ಯಸೂಚಕ);
            // If it does not exist, create it
            ಆದರೆ (pair.ಎರಡನೇ) {
                // Create the vertex
                Sched_Sen_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ಹೊಸ Sched_Sen_ಶೃಂಗ{m_graphp, senItemp};

                // Connect up the variable references
                ಆದರೆ (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp, get_Var_ಶೃಂಗ(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.ಮೊದಲ->ಎರಡನೇ = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.ಮೊದಲ->ಎರಡನೇ);
        }
        ಹಿಂತಿರುಗಿ senItemp->user1u().ಗಮ್ಯ<Sched_Sen_ಶೃಂಗ*>();
    }

    ಶೂನ್ಯ ಭೇಟಿ_Logic(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        Sched_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ logicVtxp
            = ಹೊಸ Sched_Logic_ಶೃಂಗ{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        ಆದರೆ (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([ಇದು, nodep, logicVtxp](Ast_Sen_ಅಂಶ* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ eventVtxp = get_Sen_ಶೃಂಗ(senItemp);
                ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::Var_Scope_ಗಣ force_Read_ಅಂಚು_Ignores;
        V3Sched::util::collect_Force_Read_ಅಂಚು_Ignores(nodep, force_Read_ಅಂಚು_Ignores);
        nodep->foreach([ಇದು, logicVtxp, &force_Read_ಅಂಚು_Ignores](ಸ್ಥಿರಾಂಕ AstVarRef* vrefp) {
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vrefp->varScopep();
            ಆದರೆ (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !force_Read_ಅಂಚು_Ignores.ಎಣಿಕೆ_2(vscp)) {
                ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp, get_Var_ಶೃಂಗ(vscp), logicVtxp, 10};
            }
            ಆದರೆ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp, logicVtxp, get_Var_ಶೃಂಗ(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        ಆದರೆ (m_dpiExportTriggerp) {
            nodep->foreach([ಇದು, logicVtxp](ಸ್ಥಿರಾಂಕ AstCCall* callp) {
                ಆದರೆ (!callp->funcp()->dpiImportWrapper()) ಹಿಂತಿರುಗಿ;
                ಆದರೆ (!callp->funcp()->dpiContext()) ಹಿಂತಿರುಗಿ;
                ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp, logicVtxp, get_Var_ಶೃಂಗ(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        ಆದರೆ (senTreep->hasHybrid()) {
            Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
            senTreep->foreach([](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {  //
                refp->varScopep()->user2(ನಿಜ);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override { ಭೇಟಿ_Logic(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override { ಭೇಟಿ_Logic(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggle* nodep) override { ಭೇಟಿ_Logic(nodep); }

    // Pre and Post logic are handled separately
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPre* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    Sched_ನಕ್ಷೆ_Builder(ಸ್ಥಿರಾಂಕ LogicByScope& clockedLogic, ಸ್ಥಿರಾಂಕ LogicByScope& combinationalLogic,
                      ಸ್ಥಿರಾಂಕ LogicByScope& hybridLogic) {
        // Build the data flow graph
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = [ಇದು](ಸ್ಥಿರಾಂಕ LogicByScope& lbs) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : lbs) {
                m_scopep = ನಮೂದು.m_scopep;
                iterate(ನಮೂದು.m_activep);
                m_scopep = ಶೂನ್ಯಸೂಚಕ;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ಹಿಂತಿರುಗಿ ಸುಳ್ಳು; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ಹಿಂತಿರುಗಿ ನಿಜ; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { ಹಿಂತಿರುಗಿ !vscp->user2(); };
        iter(hybridLogic);
    }

ಸಾರ್ವಜನಿಕ:
    // Build the dataflow graph for partitioning
    ಸ್ಥಿರ std::unique_ptr<V3_ನಕ್ಷೆ> build(ಸ್ಥಿರಾಂಕ LogicByScope& clockedLogic,
                                          ಸ್ಥಿರಾಂಕ LogicByScope& combinationalLogic,
                                          ಸ್ಥಿರಾಂಕ LogicByScope& hybridLogic) {
        ಸ್ಥಿರಾಂಕ Sched_ನಕ್ಷೆ_Builder visitor{clockedLogic, combinationalLogic, hybridLogic};
        ಹಿಂತಿರುಗಿ std::unique_ptr<V3_ನಕ್ಷೆ>{visitor.m_graphp};
    }
};

ಶೂನ್ಯ color_ಸಕ್ರಿಯ_Region(V3_ನಕ್ಷೆ& ನಕ್ಷೆ) {
    // Work queue for depth first traversal
    std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಶೃಂಗ*> ಸರತಿ{};

    // Trace from all SchedSenVertex
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : ನಕ್ಷೆ.vertices()) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಸಕ್ರಿಯ_Event_Vtxp = vtx.cast<Sched_Sen_ಶೃಂಗ>()) {
            ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(ಸಕ್ರಿಯ_Event_Vtxp);
        }
    }

    // Depth first traversal
    ಆಗ (!ಸರತಿ.ಖಾಲಿಗಳು()) {
        // Pop next work item
        V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx = *ಸರತಿ.ಹಿಂಭಾಗ();
        ಸರತಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        // If not first encounter, move on
        ಆದರೆ (vtx.color() != 0) ಮುಂದುವರಿ;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtx.ಒಳ_ಅಂಚುಗಳು()) ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(ಅಂಚು.fromp());

        // Mark top level ports that drive a sensitivity list
        ಆದರೆ (Sched_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp = vtx.cast<Sched_Var_ಶೃಂಗ>()) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = vvtxp->varp();
            ಆದರೆ (varp->isPrimaryIO()) varp->ಗಣ_Primary_Clock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        ಆದರೆ (vtx.is<Sched_Logic_ಶೃಂಗ>()) {
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtx.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                UASSERT(ಅಂಚು.ತುದಿ()->is<Sched_Var_ಶೃಂಗ>(), "Should be var vertex");
                ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(ಅಂಚು.ತುದಿ());
            }
        }
    }
}

}  // namespace

LogicRegions ವಿಭಜನೆ(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ಸ್ಥಿರಾಂಕ std::unique_ptr<V3_ನಕ್ಷೆ> graphp
        = Sched_ನಕ್ಷೆ_Builder::build(clockedLogic, combinationalLogic, hybridLogic);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    color_ಸಕ್ರಿಯ_Region(*(graphp.get()));
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", ನಿಜ);

    LogicRegions ಫಲಿತಾಂಶ;

    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ lvtxp = vtx.cast<Sched_Logic_ಶೃಂಗ>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            ತಾರ್ಕಿಕ ಗಮ್ಯ_Act = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            ಆದರೆ (v3Global.ಬಳಸುತ್ತದೆ_Zero_Delay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) ಗಮ್ಯ_Act = ನಿಜ;
            LogicByScope& lbs = ಗಮ್ಯ_Act ? ಫಲಿತಾಂಶ.m_act : ಫಲಿತಾಂಶ.m_nba;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ logicp = lvtxp->logicp();
            logicp->unlink_Fr_ಹಿಂಭಾಗ();
            lbs.ಕೂಡಿಸು(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;  // AstVarScope::user1() -> bool: read in Active region
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;  // AstVarScope::user2() -> bool: written in Active region

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ markVars = [](Ast_ಗಂಟು* nodep) {
            nodep->foreach([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* vrefp) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vrefp->varScopep();
                ಆದರೆ (vrefp->access().isReadOrRW()) vscp->user1(ನಿಜ);
                ಆದರೆ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(ನಿಜ);
            });
        };

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : ಫಲಿತಾಂಶ.m_act) {
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : clockedLogic) {
            AstScope* ಸ್ಥಿರಾಂಕ scopep = ನಮೂದು.m_scopep;
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
            ಗಾಗಿ (Ast_ಗಂಟು *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ಆದರೆ (AstAlwaysPre* ಸ್ಥಿರಾಂಕ logicp = VN_CAST(nodep, AlwaysPre)) {
                    ತಾರ್ಕಿಕ ಗಮ್ಯ_ಸಕ್ರಿಯ_Region = ಸುಳ್ಳು;
                    logicp->foreach([&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* vrefp) {
                        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vrefp->varScopep();
                        ಆದರೆ (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            ಆದರೆ (vscp->user2()) ಗಮ್ಯ_ಸಕ್ರಿಯ_Region = ನಿಜ;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            // Variable written in Pre, and referenced in active region
                            ಆದರೆ (vscp->user1() || vscp->user2()) ಗಮ್ಯ_ಸಕ್ರಿಯ_Region = ನಿಜ;
                        }
                    });
                    LogicByScope& lbs = ಗಮ್ಯ_ಸಕ್ರಿಯ_Region ? ಫಲಿತಾಂಶ.m_pre : ಫಲಿತಾಂಶ.m_nba;
                    logicp->unlink_Fr_ಹಿಂಭಾಗ();
                    lbs.ಕೂಡಿಸು(scopep, activep->sentreep(), logicp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlink_Fr_ಹಿಂಭಾಗ();
                    ಫಲಿತಾಂಶ.m_nba.ಕೂಡಿಸು(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

}  // namespace V3Sched
