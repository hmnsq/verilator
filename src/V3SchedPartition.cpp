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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3EmitV.h"
#समावेश "V3Graph.h"
#समावेश "V3Sched.h"

#समावेश <tuple>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

श्रेणी SchedSenVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    स्थिर AstSenItem* स्थिर m_senItemp;

खुला:
    SchedSenVertex(V3Graph* graphp, स्थिर AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() स्थिर override {
        std::ostringstream os;
        V3EmitV::verilog_For_वृक्ष(const_cast<AstSenItem*>(m_senItemp), os);
        वापस os.str();
    }
    string dotShape() स्थिर override { वापस "doubleoctagon"; }
    string dotColor() स्थिर override { वापस "red"; }
    // LCOV_EXCL_STOP
};

श्रेणी SchedLogicVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* स्थिर m_scopep;
    Ast_Sen_वृक्ष* स्थिर m_senTreep;
    Ast_गाँठ* स्थिर m_logicp;

खुला:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_वृक्ष* senTreep, Ast_गाँठ* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() स्थिर { वापस m_scopep; }
    Ast_Sen_वृक्ष* senTreep() स्थिर { वापस m_senTreep; }
    Ast_गाँठ* logicp() स्थिर { वापस m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() स्थिर override VL_MT_STABLE {
        वापस m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() स्थिर override { वापस "rectangle"; }
    // LCOV_EXCL_STOP
};

श्रेणी SchedVarVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    स्थिर AstVarScope* स्थिर m_vscp;

खुला:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() स्थिर { वापस m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() स्थिर override VL_MT_STABLE { वापस m_vscp->name(); }
    string dotShape() स्थिर override {
        वापस m_vscp->scopep()->is_शीर्ष() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() स्थिर override {
        वापस m_vscp->scopep()->is_शीर्ष() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

श्रेणी SchedGraphBuilder final : खुला VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    स्थिर VNUser1InUse m_user1InUse;
    स्थिर VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* स्थिर m_graphp = नया V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_वृक्ष* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<bool(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* स्थिर m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) स्थिर {
        अगर (!vscp->user1p()) {
            SchedVarVertex* स्थिर vtxp = नया SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            अगर (vscp->varp()->isWrittenByDpi()) {
                नया V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        वापस vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        अगर (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            स्थिर स्वतः pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            अगर (pair.second) {
                // Create the vertex
                SchedSenVertex* स्थिर vtxp = नया SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                अगर (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        नया V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        वापस senItemp->user1u().to<SchedSenVertex*>();
    }

    व्योम visitLogic(Ast_गाँठ* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* स्थिर logicVtxp
            = नया SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        अगर (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([यह, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* स्थिर eventVtxp = getSenVertex(senItemp);
                नया V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([यह, logicVtxp, &forceReadEdgeIgnores](स्थिर AstVarRef* vrefp) {
            AstVarScope* स्थिर vscp = vrefp->varScopep();
            अगर (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.गिनती(vscp)) {
                नया V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            अगर (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                नया V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        अगर (m_dpiExportTriggerp) {
            nodep->foreach([यह, logicVtxp](स्थिर AstCCall* callp) {
                अगर (!callp->funcp()->dpiImportWrapper()) वापस;
                अगर (!callp->funcp()->dpiContext()) वापस;
                नया V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    व्योम visit(AstActive* nodep) override {
        Ast_Sen_वृक्ष* स्थिर senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        अगर (senTreep->hasHybrid()) {
            Ast_गाँठ::user2_Clear_वृक्ष();
            senTreep->foreach([](स्थिर AstVarRef* refp) {  //
                refp->varScopep()->user2(true);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ_Procedure* nodep) override { visitLogic(nodep); }
    व्योम visit(Ast_गाँठ_Assign* nodep) override { visitLogic(nodep); }
    व्योम visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    व्योम visit(AstAlwaysPre* nodep) override {}
    व्योम visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    व्योम visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    व्योम visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    व्योम visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    व्योम visit(Ast_गाँठ* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(स्थिर LogicByScope& clockedLogic, स्थिर LogicByScope& combinationalLogic,
                      स्थिर LogicByScope& hybridLogic) {
        // Build the data flow graph
        स्थिर स्वतः iter = [यह](स्थिर LogicByScope& lbs) {
            क्रम (स्थिर स्वतः& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { वापस false; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { वापस true; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { वापस !vscp->user2(); };
        iter(hybridLogic);
    }

खुला:
    // Build the dataflow graph for partitioning
    अटल std::unique_ptr<V3Graph> build(स्थिर LogicByScope& clockedLogic,
                                          स्थिर LogicByScope& combinationalLogic,
                                          स्थिर LogicByScope& hybridLogic) {
        स्थिर SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        वापस std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

व्योम colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    क्रम (V3GraphVertex& vtx : graph.vertices()) {
        अगर (स्थिर स्वतः activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.धकेलो_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    जबतक (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.निकालो_back();
        // If not first encounter, move on
        अगर (vtx.color() != 0) जारी;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        क्रम (V3GraphEdge& edge : vtx.inEdges()) queue.धकेलो_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        अगर (SchedVarVertex* स्थिर vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* स्थिर varp = vvtxp->varp();
            अगर (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        अगर (vtx.is<SchedLogicVertex>()) {
            क्रम (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.शीर्ष()->is<SchedVarVertex>(), "Should be var vertex");
                queue.धकेलो_back(edge.शीर्ष());
            }
        }
    }
}

}  // namespace

LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    स्थिर std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.लो()));
    अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", true);

    LogicRegions परिणाम;

    क्रम (V3GraphVertex& vtx : graphp->vertices()) {
        अगर (स्थिर स्वतः lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            bool toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            अगर (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = true;
            LogicByScope& lbs = toAct ? परिणाम.m_act : परिणाम.m_nba;
            Ast_गाँठ* स्थिर logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        स्थिर VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        स्थिर VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        स्थिर स्वतः markVars = [](Ast_गाँठ* nodep) {
            nodep->foreach([](स्थिर Ast_गाँठ_Var_Ref* vrefp) {
                AstVarScope* स्थिर vscp = vrefp->varScopep();
                अगर (vrefp->access().isReadOrRW()) vscp->user1(true);
                अगर (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(true);
            });
        };

        क्रम (स्थिर स्वतः& entry : परिणाम.m_act) {
            AstActive* स्थिर activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        क्रम (स्थिर स्वतः& entry : clockedLogic) {
            AstScope* स्थिर scopep = entry.m_scopep;
            AstActive* स्थिर activep = entry.m_activep;
            क्रम (Ast_गाँठ *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                अगर (AstAlwaysPre* स्थिर logicp = VN_CAST(nodep, AlwaysPre)) {
                    bool toActiveRegion = false;
                    logicp->foreach([&](स्थिर Ast_गाँठ_Var_Ref* vrefp) {
                        AstVarScope* स्थिर vscp = vrefp->varScopep();
                        अगर (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            अगर (vscp->user2()) toActiveRegion = true;
                        } अन्यथा {
                            // Variable written in Pre, and referenced in active region
                            अगर (vscp->user1() || vscp->user2()) toActiveRegion = true;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? परिणाम.m_pre : परिणाम.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } अन्यथा {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlinkFrBack();
                    परिणाम.m_nba.add(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    वापस परिणाम;
}

}  // namespace V3Sched
