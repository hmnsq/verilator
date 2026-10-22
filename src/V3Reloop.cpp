// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Recreate loops to help pack caches
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
// V3Reloop's Transformations:
//
// Each CFunc:
//    Look for a series of assignments that would look better in a loop:
//
//      ASSIGN(ARRAYREF(var, #), ARRAYREF(var, #+C))
//      ASSIGN(ARRAYREF(var, #+1), ARRAYREF(var, #+1+C))
//      ->
//      Create __Vilp local variable
//      FOR(__Vilp = low; __Vilp <= high; ++__Vlip)
//         ASSIGN(ARRAYREF(var, __Vilp), ARRAYREF(var, __Vilp + C))
//
//   Likewise vector assign to the same constant converted to a loop.
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Reloop.h"

#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి ReloopVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_వస్తువులు;  // Statistic tracking
    AstCFunc* m_cfuncp = శూన్యసూచిక;  // Current block

    std::సదిశ<Ast_కణుపు_కేటాయించు*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = శూన్యసూచిక;  // Parent C function
    స్థిరరాశి Ast_కణుపు* m_mgNextp = శూన్యసూచిక;  // Next node
    స్థిరరాశి Ast_కణుపు_Sel* m_mgSelLp = శూన్యసూచిక;  // Parent select, nullptr = idle
    స్థిరరాశి Ast_కణుపు_Sel* m_mgSelRp = శూన్యసూచిక;  // Parent select, nullptr = constant
    స్థిరరాశి Ast_కణుపు_Var_Ref* m_mgVarrefLp = శూన్యసూచిక;  // Parent varref
    స్థిరరాశి Ast_కణుపు_Var_Ref* m_mgVarrefRp = శూన్యసూచిక;  // Parent varref, nullptr = constant
    int64_t m_mg_స్థానభ్రంశం = 0;  // Index offset
    స్థిరరాశి AstConst* m_mgConstRp = శూన్యసూచిక;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_సూచీ_Lo = 0;  // Merge range
    uint32_t m_mg_సూచీ_Hi = 0;  // Merge range

    // METHODS

    స్థిర AstVar* create_Var_తాత్కాలిక(File_పంక్తి* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        స్థిరరాశి string newvarname{"__Vilp" + std::గమ్యం_string(cfuncp->user1Inc() + 1)};
        AstVar* స్థిరరాశి varp
            = కొత్త AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->కూడు_Varsp(varp);
        తిరిగి varp;
    }
    శూన్య విలీనం_చివర() {
        ఒకవేళ (!m_mgAssignps.ఖాళీలు()) {
            స్థిరరాశి uint32_t వస్తువులు = m_mg_సూచీ_Hi - m_mg_సూచీ_Lo + 1;
            UINFO(9, "End merge iter=" << వస్తువులు << " " << m_mg_సూచీ_Hi << ":" << m_mg_సూచీ_Lo << " "
                                       << m_mg_స్థానభ్రంశం << " " << m_mgAssignps[0]);
            ఒకవేళ (వస్తువులు >= static_cast<uint32_t>(v3Global.opt.reloop_హద్దు())) {
                UINFO(6, "Reloop merging items=" << వస్తువులు << " " << m_mg_సూచీ_Hi << ":"
                                                 << m_mg_సూచీ_Lo << " " << m_mg_స్థానభ్రంశం << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_వస్తువులు += వస్తువులు;

                // Transform first assign into for loop body
                Ast_కణుపు_కేటాయించు* స్థిరరాశి bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_పంక్తి* స్థిరరాశి fl = bodyp->fileline();
                AstVar* స్థిరరాశి itp = create_Var_తాత్కాలిక(fl, m_mgCfuncp);

                ఒకవేళ (m_mg_స్థానభ్రంశం > 0) {
                    UASSERT_OBJ(m_mg_సూచీ_Lo >= m_mg_స్థానభ్రంశం, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_సూచీ_Lo -= m_mg_స్థానభ్రంశం;
                    m_mg_సూచీ_Hi -= m_mg_స్థానభ్రంశం;
                }

                Ast_కణుపు* స్థిరరాశి initp = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, itp, VAccess::WRITE},
                                                     కొత్త AstConst{fl, m_mg_సూచీ_Lo}};
                Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstLte{fl, కొత్త AstVarRef{fl, itp, VAccess::READ},
                                                      కొత్త AstConst{fl, m_mg_సూచీ_Hi}};
                Ast_కణుపు* స్థిరరాశి incp = కొత్త Ast_కేటాయించు{
                    fl, కొత్త AstVarRef{fl, itp, VAccess::WRITE},
                    కొత్త Ast_కూడు{fl, కొత్త AstConst{fl, 1}, కొత్త AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{fl};
                loopp->కూడు_Stmtsp(కొత్త AstLoopTest{fl, loopp, condp});
                initp->కూడు_తదుపరి(loopp);
                bodyp->replaceWith(initp);
                loopp->కూడు_Stmtsp(bodyp);
                loopp->కూడు_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_కణుపు_సమాసం* స్థిరరాశి offsetp
                    = m_mg_స్థానభ్రంశం == 0 ? శూన్యసూచిక : కొత్త AstConst(fl, std::నిరపేక్ష(m_mg_స్థానభ్రంశం));
                Ast_కణుపు_సమాసం* స్థిరరాశి lbitp = m_mgSelLp->bitp();
                Ast_కణుపు_సమాసం* స్థిరరాశి lvrefp = కొత్త AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_స్థానభ్రంశం > 0 ? కొత్త Ast_కూడు{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_వృక్షం(), lbitp);
                ఒకవేళ (m_mgSelRp) {  // else constant and no replace
                    Ast_కణుపు_సమాసం* స్థిరరాశి rbitp = m_mgSelRp->bitp();
                    Ast_కణుపు_సమాసం* స్థిరరాశి rvrefp = కొత్త AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_స్థానభ్రంశం < 0 ? కొత్త Ast_కూడు{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_వృక్షం(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                కొరకు (Ast_కణుపు_కేటాయించు* assp : m_mgAssignps) {
                    ఒకవేళ (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_వెనుక()->delete_వృక్షం(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = శూన్యసూచిక;
            m_mgSelRp = శూన్యసూచిక;
            m_mgVarrefLp = శూన్యసూచిక;
            m_mgVarrefRp = శూన్యసూచిక;
            m_mg_స్థానభ్రంశం = 0;
            m_mgConstRp = శూన్యసూచిక;
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        విలీనం_చివర();  // Finish last pending merge, if any
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (!m_cfuncp) తిరిగి;

        // Left select WordSel or ArraySel
        Ast_కణుపు_Sel* స్థిరరాశి lselp = VN_CAST(nodep->lhsp(), కణుపు_Sel);
        ఒకవేళ (!lselp) {  // Not ever merged
            విలీనం_చివర();
            తిరిగి;
        }
        // Of a constant index
        స్థిరరాశి AstConst* స్థిరరాశి lbitp = VN_CAST(lselp->bitp(), Const);
        ఒకవేళ (!lbitp) {
            విలీనం_చివర();
            తిరిగి;
        }
        ఒకవేళ (lbitp->వెడల్పు() > 32) {  // Assoc arrays can do this
            విలీనం_చివర();
            తిరిగి;
        }
        స్థిరరాశి uint32_t lindex = lbitp->గమ్యం_UInt();
        // Of variable
        స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి lvarrefp = VN_CAST(lselp->fromp(), కణుపు_Var_Ref);
        ఒకవేళ (!lvarrefp) {
            విలీనం_చివర();
            తిరిగి;
        }

        // RHS is a constant or a select
        స్థిరరాశి AstConst* స్థిరరాశి rconstp = VN_CAST(nodep->rhsp(), Const);
        స్థిరరాశి Ast_కణుపు_Sel* స్థిరరాశి rselp = VN_CAST(nodep->rhsp(), కణుపు_Sel);
        స్థిరరాశి Ast_కణుపు_Var_Ref* rvarrefp = శూన్యసూచిక;
        uint32_t rindex = lindex;
        ఒకవేళ (rconstp) {  // Ok
        } లేకపోతే ఒకవేళ (rselp) {
            స్థిరరాశి AstConst* స్థిరరాశి rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), కణుపు_Var_Ref);
            ఒకవేళ (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                విలీనం_చివర();
                తిరిగి;
            }
            rindex = rbitp->గమ్యం_UInt();
        } లేకపోతే {
            విలీనం_చివర();
            తిరిగి;
        }

        ఒకవేళ (m_mgSelLp) {  // Old merge
            ఒకవేళ (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_సూచీ_Lo - 1 || lindex == m_mg_సూచీ_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mg_స్థానభ్రంశం)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                ఒకవేళ (lindex == m_mg_సూచీ_Lo - 1) {
                    m_mg_సూచీ_Lo = lindex;
                } లేకపోతే ఒకవేళ (lindex == m_mg_సూచీ_Hi + 1) {
                    m_mg_సూచీ_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_సూచీ_Hi << ":" << m_mg_సూచీ_Lo
                                             << " " << nodep);
                m_mgAssignps.నెట్టు_వెనుక(nodep);
                m_mgNextp = nodep->nextp();
                తిరిగి;
            } లేకపోతే {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_సూచీ_Hi << ":" << m_mg_సూచీ_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                విలీనం_చివర();
            }
        }

        // Merge start
        m_mgAssignps.నెట్టు_వెనుక(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_స్థానభ్రంశం = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_సూచీ_Lo = lindex;
        m_mg_సూచీ_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_స్థానభ్రంశం << nodep);
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    శూన్య సందర్శించు(AstVar*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::కూడు_Stat("Optimizations, Reloop iterations", m_stat_Re_వస్తువులు);
    }
};

//######################################################################
// Reloop class functions

శూన్య V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("reloop", 0, dump_వృక్షం_Either_Level() >= 6);
}
