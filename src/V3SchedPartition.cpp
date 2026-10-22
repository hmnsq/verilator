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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3EmitV.h"
#ضمن "V3Graph.h"
#ضمن "V3Sched.h"

#ضمن <tuple>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {

نطاق {

صنف SchedSenVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    ثابت AstSenItem* ثابت m_senItemp;

عام:
    SchedSenVertex(V3Graph* graphp, ثابت AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ثابت override {
        std::ostringstream os;
        V3EmitV::verilog_For_ڤوکوق(const_cast<AstSenItem*>(m_senItemp), os);
        أرجع os.str();
    }
    string dotShape() ثابت override { أرجع "doubleoctagon"; }
    string dotColor() ثابت override { أرجع "red"; }
    // LCOV_EXCL_STOP
};

صنف SchedLogicVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* ثابت m_scopep;
    Ast_Sen_ڤوکوق* ثابت m_senTreep;
    Ast_نود* ثابت m_logicp;

عام:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_ڤوکوق* senTreep, Ast_نود* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ثابت { أرجع m_scopep; }
    Ast_Sen_ڤوکوق* senTreep() ثابت { أرجع m_senTreep; }
    Ast_نود* logicp() ثابت { أرجع m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE {
        أرجع m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ثابت override { أرجع "rectangle"; }
    // LCOV_EXCL_STOP
};

صنف SchedVarVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    ثابت AstVarScope* ثابت m_vscp;

عام:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ثابت { أرجع m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { أرجع m_vscp->name(); }
    string dotShape() ثابت override {
        أرجع m_vscp->scopep()->is_اتس() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ثابت override {
        أرجع m_vscp->scopep()->is_اتس() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

صنف SchedGraphBuilder final : عام VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* ثابت m_graphp = جديد V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_ڤوکوق* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<منطقي(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ثابت m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) ثابت {
        إذا (!vscp->user1p()) {
            SchedVarVertex* ثابت vtxp = جديد SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            إذا (vscp->varp()->isWrittenByDpi()) {
                جديد V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        أرجع vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        إذا (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ثابت auto pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            إذا (pair.second) {
                // Create the vertex
                SchedSenVertex* ثابت vtxp = جديد SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                إذا (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        جديد V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        أرجع senItemp->user1u().to<SchedSenVertex*>();
    }

    فراغ visitLogic(Ast_نود* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* ثابت logicVtxp
            = جديد SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        إذا (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([هذا, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* ثابت eventVtxp = getSenVertex(senItemp);
                جديد V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([هذا, logicVtxp, &forceReadEdgeIgnores](ثابت AstVarRef* vrefp) {
            AstVarScope* ثابت vscp = vrefp->varScopep();
            إذا (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.کيراءن(vscp)) {
                جديد V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            إذا (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                جديد V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        إذا (m_dpiExportTriggerp) {
            nodep->foreach([هذا, logicVtxp](ثابت AstCCall* callp) {
                إذا (!callp->funcp()->dpiImportWrapper()) أرجع;
                إذا (!callp->funcp()->dpiContext()) أرجع;
                جديد V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    فراغ visit(AstActive* nodep) override {
        Ast_Sen_ڤوکوق* ثابت senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        إذا (senTreep->hasHybrid()) {
            Ast_نود::user2_Clear_ڤوکوق();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user2(true);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    فراغ visit(Ast_نود_Procedure* nodep) override { visitLogic(nodep); }
    فراغ visit(Ast_نود_Assign* nodep) override { visitLogic(nodep); }
    فراغ visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    فراغ visit(AstAlwaysPre* nodep) override {}
    فراغ visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    فراغ visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    فراغ visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    فراغ visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    فراغ visit(Ast_نود* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(ثابت LogicByScope& clockedLogic, ثابت LogicByScope& combinationalLogic,
                      ثابت LogicByScope& hybridLogic) {
        // Build the data flow graph
        ثابت auto iter = [هذا](ثابت LogicByScope& lbs) {
            لكل (ثابت auto& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { أرجع خطأ; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { أرجع true; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { أرجع !vscp->user2(); };
        iter(hybridLogic);
    }

عام:
    // Build the dataflow graph for partitioning
    ساكن std::unique_ptr<V3Graph> build(ثابت LogicByScope& clockedLogic,
                                          ثابت LogicByScope& combinationalLogic,
                                          ثابت LogicByScope& hybridLogic) {
        ثابت SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        أرجع std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

فراغ colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    لكل (V3GraphVertex& vtx : graph.vertices()) {
        إذا (ثابت auto activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.تولق_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    طالما (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.کلوار_back();
        // If not first encounter, move on
        إذا (vtx.color() != 0) استمر;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        لكل (V3GraphEdge& edge : vtx.inEdges()) queue.تولق_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        إذا (SchedVarVertex* ثابت vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* ثابت varp = vvtxp->varp();
            إذا (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        إذا (vtx.is<SchedLogicVertex>()) {
            لكل (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.اتس()->is<SchedVarVertex>(), "Should be var vertex");
                queue.تولق_back(edge.اتس());
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
    إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", true);

    LogicRegions result;

    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        إذا (ثابت auto lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            منطقي toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            إذا (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = true;
            LogicByScope& lbs = toAct ? result.m_act : result.m_nba;
            Ast_نود* ثابت logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ثابت VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        ثابت VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        ثابت auto markVars = [](Ast_نود* nodep) {
            nodep->foreach([](ثابت Ast_نود_Var_Ref* vrefp) {
                AstVarScope* ثابت vscp = vrefp->varScopep();
                إذا (vrefp->access().isReadOrRW()) vscp->user1(true);
                إذا (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(true);
            });
        };

        لكل (ثابت auto& entry : result.m_act) {
            AstActive* ثابت activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        لكل (ثابت auto& entry : clockedLogic) {
            AstScope* ثابت scopep = entry.m_scopep;
            AstActive* ثابت activep = entry.m_activep;
            لكل (Ast_نود *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                إذا (AstAlwaysPre* ثابت logicp = VN_CAST(nodep, AlwaysPre)) {
                    منطقي toActiveRegion = خطأ;
                    logicp->foreach([&](ثابت Ast_نود_Var_Ref* vrefp) {
                        AstVarScope* ثابت vscp = vrefp->varScopep();
                        إذا (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            إذا (vscp->user2()) toActiveRegion = true;
                        } وإلا {
                            // Variable written in Pre, and referenced in active region
                            إذا (vscp->user1() || vscp->user2()) toActiveRegion = true;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? result.m_pre : result.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } وإلا {
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

    أرجع result;
}

}  // namespace V3Sched
