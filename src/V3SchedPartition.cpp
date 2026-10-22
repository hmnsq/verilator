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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव <tuple>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

नामाकाश {

वर्ग SchedSenVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    ध्रुव AstSenItem* ध्रुव m_senItemp;

सार्वजनिक:
    SchedSenVertex(V3Graph* graphp, ध्रुव AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ध्रुव override {
        std::ostringstream os;
        V3EmitV::verilog_For_वृक्ष(const_cast<AstSenItem*>(m_senItemp), os);
        प्रत्यागम os.str();
    }
    string dotShape() ध्रुव override { प्रत्यागम "doubleoctagon"; }
    string dotColor() ध्रुव override { प्रत्यागम "red"; }
    // LCOV_EXCL_STOP
};

वर्ग SchedLogicVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* ध्रुव m_scopep;
    Ast_Sen_वृक्ष* ध्रुव m_senTreep;
    Ast_गाँठ* ध्रुव m_logicp;

सार्वजनिक:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_वृक्ष* senTreep, Ast_गाँठ* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }
    Ast_Sen_वृक्ष* senTreep() ध्रुव { प्रत्यागम m_senTreep; }
    Ast_गाँठ* logicp() ध्रुव { प्रत्यागम m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ध्रुव override VL_MT_STABLE {
        प्रत्यागम m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ध्रुव override { प्रत्यागम "rectangle"; }
    // LCOV_EXCL_STOP
};

वर्ग SchedVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    ध्रुव AstVarScope* ध्रुव m_vscp;

सार्वजनिक:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ध्रुव { प्रत्यागम m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_vscp->name(); }
    string dotShape() ध्रुव override {
        प्रत्यागम m_vscp->scopep()->is_शीर्ष() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ध्रुव override {
        प्रत्यागम m_vscp->scopep()->is_शीर्ष() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

वर्ग SchedGraphBuilder final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ध्रुव VNUser1InUse m_user1InUse;
    ध्रुव VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* ध्रुव m_graphp = नूतन V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_वृक्ष* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<तार्किक(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ध्रुव m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) ध्रुव {
        यदि (!vscp->user1p()) {
            SchedVarVertex* ध्रुव vtxp = नूतन SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            यदि (vscp->varp()->isWrittenByDpi()) {
                नूतन V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        प्रत्यागम vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        यदि (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ध्रुव स्वतः pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            यदि (pair.second) {
                // Create the vertex
                SchedSenVertex* ध्रुव vtxp = नूतन SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                यदि (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        नूतन V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        प्रत्यागम senItemp->user1u().to<SchedSenVertex*>();
    }

    शून्य visitLogic(Ast_गाँठ* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* ध्रुव logicVtxp
            = नूतन SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        यदि (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([एतत्, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* ध्रुव eventVtxp = getSenVertex(senItemp);
                नूतन V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([एतत्, logicVtxp, &forceReadEdgeIgnores](ध्रुव AstVarRef* vrefp) {
            AstVarScope* ध्रुव vscp = vrefp->varScopep();
            यदि (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.गिनती(vscp)) {
                नूतन V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            यदि (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                नूतन V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        यदि (m_dpiExportTriggerp) {
            nodep->foreach([एतत्, logicVtxp](ध्रुव AstCCall* callp) {
                यदि (!callp->funcp()->dpiImportWrapper()) प्रत्यागम;
                यदि (!callp->funcp()->dpiContext()) प्रत्यागम;
                नूतन V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    शून्य visit(AstActive* nodep) override {
        Ast_Sen_वृक्ष* ध्रुव senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        यदि (senTreep->hasHybrid()) {
            Ast_गाँठ::user2_Clear_वृक्ष();
            senTreep->foreach([](ध्रुव AstVarRef* refp) {  //
                refp->varScopep()->user2(सत्य);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठ_Procedure* nodep) override { visitLogic(nodep); }
    शून्य visit(Ast_गाँठ_Assign* nodep) override { visitLogic(nodep); }
    शून्य visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    शून्य visit(AstAlwaysPre* nodep) override {}
    शून्य visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    शून्य visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    शून्य visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    शून्य visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    शून्य visit(Ast_गाँठ* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(ध्रुव LogicByScope& clockedLogic, ध्रुव LogicByScope& combinationalLogic,
                      ध्रुव LogicByScope& hybridLogic) {
        // Build the data flow graph
        ध्रुव स्वतः iter = [एतत्](ध्रुव LogicByScope& lbs) {
            प्रति (ध्रुव स्वतः& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { प्रत्यागम असत्य; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { प्रत्यागम सत्य; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { प्रत्यागम !vscp->user2(); };
        iter(hybridLogic);
    }

सार्वजनिक:
    // Build the dataflow graph for partitioning
    स्थिर std::unique_ptr<V3Graph> build(ध्रुव LogicByScope& clockedLogic,
                                          ध्रुव LogicByScope& combinationalLogic,
                                          ध्रुव LogicByScope& hybridLogic) {
        ध्रुव SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        प्रत्यागम std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

शून्य colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    प्रति (V3GraphVertex& vtx : graph.vertices()) {
        यदि (ध्रुव स्वतः activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.धकेलो_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    यावत् (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.निकालो_back();
        // If not first encounter, move on
        यदि (vtx.color() != 0) अनुवर्त;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        प्रति (V3GraphEdge& edge : vtx.inEdges()) queue.धकेलो_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        यदि (SchedVarVertex* ध्रुव vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* ध्रुव varp = vvtxp->varp();
            यदि (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        यदि (vtx.is<SchedLogicVertex>()) {
            प्रति (V3GraphEdge& edge : vtx.outEdges()) {
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
    ध्रुव std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", सत्य);

    LogicRegions result;

    प्रति (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (ध्रुव स्वतः lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            तार्किक toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            यदि (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = सत्य;
            LogicByScope& lbs = toAct ? result.m_act : result.m_nba;
            Ast_गाँठ* ध्रुव logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ध्रुव VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        ध्रुव VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        ध्रुव स्वतः markVars = [](Ast_गाँठ* nodep) {
            nodep->foreach([](ध्रुव Ast_गाँठ_Var_Ref* vrefp) {
                AstVarScope* ध्रुव vscp = vrefp->varScopep();
                यदि (vrefp->access().isReadOrRW()) vscp->user1(सत्य);
                यदि (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(सत्य);
            });
        };

        प्रति (ध्रुव स्वतः& entry : result.m_act) {
            AstActive* ध्रुव activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        प्रति (ध्रुव स्वतः& entry : clockedLogic) {
            AstScope* ध्रुव scopep = entry.m_scopep;
            AstActive* ध्रुव activep = entry.m_activep;
            प्रति (Ast_गाँठ *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                यदि (AstAlwaysPre* ध्रुव logicp = VN_CAST(nodep, AlwaysPre)) {
                    तार्किक toActiveRegion = असत्य;
                    logicp->foreach([&](ध्रुव Ast_गाँठ_Var_Ref* vrefp) {
                        AstVarScope* ध्रुव vscp = vrefp->varScopep();
                        यदि (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            यदि (vscp->user2()) toActiveRegion = सत्य;
                        } अन्यथा {
                            // Variable written in Pre, and referenced in active region
                            यदि (vscp->user1() || vscp->user2()) toActiveRegion = सत्य;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? result.m_pre : result.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } अन्यथा {
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

    प्रत्यागम result;
}

}  // namespace V3Sched
