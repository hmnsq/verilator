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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Reloop.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ ReloopVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_ਚੀਜ਼ਾਂ;  // Statistic tracking
    AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current block

    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਸੌਂਪੋ*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Parent C function
    ਅਚਲ Ast_ਗੰਢ* m_mgNextp = ਖਾਲੀ_ਸੂਚਕ;  // Next node
    ਅਚਲ Ast_ਗੰਢ_Sel* m_mgSelLp = ਖਾਲੀ_ਸੂਚਕ;  // Parent select, nullptr = idle
    ਅਚਲ Ast_ਗੰਢ_Sel* m_mgSelRp = ਖਾਲੀ_ਸੂਚਕ;  // Parent select, nullptr = constant
    ਅਚਲ Ast_ਗੰਢ_Var_Ref* m_mgVarrefLp = ਖਾਲੀ_ਸੂਚਕ;  // Parent varref
    ਅਚਲ Ast_ਗੰਢ_Var_Ref* m_mgVarrefRp = ਖਾਲੀ_ਸੂਚਕ;  // Parent varref, nullptr = constant
    int64_t m_mg_ਵਿੱਥ = 0;  // Index offset
    ਅਚਲ AstConst* m_mgConstRp = ਖਾਲੀ_ਸੂਚਕ;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_ਸੂਚਕਾਂਕ_Lo = 0;  // Merge range
    uint32_t m_mg_ਸੂਚਕਾਂਕ_Hi = 0;  // Merge range

    // METHODS

    ਸਥਿਰ AstVar* create_Var_ਆਰਜ਼ੀ(File_ਸਤਰ* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ਅਚਲ string newvarname{"__Vilp" + std::ਮੰਜ਼ਿਲ_string(cfuncp->user1Inc() + 1)};
        AstVar* ਅਚਲ varp
            = ਨਵਾਂ AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->ਜੋੜੋ_Varsp(varp);
        ਵਾਪਸ varp;
    }
    ਖਾਲੀ ਮਿਲਾਓ_ਸਿਰਾ() {
        ਜੇ (!m_mgAssignps.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ uint32_t ਚੀਜ਼ਾਂ = m_mg_ਸੂਚਕਾਂਕ_Hi - m_mg_ਸੂਚਕਾਂਕ_Lo + 1;
            UINFO(9, "End merge iter=" << ਚੀਜ਼ਾਂ << " " << m_mg_ਸੂਚਕਾਂਕ_Hi << ":" << m_mg_ਸੂਚਕਾਂਕ_Lo << " "
                                       << m_mg_ਵਿੱਥ << " " << m_mgAssignps[0]);
            ਜੇ (ਚੀਜ਼ਾਂ >= static_cast<uint32_t>(v3Global.opt.reloop_ਹੱਦਬੰਦੀ())) {
                UINFO(6, "Reloop merging items=" << ਚੀਜ਼ਾਂ << " " << m_mg_ਸੂਚਕਾਂਕ_Hi << ":"
                                                 << m_mg_ਸੂਚਕਾਂਕ_Lo << " " << m_mg_ਵਿੱਥ << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_ਚੀਜ਼ਾਂ += ਚੀਜ਼ਾਂ;

                // Transform first assign into for loop body
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_ਸਤਰ* ਅਚਲ fl = bodyp->fileline();
                AstVar* ਅਚਲ itp = create_Var_ਆਰਜ਼ੀ(fl, m_mgCfuncp);

                ਜੇ (m_mg_ਵਿੱਥ > 0) {
                    UASSERT_OBJ(m_mg_ਸੂਚਕਾਂਕ_Lo >= m_mg_ਵਿੱਥ, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_ਸੂਚਕਾਂਕ_Lo -= m_mg_ਵਿੱਥ;
                    m_mg_ਸੂਚਕਾਂਕ_Hi -= m_mg_ਵਿੱਥ;
                }

                Ast_ਗੰਢ* ਅਚਲ initp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, itp, VAccess::WRITE},
                                                     ਨਵਾਂ AstConst{fl, m_mg_ਸੂਚਕਾਂਕ_Lo}};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstLte{fl, ਨਵਾਂ AstVarRef{fl, itp, VAccess::READ},
                                                      ਨਵਾਂ AstConst{fl, m_mg_ਸੂਚਕਾਂਕ_Hi}};
                Ast_ਗੰਢ* ਅਚਲ incp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    fl, ਨਵਾਂ AstVarRef{fl, itp, VAccess::WRITE},
                    ਨਵਾਂ Ast_ਜੋੜੋ{fl, ਨਵਾਂ AstConst{fl, 1}, ਨਵਾਂ AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{fl};
                loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{fl, loopp, condp});
                initp->ਜੋੜੋ_ਅਗਲਾ(loopp);
                bodyp->replaceWith(initp);
                loopp->ਜੋੜੋ_Stmtsp(bodyp);
                loopp->ਜੋੜੋ_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ offsetp
                    = m_mg_ਵਿੱਥ == 0 ? ਖਾਲੀ_ਸੂਚਕ : ਨਵਾਂ AstConst(fl, std::ਨਿਰਪੇਖ(m_mg_ਵਿੱਥ));
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lbitp = m_mgSelLp->bitp();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lvrefp = ਨਵਾਂ AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_ਵਿੱਥ > 0 ? ਨਵਾਂ Ast_ਜੋੜੋ{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_ਰੁੱਖ(), lbitp);
                ਜੇ (m_mgSelRp) {  // else constant and no replace
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rbitp = m_mgSelRp->bitp();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rvrefp = ਨਵਾਂ AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_ਵਿੱਥ < 0 ? ਨਵਾਂ Ast_ਜੋੜੋ{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_ਰੁੱਖ(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                ਲਈ (Ast_ਗੰਢ_ਸੌਂਪੋ* assp : m_mgAssignps) {
                    ਜੇ (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = ਖਾਲੀ_ਸੂਚਕ;
            m_mgSelRp = ਖਾਲੀ_ਸੂਚਕ;
            m_mgVarrefLp = ਖਾਲੀ_ਸੂਚਕ;
            m_mgVarrefRp = ਖਾਲੀ_ਸੂਚਕ;
            m_mg_ਵਿੱਥ = 0;
            m_mgConstRp = ਖਾਲੀ_ਸੂਚਕ;
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ਮਿਲਾਓ_ਸਿਰਾ();  // Finish last pending merge, if any
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (!m_cfuncp) ਵਾਪਸ;

        // Left select WordSel or ArraySel
        Ast_ਗੰਢ_Sel* ਅਚਲ lselp = VN_CAST(nodep->lhsp(), ਗੰਢ_Sel);
        ਜੇ (!lselp) {  // Not ever merged
            ਮਿਲਾਓ_ਸਿਰਾ();
            ਵਾਪਸ;
        }
        // Of a constant index
        ਅਚਲ AstConst* ਅਚਲ lbitp = VN_CAST(lselp->bitp(), Const);
        ਜੇ (!lbitp) {
            ਮਿਲਾਓ_ਸਿਰਾ();
            ਵਾਪਸ;
        }
        ਜੇ (lbitp->ਚੌੜਾਈ() > 32) {  // Assoc arrays can do this
            ਮਿਲਾਓ_ਸਿਰਾ();
            ਵਾਪਸ;
        }
        ਅਚਲ uint32_t lindex = lbitp->ਮੰਜ਼ਿਲ_UInt();
        // Of variable
        ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ lvarrefp = VN_CAST(lselp->fromp(), ਗੰਢ_Var_Ref);
        ਜੇ (!lvarrefp) {
            ਮਿਲਾਓ_ਸਿਰਾ();
            ਵਾਪਸ;
        }

        // RHS is a constant or a select
        ਅਚਲ AstConst* ਅਚਲ rconstp = VN_CAST(nodep->rhsp(), Const);
        ਅਚਲ Ast_ਗੰਢ_Sel* ਅਚਲ rselp = VN_CAST(nodep->rhsp(), ਗੰਢ_Sel);
        ਅਚਲ Ast_ਗੰਢ_Var_Ref* rvarrefp = ਖਾਲੀ_ਸੂਚਕ;
        uint32_t rindex = lindex;
        ਜੇ (rconstp) {  // Ok
        } ਨਹੀਂਤਾਂ ਜੇ (rselp) {
            ਅਚਲ AstConst* ਅਚਲ rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), ਗੰਢ_Var_Ref);
            ਜੇ (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                ਮਿਲਾਓ_ਸਿਰਾ();
                ਵਾਪਸ;
            }
            rindex = rbitp->ਮੰਜ਼ਿਲ_UInt();
        } ਨਹੀਂਤਾਂ {
            ਮਿਲਾਓ_ਸਿਰਾ();
            ਵਾਪਸ;
        }

        ਜੇ (m_mgSelLp) {  // Old merge
            ਜੇ (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_ਸੂਚਕਾਂਕ_Lo - 1 || lindex == m_mg_ਸੂਚਕਾਂਕ_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mg_ਵਿੱਥ)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                ਜੇ (lindex == m_mg_ਸੂਚਕਾਂਕ_Lo - 1) {
                    m_mg_ਸੂਚਕਾਂਕ_Lo = lindex;
                } ਨਹੀਂਤਾਂ ਜੇ (lindex == m_mg_ਸੂਚਕਾਂਕ_Hi + 1) {
                    m_mg_ਸੂਚਕਾਂਕ_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_ਸੂਚਕਾਂਕ_Hi << ":" << m_mg_ਸੂਚਕਾਂਕ_Lo
                                             << " " << nodep);
                m_mgAssignps.ਧੱਕੋ_ਪਿੱਠ(nodep);
                m_mgNextp = nodep->nextp();
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_ਸੂਚਕਾਂਕ_Hi << ":" << m_mg_ਸੂਚਕਾਂਕ_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                ਮਿਲਾਓ_ਸਿਰਾ();
            }
        }

        // Merge start
        m_mgAssignps.ਧੱਕੋ_ਪਿੱਠ(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_ਵਿੱਥ = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_ਸੂਚਕਾਂਕ_Lo = lindex;
        m_mg_ਸੂਚਕਾਂਕ_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_ਵਿੱਥ << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Reloop iterations", m_stat_Re_ਚੀਜ਼ਾਂ);
    }
};

//######################################################################
// Reloop class functions

ਖਾਲੀ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("reloop", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
