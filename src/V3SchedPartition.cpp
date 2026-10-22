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

ವರ್ಗ SchedSenVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    ಧ್ರುವ AstSenItem* ಧ್ರುವ m_senItemp;

ಸಾರ್ವಜನಿಕ:
    SchedSenVertex(V3Graph* graphp, ಧ್ರುವ AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ಧ್ರುವ override {
        std::ostringstream os;
        V3EmitV::verilog_For_ವೃಕ್ಷ(const_cast<AstSenItem*>(m_senItemp), os);
        ಪ್ರತ್ಯಾಗಮ os.str();
    }
    string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "doubleoctagon"; }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "red"; }
    // LCOV_EXCL_STOP
};

ವರ್ಗ SchedLogicVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* ಧ್ರುವ m_scopep;
    Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ m_senTreep;
    Ast_ಗಂಟು* ಧ್ರುವ m_logicp;

ಸಾರ್ವಜನಿಕ:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_ವೃಕ್ಷ* senTreep, Ast_ಗಂಟು* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopep; }
    Ast_Sen_ವೃಕ್ಷ* senTreep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_senTreep; }
    Ast_ಗಂಟು* logicp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "rectangle"; }
    // LCOV_EXCL_STOP
};

ವರ್ಗ SchedVarVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    ಧ್ರುವ AstVarScope* ಧ್ರುವ m_vscp;

ಸಾರ್ವಜನಿಕ:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_vscp->name(); }
    string dotShape() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_vscp->scopep()->is_ತುದಿ() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_vscp->scopep()->is_ತುದಿ() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

ವರ್ಗ SchedGraphBuilder final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ಧ್ರುವ VNUser1InUse m_user1InUse;
    ಧ್ರುವ VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* ಧ್ರುವ m_graphp = ನೂತನ V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_ವೃಕ್ಷ* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<ತಾರ್ಕಿಕ(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ಧ್ರುವ m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) ಧ್ರುವ {
        ಯದಿ (!vscp->user1p()) {
            SchedVarVertex* ಧ್ರುವ vtxp = ನೂತನ SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            ಯದಿ (vscp->varp()->isWrittenByDpi()) {
                ನೂತನ V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        ಪ್ರತ್ಯಾಗಮ vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        ಯದಿ (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ಧ್ರುವ auto pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            ಯದಿ (pair.second) {
                // Create the vertex
                SchedSenVertex* ಧ್ರುವ vtxp = ನೂತನ SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                ಯದಿ (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        ನೂತನ V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        ಪ್ರತ್ಯಾಗಮ senItemp->user1u().to<SchedSenVertex*>();
    }

    ಶೂನ್ಯ visitLogic(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* ಧ್ರುವ logicVtxp
            = ನೂತನ SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        ಯದಿ (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([ಏತತ್, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* ಧ್ರುವ eventVtxp = getSenVertex(senItemp);
                ನೂತನ V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([ಏತತ್, logicVtxp, &forceReadEdgeIgnores](ಧ್ರುವ AstVarRef* vrefp) {
            AstVarScope* ಧ್ರುವ vscp = vrefp->varScopep();
            ಯದಿ (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.ಎಣಿಕೆ(vscp)) {
                ನೂತನ V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            ಯದಿ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                ನೂತನ V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        ಯದಿ (m_dpiExportTriggerp) {
            nodep->foreach([ಏತತ್, logicVtxp](ಧ್ರುವ AstCCall* callp) {
                ಯದಿ (!callp->funcp()->dpiImportWrapper()) ಪ್ರತ್ಯಾಗಮ;
                ಯದಿ (!callp->funcp()->dpiContext()) ಪ್ರತ್ಯಾಗಮ;
                ನೂತನ V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        ಯದಿ (senTreep->hasHybrid()) {
            Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
            senTreep->foreach([](ಧ್ರುವ AstVarRef* refp) {  //
                refp->varScopep()->user2(ಸತ್ಯ);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override { visitLogic(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override { visitLogic(nodep); }
    ಶೂನ್ಯ visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    ಶೂನ್ಯ visit(AstAlwaysPre* nodep) override {}
    ಶೂನ್ಯ visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    ಶೂನ್ಯ visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    ಶೂನ್ಯ visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    ಶೂನ್ಯ visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(ಧ್ರುವ LogicByScope& clockedLogic, ಧ್ರುವ LogicByScope& combinationalLogic,
                      ಧ್ರುವ LogicByScope& hybridLogic) {
        // Build the data flow graph
        ಧ್ರುವ auto iter = [ಏತತ್](ಧ್ರುವ LogicByScope& lbs) {
            ಪ್ರತಿ (ಧ್ರುವ auto& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { ಪ್ರತ್ಯಾಗಮ !vscp->user2(); };
        iter(hybridLogic);
    }

ಸಾರ್ವಜನಿಕ:
    // Build the dataflow graph for partitioning
    ಸ್ಥಿರ std::unique_ptr<V3Graph> build(ಧ್ರುವ LogicByScope& clockedLogic,
                                          ಧ್ರುವ LogicByScope& combinationalLogic,
                                          ಧ್ರುವ LogicByScope& hybridLogic) {
        ಧ್ರುವ SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        ಪ್ರತ್ಯಾಗಮ std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

ಶೂನ್ಯ colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    ಪ್ರತಿ (V3GraphVertex& vtx : graph.vertices()) {
        ಯದಿ (ಧ್ರುವ auto activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.ತಳ್ಳು_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    ಯಾವತ್ (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.ತೆಗೆ_back();
        // If not first encounter, move on
        ಯದಿ (vtx.color() != 0) ಅನುವರ್ತ;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        ಪ್ರತಿ (V3GraphEdge& edge : vtx.inEdges()) queue.ತಳ್ಳು_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        ಯದಿ (SchedVarVertex* ಧ್ರುವ vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* ಧ್ರುವ varp = vvtxp->varp();
            ಯದಿ (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        ಯದಿ (vtx.is<SchedLogicVertex>()) {
            ಪ್ರತಿ (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.ತುದಿ()->is<SchedVarVertex>(), "Should be var vertex");
                queue.ತಳ್ಳು_back(edge.ತುದಿ());
            }
        }
    }
}

}  // namespace

LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ಧ್ರುವ std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", ಸತ್ಯ);

    LogicRegions result;

    ಪ್ರತಿ (V3GraphVertex& vtx : graphp->vertices()) {
        ಯದಿ (ಧ್ರುವ auto lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            ತಾರ್ಕಿಕ toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            ಯದಿ (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = ಸತ್ಯ;
            LogicByScope& lbs = toAct ? result.m_act : result.m_nba;
            Ast_ಗಂಟು* ಧ್ರುವ logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ಧ್ರುವ VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        ಧ್ರುವ VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        ಧ್ರುವ auto markVars = [](Ast_ಗಂಟು* nodep) {
            nodep->foreach([](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* vrefp) {
                AstVarScope* ಧ್ರುವ vscp = vrefp->varScopep();
                ಯದಿ (vrefp->access().isReadOrRW()) vscp->user1(ಸತ್ಯ);
                ಯದಿ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(ಸತ್ಯ);
            });
        };

        ಪ್ರತಿ (ಧ್ರುವ auto& entry : result.m_act) {
            AstActive* ಧ್ರುವ activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        ಪ್ರತಿ (ಧ್ರುವ auto& entry : clockedLogic) {
            AstScope* ಧ್ರುವ scopep = entry.m_scopep;
            AstActive* ಧ್ರುವ activep = entry.m_activep;
            ಪ್ರತಿ (Ast_ಗಂಟು *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ಯದಿ (AstAlwaysPre* ಧ್ರುವ logicp = VN_CAST(nodep, AlwaysPre)) {
                    ತಾರ್ಕಿಕ toActiveRegion = ಅಸತ್ಯ;
                    logicp->foreach([&](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* vrefp) {
                        AstVarScope* ಧ್ರುವ vscp = vrefp->varScopep();
                        ಯದಿ (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            ಯದಿ (vscp->user2()) toActiveRegion = ಸತ್ಯ;
                        } ಅನ್ಯಥಾ {
                            // Variable written in Pre, and referenced in active region
                            ಯದಿ (vscp->user1() || vscp->user2()) toActiveRegion = ಸತ್ಯ;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? result.m_pre : result.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } ಅನ್ಯಥಾ {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlinkFrBack();
                    result.m_nba.add(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    ಪ್ರತ್ಯಾಗಮ result;
}

}  // namespace V3Sched
