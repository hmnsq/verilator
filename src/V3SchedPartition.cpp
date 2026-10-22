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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Sched.h"

#അന്തര്ഭാവ <tuple>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ V3Sched {

നാമാകാശ {

വർഗ്ഗം SchedSenVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(SchedSenVertex, V3GraphVertex)
    സ്ഥിരം AstSenItem* സ്ഥിരം m_senItemp;

പൊതു:
    SchedSenVertex(V3Graph* graphp, സ്ഥിരം AstSenItem* senItemp)
        : V3GraphVertex{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() സ്ഥിരം override {
        std::ostringstream os;
        V3EmitV::verilog_For_വൃക്ഷം(const_cast<AstSenItem*>(m_senItemp), os);
        തിരികെ os.str();
    }
    string dotShape() സ്ഥിരം override { തിരികെ "doubleoctagon"; }
    string dotColor() സ്ഥിരം override { തിരികെ "red"; }
    // LCOV_EXCL_STOP
};

വർഗ്ഗം SchedLogicVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(SchedLogicVertex, V3GraphVertex)
    AstScope* സ്ഥിരം m_scopep;
    Ast_Sen_വൃക്ഷം* സ്ഥിരം m_senTreep;
    Ast_കണ്ണി* സ്ഥിരം m_logicp;

പൊതു:
    SchedLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_വൃക്ഷം* senTreep, Ast_കണ്ണി* logicp)
        : V3GraphVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() സ്ഥിരം { തിരികെ m_scopep; }
    Ast_Sen_വൃക്ഷം* senTreep() സ്ഥിരം { തിരികെ m_senTreep; }
    Ast_കണ്ണി* logicp() സ്ഥിരം { തിരികെ m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() സ്ഥിരം override VL_MT_STABLE {
        തിരികെ m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() സ്ഥിരം override { തിരികെ "rectangle"; }
    // LCOV_EXCL_STOP
};

വർഗ്ഗം SchedVarVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(SchedVarVertex, V3GraphVertex)
    സ്ഥിരം AstVarScope* സ്ഥിരം m_vscp;

പൊതു:
    SchedVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() സ്ഥിരം { തിരികെ m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_vscp->name(); }
    string dotShape() സ്ഥിരം override {
        തിരികെ m_vscp->scopep()->is_മുകൾ() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() സ്ഥിരം override {
        തിരികെ m_vscp->scopep()->is_മുകൾ() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

വർഗ്ഗം SchedGraphBuilder final : പൊതു VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    സ്ഥിരം VNUser1InUse m_user1InUse;
    സ്ഥിരം VNUser2InUse m_user2InUse;

    // STATE
    V3Graph* സ്ഥിരം m_graphp = പുതിയ V3Graph;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<AstSenItem>, SchedSenVertex*> m_senVertices;
    AstScope* m_scopep = nullptr;  // AstScope of the current AstActive
    Ast_Sen_വൃക്ഷം* m_senTreep = nullptr;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<തർക്കം(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* സ്ഥിരം m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    SchedVarVertex* getVarVertex(AstVarScope* vscp) സ്ഥിരം {
        എങ്കിൽ (!vscp->user1p()) {
            SchedVarVertex* സ്ഥിരം vtxp = പുതിയ SchedVarVertex{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            എങ്കിൽ (vscp->varp()->isWrittenByDpi()) {
                പുതിയ V3GraphEdge{m_graphp, getVarVertex(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        തിരികെ vscp->user1u().to<SchedVarVertex*>();
    }

    SchedSenVertex* getSenVertex(AstSenItem* senItemp) {
        എങ്കിൽ (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            സ്ഥിരം auto pair = m_senVertices.emplace(*senItemp, nullptr);
            // If it does not exist, create it
            എങ്കിൽ (pair.second) {
                // Create the vertex
                SchedSenVertex* സ്ഥിരം vtxp = പുതിയ SchedSenVertex{m_graphp, senItemp};

                // Connect up the variable references
                എങ്കിൽ (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        പുതിയ V3GraphEdge{m_graphp, getVarVertex(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.first->second = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.first->second);
        }
        തിരികെ senItemp->user1u().to<SchedSenVertex*>();
    }

    ശൂന്യം visitLogic(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        SchedLogicVertex* സ്ഥിരം logicVtxp
            = പുതിയ SchedLogicVertex{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        എങ്കിൽ (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([ഇത്, nodep, logicVtxp](AstSenItem* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3GraphVertex* സ്ഥിരം eventVtxp = getSenVertex(senItemp);
                പുതിയ V3GraphEdge{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::VarScopeSet forceReadEdgeIgnores;
        V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);
        nodep->foreach([ഇത്, logicVtxp, &forceReadEdgeIgnores](സ്ഥിരം AstVarRef* vrefp) {
            AstVarScope* സ്ഥിരം vscp = vrefp->varScopep();
            എങ്കിൽ (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !forceReadEdgeIgnores.എണ്ണം(vscp)) {
                പുതിയ V3GraphEdge{m_graphp, getVarVertex(vscp), logicVtxp, 10};
            }
            എങ്കിൽ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                പുതിയ V3GraphEdge{m_graphp, logicVtxp, getVarVertex(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        എങ്കിൽ (m_dpiExportTriggerp) {
            nodep->foreach([ഇത്, logicVtxp](സ്ഥിരം AstCCall* callp) {
                എങ്കിൽ (!callp->funcp()->dpiImportWrapper()) തിരികെ;
                എങ്കിൽ (!callp->funcp()->dpiContext()) തിരികെ;
                പുതിയ V3GraphEdge{m_graphp, logicVtxp, getVarVertex(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    ശൂന്യം visit(AstActive* nodep) override {
        Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        എങ്കിൽ (senTreep->hasHybrid()) {
            Ast_കണ്ണി::user2_Clear_വൃക്ഷം();
            senTreep->foreach([](സ്ഥിരം AstVarRef* refp) {  //
                refp->varScopep()->user2(സത്യം);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override { visitLogic(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override { visitLogic(nodep); }
    ശൂന്യം visit(AstCoverToggle* nodep) override { visitLogic(nodep); }

    // Pre and Post logic are handled separately
    ശൂന്യം visit(AstAlwaysPre* nodep) override {}
    ശൂന്യം visit(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    ശൂന്യം visit(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    ശൂന്യം visit(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    ശൂന്യം visit(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    SchedGraphBuilder(സ്ഥിരം LogicByScope& clockedLogic, സ്ഥിരം LogicByScope& combinationalLogic,
                      സ്ഥിരം LogicByScope& hybridLogic) {
        // Build the data flow graph
        സ്ഥിരം auto iter = [ഇത്](സ്ഥിരം LogicByScope& lbs) {
            വേണ്ടി (സ്ഥിരം auto& entry : lbs) {
                m_scopep = entry.m_scopep;
                iterate(entry.m_activep);
                m_scopep = nullptr;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { തിരികെ അസത്യം; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { തിരികെ സത്യം; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { തിരികെ !vscp->user2(); };
        iter(hybridLogic);
    }

പൊതു:
    // Build the dataflow graph for partitioning
    സ്ഥിതം std::unique_ptr<V3Graph> build(സ്ഥിരം LogicByScope& clockedLogic,
                                          സ്ഥിരം LogicByScope& combinationalLogic,
                                          സ്ഥിരം LogicByScope& hybridLogic) {
        സ്ഥിരം SchedGraphBuilder visitor{clockedLogic, combinationalLogic, hybridLogic};
        തിരികെ std::unique_ptr<V3Graph>{visitor.m_graphp};
    }
};

ശൂന്യം colorActiveRegion(V3Graph& graph) {
    // Work queue for depth first traversal
    std::vector<V3GraphVertex*> queue{};

    // Trace from all SchedSenVertex
    വേണ്ടി (V3GraphVertex& vtx : graph.vertices()) {
        എങ്കിൽ (സ്ഥിരം auto activeEventVtxp = vtx.cast<SchedSenVertex>()) {
            queue.തള്ളുക_back(activeEventVtxp);
        }
    }

    // Depth first traversal
    ആകുമ്പോൾ (!queue.empty()) {
        // Pop next work item
        V3GraphVertex& vtx = *queue.back();
        queue.എടുക്കുക_back();
        // If not first encounter, move on
        എങ്കിൽ (vtx.color() != 0) തുടരുക;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        വേണ്ടി (V3GraphEdge& edge : vtx.inEdges()) queue.തള്ളുക_back(edge.fromp());

        // Mark top level ports that drive a sensitivity list
        എങ്കിൽ (SchedVarVertex* സ്ഥിരം vvtxp = vtx.cast<SchedVarVertex>()) {
            AstVar* സ്ഥിരം varp = vvtxp->varp();
            എങ്കിൽ (varp->isPrimaryIO()) varp->setPrimaryClock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        എങ്കിൽ (vtx.is<SchedLogicVertex>()) {
            വേണ്ടി (V3GraphEdge& edge : vtx.outEdges()) {
                UASSERT(edge.മുകൾ()->is<SchedVarVertex>(), "Should be var vertex");
                queue.തള്ളുക_back(edge.മുകൾ());
            }
        }
    }
}

}  // namespace

LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    സ്ഥിരം std::unique_ptr<V3Graph> graphp
        = SchedGraphBuilder::build(clockedLogic, combinationalLogic, hybridLogic);
    എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    colorActiveRegion(*(graphp.get()));
    എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", സത്യം);

    LogicRegions result;

    വേണ്ടി (V3GraphVertex& vtx : graphp->vertices()) {
        എങ്കിൽ (സ്ഥിരം auto lvtxp = vtx.cast<SchedLogicVertex>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            തർക്കം toAct = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            എങ്കിൽ (v3Global.usesZeroDelay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) toAct = സത്യം;
            LogicByScope& lbs = toAct ? result.m_act : result.m_nba;
            Ast_കണ്ണി* സ്ഥിരം logicp = lvtxp->logicp();
            logicp->unlinkFrBack();
            lbs.add(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        സ്ഥിരം VNUser1InUse user1InUse;  // AstVarScope::user1() -> bool: read in Active region
        സ്ഥിരം VNUser2InUse user2InUse;  // AstVarScope::user2() -> bool: written in Active region

        സ്ഥിരം auto markVars = [](Ast_കണ്ണി* nodep) {
            nodep->foreach([](സ്ഥിരം Ast_കണ്ണി_Var_Ref* vrefp) {
                AstVarScope* സ്ഥിരം vscp = vrefp->varScopep();
                എങ്കിൽ (vrefp->access().isReadOrRW()) vscp->user1(സത്യം);
                എങ്കിൽ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(സത്യം);
            });
        };

        വേണ്ടി (സ്ഥിരം auto& entry : result.m_act) {
            AstActive* സ്ഥിരം activep = entry.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        വേണ്ടി (സ്ഥിരം auto& entry : clockedLogic) {
            AstScope* സ്ഥിരം scopep = entry.m_scopep;
            AstActive* സ്ഥിരം activep = entry.m_activep;
            വേണ്ടി (Ast_കണ്ണി *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                എങ്കിൽ (AstAlwaysPre* സ്ഥിരം logicp = VN_CAST(nodep, AlwaysPre)) {
                    തർക്കം toActiveRegion = അസത്യം;
                    logicp->foreach([&](സ്ഥിരം Ast_കണ്ണി_Var_Ref* vrefp) {
                        AstVarScope* സ്ഥിരം vscp = vrefp->varScopep();
                        എങ്കിൽ (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            എങ്കിൽ (vscp->user2()) toActiveRegion = സത്യം;
                        } അല്ലെങ്കിൽ {
                            // Variable written in Pre, and referenced in active region
                            എങ്കിൽ (vscp->user1() || vscp->user2()) toActiveRegion = സത്യം;
                        }
                    });
                    LogicByScope& lbs = toActiveRegion ? result.m_pre : result.m_nba;
                    logicp->unlinkFrBack();
                    lbs.add(scopep, activep->sentreep(), logicp);
                } അല്ലെങ്കിൽ {
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

    തിരികെ result;
}

}  // namespace V3Sched
