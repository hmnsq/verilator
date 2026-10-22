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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Reloop.h"

#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف ReloopVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_شيون;  // Statistic tracking
    AstCFunc* m_cfuncp = خالي_اشارو;  // Current block

    std::ويڪٽر<Ast_ڳنڍ_سونپيو*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = خالي_اشارو;  // Parent C function
    ثابت Ast_ڳنڍ* m_mgNextp = خالي_اشارو;  // Next node
    ثابت Ast_ڳنڍ_Sel* m_mgSelLp = خالي_اشارو;  // Parent select, nullptr = idle
    ثابت Ast_ڳنڍ_Sel* m_mgSelRp = خالي_اشارو;  // Parent select, nullptr = constant
    ثابت Ast_ڳنڍ_Var_Ref* m_mgVarrefLp = خالي_اشارو;  // Parent varref
    ثابت Ast_ڳنڍ_Var_Ref* m_mgVarrefRp = خالي_اشارو;  // Parent varref, nullptr = constant
    int64_t m_mg_وٿي = 0;  // Index offset
    ثابت AstConst* m_mgConstRp = خالي_اشارو;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_اشاريو_Lo = 0;  // Merge range
    uint32_t m_mg_اشاريو_Hi = 0;  // Merge range

    // METHODS

    ساكن AstVar* create_Var_عارضي(File_سٽ* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ثابت string newvarname{"__Vilp" + std::منزل_string(cfuncp->user1Inc() + 1)};
        AstVar* ثابت varp
            = جديد AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->جوڙيو_Varsp(varp);
        أرجع varp;
    }
    فراغ ملايو_پڇاڙي() {
        إذا (!m_mgAssignps.خالي_جايون()) {
            ثابت uint32_t شيون = m_mg_اشاريو_Hi - m_mg_اشاريو_Lo + 1;
            UINFO(9, "End merge iter=" << شيون << " " << m_mg_اشاريو_Hi << ":" << m_mg_اشاريو_Lo << " "
                                       << m_mg_وٿي << " " << m_mgAssignps[0]);
            إذا (شيون >= static_cast<uint32_t>(v3Global.opt.reloop_حد_بندي())) {
                UINFO(6, "Reloop merging items=" << شيون << " " << m_mg_اشاريو_Hi << ":"
                                                 << m_mg_اشاريو_Lo << " " << m_mg_وٿي << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_شيون += شيون;

                // Transform first assign into for loop body
                Ast_ڳنڍ_سونپيو* ثابت bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_سٽ* ثابت fl = bodyp->fileline();
                AstVar* ثابت itp = create_Var_عارضي(fl, m_mgCfuncp);

                إذا (m_mg_وٿي > 0) {
                    UASSERT_OBJ(m_mg_اشاريو_Lo >= m_mg_وٿي, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_اشاريو_Lo -= m_mg_وٿي;
                    m_mg_اشاريو_Hi -= m_mg_وٿي;
                }

                Ast_ڳنڍ* ثابت initp = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                                                     جديد AstConst{fl, m_mg_اشاريو_Lo}};
                Ast_ڳنڍ_اظهار* ثابت condp = جديد AstLte{fl, جديد AstVarRef{fl, itp, VAccess::READ},
                                                      جديد AstConst{fl, m_mg_اشاريو_Hi}};
                Ast_ڳنڍ* ثابت incp = جديد Ast_سونپيو{
                    fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                    جديد Ast_جوڙيو{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ثابت loopp = جديد AstLoop{fl};
                loopp->جوڙيو_Stmtsp(جديد AstLoopTest{fl, loopp, condp});
                initp->جوڙيو_ايندڙ(loopp);
                bodyp->replaceWith(initp);
                loopp->جوڙيو_Stmtsp(bodyp);
                loopp->جوڙيو_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_ڳنڍ_اظهار* ثابت offsetp
                    = m_mg_وٿي == 0 ? خالي_اشارو : جديد AstConst(fl, std::مطلق_قدر(m_mg_وٿي));
                Ast_ڳنڍ_اظهار* ثابت lbitp = m_mgSelLp->bitp();
                Ast_ڳنڍ_اظهار* ثابت lvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_وٿي > 0 ? جديد Ast_جوڙيو{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_وڻ(), lbitp);
                إذا (m_mgSelRp) {  // else constant and no replace
                    Ast_ڳنڍ_اظهار* ثابت rbitp = m_mgSelRp->bitp();
                    Ast_ڳنڍ_اظهار* ثابت rvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_وٿي < 0 ? جديد Ast_جوڙيو{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_وڻ(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                لكل (Ast_ڳنڍ_سونپيو* assp : m_mgAssignps) {
                    إذا (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_پٺ()->delete_وڻ(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = خالي_اشارو;
            m_mgSelRp = خالي_اشارو;
            m_mgVarrefLp = خالي_اشارو;
            m_mgVarrefRp = خالي_اشارو;
            m_mg_وٿي = 0;
            m_mgConstRp = خالي_اشارو;
        }
    }

    // VISITORS
    فراغ گهمو(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ملايو_پڇاڙي();  // Finish last pending merge, if any
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        إذا (!m_cfuncp) أرجع;

        // Left select WordSel or ArraySel
        Ast_ڳنڍ_Sel* ثابت lselp = VN_CAST(nodep->lhsp(), ڳنڍ_Sel);
        إذا (!lselp) {  // Not ever merged
            ملايو_پڇاڙي();
            أرجع;
        }
        // Of a constant index
        ثابت AstConst* ثابت lbitp = VN_CAST(lselp->bitp(), Const);
        إذا (!lbitp) {
            ملايو_پڇاڙي();
            أرجع;
        }
        إذا (lbitp->ويڪر() > 32) {  // Assoc arrays can do this
            ملايو_پڇاڙي();
            أرجع;
        }
        ثابت uint32_t lindex = lbitp->منزل_UInt();
        // Of variable
        ثابت Ast_ڳنڍ_Var_Ref* ثابت lvarrefp = VN_CAST(lselp->fromp(), ڳنڍ_Var_Ref);
        إذا (!lvarrefp) {
            ملايو_پڇاڙي();
            أرجع;
        }

        // RHS is a constant or a select
        ثابت AstConst* ثابت rconstp = VN_CAST(nodep->rhsp(), Const);
        ثابت Ast_ڳنڍ_Sel* ثابت rselp = VN_CAST(nodep->rhsp(), ڳنڍ_Sel);
        ثابت Ast_ڳنڍ_Var_Ref* rvarrefp = خالي_اشارو;
        uint32_t rindex = lindex;
        إذا (rconstp) {  // Ok
        } وإلا إذا (rselp) {
            ثابت AstConst* ثابت rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), ڳنڍ_Var_Ref);
            إذا (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                ملايو_پڇاڙي();
                أرجع;
            }
            rindex = rbitp->منزل_UInt();
        } وإلا {
            ملايو_پڇاڙي();
            أرجع;
        }

        إذا (m_mgSelLp) {  // Old merge
            إذا (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_اشاريو_Lo - 1 || lindex == m_mg_اشاريو_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mg_وٿي)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                إذا (lindex == m_mg_اشاريو_Lo - 1) {
                    m_mg_اشاريو_Lo = lindex;
                } وإلا إذا (lindex == m_mg_اشاريو_Hi + 1) {
                    m_mg_اشاريو_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_اشاريو_Hi << ":" << m_mg_اشاريو_Lo
                                             << " " << nodep);
                m_mgAssignps.ڌڪو_پٺ(nodep);
                m_mgNextp = nodep->nextp();
                أرجع;
            } وإلا {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_اشاريو_Hi << ":" << m_mg_اشاريو_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                ملايو_پڇاڙي();
            }
        }

        // Merge start
        m_mgAssignps.ڌڪو_پٺ(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_وٿي = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_اشاريو_Lo = lindex;
        m_mg_اشاريو_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_وٿي << nodep);
    }
    فراغ گهمو(Ast_اظهار_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    فراغ گهمو(AstVar*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ_اظهار*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::جوڙيو_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::جوڙيو_Stat("Optimizations, Reloop iterations", m_stat_Re_شيون);
    }
};

//######################################################################
// Reloop class functions

فراغ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("reloop", 0, dump_وڻ_Either_Level() >= 6);
}
