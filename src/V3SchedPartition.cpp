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

نامخلا V3Sched {

نامخلا {

جماعت Sched_Sen_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Sen_راس, V3_گراف_راس)
    ثابت Ast_Sen_شے* ثابت m_senItemp;

عوامی:
    Sched_Sen_راس(V3_گراف* graphp, ثابت Ast_Sen_شے* senItemp)
        : V3_گراف_راس{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ثابت override {
        std::ostringstream os;
        V3EmitV::verilog_For_درخت(const_cast<Ast_Sen_شے*>(m_senItemp), os);
        واپس os.str();
    }
    string dotShape() ثابت override { واپس "doubleoctagon"; }
    string dotColor() ثابت override { واپس "red"; }
    // LCOV_EXCL_STOP
};

جماعت Sched_Logic_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Logic_راس, V3_گراف_راس)
    AstScope* ثابت m_scopep;
    Ast_Sen_درخت* ثابت m_senTreep;
    Ast_گرہ* ثابت m_logicp;

عوامی:
    Sched_Logic_راس(V3_گراف* graphp, AstScope* scopep, Ast_Sen_درخت* senTreep, Ast_گرہ* logicp)
        : V3_گراف_راس{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ثابت { واپس m_scopep; }
    Ast_Sen_درخت* senTreep() ثابت { واپس m_senTreep; }
    Ast_گرہ* logicp() ثابت { واپس m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE {
        واپس m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ثابت override { واپس "rectangle"; }
    // LCOV_EXCL_STOP
};

جماعت Sched_Var_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Var_راس, V3_گراف_راس)
    ثابت AstVarScope* ثابت m_vscp;

عوامی:
    Sched_Var_راس(V3_گراف* graphp, AstVarScope* vscp)
        : V3_گراف_راس{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ثابت { واپس m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { واپس m_vscp->name(); }
    string dotShape() ثابت override {
        واپس m_vscp->scopep()->is_چوٹی() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ثابت override {
        واپس m_vscp->scopep()->is_چوٹی() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

جماعت Sched_گراف_Builder final : عوامی VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;
    ثابت VNUser2_اندر_Use m_user2_اندر_Use;

    // STATE
    V3_گراف* ثابت m_graphp = نیا V3_گراف;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<Ast_Sen_شے>, Sched_Sen_راس*> m_senVertices;
    AstScope* m_scopep = خالی_اشاریہ;  // AstScope of the current AstActive
    Ast_Sen_درخت* m_senTreep = خالی_اشاریہ;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<منطقی(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ثابت m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    Sched_Var_راس* get_Var_راس(AstVarScope* vscp) ثابت {
        اگر (!vscp->user1p()) {
            Sched_Var_راس* ثابت vtxp = نیا Sched_Var_راس{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            اگر (vscp->varp()->isWrittenByDpi()) {
                نیا V3_گراف_کنارہ{m_graphp, get_Var_راس(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        واپس vscp->user1u().مقصد<Sched_Var_راس*>();
    }

    Sched_Sen_راس* get_Sen_راس(Ast_Sen_شے* senItemp) {
        اگر (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ثابت خودکار pair = m_senVertices.emplace(*senItemp, خالی_اشاریہ);
            // If it does not exist, create it
            اگر (pair.دوسرا) {
                // Create the vertex
                Sched_Sen_راس* ثابت vtxp = نیا Sched_Sen_راس{m_graphp, senItemp};

                // Connect up the variable references
                اگر (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        نیا V3_گراف_کنارہ{m_graphp, get_Var_راس(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.پہلا->دوسرا = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.پہلا->دوسرا);
        }
        واپس senItemp->user1u().مقصد<Sched_Sen_راس*>();
    }

    خالی دورہ_Logic(Ast_گرہ* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        Sched_Logic_راس* ثابت logicVtxp
            = نیا Sched_Logic_راس{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        اگر (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([یہ, nodep, logicVtxp](Ast_Sen_شے* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3_گراف_راس* ثابت eventVtxp = get_Sen_راس(senItemp);
                نیا V3_گراف_کنارہ{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::Var_Scope_سیٹ force_Read_کنارہ_Ignores;
        V3Sched::util::collect_Force_Read_کنارہ_Ignores(nodep, force_Read_کنارہ_Ignores);
        nodep->foreach([یہ, logicVtxp, &force_Read_کنارہ_Ignores](ثابت AstVarRef* vrefp) {
            AstVarScope* ثابت vscp = vrefp->varScopep();
            اگر (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !force_Read_کنارہ_Ignores.گنتی(vscp)) {
                نیا V3_گراف_کنارہ{m_graphp, get_Var_راس(vscp), logicVtxp, 10};
            }
            اگر (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                نیا V3_گراف_کنارہ{m_graphp, logicVtxp, get_Var_راس(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        اگر (m_dpiExportTriggerp) {
            nodep->foreach([یہ, logicVtxp](ثابت AstCCall* callp) {
                اگر (!callp->funcp()->dpiImportWrapper()) واپس;
                اگر (!callp->funcp()->dpiContext()) واپس;
                نیا V3_گراف_کنارہ{m_graphp, logicVtxp, get_Var_راس(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    خالی دورہ(Ast_فعال* nodep) override {
        Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        اگر (senTreep->hasHybrid()) {
            Ast_گرہ::user2_Clear_درخت();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user2(سچ);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ_Procedure* nodep) override { دورہ_Logic(nodep); }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override { دورہ_Logic(nodep); }
    خالی دورہ(AstCoverToggle* nodep) override { دورہ_Logic(nodep); }

    // Pre and Post logic are handled separately
    خالی دورہ(AstAlwaysPre* nodep) override {}
    خالی دورہ(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    خالی دورہ(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    خالی دورہ(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    خالی دورہ(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    خالی دورہ(Ast_گرہ* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    Sched_گراف_Builder(ثابت LogicByScope& clockedLogic, ثابت LogicByScope& combinationalLogic,
                      ثابت LogicByScope& hybridLogic) {
        // Build the data flow graph
        ثابت خودکار iter = [یہ](ثابت LogicByScope& lbs) {
            برائے (ثابت خودکار& اندراج_2 : lbs) {
                m_scopep = اندراج_2.m_scopep;
                iterate(اندراج_2.m_activep);
                m_scopep = خالی_اشاریہ;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { واپس جھوٹ; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { واپس سچ; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { واپس !vscp->user2(); };
        iter(hybridLogic);
    }

عوامی:
    // Build the dataflow graph for partitioning
    ساکن std::unique_ptr<V3_گراف> build(ثابت LogicByScope& clockedLogic,
                                          ثابت LogicByScope& combinationalLogic,
                                          ثابت LogicByScope& hybridLogic) {
        ثابت Sched_گراف_Builder visitor{clockedLogic, combinationalLogic, hybridLogic};
        واپس std::unique_ptr<V3_گراف>{visitor.m_graphp};
    }
};

خالی color_فعال_Region(V3_گراف& گراف) {
    // Work queue for depth first traversal
    std::سمتیہ<V3_گراف_راس*> قطار{};

    // Trace from all SchedSenVertex
    برائے (V3_گراف_راس& vtx : گراف.vertices()) {
        اگر (ثابت خودکار فعال_Event_Vtxp = vtx.cast<Sched_Sen_راس>()) {
            قطار.ڈالو_عقب(فعال_Event_Vtxp);
        }
    }

    // Depth first traversal
    جب (!قطار.خالی_خانے()) {
        // Pop next work item
        V3_گراف_راس& vtx = *قطار.عقب();
        قطار.نکالو_عقب();
        // If not first encounter, move on
        اگر (vtx.color() != 0) جاری;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        برائے (V3_گراف_کنارہ& کنارہ : vtx.اندر_کنارے()) قطار.ڈالو_عقب(کنارہ.fromp());

        // Mark top level ports that drive a sensitivity list
        اگر (Sched_Var_راس* ثابت vvtxp = vtx.cast<Sched_Var_راس>()) {
            AstVar* ثابت varp = vvtxp->varp();
            اگر (varp->isPrimaryIO()) varp->سیٹ_Primary_Clock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        اگر (vtx.is<Sched_Logic_راس>()) {
            برائے (V3_گراف_کنارہ& کنارہ : vtx.برآمد_کنارے()) {
                UASSERT(کنارہ.چوٹی()->is<Sched_Var_راس>(), "Should be var vertex");
                قطار.ڈالو_عقب(کنارہ.چوٹی());
            }
        }
    }
}

}  // namespace

LogicRegions تقسیم(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ثابت std::unique_ptr<V3_گراف> graphp
        = Sched_گراف_Builder::build(clockedLogic, combinationalLogic, hybridLogic);
    اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    color_فعال_Region(*(graphp.get()));
    اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", سچ);

    LogicRegions نتیجہ;

    برائے (V3_گراف_راس& vtx : graphp->vertices()) {
        اگر (ثابت خودکار lvtxp = vtx.cast<Sched_Logic_راس>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            منطقی مقصد_Act = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            اگر (v3Global.استعمال_کرتا_Zero_Delay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) مقصد_Act = سچ;
            LogicByScope& lbs = مقصد_Act ? نتیجہ.m_act : نتیجہ.m_nba;
            Ast_گرہ* ثابت logicp = lvtxp->logicp();
            logicp->unlink_Fr_عقب();
            lbs.جوڑو(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ثابت VNUser1_اندر_Use user1_اندر_Use;  // AstVarScope::user1() -> bool: read in Active region
        ثابت VNUser2_اندر_Use user2_اندر_Use;  // AstVarScope::user2() -> bool: written in Active region

        ثابت خودکار markVars = [](Ast_گرہ* nodep) {
            nodep->foreach([](ثابت Ast_گرہ_Var_Ref* vrefp) {
                AstVarScope* ثابت vscp = vrefp->varScopep();
                اگر (vrefp->access().isReadOrRW()) vscp->user1(سچ);
                اگر (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(سچ);
            });
        };

        برائے (ثابت خودکار& اندراج_2 : نتیجہ.m_act) {
            Ast_فعال* ثابت activep = اندراج_2.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        برائے (ثابت خودکار& اندراج_2 : clockedLogic) {
            AstScope* ثابت scopep = اندراج_2.m_scopep;
            Ast_فعال* ثابت activep = اندراج_2.m_activep;
            برائے (Ast_گرہ *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                اگر (AstAlwaysPre* ثابت logicp = VN_CAST(nodep, AlwaysPre)) {
                    منطقی مقصد_فعال_Region = جھوٹ;
                    logicp->foreach([&](ثابت Ast_گرہ_Var_Ref* vrefp) {
                        AstVarScope* ثابت vscp = vrefp->varScopep();
                        اگر (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            اگر (vscp->user2()) مقصد_فعال_Region = سچ;
                        } ورنہ {
                            // Variable written in Pre, and referenced in active region
                            اگر (vscp->user1() || vscp->user2()) مقصد_فعال_Region = سچ;
                        }
                    });
                    LogicByScope& lbs = مقصد_فعال_Region ? نتیجہ.m_pre : نتیجہ.m_nba;
                    logicp->unlink_Fr_عقب();
                    lbs.جوڑو(scopep, activep->sentreep(), logicp);
                } ورنہ {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlink_Fr_عقب();
                    نتیجہ.m_nba.جوڑو(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    واپس نتیجہ;
}

}  // namespace V3Sched
