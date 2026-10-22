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

صنف Sched_Sen_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Sched_Sen_راس, V3_ګراف_راس)
    ثابت Ast_Sen_توکی* ثابت m_senItemp;

عام:
    Sched_Sen_راس(V3_ګراف* graphp, ثابت Ast_Sen_توکی* senItemp)
        : V3_ګراف_راس{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ثابت override {
        std::ostringstream os;
        V3EmitV::verilog_For_ونه(const_cast<Ast_Sen_توکی*>(m_senItemp), os);
        أرجع os.str();
    }
    string dotShape() ثابت override { أرجع "doubleoctagon"; }
    string dotColor() ثابت override { أرجع "red"; }
    // LCOV_EXCL_STOP
};

صنف Sched_Logic_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Sched_Logic_راس, V3_ګراف_راس)
    AstScope* ثابت m_scopep;
    Ast_Sen_ونه* ثابت m_senTreep;
    Ast_غوټه* ثابت m_logicp;

عام:
    Sched_Logic_راس(V3_ګراف* graphp, AstScope* scopep, Ast_Sen_ونه* senTreep, Ast_غوټه* logicp)
        : V3_ګراف_راس{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ثابت { أرجع m_scopep; }
    Ast_Sen_ونه* senTreep() ثابت { أرجع m_senTreep; }
    Ast_غوټه* logicp() ثابت { أرجع m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE {
        أرجع m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ثابت override { أرجع "rectangle"; }
    // LCOV_EXCL_STOP
};

صنف Sched_Var_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Sched_Var_راس, V3_ګراف_راس)
    ثابت AstVarScope* ثابت m_vscp;

عام:
    Sched_Var_راس(V3_ګراف* graphp, AstVarScope* vscp)
        : V3_ګراف_راس{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ثابت { أرجع m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { أرجع m_vscp->name(); }
    string dotShape() ثابت override {
        أرجع m_vscp->scopep()->is_سر() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ثابت override {
        أرجع m_vscp->scopep()->is_سر() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

صنف Sched_ګراف_Builder final : عام VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;
    ثابت VNUser2_دننه_Use m_user2_دننه_Use;

    // STATE
    V3_ګراف* ثابت m_graphp = جديد V3_ګراف;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<Ast_Sen_توکی>, Sched_Sen_راس*> m_senVertices;
    AstScope* m_scopep = تش_اشاره;  // AstScope of the current AstActive
    Ast_Sen_ونه* m_senTreep = تش_اشاره;  // AstSenTree of the current AstActive
    // Predicate for whether a read of the given variable triggers this block
    std::function<منطقي(AstVarScope*)> m_readTriggersThisLogic;
    // The DPI export trigger variable, if any
    AstVarScope* ثابت m_dpiExportTriggerp = v3Global.rootp()->dpiExportTriggerp();

    Sched_Var_راس* get_Var_راس(AstVarScope* vscp) ثابت {
        إذا (!vscp->user1p()) {
            Sched_Var_راس* ثابت vtxp = جديد Sched_Var_راس{m_graphp, vscp};
            // If this variable can be written via a DPI export, add a source edge from the
            // DPI export trigger vertex. This ensures calls to DPI exports that might write a
            // clock end up in the 'act' region.
            إذا (vscp->varp()->isWrittenByDpi()) {
                جديد V3_ګراف_څنډه{m_graphp, get_Var_راس(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        أرجع vscp->user1u().منزل<Sched_Var_راس*>();
    }

    Sched_Sen_راس* get_Sen_راس(Ast_Sen_توکی* senItemp) {
        إذا (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ثابت خپلکاری pair = m_senVertices.emplace(*senItemp, تش_اشاره);
            // If it does not exist, create it
            إذا (pair.دوهم) {
                // Create the vertex
                Sched_Sen_راس* ثابت vtxp = جديد Sched_Sen_راس{m_graphp, senItemp};

                // Connect up the variable references
                إذا (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        جديد V3_ګراف_څنډه{m_graphp, get_Var_راس(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.لومړی->دوهم = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.لومړی->دوهم);
        }
        أرجع senItemp->user1u().منزل<Sched_Sen_راس*>();
    }

    فراغ لیدنه_Logic(Ast_غوټه* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        Sched_Logic_راس* ثابت logicVtxp
            = جديد Sched_Logic_راس{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        إذا (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([هذا, nodep, logicVtxp](Ast_Sen_توکی* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3_ګراف_راس* ثابت eventVtxp = get_Sen_راس(senItemp);
                جديد V3_ګراف_څنډه{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::Var_Scope_سېټ force_Read_څنډه_Ignores;
        V3Sched::util::collect_Force_Read_څنډه_Ignores(nodep, force_Read_څنډه_Ignores);
        nodep->foreach([هذا, logicVtxp, &force_Read_څنډه_Ignores](ثابت AstVarRef* vrefp) {
            AstVarScope* ثابت vscp = vrefp->varScopep();
            إذا (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !force_Read_څنډه_Ignores.شمېر(vscp)) {
                جديد V3_ګراف_څنډه{m_graphp, get_Var_راس(vscp), logicVtxp, 10};
            }
            إذا (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                جديد V3_ګراف_څنډه{m_graphp, logicVtxp, get_Var_راس(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        إذا (m_dpiExportTriggerp) {
            nodep->foreach([هذا, logicVtxp](ثابت AstCCall* callp) {
                إذا (!callp->funcp()->dpiImportWrapper()) أرجع;
                إذا (!callp->funcp()->dpiContext()) أرجع;
                جديد V3_ګراف_څنډه{m_graphp, logicVtxp, get_Var_راس(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    فراغ لیدنه(Ast_فعال* nodep) override {
        Ast_Sen_ونه* ثابت senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        إذا (senTreep->hasHybrid()) {
            Ast_غوټه::user2_Clear_ونه();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user2(ریښتیا);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override { لیدنه_Logic(nodep); }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override { لیدنه_Logic(nodep); }
    فراغ لیدنه(AstCoverToggle* nodep) override { لیدنه_Logic(nodep); }

    // Pre and Post logic are handled separately
    فراغ لیدنه(AstAlwaysPre* nodep) override {}
    فراغ لیدنه(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    فراغ لیدنه(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    فراغ لیدنه(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    فراغ لیدنه(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    فراغ لیدنه(Ast_غوټه* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    Sched_ګراف_Builder(ثابت LogicByScope& clockedLogic, ثابت LogicByScope& combinationalLogic,
                      ثابت LogicByScope& hybridLogic) {
        // Build the data flow graph
        ثابت خپلکاری iter = [هذا](ثابت LogicByScope& lbs) {
            لكل (ثابت خپلکاری& ثبت : lbs) {
                m_scopep = ثبت.m_scopep;
                iterate(ثبت.m_activep);
                m_scopep = تش_اشاره;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { أرجع خطأ; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { أرجع ریښتیا; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { أرجع !vscp->user2(); };
        iter(hybridLogic);
    }

عام:
    // Build the dataflow graph for partitioning
    ساكن std::unique_ptr<V3_ګراف> build(ثابت LogicByScope& clockedLogic,
                                          ثابت LogicByScope& combinationalLogic,
                                          ثابت LogicByScope& hybridLogic) {
        ثابت Sched_ګراف_Builder visitor{clockedLogic, combinationalLogic, hybridLogic};
        أرجع std::unique_ptr<V3_ګراف>{visitor.m_graphp};
    }
};

فراغ color_فعال_Region(V3_ګراف& ګراف) {
    // Work queue for depth first traversal
    std::ویکټور<V3_ګراف_راس*> کتار{};

    // Trace from all SchedSenVertex
    لكل (V3_ګراف_راس& vtx : ګراف.vertices()) {
        إذا (ثابت خپلکاری فعال_Event_Vtxp = vtx.cast<Sched_Sen_راس>()) {
            کتار.ټېل_شا(فعال_Event_Vtxp);
        }
    }

    // Depth first traversal
    طالما (!کتار.تشه()) {
        // Pop next work item
        V3_ګراف_راس& vtx = *کتار.شا();
        کتار.راوباسه_شا();
        // If not first encounter, move on
        إذا (vtx.color() != 0) استمر;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        لكل (V3_ګراف_څنډه& څنډه : vtx.دننه_څنډې()) کتار.ټېل_شا(څنډه.fromp());

        // Mark top level ports that drive a sensitivity list
        إذا (Sched_Var_راس* ثابت vvtxp = vtx.cast<Sched_Var_راس>()) {
            AstVar* ثابت varp = vvtxp->varp();
            إذا (varp->isPrimaryIO()) varp->سېټ_Primary_Clock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        إذا (vtx.is<Sched_Logic_راس>()) {
            لكل (V3_ګراف_څنډه& څنډه : vtx.بهر_څنډې()) {
                UASSERT(څنډه.سر()->is<Sched_Var_راس>(), "Should be var vertex");
                کتار.ټېل_شا(څنډه.سر());
            }
        }
    }
}

}  // namespace

LogicRegions برخه_کول(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ثابت std::unique_ptr<V3_ګراف> graphp
        = Sched_ګراف_Builder::build(clockedLogic, combinationalLogic, hybridLogic);
    إذا (dump_ګراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    color_فعال_Region(*(graphp.get()));
    إذا (dump_ګراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", ریښتیا);

    LogicRegions پایله;

    لكل (V3_ګراف_راس& vtx : graphp->vertices()) {
        إذا (ثابت خپلکاری lvtxp = vtx.cast<Sched_Logic_راس>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            منطقي منزل_Act = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            إذا (v3Global.کاروي_Zero_Delay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) منزل_Act = ریښتیا;
            LogicByScope& lbs = منزل_Act ? پایله.m_act : پایله.m_nba;
            Ast_غوټه* ثابت logicp = lvtxp->logicp();
            logicp->unlink_Fr_شا();
            lbs.جمع_کړه(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ثابت VNUser1_دننه_Use user1_دننه_Use;  // AstVarScope::user1() -> bool: read in Active region
        ثابت VNUser2_دننه_Use user2_دننه_Use;  // AstVarScope::user2() -> bool: written in Active region

        ثابت خپلکاری markVars = [](Ast_غوټه* nodep) {
            nodep->foreach([](ثابت Ast_غوټه_Var_Ref* vrefp) {
                AstVarScope* ثابت vscp = vrefp->varScopep();
                إذا (vrefp->access().isReadOrRW()) vscp->user1(ریښتیا);
                إذا (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(ریښتیا);
            });
        };

        لكل (ثابت خپلکاری& ثبت : پایله.m_act) {
            Ast_فعال* ثابت activep = ثبت.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        لكل (ثابت خپلکاری& ثبت : clockedLogic) {
            AstScope* ثابت scopep = ثبت.m_scopep;
            Ast_فعال* ثابت activep = ثبت.m_activep;
            لكل (Ast_غوټه *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                إذا (AstAlwaysPre* ثابت logicp = VN_CAST(nodep, AlwaysPre)) {
                    منطقي منزل_فعال_Region = خطأ;
                    logicp->foreach([&](ثابت Ast_غوټه_Var_Ref* vrefp) {
                        AstVarScope* ثابت vscp = vrefp->varScopep();
                        إذا (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            إذا (vscp->user2()) منزل_فعال_Region = ریښتیا;
                        } وإلا {
                            // Variable written in Pre, and referenced in active region
                            إذا (vscp->user1() || vscp->user2()) منزل_فعال_Region = ریښتیا;
                        }
                    });
                    LogicByScope& lbs = منزل_فعال_Region ? پایله.m_pre : پایله.m_nba;
                    logicp->unlink_Fr_شا();
                    lbs.جمع_کړه(scopep, activep->sentreep(), logicp);
                } وإلا {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlink_Fr_شا();
                    پایله.m_nba.جمع_کړه(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    أرجع پایله;
}

}  // namespace V3Sched
