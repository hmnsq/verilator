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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Sched.h"

#అన్తర్భావ <tuple>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {

నామాకాశ {

తరగతి Sched_Sen_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Sched_Sen_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    స్థిరరాశి Ast_Sen_అంశం* స్థిరరాశి m_senItemp;

బహిరంగ:
    Sched_Sen_శీర్షం(V3_లేఖాచిత్రం* graphp, స్థిరరాశి Ast_Sen_అంశం* senItemp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() స్థిరరాశి override {
        std::ostringstream os;
        V3EmitV::verilog_For_వృక్షం(const_cast<Ast_Sen_అంశం*>(m_senItemp), os);
        తిరిగి os.str();
    }
    string dotShape() స్థిరరాశి override { తిరిగి "doubleoctagon"; }
    string dotColor() స్థిరరాశి override { తిరిగి "red"; }
    // LCOV_EXCL_STOP
};

తరగతి Sched_Logic_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Sched_Logic_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    AstScope* స్థిరరాశి m_scopep;
    Ast_Sen_వృక్షం* స్థిరరాశి m_senTreep;
    Ast_కణుపు* స్థిరరాశి m_logicp;

బహిరంగ:
    Sched_Logic_శీర్షం(V3_లేఖాచిత్రం* graphp, AstScope* scopep, Ast_Sen_వృక్షం* senTreep, Ast_కణుపు* logicp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() స్థిరరాశి { తిరిగి m_scopep; }
    Ast_Sen_వృక్షం* senTreep() స్థిరరాశి { తిరిగి m_senTreep; }
    Ast_కణుపు* logicp() స్థిరరాశి { తిరిగి m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() స్థిరరాశి override { తిరిగి "rectangle"; }
    // LCOV_EXCL_STOP
};

తరగతి Sched_Var_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Sched_Var_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    స్థిరరాశి AstVarScope* స్థిరరాశి m_vscp;

బహిరంగ:
    Sched_Var_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* vscp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() స్థిరరాశి { తిరిగి m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_vscp->name(); }
    string dotShape() స్థిరరాశి override {
        తిరిగి m_vscp->scopep()->is_శిఖరం() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() స్థిరరాశి override {
        తిరిగి m_vscp->scopep()->is_శిఖరం() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

తరగతి Sched_లేఖాచిత్రం_Builder final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;
    స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;

    // STATE
    V3_లేఖాచిత్రం* స్థిరరాశి m_graphp = కొత్త V3_లేఖాచిత్రం;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<Ast_Sen_అంశం>, Sched_Sen_శీర్షం*> m_senVertices;
    AstScope* m_scopep = శూన్యసూచిక;  // AstScope of the current AstActive
    Ast_Sen_వృక్షం* m_senTreep = శూన్యసూచిక;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<తార్కిక(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* స్థిరరాశి m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    Sched_Var_శీర్షం* get_Var_శీర్షం(AstVarScope* vscp) స్థిరరాశి {
        ఒకవేళ (!vscp->user1p()) {
            Sched_Var_శీర్షం* స్థిరరాశి vtxp = కొత్త Sched_Var_శీర్షం{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            ఒకవేళ (vscp->varp()->isWrittenByDpi()) {
                కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp, get_Var_శీర్షం(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        తిరిగి vscp->user1u().గమ్యం<Sched_Var_శీర్షం*>();
    }

    Sched_Sen_శీర్షం* get_Sen_శీర్షం(Ast_Sen_అంశం* senItemp) {
        ఒకవేళ (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            స్థిరరాశి స్వయంచాలక pair = m_senVertices.emplace(*senItemp, శూన్యసూచిక);
            // If it does not exist, create it
            ఒకవేళ (pair.రెండవ) {
                // Create the vertex
                Sched_Sen_శీర్షం* స్థిరరాశి vtxp = కొత్త Sched_Sen_శీర్షం{m_graphp, senItemp};

                // Connect up the variable references
                ఒకవేళ (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp, get_Var_శీర్షం(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.మొదటి->రెండవ = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.మొదటి->రెండవ);
        }
        తిరిగి senItemp->user1u().గమ్యం<Sched_Sen_శీర్షం*>();
    }

    శూన్య సందర్శించు_Logic(Ast_కణుపు* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        Sched_Logic_శీర్షం* స్థిరరాశి logicVtxp
            = కొత్త Sched_Logic_శీర్షం{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        ఒకవేళ (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([ఇది, nodep, logicVtxp](Ast_Sen_అంశం* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి eventVtxp = get_Sen_శీర్షం(senItemp);
                కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::Var_Scope_సమితి force_Read_అంచు_Ignores;
        V3Sched::util::collect_Force_Read_అంచు_Ignores(nodep, force_Read_అంచు_Ignores);
        nodep->foreach([ఇది, logicVtxp, &force_Read_అంచు_Ignores](స్థిరరాశి AstVarRef* vrefp) {
            AstVarScope* స్థిరరాశి vscp = vrefp->varScopep();
            ఒకవేళ (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !force_Read_అంచు_Ignores.లెక్క(vscp)) {
                కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp, get_Var_శీర్షం(vscp), logicVtxp, 10};
            }
            ఒకవేళ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp, logicVtxp, get_Var_శీర్షం(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        ఒకవేళ (m_dpiExportTriggerp) {
            nodep->foreach([ఇది, logicVtxp](స్థిరరాశి AstCCall* callp) {
                ఒకవేళ (!callp->funcp()->dpiImportWrapper()) తిరిగి;
                ఒకవేళ (!callp->funcp()->dpiContext()) తిరిగి;
                కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp, logicVtxp, get_Var_శీర్షం(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        Ast_Sen_వృక్షం* స్థిరరాశి senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        ఒకవేళ (senTreep->hasHybrid()) {
            Ast_కణుపు::user2_Clear_వృక్షం();
            senTreep->foreach([](స్థిరరాశి AstVarRef* refp) {  //
                refp->varScopep()->user2(నిజం);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override { సందర్శించు_Logic(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override { సందర్శించు_Logic(nodep); }
    శూన్య సందర్శించు(AstCoverToggle* nodep) override { సందర్శించు_Logic(nodep); }

    // Pre and Post logic are handled separately
    శూన్య సందర్శించు(AstAlwaysPre* nodep) override {}
    శూన్య సందర్శించు(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    శూన్య సందర్శించు(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    శూన్య సందర్శించు(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    శూన్య సందర్శించు(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    Sched_లేఖాచిత్రం_Builder(స్థిరరాశి LogicByScope& clockedLogic, స్థిరరాశి LogicByScope& combinationalLogic,
                      స్థిరరాశి LogicByScope& hybridLogic) {
        // Build the data flow graph
        స్థిరరాశి స్వయంచాలక iter = [ఇది](స్థిరరాశి LogicByScope& lbs) {
            కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : lbs) {
                m_scopep = నమోదు_2.m_scopep;
                iterate(నమోదు_2.m_activep);
                m_scopep = శూన్యసూచిక;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { తిరిగి అబద్ధం; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { తిరిగి నిజం; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { తిరిగి !vscp->user2(); };
        iter(hybridLogic);
    }

బహిరంగ:
    // Build the dataflow graph for partitioning
    స్థిర std::unique_ptr<V3_లేఖాచిత్రం> build(స్థిరరాశి LogicByScope& clockedLogic,
                                          స్థిరరాశి LogicByScope& combinationalLogic,
                                          స్థిరరాశి LogicByScope& hybridLogic) {
        స్థిరరాశి Sched_లేఖాచిత్రం_Builder visitor{clockedLogic, combinationalLogic, hybridLogic};
        తిరిగి std::unique_ptr<V3_లేఖాచిత్రం>{visitor.m_graphp};
    }
};

శూన్య color_చురుకైన_Region(V3_లేఖాచిత్రం& లేఖాచిత్రం) {
    // Work queue for depth first traversal
    std::సదిశ<V3_లేఖాచిత్రం_శీర్షం*> వరుస{};

    // Trace from all SchedSenVertex
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : లేఖాచిత్రం.vertices()) {
        ఒకవేళ (స్థిరరాశి స్వయంచాలక చురుకైన_Event_Vtxp = vtx.cast<Sched_Sen_శీర్షం>()) {
            వరుస.నెట్టు_వెనుక(చురుకైన_Event_Vtxp);
        }
    }

    // Depth first traversal
    అయితే (!వరుస.ఖాళీలు()) {
        // Pop next work item
        V3_లేఖాచిత్రం_శీర్షం& vtx = *వరుస.వెనుక();
        వరుస.తీయి_వెనుక();
        // If not first encounter, move on
        ఒకవేళ (vtx.color() != 0) కొనసాగు;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtx.లోపల_అంచులు()) వరుస.నెట్టు_వెనుక(అంచు.fromp());

        // Mark top level ports that drive a sensitivity list
        ఒకవేళ (Sched_Var_శీర్షం* స్థిరరాశి vvtxp = vtx.cast<Sched_Var_శీర్షం>()) {
            AstVar* స్థిరరాశి varp = vvtxp->varp();
            ఒకవేళ (varp->isPrimaryIO()) varp->సమితి_Primary_Clock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        ఒకవేళ (vtx.is<Sched_Logic_శీర్షం>()) {
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtx.బహిర్గతం_అంచులు()) {
                UASSERT(అంచు.శిఖరం()->is<Sched_Var_శీర్షం>(), "Should be var vertex");
                వరుస.నెట్టు_వెనుక(అంచు.శిఖరం());
            }
        }
    }
}

}  // namespace

LogicRegions విభజన(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    స్థిరరాశి std::unique_ptr<V3_లేఖాచిత్రం> graphp
        = Sched_లేఖాచిత్రం_Builder::build(clockedLogic, combinationalLogic, hybridLogic);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    color_చురుకైన_Region(*(graphp.get()));
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", నిజం);

    LogicRegions ఫలితం;

    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
        ఒకవేళ (స్థిరరాశి స్వయంచాలక lvtxp = vtx.cast<Sched_Logic_శీర్షం>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            తార్కిక గమ్యం_Act = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            ఒకవేళ (v3Global.ఉపయోగిస్తుంది_Zero_Delay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) గమ్యం_Act = నిజం;
            LogicByScope& lbs = గమ్యం_Act ? ఫలితం.m_act : ఫలితం.m_nba;
            Ast_కణుపు* స్థిరరాశి logicp = lvtxp->logicp();
            logicp->unlink_Fr_వెనుక();
            lbs.కూడు(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;  // AstVarScope::user1() -> bool: read in Active region
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;  // AstVarScope::user2() -> bool: written in Active region

        స్థిరరాశి స్వయంచాలక markVars = [](Ast_కణుపు* nodep) {
            nodep->foreach([](స్థిరరాశి Ast_కణుపు_Var_Ref* vrefp) {
                AstVarScope* స్థిరరాశి vscp = vrefp->varScopep();
                ఒకవేళ (vrefp->access().isReadOrRW()) vscp->user1(నిజం);
                ఒకవేళ (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(నిజం);
            });
        };

        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : ఫలితం.m_act) {
            Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : clockedLogic) {
            AstScope* స్థిరరాశి scopep = నమోదు_2.m_scopep;
            Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
            కొరకు (Ast_కణుపు *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                ఒకవేళ (AstAlwaysPre* స్థిరరాశి logicp = VN_CAST(nodep, AlwaysPre)) {
                    తార్కిక గమ్యం_చురుకైన_Region = అబద్ధం;
                    logicp->foreach([&](స్థిరరాశి Ast_కణుపు_Var_Ref* vrefp) {
                        AstVarScope* స్థిరరాశి vscp = vrefp->varScopep();
                        ఒకవేళ (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            ఒకవేళ (vscp->user2()) గమ్యం_చురుకైన_Region = నిజం;
                        } లేకపోతే {
                            // Variable written in Pre, and referenced in active region
                            ఒకవేళ (vscp->user1() || vscp->user2()) గమ్యం_చురుకైన_Region = నిజం;
                        }
                    });
                    LogicByScope& lbs = గమ్యం_చురుకైన_Region ? ఫలితం.m_pre : ఫలితం.m_nba;
                    logicp->unlink_Fr_వెనుక();
                    lbs.కూడు(scopep, activep->sentreep(), logicp);
                } లేకపోతే {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlink_Fr_వెనుక();
                    ఫలితం.m_nba.కూడు(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    తిరిగి ఫలితం;
}

}  // namespace V3Sched
