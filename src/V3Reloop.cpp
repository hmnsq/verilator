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
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_توکي;  // Statistic tracking
    AstCFunc* m_cfuncp = تش_اشاره;  // Current block

    std::ویکټور<Ast_غوټه_ټاکل*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = تش_اشاره;  // Parent C function
    ثابت Ast_غوټه* m_mgNextp = تش_اشاره;  // Next node
    ثابت Ast_غوټه_Sel* m_mgSelLp = تش_اشاره;  // Parent select, nullptr = idle
    ثابت Ast_غوټه_Sel* m_mgSelRp = تش_اشاره;  // Parent select, nullptr = constant
    ثابت Ast_غوټه_Var_Ref* m_mgVarrefLp = تش_اشاره;  // Parent varref
    ثابت Ast_غوټه_Var_Ref* m_mgVarrefRp = تش_اشاره;  // Parent varref, nullptr = constant
    int64_t m_mg_دننه_واټن = 0;  // Index offset
    ثابت AstConst* m_mgConstRp = تش_اشاره;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_شاخص_Lo = 0;  // Merge range
    uint32_t m_mg_شاخص_Hi = 0;  // Merge range

    // METHODS

    ساكن AstVar* create_Var_لنډمهاله(File_کرښه* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ثابت string newvarname{"__Vilp" + std::منزل_string(cfuncp->user1Inc() + 1)};
        AstVar* ثابت varp
            = جديد AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->جمع_کړه_Varsp(varp);
        أرجع varp;
    }
    فراغ یوځای_پای_ټکی() {
        إذا (!m_mgAssignps.تشه()) {
            ثابت uint32_t توکي = m_mg_شاخص_Hi - m_mg_شاخص_Lo + 1;
            UINFO(9, "End merge iter=" << توکي << " " << m_mg_شاخص_Hi << ":" << m_mg_شاخص_Lo << " "
                                       << m_mg_دننه_واټن << " " << m_mgAssignps[0]);
            إذا (توکي >= static_cast<uint32_t>(v3Global.opt.reloop_حد())) {
                UINFO(6, "Reloop merging items=" << توکي << " " << m_mg_شاخص_Hi << ":"
                                                 << m_mg_شاخص_Lo << " " << m_mg_دننه_واټن << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_توکي += توکي;

                // Transform first assign into for loop body
                Ast_غوټه_ټاکل* ثابت bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_کرښه* ثابت fl = bodyp->fileline();
                AstVar* ثابت itp = create_Var_لنډمهاله(fl, m_mgCfuncp);

                إذا (m_mg_دننه_واټن > 0) {
                    UASSERT_OBJ(m_mg_شاخص_Lo >= m_mg_دننه_واټن, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_شاخص_Lo -= m_mg_دننه_واټن;
                    m_mg_شاخص_Hi -= m_mg_دننه_واټن;
                }

                Ast_غوټه* ثابت initp = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                                                     جديد AstConst{fl, m_mg_شاخص_Lo}};
                Ast_غوټه_بیان* ثابت condp = جديد AstLte{fl, جديد AstVarRef{fl, itp, VAccess::READ},
                                                      جديد AstConst{fl, m_mg_شاخص_Hi}};
                Ast_غوټه* ثابت incp = جديد Ast_ټاکل{
                    fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                    جديد Ast_جمع_کړه{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ثابت loopp = جديد AstLoop{fl};
                loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{fl, loopp, condp});
                initp->جمع_کړه_بل(loopp);
                bodyp->replaceWith(initp);
                loopp->جمع_کړه_Stmtsp(bodyp);
                loopp->جمع_کړه_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_غوټه_بیان* ثابت offsetp
                    = m_mg_دننه_واټن == 0 ? تش_اشاره : جديد AstConst(fl, std::مطلق_قیمت(m_mg_دننه_واټن));
                Ast_غوټه_بیان* ثابت lbitp = m_mgSelLp->bitp();
                Ast_غوټه_بیان* ثابت lvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_دننه_واټن > 0 ? جديد Ast_جمع_کړه{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_ونه(), lbitp);
                إذا (m_mgSelRp) {  // else constant and no replace
                    Ast_غوټه_بیان* ثابت rbitp = m_mgSelRp->bitp();
                    Ast_غوټه_بیان* ثابت rvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_دننه_واټن < 0 ? جديد Ast_جمع_کړه{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_ونه(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                لكل (Ast_غوټه_ټاکل* assp : m_mgAssignps) {
                    إذا (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_شا()->delete_ونه(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = تش_اشاره;
            m_mgSelRp = تش_اشاره;
            m_mgVarrefLp = تش_اشاره;
            m_mgVarrefRp = تش_اشاره;
            m_mg_دننه_واټن = 0;
            m_mgConstRp = تش_اشاره;
        }
    }

    // VISITORS
    فراغ لیدنه(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        یوځای_پای_ټکی();  // Finish last pending merge, if any
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        إذا (!m_cfuncp) أرجع;

        // Left select WordSel or ArraySel
        Ast_غوټه_Sel* ثابت lselp = VN_CAST(nodep->lhsp(), غوټه_Sel);
        إذا (!lselp) {  // Not ever merged
            یوځای_پای_ټکی();
            أرجع;
        }
        // Of a constant index
        ثابت AstConst* ثابت lbitp = VN_CAST(lselp->bitp(), Const);
        إذا (!lbitp) {
            یوځای_پای_ټکی();
            أرجع;
        }
        إذا (lbitp->پلنوالی() > 32) {  // Assoc arrays can do this
            یوځای_پای_ټکی();
            أرجع;
        }
        ثابت uint32_t lindex = lbitp->منزل_UInt();
        // Of variable
        ثابت Ast_غوټه_Var_Ref* ثابت lvarrefp = VN_CAST(lselp->fromp(), غوټه_Var_Ref);
        إذا (!lvarrefp) {
            یوځای_پای_ټکی();
            أرجع;
        }

        // RHS is a constant or a select
        ثابت AstConst* ثابت rconstp = VN_CAST(nodep->rhsp(), Const);
        ثابت Ast_غوټه_Sel* ثابت rselp = VN_CAST(nodep->rhsp(), غوټه_Sel);
        ثابت Ast_غوټه_Var_Ref* rvarrefp = تش_اشاره;
        uint32_t rindex = lindex;
        إذا (rconstp) {  // Ok
        } وإلا إذا (rselp) {
            ثابت AstConst* ثابت rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), غوټه_Var_Ref);
            إذا (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                یوځای_پای_ټکی();
                أرجع;
            }
            rindex = rbitp->منزل_UInt();
        } وإلا {
            یوځای_پای_ټکی();
            أرجع;
        }

        إذا (m_mgSelLp) {  // Old merge
            إذا (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_شاخص_Lo - 1 || lindex == m_mg_شاخص_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mg_دننه_واټن)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                إذا (lindex == m_mg_شاخص_Lo - 1) {
                    m_mg_شاخص_Lo = lindex;
                } وإلا إذا (lindex == m_mg_شاخص_Hi + 1) {
                    m_mg_شاخص_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_شاخص_Hi << ":" << m_mg_شاخص_Lo
                                             << " " << nodep);
                m_mgAssignps.ټېل_شا(nodep);
                m_mgNextp = nodep->nextp();
                أرجع;
            } وإلا {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_شاخص_Hi << ":" << m_mg_شاخص_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                یوځای_پای_ټکی();
            }
        }

        // Merge start
        m_mgAssignps.ټېل_شا(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_دننه_واټن = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_شاخص_Lo = lindex;
        m_mg_شاخص_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_دننه_واټن << nodep);
    }
    فراغ لیدنه(Ast_بیان_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    فراغ لیدنه(AstVar*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه_بیان*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::جمع_کړه_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::جمع_کړه_Stat("Optimizations, Reloop iterations", m_stat_Re_توکي);
    }
};

//######################################################################
// Reloop class functions

فراغ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("reloop", 0, dump_ونه_Either_Level() >= 6);
}
