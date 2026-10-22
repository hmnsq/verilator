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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Sched.h"

#অন্তর্ভাব <tuple>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

নামাকাশ {

শ্রেণী SchedSenVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    ধ্রুবক AstSenItem* ধ্রুবক m_senItemp;

সর্বজনীন:
    SchedSenVertex(V3Graph* graphp, ধ্রুবক AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ধ্রুবক override {
        std::ostringstream os;
        V3EmitV::verilog_For_বৃক্ষ(const_cast<AstSenItem*>(m_senItemp), os);
        ফেরত os.str();
    }
    string dotShape() ধ্রুবক override { ফেরত "doubleoctagon"; }
    string dotColor() ধ্রুবক override { ফেরত "red"; }
    // LCOV_EXCL_STOP
};

শ্রেণী SchedLogicVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* ধ্রুবক m_scopep;
    Ast_Sen_বৃক্ষ* ধ্রুবক m_senTreep;
    Ast_গ্রন্থি* ধ্রুবক m_logicp;

সর্বজনীন:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_বৃক্ষ* senTreep, Ast_গ্রন্থি* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ধ্রুবক { ফেরত m_scopep; }
    Ast_Sen_বৃক্ষ* senTreep() ধ্রুবক { ফেরত m_senTreep; }
    Ast_গ্রন্থি* logicp() ধ্রুবক { ফেরত m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ধ্রুবক override { ফেরত "rectangle"; }
    // LCOV_EXCL_STOP
};

শ্রেণী SchedVarVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    ধ্রুবক AstVarScope* ধ্রুবক m_vscp;

সর্বজনীন:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ধ্রুবক { ফেরত m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_vscp->name(); }
    string dotShape() ধ্রুবক override {
        ফেরত m_vscp->scopep()->is_শীর্ষ() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ধ্রুবক override {
        ফেরত m_vscp->scopep()->is_শীর্ষ() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

শ্রেণী SchedGraphBuilder final : সর্বজনীন VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ধ্রুবক VNUser1InUse m_user1InUse;
    ধ্রুবক VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* ধ্রুবক m_graphp = নতুন V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_বৃক্ষ* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<যৌক্তিক(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ধ্রুবক m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) ধ্রুবক {
        যদি (!vscp->user1p()) {
            SchedVarVertex* ধ্রুবক vtxp = নতুন SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            যদি (vscp->varp()->isWrittenByDpi()) {
                নতুন V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        ফেরত vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        যদি (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ধ্রুবক auto pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            যদি (pair.second) {
                // Create the vertex
                SchedSenVertex* ধ্রুবক vtxp = নতুন SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                যদি (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        নতুন V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        ফেরত senItemp->user1u().to<SchedSenVertex*>();
    }

    শূন্য visitLogic(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* ধ্রুবক logicVtxp
            = নতুন SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        যদি (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([এই, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* ধ্রুবক eventVtxp = getSenVertex(senItemp);
                নতুন V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([এই, logicVtxp, &forceReadEdgeIgnores](ধ্রুবক AstVarRef* vrefp) {
            AstVarScope* ধ্রুবক vscp = vrefp->varScopep();
            যদি (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.গুনতি(vscp)) {
                নতুন V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            যদি (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                নতুন V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        যদি (m_dpiExportTriggerp) {
            nodep->foreach([এই, logicVtxp](ধ্রুবক AstCCall* callp) {
                যদি (!callp->funcp()->dpiImportWrapper()) ফেরত;
                যদি (!callp->funcp()->dpiContext()) ফেরত;
                নতুন V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    শূন্য visit(AstActive* nodep) override {
        Ast_Sen_বৃক্ষ* ধ্রুবক senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        যদি (senTreep->hasHybrid()) {
            Ast_গ্রন্থি::user2_Clear_বৃক্ষ();
            senTreep->foreach([](ধ্রুবক AstVarRef* refp) {  //
                refp->varScopep()->user2(সত্য);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override { visitLogic(nodep); }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override { visitLogic(nodep); }
    শূন্য visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    শূন্য visit(AstAlwaysPre* nodep) override {}
    শূন্য visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    শূন্য visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    শূন্য visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    শূন্য visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    শূন্য visit(Ast_গ্রন্থি* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(ধ্রুবক LogicByScope& clockedLogic, ধ্রুবক LogicByScope& combinationalLogic,
                      ধ্রুবক LogicByScope& hybridLogic) {
        // Build the data flow graph
        ধ্রুবক auto iter = [এই](ধ্রুবক LogicByScope& lbs) {
            জন্য (ধ্রুবক auto& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ফেরত মিথ্যা; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ফেরত সত্য; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { ফেরত !vscp->user2(); };
        iter(hybridLogic);
    }

সর্বজনীন:
    // Build the dataflow graph for partitioning
    স্থির std::unique_ptr<V3Graph> build(ধ্রুবক LogicByScope& clockedLogic,
                                          ধ্রুবক LogicByScope& combinationalLogic,
                                          ধ্রুবক LogicByScope& hybridLogic) {
        ধ্রুবক SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        ফেরত std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

শূন্য colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    জন্য (V3GraphVertex& vtx : graph.vertices()) {
        যদি (ধ্রুবক auto activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.ঠেলো_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    যখন (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.বের_করো_back();
        // If not first encounter, move on
        যদি (vtx.color() != 0) চালু;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        জন্য (V3GraphEdge& edge : vtx.inEdges()) queue.ঠেলো_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        যদি (SchedVarVertex* ধ্রুবক vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* ধ্রুবক varp = vvtxp->varp();
            যদি (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        যদি (vtx.is<SchedLogicVertex>()) {
            জন্য (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.শীর্ষ()->is<SchedVarVertex>(), "Should be var vertex");
                queue.ঠেলো_back(edge.শীর্ষ());
            }
        }
    }
}

}  // namespace

LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ধ্রুবক std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", সত্য);

    LogicRegions ফলাফল;

    জন্য (V3GraphVertex& vtx : graphp->vertices()) {
        যদি (ধ্রুবক auto lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            যৌক্তিক toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            যদি (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = সত্য;
            LogicByScope& lbs = toAct ? ফলাফল.m_act : ফলাফল.m_nba;
            Ast_গ্রন্থি* ধ্রুবক logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ধ্রুবক VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        ধ্রুবক VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        ধ্রুবক auto markVars = [](Ast_গ্রন্থি* nodep) {
            nodep->foreach([](ধ্রুবক Ast_গ্রন্থি_Var_Ref* vrefp) {
                AstVarScope* ধ্রুবক vscp = vrefp->varScopep();
                যদি (vrefp->access().isReadOrRW()) vscp->user1(সত্য);
                যদি (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(সত্য);
            });
        };

        জন্য (ধ্রুবক auto& entry : ফলাফল.m_act) {
            AstActive* ধ্রুবক activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        জন্য (ধ্রুবক auto& entry : clockedLogic) {
            AstScope* ধ্রুবক scopep = entry.m_scopep;
            AstActive* ধ্রুবক activep = entry.m_activep;
            জন্য (Ast_গ্রন্থি *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                যদি (AstAlwaysPre* ধ্রুবক logicp = VN_CAST(nodep, AlwaysPre)) {
                    যৌক্তিক toActiveRegion = মিথ্যা;
                    logicp->foreach([&](ধ্রুবক Ast_গ্রন্থি_Var_Ref* vrefp) {
                        AstVarScope* ধ্রুবক vscp = vrefp->varScopep();
                        যদি (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            যদি (vscp->user2()) toActiveRegion = সত্য;
                        } অন্যথা {
                            // Variable written in Pre, and referenced in active region
                            যদি (vscp->user1() || vscp->user2()) toActiveRegion = সত্য;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? ফলাফল.m_pre : ফলাফল.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } অন্যথা {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlinkFrBack();
                    ফলাফল.m_nba.add(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    ফেরত ফলাফল;
}

}  // namespace V3Sched
