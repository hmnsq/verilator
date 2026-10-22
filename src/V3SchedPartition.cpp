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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

#ଅନ୍ତର୍ଭାଵ <tuple>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {

ନାମାକାଶ {

ଶ୍ରେଣୀ SchedSenVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    ଅଚଳ AstSenItem* ଅଚଳ m_senItemp;

ସର୍ବସାଧାରଣ:
    SchedSenVertex(V3Graph* graphp, ଅଚଳ AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ଅଚଳ override {
        std::ostringstream os;
        V3EmitV::verilog_For_ବୃକ୍ଷ(const_cast<AstSenItem*>(m_senItemp), os);
        ଫେରନ୍ତୁ os.str();
    }
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "doubleoctagon"; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "red"; }
    // LCOV_EXCL_STOP
};

ଶ୍ରେଣୀ SchedLogicVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* ଅଚଳ m_scopep;
    Ast_Sen_ବୃକ୍ଷ* ଅଚଳ m_senTreep;
    Ast_ଗଣ୍ଠି* ଅଚଳ m_logicp;

ସର୍ବସାଧାରଣ:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_ବୃକ୍ଷ* senTreep, Ast_ଗଣ୍ଠି* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_scopep; }
    Ast_Sen_ବୃକ୍ଷ* senTreep() ଅଚଳ { ଫେରନ୍ତୁ m_senTreep; }
    Ast_ଗଣ୍ଠି* logicp() ଅଚଳ { ଫେରନ୍ତୁ m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "rectangle"; }
    // LCOV_EXCL_STOP
};

ଶ୍ରେଣୀ SchedVarVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    ଅଚଳ AstVarScope* ଅଚଳ m_vscp;

ସର୍ବସାଧାରଣ:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ଅଚଳ { ଫେରନ୍ତୁ m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_vscp->name(); }
    string dotShape() ଅଚଳ override {
        ଫେରନ୍ତୁ m_vscp->scopep()->is_ଶୀର୍ଷ() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ଅଚଳ override {
        ଫେରନ୍ତୁ m_vscp->scopep()->is_ଶୀର୍ଷ() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

ଶ୍ରେଣୀ SchedGraphBuilder final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ଅଚଳ VNUser1InUse m_user1InUse;
    ଅଚଳ VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* ଅଚଳ m_graphp = ନୂତନ V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_ବୃକ୍ଷ* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<ତାର୍କିକ(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ଅଚଳ m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) ଅଚଳ {
        ଯଦି (!vscp->user1p()) {
            SchedVarVertex* ଅଚଳ vtxp = ନୂତନ SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            ଯଦି (vscp->varp()->isWrittenByDpi()) {
                ନୂତନ V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        ଫେରନ୍ତୁ vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        ଯଦି (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ଅଚଳ auto pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            ଯଦି (pair.second) {
                // Create the vertex
                SchedSenVertex* ଅଚଳ vtxp = ନୂତନ SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                ଯଦି (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        ନୂତନ V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        ଫେରନ୍ତୁ senItemp->user1u().to<SchedSenVertex*>();
    }

    ଶୂନ୍ୟ visitLogic(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* ଅଚଳ logicVtxp
            = ନୂତନ SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        ଯଦି (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([ଏହି, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* ଅଚଳ eventVtxp = getSenVertex(senItemp);
                ନୂତନ V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([ଏହି, logicVtxp, &forceReadEdgeIgnores](ଅଚଳ AstVarRef* vrefp) {
            AstVarScope* ଅଚଳ vscp = vrefp->varScopep();
            ଯଦି (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.ଗଣନା_2(vscp)) {
                ନୂତନ V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            ଯଦି (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                ନୂତନ V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        ଯଦି (m_dpiExportTriggerp) {
            nodep->foreach([ଏହି, logicVtxp](ଅଚଳ AstCCall* callp) {
                ଯଦି (!callp->funcp()->dpiImportWrapper()) ଫେରନ୍ତୁ;
                ଯଦି (!callp->funcp()->dpiContext()) ଫେରନ୍ତୁ;
                ନୂତନ V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        ଯଦି (senTreep->hasHybrid()) {
            Ast_ଗଣ୍ଠି::user2_Clear_ବୃକ୍ଷ();
            senTreep->foreach([](ଅଚଳ AstVarRef* refp) {  //
                refp->varScopep()->user2(ସତ୍ୟ);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override { visitLogic(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override { visitLogic(nodep); }
    ଶୂନ୍ୟ visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    ଶୂନ୍ୟ visit(AstAlwaysPre* nodep) override {}
    ଶୂନ୍ୟ visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    ଶୂନ୍ୟ visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    ଶୂନ୍ୟ visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    ଶୂନ୍ୟ visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(ଅଚଳ LogicByScope& clockedLogic, ଅଚଳ LogicByScope& combinationalLogic,
                      ଅଚଳ LogicByScope& hybridLogic) {
        // Build the data flow graph
        ଅଚଳ auto iter = [ଏହି](ଅଚଳ LogicByScope& lbs) {
            ପାଇଁ (ଅଚଳ auto& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ଫେରନ୍ତୁ ମିଥ୍ୟା; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { ଫେରନ୍ତୁ ସତ୍ୟ; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { ଫେରନ୍ତୁ !vscp->user2(); };
        iter(hybridLogic);
    }

ସର୍ବସାଧାରଣ:
    // Build the dataflow graph for partitioning
    ସ୍ଥିର std::unique_ptr<V3Graph> build(ଅଚଳ LogicByScope& clockedLogic,
                                          ଅଚଳ LogicByScope& combinationalLogic,
                                          ଅଚଳ LogicByScope& hybridLogic) {
        ଅଚଳ SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        ଫେରନ୍ତୁ std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

ଶୂନ୍ୟ colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    ପାଇଁ (V3GraphVertex& vtx : graph.vertices()) {
        ଯଦି (ଅଚଳ auto activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.ଠେଲ_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    ଯେତେବେଳେ (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.କାଢ଼_back();
        // If not first encounter, move on
        ଯଦି (vtx.color() != 0) ଜାରି;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        ପାଇଁ (V3GraphEdge& edge : vtx.inEdges()) queue.ଠେଲ_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        ଯଦି (SchedVarVertex* ଅଚଳ vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* ଅଚଳ varp = vvtxp->varp();
            ଯଦି (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        ଯଦି (vtx.is<SchedLogicVertex>()) {
            ପାଇଁ (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.ଶୀର୍ଷ()->is<SchedVarVertex>(), "Should be var vertex");
                queue.ଠେଲ_back(edge.ଶୀର୍ଷ());
            }
        }
    }
}

}  // namespace

LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ଅଚଳ std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", ସତ୍ୟ);

    LogicRegions result;

    ପାଇଁ (V3GraphVertex& vtx : graphp->vertices()) {
        ଯଦି (ଅଚଳ auto lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            ତାର୍କିକ toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            ଯଦି (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = ସତ୍ୟ;
            LogicByScope& lbs = toAct ? result.m_act : result.m_nba;
            Ast_ଗଣ୍ଠି* ଅଚଳ logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ଅଚଳ VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        ଅଚଳ VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        ଅଚଳ auto markVars = [](Ast_ଗଣ୍ଠି* nodep) {
            nodep->foreach([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* vrefp) {
                AstVarScope* ଅଚଳ vscp = vrefp->varScopep();
                ଯଦି (vrefp->access().isReadOrRW()) vscp->user1(ସତ୍ୟ);
                ଯଦି (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(ସତ୍ୟ);
            });
        };

        ପାଇଁ (ଅଚଳ auto& entry : result.m_act) {
            AstActive* ଅଚଳ activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        ପାଇଁ (ଅଚଳ auto& entry : clockedLogic) {
            AstScope* ଅଚଳ scopep = entry.m_scopep;
            AstActive* ଅଚଳ activep = entry.m_activep;
            ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ଯଦି (AstAlwaysPre* ଅଚଳ logicp = VN_CAST(nodep, AlwaysPre)) {
                    ତାର୍କିକ toActiveRegion = ମିଥ୍ୟା;
                    logicp->foreach([&](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* vrefp) {
                        AstVarScope* ଅଚଳ vscp = vrefp->varScopep();
                        ଯଦି (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            ଯଦି (vscp->user2()) toActiveRegion = ସତ୍ୟ;
                        } ଅନ୍ୟଥା {
                            // Variable written in Pre, and referenced in active region
                            ଯଦି (vscp->user1() || vscp->user2()) toActiveRegion = ସତ୍ୟ;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? result.m_pre : result.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } ଅନ୍ୟଥା {
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

    ଫେରନ୍ତୁ result;
}

}  // namespace V3Sched
