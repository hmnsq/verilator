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

صنف Sched_Sen_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Sen_راس, V3_گراف_راس)
    ثابت Ast_Sen_شے* ثابت m_senItemp;

عام:
    Sched_Sen_راس(V3_گراف* graphp, ثابت Ast_Sen_شے* senItemp)
        : V3_گراف_راس{graphp}
        , m_senItemp{senItemp} {}

    // LCOV_EXCL_START // Debug code
    string name() ثابت override {
        std::ostringstream os;
        V3EmitV::verilog_For_درخت(const_cast<Ast_Sen_شے*>(m_senItemp), os);
        أرجع os.str();
    }
    string dotShape() ثابت override { أرجع "doubleoctagon"; }
    string dotColor() ثابت override { أرجع "red"; }
    // LCOV_EXCL_STOP
};

صنف Sched_Logic_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Logic_راس, V3_گراف_راس)
    AstScope* ثابت m_scopep;
    Ast_Sen_درخت* ثابت m_senTreep;
    Ast_گرہ* ثابت m_logicp;

عام:
    Sched_Logic_راس(V3_گراف* graphp, AstScope* scopep, Ast_Sen_درخت* senTreep, Ast_گرہ* logicp)
        : V3_گراف_راس{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp} {}
    AstScope* scopep() ثابت { أرجع m_scopep; }
    Ast_Sen_درخت* senTreep() ثابت { أرجع m_senTreep; }
    Ast_گرہ* logicp() ثابت { أرجع m_logicp; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE {
        أرجع m_logicp->typeName() + ("\n" + m_logicp->fileline()->ascii());
    };
    string dotShape() ثابت override { أرجع "rectangle"; }
    // LCOV_EXCL_STOP
};

صنف Sched_Var_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Var_راس, V3_گراف_راس)
    ثابت AstVarScope* ثابت m_vscp;

عام:
    Sched_Var_راس(V3_گراف* graphp, AstVarScope* vscp)
        : V3_گراف_راس{graphp}
        , m_vscp{vscp} {}

    AstVar* varp() ثابت { أرجع m_vscp->varp(); }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { أرجع m_vscp->name(); }
    string dotShape() ثابت override {
        أرجع m_vscp->scopep()->is_چوٹی() && m_vscp->varp()->isNonOutput() ? "invhouse" : "ellipse";
    }
    string dotColor() ثابت override {
        أرجع m_vscp->scopep()->is_چوٹی() && m_vscp->varp()->isNonOutput() ? "green" : "black";
    }
    // LCOV_EXCL_STOP
};

صنف Sched_گراف_Builder final : عام VNVisitor {
    // NODE STATE
    // AstVarScope::user1p() -> SchedVarVertex
    // AstSenItem::user1p() -> SchedSenVertex
    // AstVarScope::user2() -> bool: Read of this AstVarScope triggers this logic.
    //                         Used only for hybrid logic.
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;
    ثابت VNUser2_وچ_Use m_user2_وچ_Use;

    // STATE
    V3_گراف* ثابت m_graphp = جديد V3_گراف;  // The dataflow graph being built
    // The vertices associated with a unique AstSenItem
    std::unordered_map<VNRef<Ast_Sen_شے>, Sched_Sen_راس*> m_senVertices;
    AstScope* m_scopep = خالی_سوچک;  // AstScope of the current AstActive
    Ast_Sen_درخت* m_senTreep = خالی_سوچک;  // AstSenTree of the current AstActive
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
                جديد V3_گراف_کنارا{m_graphp, get_Var_راس(m_dpiExportTriggerp), vtxp, 1};
            }
            vscp->user1p(vtxp);
        }
        أرجع vscp->user1u().منزل<Sched_Var_راس*>();
    }

    Sched_Sen_راس* get_Sen_راس(Ast_Sen_شے* senItemp) {
        إذا (!senItemp->user1p()) {
            // There is a unique SchedSenVertex for each globally unique AstSenItem. Multiple
            // AstSenTree might use the same AstSenItem (e.g.: posedge clk1 or rst, posedge clk2 or
            // rst), so we use a hash map to get the unique SchedSenVertex. (Note: This creates
            // separate vertices for ET_CHANGED and ET_HYBRID over the same expression, but that is
            // OK for now).
            ثابت خودکار pair = m_senVertices.emplace(*senItemp, خالی_سوچک);
            // If it does not exist, create it
            إذا (pair.دوجا) {
                // Create the vertex
                Sched_Sen_راس* ثابت vtxp = جديد Sched_Sen_راس{m_graphp, senItemp};

                // Connect up the variable references
                إذا (senItemp->sensp()) {
                    senItemp->sensp()->foreach([&](AstVarRef* refp) {
                        جديد V3_گراف_کنارا{m_graphp, get_Var_راس(refp->varScopep()), vtxp, 1};
                    });
                }

                // Store back to hash map so we can find it next time
                pair.پہلا->دوجا = vtxp;
            }

            // Cache sensitivity vertex
            senItemp->user1p(pair.پہلا->دوجا);
        }
        أرجع senItemp->user1u().منزل<Sched_Sen_راس*>();
    }

    فراغ ویکھو_Logic(Ast_گرہ* nodep) {
        UASSERT_OBJ(m_senTreep, nodep, "Should be under AstActive");

        Sched_Logic_راس* ثابت logicVtxp
            = جديد Sched_Logic_راس{m_graphp, m_scopep, m_senTreep, nodep};

        // Clocked or hybrid logic has explicit sensitivity, so add edge from sensitivity vertex
        إذا (!m_senTreep->hasCombo()) {
            m_senTreep->foreach([هذا, nodep, logicVtxp](Ast_Sen_شے* senItemp) {
                UASSERT_OBJ(senItemp->isClocked() || senItemp->isHybrid(), nodep,
                            "Non-clocked SenItem under clocked SenTree");
                V3_گراف_راس* ثابت eventVtxp = get_Sen_راس(senItemp);
                جديد V3_گراف_کنارا{m_graphp, eventVtxp, logicVtxp, 10};
            });
        }

        // Add edges based on references
        V3Sched::util::Var_Scope_سیٹ force_Read_کنارا_Ignores;
        V3Sched::util::collect_Force_Read_کنارا_Ignores(nodep, force_Read_کنارا_Ignores);
        nodep->foreach([هذا, logicVtxp, &force_Read_کنارا_Ignores](ثابت AstVarRef* vrefp) {
            AstVarScope* ثابت vscp = vrefp->varScopep();
            إذا (vrefp->access().isReadOrRW() && m_readTriggersThisLogic(vscp)
                && !force_Read_کنارا_Ignores.گنتی(vscp)) {
                جديد V3_گراف_کنارا{m_graphp, get_Var_راس(vscp), logicVtxp, 10};
            }
            إذا (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite()) {
                جديد V3_گراف_کنارا{m_graphp, logicVtxp, get_Var_راس(vscp), 10};
            }
        });

        // If the logic calls a 'context' DPI import, it might fire the DPI Export trigger
        إذا (m_dpiExportTriggerp) {
            nodep->foreach([هذا, logicVtxp](ثابت AstCCall* callp) {
                إذا (!callp->funcp()->dpiImportWrapper()) أرجع;
                إذا (!callp->funcp()->dpiContext()) أرجع;
                جديد V3_گراف_کنارا{m_graphp, logicVtxp, get_Var_راس(m_dpiExportTriggerp), 10};
            });
        }
    }

    // VISIT methods
    فراغ ویکھو(Ast_سرگرم* nodep) override {
        Ast_Sen_درخت* ثابت senTreep = nodep->sentreep();
        UASSERT_OBJ(senTreep->hasClocked() || senTreep->hasCombo() || senTreep->hasHybrid(), nodep,
                    "Unhandled");
        UASSERT_OBJ(!m_senTreep, nodep, "Should not nest");

        // Mark explicit sensitivities as not triggering these blocks
        إذا (senTreep->hasHybrid()) {
            Ast_گرہ::user2_Clear_درخت();
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user2(سچ);
            });
        }

        VL_RESTORER(m_senTreep);
        m_senTreep = senTreep;
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_گرہ_Procedure* nodep) override { ویکھو_Logic(nodep); }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override { ویکھو_Logic(nodep); }
    فراغ ویکھو(AstCoverToggle* nodep) override { ویکھو_Logic(nodep); }

    // Pre and Post logic are handled separately
    فراغ ویکھو(AstAlwaysPre* nodep) override {}
    فراغ ویکھو(AstAlwaysPost* nodep) override {}

    // LCOV_EXCL_START
    // Ignore
    فراغ ویکھو(AstInitialStatic* nodep) override { nodep->v3fatalSrc("Should not need ordering"); }
    فراغ ویکھو(AstInitial* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }
    فراغ ویکھو(AstFinal* nodep) override {  //
        nodep->v3fatalSrc("Should not need ordering");
    }

    // Default - Any other AstActive content not handled above will hit this
    فراغ ویکھو(Ast_گرہ* nodep) override {  //
        nodep->v3fatalSrc("Should be handled above");
    }
    // LCOV_EXCL_STOP

    Sched_گراف_Builder(ثابت LogicByScope& clockedLogic, ثابت LogicByScope& combinationalLogic,
                      ثابت LogicByScope& hybridLogic) {
        // Build the data flow graph
        ثابت خودکار iter = [هذا](ثابت LogicByScope& lbs) {
            لكل (ثابت خودکار& اندراج : lbs) {
                m_scopep = اندراج.m_scopep;
                iterate(اندراج.m_activep);
                m_scopep = خالی_سوچک;
            }
        };
        // Clocked logic is never triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { أرجع خطأ; };
        iter(clockedLogic);
        // Combinational logic is always triggered by reads
        m_readTriggersThisLogic = [](AstVarScope*) { أرجع سچ; };
        iter(combinationalLogic);
        // Hybrid logic is triggered by all reads, except for reads of the explicit sensitivities
        m_readTriggersThisLogic = [](AstVarScope* vscp) { أرجع !vscp->user2(); };
        iter(hybridLogic);
    }

عام:
    // Build the dataflow graph for partitioning
    ساكن std::unique_ptr<V3_گراف> build(ثابت LogicByScope& clockedLogic,
                                          ثابت LogicByScope& combinationalLogic,
                                          ثابت LogicByScope& hybridLogic) {
        ثابت Sched_گراف_Builder visitor{clockedLogic, combinationalLogic, hybridLogic};
        أرجع std::unique_ptr<V3_گراف>{visitor.m_graphp};
    }
};

فراغ color_سرگرم_Region(V3_گراف& گراف) {
    // Work queue for depth first traversal
    std::سمتیہ<V3_گراف_راس*> قطار{};

    // Trace from all SchedSenVertex
    لكل (V3_گراف_راس& vtx : گراف.vertices()) {
        إذا (ثابت خودکار سرگرم_Event_Vtxp = vtx.cast<Sched_Sen_راس>()) {
            قطار.ڈالو_پٹھ(سرگرم_Event_Vtxp);
        }
    }

    // Depth first traversal
    طالما (!قطار.خالی_تھاواں()) {
        // Pop next work item
        V3_گراف_راس& vtx = *قطار.پٹھ();
        قطار.نکالو_پٹھ();
        // If not first encounter, move on
        إذا (vtx.color() != 0) استمر;

        // Mark vertex as being in active region
        vtx.color(1);

        // Enqueue all parent vertices that feed this vertex.
        لكل (V3_گراف_کنارا& کنارا : vtx.وچ_کنارے()) قطار.ڈالو_پٹھ(کنارا.fromp());

        // Mark top level ports that drive a sensitivity list
        إذا (Sched_Var_راس* ثابت vvtxp = vtx.cast<Sched_Var_راس>()) {
            AstVar* ثابت varp = vvtxp->varp();
            إذا (varp->isPrimaryIO()) varp->سیٹ_Primary_Clock();
        }

        // If this is a logic vertex, also enqueue all variable vertices that are driven from this
        // logic. This will ensure that if a variable is set in the active region, then all
        // settings of that variable will be in the active region.
        إذا (vtx.is<Sched_Logic_راس>()) {
            لكل (V3_گراف_کنارا& کنارا : vtx.باہرلا_کنارے()) {
                UASSERT(کنارا.چوٹی()->is<Sched_Var_راس>(), "Should be var vertex");
                قطار.ڈالو_پٹھ(کنارا.چوٹی());
            }
        }
    }
}

}  // namespace

LogicRegions حصے_کرو(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) {
    UINFO(2, __FUNCTION__ << ":");

    // Build the graph
    ثابت std::unique_ptr<V3_گراف> graphp
        = Sched_گراف_Builder::build(clockedLogic, combinationalLogic, hybridLogic);
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched");

    // Partition into Active and NBA regions
    color_سرگرم_Region(*(graphp.get()));
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-partitioned", سچ);

    LogicRegions نتیجہ;

    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (ثابت خودکار lvtxp = vtx.cast<Sched_Logic_راس>()) {
            // Move to 'act'/'nba' based on coloring by default ...
            منطقي منزل_Act = lvtxp->color();
            // ... however, if a #0 delay is possible, then the 'inact' region is required,
            // in which case **EVERYTHING** that is not a Post block needs to go to 'act'.
            // This severely limits downstream optimizations (e.g. V3LifePost), and severely
            // reduces available parallelism in 'nba' for multi-threaded execution.
            إذا (v3Global.ورتدا_Zero_Delay() && !VN_IS(lvtxp->logicp(), AlwaysPost)) منزل_Act = سچ;
            LogicByScope& lbs = منزل_Act ? نتیجہ.m_act : نتیجہ.m_nba;
            Ast_گرہ* ثابت logicp = lvtxp->logicp();
            logicp->unlink_Fr_پٹھ();
            lbs.جوڑو(lvtxp->scopep(), lvtxp->senTreep(), logicp);
        }
    }

    // Partition the Pre logic
    {
        ثابت VNUser1_وچ_Use user1_وچ_Use;  // AstVarScope::user1() -> bool: read in Active region
        ثابت VNUser2_وچ_Use user2_وچ_Use;  // AstVarScope::user2() -> bool: written in Active region

        ثابت خودکار markVars = [](Ast_گرہ* nodep) {
            nodep->foreach([](ثابت Ast_گرہ_Var_Ref* vrefp) {
                AstVarScope* ثابت vscp = vrefp->varScopep();
                إذا (vrefp->access().isReadOrRW()) vscp->user1(سچ);
                إذا (vrefp->access().isWriteOrRW() && !vrefp->varp()->ignoreSchedWrite())
                    vscp->user2(سچ);
            });
        };

        لكل (ثابت خودکار& اندراج : نتیجہ.m_act) {
            Ast_سرگرم* ثابت activep = اندراج.m_activep;
            markVars(activep->sentreep());
            markVars(activep);
        }

        // AstAlwaysPre and AstAlwaysPost should only appear under a clocked
        // AstActive, and should be the only thing left at this point.
        لكل (ثابت خودکار& اندراج : clockedLogic) {
            AstScope* ثابت scopep = اندراج.m_scopep;
            Ast_سرگرم* ثابت activep = اندراج.m_activep;
            لكل (Ast_گرہ *nodep = activep->stmtsp(), *nextp; nodep; nodep = nextp) {
                nextp = nodep->nextp();
                إذا (AstAlwaysPre* ثابت logicp = VN_CAST(nodep, AlwaysPre)) {
                    منطقي منزل_سرگرم_Region = خطأ;
                    logicp->foreach([&](ثابت Ast_گرہ_Var_Ref* vrefp) {
                        AstVarScope* ثابت vscp = vrefp->varScopep();
                        إذا (vrefp->access().isReadOnly()) {
                            // Variable only read in Pre, and is written in active region
                            إذا (vscp->user2()) منزل_سرگرم_Region = سچ;
                        } وإلا {
                            // Variable written in Pre, and referenced in active region
                            إذا (vscp->user1() || vscp->user2()) منزل_سرگرم_Region = سچ;
                        }
                    });
                    LogicByScope& lbs = منزل_سرگرم_Region ? نتیجہ.m_pre : نتیجہ.m_nba;
                    logicp->unlink_Fr_پٹھ();
                    lbs.جوڑو(scopep, activep->sentreep(), logicp);
                } وإلا {
                    UASSERT_OBJ(VN_IS(nodep, AlwaysPost), nodep,
                                "Unexpected node type " << nodep->typeName());
                    nodep->unlink_Fr_پٹھ();
                    نتیجہ.m_nba.جوڑو(scopep, activep->sentreep(), nodep);
                }
            }
        }
    }

    // Clean up remains of inputs
    clockedLogic.deleteActives();
    combinationalLogic.deleteActives();
    hybridLogic.deleteActives();

    أرجع نتیجہ;
}

}  // namespace V3Sched
