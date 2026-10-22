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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Sched.h"

#અન્તર્ભાવ <tuple>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {

નામાકાશ {

વર્ગ SchedSenVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    અચલ AstSenItem* અચલ m_senItemp;

જાહેર:
    SchedSenVertex(V3Graph* graphp, અચલ AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() અચલ override {
        std::ostringstream os;
        V3EmitV::verilog_For_વૃક્ષ(const_cast<AstSenItem*>(m_senItemp), os);
        પાછા os.str();
    }
    string dotShape() અચલ override { પાછા "doubleoctagon"; }
    string dotColor() અચલ override { પાછા "red"; }
    // LCOV_EXCL_STOP
};

વર્ગ SchedLogicVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* અચલ m_scopep;
    Ast_Sen_વૃક્ષ* અચલ m_senTreep;
    Ast_ગાંઠ* અચલ m_logicp;

જાહેર:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_વૃક્ષ* senTreep, Ast_ગાંઠ* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() અચલ { પાછા m_scopep; }
    Ast_Sen_વૃક્ષ* senTreep() અચલ { પાછા m_senTreep; }
    Ast_ગાંઠ* logicp() અચલ { પાછા m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() અચલ override VL_MT_STABLE {
        પાછા m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() અચલ override { પાછા "rectangle"; }
    // LCOV_EXCL_STOP
};

વર્ગ SchedVarVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    અચલ AstVarScope* અચલ m_vscp;

જાહેર:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() અચલ { પાછા m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() અચલ override VL_MT_STABLE { પાછા m_vscp->name(); }
    string dotShape() અચલ override {
        પાછા m_vscp->scopep()->is_ટોચ() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() અચલ override {
        પાછા m_vscp->scopep()->is_ટોચ() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

વર્ગ SchedGraphBuilder final : જાહેર VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    અચલ VNUser1InUse m_user1InUse;
    અચલ VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* અચલ m_graphp = નવું V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_વૃક્ષ* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<તાર્કિક(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* અચલ m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) અચલ {
        જો (!vscp->user1p()) {
            SchedVarVertex* અચલ vtxp = નવું SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            જો (vscp->varp()->isWrittenByDpi()) {
                નવું V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        પાછા vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        જો (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            અચલ auto pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            જો (pair.second) {
                // Create the vertex
                SchedSenVertex* અચલ vtxp = નવું SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                જો (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        નવું V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        પાછા senItemp->user1u().to<SchedSenVertex*>();
    }

    શૂન્ય visitLogic(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* અચલ logicVtxp
            = નવું SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        જો (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([આ, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* અચલ eventVtxp = getSenVertex(senItemp);
                નવું V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([આ, logicVtxp, &forceReadEdgeIgnores](અચલ AstVarRef* vrefp) {
            AstVarScope* અચલ vscp = vrefp->varScopep();
            જો (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.ગણતરી(vscp)) {
                નવું V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            જો (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                નવું V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        જો (m_dpiExportTriggerp) {
            nodep->foreach([આ, logicVtxp](અચલ AstCCall* callp) {
                જો (!callp->funcp()->dpiImportWrapper()) પાછા;
                જો (!callp->funcp()->dpiContext()) પાછા;
                નવું V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    શૂન્ય visit(AstActive* nodep) override {
        Ast_Sen_વૃક્ષ* અચલ senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        જો (senTreep->hasHybrid()) {
            Ast_ગાંઠ::user2_Clear_વૃક્ષ();
            senTreep->foreach([](અચલ AstVarRef* refp) {  //
                refp->varScopep()->user2(સાચું);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override { visitLogic(nodep); }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override { visitLogic(nodep); }
    શૂન્ય visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    શૂન્ય visit(AstAlwaysPre* nodep) override {}
    શૂન્ય visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    શૂન્ય visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    શૂન્ય visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    શૂન્ય visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(અચલ LogicByScope& clockedLogic, અચલ LogicByScope& combinationalLogic,
                      અચલ LogicByScope& hybridLogic) {
        // Build the data flow graph
        અચલ auto iter = [આ](અચલ LogicByScope& lbs) {
            માટે (અચલ auto& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { પાછા ખોટું; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { પાછા સાચું; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { પાછા !vscp->user2(); };
        iter(hybridLogic);
    }

જાહેર:
    // Build the dataflow graph for partitioning
    સ્થિર std::unique_ptr<V3Graph> build(અચલ LogicByScope& clockedLogic,
                                          અચલ LogicByScope& combinationalLogic,
                                          અચલ LogicByScope& hybridLogic) {
        અચલ SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        પાછા std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

શૂન્ય colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    માટે (V3GraphVertex& vtx : graph.vertices()) {
        જો (અચલ auto activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.ધકેલો_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    જ્યારે (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.કાઢો_back();
        // If not first encounter, move on
        જો (vtx.color() != 0) ચાલુ;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        માટે (V3GraphEdge& edge : vtx.inEdges()) queue.ધકેલો_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        જો (SchedVarVertex* અચલ vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* અચલ varp = vvtxp->varp();
            જો (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        જો (vtx.is<SchedLogicVertex>()) {
            માટે (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.ટોચ()->is<SchedVarVertex>(), "Should be var vertex");
                queue.ધકેલો_back(edge.ટોચ());
            }
        }
    }
}

}  // namespace

LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    અચલ std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", સાચું);

    LogicRegions result;

    માટે (V3GraphVertex& vtx : graphp->vertices()) {
        જો (અચલ auto lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            તાર્કિક toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            જો (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = સાચું;
            LogicByScope& lbs = toAct ? result.m_act : result.m_nba;
            Ast_ગાંઠ* અચલ logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        અચલ VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        અચલ VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        અચલ auto markVars = [](Ast_ગાંઠ* nodep) {
            nodep->foreach([](અચલ Ast_ગાંઠ_Var_Ref* vrefp) {
                AstVarScope* અચલ vscp = vrefp->varScopep();
                જો (vrefp->access().isReadOrRW()) vscp->user1(સાચું);
                જો (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(સાચું);
            });
        };

        માટે (અચલ auto& entry : result.m_act) {
            AstActive* અચલ activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        માટે (અચલ auto& entry : clockedLogic) {
            AstScope* અચલ scopep = entry.m_scopep;
            AstActive* અચલ activep = entry.m_activep;
            માટે (Ast_ગાંઠ *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                જો (AstAlwaysPre* અચલ logicp = VN_CAST(nodep, AlwaysPre)) {
                    તાર્કિક toActiveRegion = ખોટું;
                    logicp->foreach([&](અચલ Ast_ગાંઠ_Var_Ref* vrefp) {
                        AstVarScope* અચલ vscp = vrefp->varScopep();
                        જો (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            જો (vscp->user2()) toActiveRegion = સાચું;
                        } નહીંતર {
                            // Variable written in Pre, and referenced in active region
                            જો (vscp->user1() || vscp->user2()) toActiveRegion = સાચું;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? result.m_pre : result.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } નહીંતર {
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

    પાછા result;
}

}  // namespace V3Sched
