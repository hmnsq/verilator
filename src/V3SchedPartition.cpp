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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3EmitV.h"
#شامل "V3Graph.h"
#شامل "V3Sched.h"

#شامل <tuple>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام V3Sched {

فضاینام {

رده SchedSenVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    ثابت AstSenItem* ثابت m_senItemp;

عمومی:
    SchedSenVertex(V3Graph* graphp, ثابت AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ثابت override {
        std::ostringstream os;
        V3EmitV::verilog_For_درخت(const_cast<AstSenItem*>(m_senItemp), os);
        بازگشت os.str();
    }
    string dotShape() ثابت override { بازگشت "doubleoctagon"; }
    string dotColor() ثابت override { بازگشت "red"; }
    // LCOV_EXCL_STOP
};

رده SchedLogicVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* ثابت m_scopep;
    Ast_Sen_درخت* ثابت m_senTreep;
    Ast_گره* ثابت m_logicp;

عمومی:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_درخت* senTreep, Ast_گره* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ثابت { بازگشت m_scopep; }
    Ast_Sen_درخت* senTreep() ثابت { بازگشت m_senTreep; }
    Ast_گره* logicp() ثابت { بازگشت m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE {
        بازگشت m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ثابت override { بازگشت "rectangle"; }
    // LCOV_EXCL_STOP
};

رده SchedVarVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    ثابت AstVarScope* ثابت m_vscp;

عمومی:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ثابت { بازگشت m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { بازگشت m_vscp->name(); }
    string dotShape() ثابت override {
        بازگشت m_vscp->scopep()->is_بالاترین() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ثابت override {
        بازگشت m_vscp->scopep()->is_بالاترین() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

رده SchedGraphBuilder final : عمومی VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* ثابت m_graphp = جدید V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_درخت* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<منطقی(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ثابت m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) ثابت {
        اگر (!vscp->user1p()) {
            SchedVarVertex* ثابت vtxp = جدید SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            اگر (vscp->varp()->isWrittenByDpi()) {
                جدید V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        بازگشت vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        اگر (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ثابت auto pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            اگر (pair.second) {
                // Create the vertex
                SchedSenVertex* ثابت vtxp = جدید SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                اگر (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        جدید V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        بازگشت senItemp->user1u().to<SchedSenVertex*>();
    }

    تهی visitLogic(Ast_گره* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* ثابت logicVtxp
            = جدید SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        اگر (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([این, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* ثابت eventVtxp = getSenVertex(senItemp);
                جدید V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([این, logicVtxp, &forceReadEdgeIgnores](ثابت AstVarRef* vrefp) {
            AstVarScope* ثابت vscp = vrefp->varScopep();
            اگر (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.شمار(vscp)) {
                جدید V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            اگر (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                جدید V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        اگر (m_dpiExportTriggerp) {
            nodep->foreach([این, logicVtxp](ثابت AstCCall* callp) {
                اگر (!callp->funcp()->dpiImportWrapper()) بازگشت;
                اگر (!callp->funcp()->dpiContext()) بازگشت;
                جدید V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    تهی visit(AstActive* nodep) override {
        Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        اگر (senTreep->hasHybrid()) {
            Ast_گره::user2_Clear_درخت();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user2(درست);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    تهی visit(Ast_گره_Procedure* nodep) override { visitLogic(nodep); }
    تهی visit(Ast_گره_Assign* nodep) override { visitLogic(nodep); }
    تهی visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    تهی visit(AstAlwaysPre* nodep) override {}
    تهی visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    تهی visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    تهی visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    تهی visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    تهی visit(Ast_گره* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(ثابت LogicByScope& clockedLogic, ثابت LogicByScope& combinationalLogic,
                      ثابت LogicByScope& hybridLogic) {
        // Build the data flow graph
        ثابت auto iter = [این](ثابت LogicByScope& lbs) {
            برای (ثابت auto& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { بازگشت نادرست; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { بازگشت درست; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { بازگشت !vscp->user2(); };
        iter(hybridLogic);
    }

عمومی:
    // Build the dataflow graph for partitioning
    ایستا std::unique_ptr<V3Graph> build(ثابت LogicByScope& clockedLogic,
                                          ثابت LogicByScope& combinationalLogic,
                                          ثابت LogicByScope& hybridLogic) {
        ثابت SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        بازگشت std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

تهی colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    برای (V3GraphVertex& vtx : graph.vertices()) {
        اگر (ثابت auto activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.افزودن_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    وقتی (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.برداشت_back();
        // If not first encounter, move on
        اگر (vtx.color() != 0) ادامه;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        برای (V3GraphEdge& edge : vtx.inEdges()) queue.افزودن_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        اگر (SchedVarVertex* ثابت vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* ثابت varp = vvtxp->varp();
            اگر (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        اگر (vtx.is<SchedLogicVertex>()) {
            برای (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.بالاترین()->is<SchedVarVertex>(), "Should be var vertex");
                queue.افزودن_back(edge.بالاترین());
            }
        }
    }
}

}  // namespace

LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ثابت std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", درست);

    LogicRegions result;

    برای (V3GraphVertex& vtx : graphp->vertices()) {
        اگر (ثابت auto lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            منطقی toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            اگر (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = درست;
            LogicByScope& lbs = toAct ? result.m_act : result.m_nba;
            Ast_گره* ثابت logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ثابت VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        ثابت VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        ثابت auto markVars = [](Ast_گره* nodep) {
            nodep->foreach([](ثابت Ast_گره_Var_Ref* vrefp) {
                AstVarScope* ثابت vscp = vrefp->varScopep();
                اگر (vrefp->access().isReadOrRW()) vscp->user1(درست);
                اگر (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(درست);
            });
        };

        برای (ثابت auto& entry : result.m_act) {
            AstActive* ثابت activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        برای (ثابت auto& entry : clockedLogic) {
            AstScope* ثابت scopep = entry.m_scopep;
            AstActive* ثابت activep = entry.m_activep;
            برای (Ast_گره *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                اگر (AstAlwaysPre* ثابت logicp = VN_CAST(nodep, AlwaysPre)) {
                    منطقی toActiveRegion = نادرست;
                    logicp->foreach([&](ثابت Ast_گره_Var_Ref* vrefp) {
                        AstVarScope* ثابت vscp = vrefp->varScopep();
                        اگر (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            اگر (vscp->user2()) toActiveRegion = درست;
                        } وگرنه {
                            // Variable written in Pre, and referenced in active region
                            اگر (vscp->user1() || vscp->user2()) toActiveRegion = درست;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? result.m_pre : result.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } وگرنه {
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

    بازگشت result;
}

}  // namespace V3Sched
