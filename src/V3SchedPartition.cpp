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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Sched.h"

#அந்தர்பாவ <tuple>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {

நாமாகாஶ {

வகுப்பு SchedSenVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    மாறிலி AstSenItem* மாறிலி m_senItemp;

பொது:
    SchedSenVertex(V3Graph* graphp, மாறிலி AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() மாறிலி override {
        std::ostringstream os;
        V3EmitV::verilog_For_மரம்(const_cast<AstSenItem*>(m_senItemp), os);
        திரும்பு os.str();
    }
    string dotShape() மாறிலி override { திரும்பு "doubleoctagon"; }
    string dotColor() மாறிலி override { திரும்பு "red"; }
    // LCOV_EXCL_STOP
};

வகுப்பு SchedLogicVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* மாறிலி m_scopep;
    Ast_Sen_மரம்* மாறிலி m_senTreep;
    Ast_முனை* மாறிலி m_logicp;

பொது:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_மரம்* senTreep, Ast_முனை* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() மாறிலி { திரும்பு m_scopep; }
    Ast_Sen_மரம்* senTreep() மாறிலி { திரும்பு m_senTreep; }
    Ast_முனை* logicp() மாறிலி { திரும்பு m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() மாறிலி override VL_MT_STABLE {
        திரும்பு m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() மாறிலி override { திரும்பு "rectangle"; }
    // LCOV_EXCL_STOP
};

வகுப்பு SchedVarVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    மாறிலி AstVarScope* மாறிலி m_vscp;

பொது:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() மாறிலி { திரும்பு m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_vscp->name(); }
    string dotShape() மாறிலி override {
        திரும்பு m_vscp->scopep()->is_உச்சி() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() மாறிலி override {
        திரும்பு m_vscp->scopep()->is_உச்சி() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

வகுப்பு SchedGraphBuilder final : பொது VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    மாறிலி VNUser1InUse m_user1InUse;
    மாறிலி VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* மாறிலி m_graphp = புதிய V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_மரம்* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<தருக்க(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* மாறிலி m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) மாறிலி {
        எனில் (!vscp->user1p()) {
            SchedVarVertex* மாறிலி vtxp = புதிய SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            எனில் (vscp->varp()->isWrittenByDpi()) {
                புதிய V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        திரும்பு vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        எனில் (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            மாறிலி auto pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            எனில் (pair.second) {
                // Create the vertex
                SchedSenVertex* மாறிலி vtxp = புதிய SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                எனில் (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        புதிய V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        திரும்பு senItemp->user1u().to<SchedSenVertex*>();
    }

    வெறுமை visitLogic(Ast_முனை* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* மாறிலி logicVtxp
            = புதிய SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        எனில் (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([இது, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* மாறிலி eventVtxp = getSenVertex(senItemp);
                புதிய V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([இது, logicVtxp, &forceReadEdgeIgnores](மாறிலி AstVarRef* vrefp) {
            AstVarScope* மாறிலி vscp = vrefp->varScopep();
            எனில் (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.மொத்தம்(vscp)) {
                புதிய V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            எனில் (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                புதிய V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        எனில் (m_dpiExportTriggerp) {
            nodep->foreach([இது, logicVtxp](மாறிலி AstCCall* callp) {
                எனில் (!callp->funcp()->dpiImportWrapper()) திரும்பு;
                எனில் (!callp->funcp()->dpiContext()) திரும்பு;
                புதிய V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    வெறுமை visit(AstActive* nodep) override {
        Ast_Sen_மரம்* மாறிலி senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        எனில் (senTreep->hasHybrid()) {
            Ast_முனை::user2_Clear_மரம்();
            senTreep->foreach([](மாறிலி AstVarRef* refp) {  //
                refp->varScopep()->user2(உண்மை);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(Ast_முனை_Procedure* nodep) override { visitLogic(nodep); }
    வெறுமை visit(Ast_முனை_Assign* nodep) override { visitLogic(nodep); }
    வெறுமை visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    வெறுமை visit(AstAlwaysPre* nodep) override {}
    வெறுமை visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    வெறுமை visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    வெறுமை visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    வெறுமை visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    வெறுமை visit(Ast_முனை* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(மாறிலி LogicByScope& clockedLogic, மாறிலி LogicByScope& combinationalLogic,
                      மாறிலி LogicByScope& hybridLogic) {
        // Build the data flow graph
        மாறிலி auto iter = [இது](மாறிலி LogicByScope& lbs) {
            க்கு (மாறிலி auto& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { திரும்பு பொய்; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { திரும்பு உண்மை; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { திரும்பு !vscp->user2(); };
        iter(hybridLogic);
    }

பொது:
    // Build the dataflow graph for partitioning
    நிலையான std::unique_ptr<V3Graph> build(மாறிலி LogicByScope& clockedLogic,
                                          மாறிலி LogicByScope& combinationalLogic,
                                          மாறிலி LogicByScope& hybridLogic) {
        மாறிலி SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        திரும்பு std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

வெறுமை colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    க்கு (V3GraphVertex& vtx : graph.vertices()) {
        எனில் (மாறிலி auto activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.தள்ளு_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    போது (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.எடு_back();
        // If not first encounter, move on
        எனில் (vtx.color() != 0) தொடர்;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        க்கு (V3GraphEdge& edge : vtx.inEdges()) queue.தள்ளு_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        எனில் (SchedVarVertex* மாறிலி vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* மாறிலி varp = vvtxp->varp();
            எனில் (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        எனில் (vtx.is<SchedLogicVertex>()) {
            க்கு (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.உச்சி()->is<SchedVarVertex>(), "Should be var vertex");
                queue.தள்ளு_back(edge.உச்சி());
            }
        }
    }
}

}  // namespace

LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    மாறிலி std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", உண்மை);

    LogicRegions result;

    க்கு (V3GraphVertex& vtx : graphp->vertices()) {
        எனில் (மாறிலி auto lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            தருக்க toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            எனில் (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = உண்மை;
            LogicByScope& lbs = toAct ? result.m_act : result.m_nba;
            Ast_முனை* மாறிலி logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        மாறிலி VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        மாறிலி VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        மாறிலி auto markVars = [](Ast_முனை* nodep) {
            nodep->foreach([](மாறிலி Ast_முனை_Var_Ref* vrefp) {
                AstVarScope* மாறிலி vscp = vrefp->varScopep();
                எனில் (vrefp->access().isReadOrRW()) vscp->user1(உண்மை);
                எனில் (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(உண்மை);
            });
        };

        க்கு (மாறிலி auto& entry : result.m_act) {
            AstActive* மாறிலி activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        க்கு (மாறிலி auto& entry : clockedLogic) {
            AstScope* மாறிலி scopep = entry.m_scopep;
            AstActive* மாறிலி activep = entry.m_activep;
            க்கு (Ast_முனை *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                எனில் (AstAlwaysPre* மாறிலி logicp = VN_CAST(nodep, AlwaysPre)) {
                    தருக்க toActiveRegion = பொய்;
                    logicp->foreach([&](மாறிலி Ast_முனை_Var_Ref* vrefp) {
                        AstVarScope* மாறிலி vscp = vrefp->varScopep();
                        எனில் (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            எனில் (vscp->user2()) toActiveRegion = உண்மை;
                        } இல்லையெனில் {
                            // Variable written in Pre, and referenced in active region
                            எனில் (vscp->user1() || vscp->user2()) toActiveRegion = உண்மை;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? result.m_pre : result.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } இல்லையெனில் {
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

    திரும்பு result;
}

}  // namespace V3Sched
