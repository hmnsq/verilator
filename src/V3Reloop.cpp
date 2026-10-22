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
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_اشیاء;  // Statistic tracking
    AstCFunc* m_cfuncp = خالی_سوچک;  // Current block

    std::سمتیہ<Ast_گرہ_سونپو*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = خالی_سوچک;  // Parent C function
    ثابت Ast_گرہ* m_mgNextp = خالی_سوچک;  // Next node
    ثابت Ast_گرہ_Sel* m_mgSelLp = خالی_سوچک;  // Parent select, nullptr = idle
    ثابت Ast_گرہ_Sel* m_mgSelRp = خالی_سوچک;  // Parent select, nullptr = constant
    ثابت Ast_گرہ_Var_Ref* m_mgVarrefLp = خالی_سوچک;  // Parent varref
    ثابت Ast_گرہ_Var_Ref* m_mgVarrefRp = خالی_سوچک;  // Parent varref, nullptr = constant
    int64_t m_mg_وتھ = 0;  // Index offset
    ثابت AstConst* m_mgConstRp = خالی_سوچک;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_اشاریہ_Lo = 0;  // Merge range
    uint32_t m_mg_اشاریہ_Hi = 0;  // Merge range

    // METHODS

    ساكن AstVar* create_Var_عارضی(File_سطر* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ثابت string newvarname{"__Vilp" + std::منزل_string(cfuncp->user1Inc() + 1)};
        AstVar* ثابت varp
            = جديد AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->جوڑو_Varsp(varp);
        أرجع varp;
    }
    فراغ ملاؤ_سرا() {
        إذا (!m_mgAssignps.خالی_تھاواں()) {
            ثابت uint32_t اشیاء = m_mg_اشاریہ_Hi - m_mg_اشاریہ_Lo + 1;
            UINFO(9, "End merge iter=" << اشیاء << " " << m_mg_اشاریہ_Hi << ":" << m_mg_اشاریہ_Lo << " "
                                       << m_mg_وتھ << " " << m_mgAssignps[0]);
            إذا (اشیاء >= static_cast<uint32_t>(v3Global.opt.reloop_حد_بندی())) {
                UINFO(6, "Reloop merging items=" << اشیاء << " " << m_mg_اشاریہ_Hi << ":"
                                                 << m_mg_اشاریہ_Lo << " " << m_mg_وتھ << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_اشیاء += اشیاء;

                // Transform first assign into for loop body
                Ast_گرہ_سونپو* ثابت bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_سطر* ثابت fl = bodyp->fileline();
                AstVar* ثابت itp = create_Var_عارضی(fl, m_mgCfuncp);

                إذا (m_mg_وتھ > 0) {
                    UASSERT_OBJ(m_mg_اشاریہ_Lo >= m_mg_وتھ, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_اشاریہ_Lo -= m_mg_وتھ;
                    m_mg_اشاریہ_Hi -= m_mg_وتھ;
                }

                Ast_گرہ* ثابت initp = جديد Ast_سونپو{fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                                                     جديد AstConst{fl, m_mg_اشاریہ_Lo}};
                Ast_گرہ_اظہاریہ* ثابت condp = جديد AstLte{fl, جديد AstVarRef{fl, itp, VAccess::READ},
                                                      جديد AstConst{fl, m_mg_اشاریہ_Hi}};
                Ast_گرہ* ثابت incp = جديد Ast_سونپو{
                    fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                    جديد Ast_جوڑو{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ثابت loopp = جديد AstLoop{fl};
                loopp->جوڑو_Stmtsp(جديد AstLoopTest{fl, loopp, condp});
                initp->جوڑو_اگلا(loopp);
                bodyp->replaceWith(initp);
                loopp->جوڑو_Stmtsp(bodyp);
                loopp->جوڑو_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_گرہ_اظہاریہ* ثابت offsetp
                    = m_mg_وتھ == 0 ? خالی_سوچک : جديد AstConst(fl, std::مطلق_قدر(m_mg_وتھ));
                Ast_گرہ_اظہاریہ* ثابت lbitp = m_mgSelLp->bitp();
                Ast_گرہ_اظہاریہ* ثابت lvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_وتھ > 0 ? جديد Ast_جوڑو{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_درخت(), lbitp);
                إذا (m_mgSelRp) {  // else constant and no replace
                    Ast_گرہ_اظہاریہ* ثابت rbitp = m_mgSelRp->bitp();
                    Ast_گرہ_اظہاریہ* ثابت rvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_وتھ < 0 ? جديد Ast_جوڑو{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_درخت(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                لكل (Ast_گرہ_سونپو* assp : m_mgAssignps) {
                    إذا (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_پٹھ()->delete_درخت(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = خالی_سوچک;
            m_mgSelRp = خالی_سوچک;
            m_mgVarrefLp = خالی_سوچک;
            m_mgVarrefRp = خالی_سوچک;
            m_mg_وتھ = 0;
            m_mgConstRp = خالی_سوچک;
        }
    }

    // VISITORS
    فراغ ویکھو(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ملاؤ_سرا();  // Finish last pending merge, if any
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        إذا (!m_cfuncp) أرجع;

        // Left select WordSel or ArraySel
        Ast_گرہ_Sel* ثابت lselp = VN_CAST(nodep->lhsp(), گرہ_Sel);
        إذا (!lselp) {  // Not ever merged
            ملاؤ_سرا();
            أرجع;
        }
        // Of a constant index
        ثابت AstConst* ثابت lbitp = VN_CAST(lselp->bitp(), Const);
        إذا (!lbitp) {
            ملاؤ_سرا();
            أرجع;
        }
        إذا (lbitp->چوڑائی() > 32) {  // Assoc arrays can do this
            ملاؤ_سرا();
            أرجع;
        }
        ثابت uint32_t lindex = lbitp->منزل_UInt();
        // Of variable
        ثابت Ast_گرہ_Var_Ref* ثابت lvarrefp = VN_CAST(lselp->fromp(), گرہ_Var_Ref);
        إذا (!lvarrefp) {
            ملاؤ_سرا();
            أرجع;
        }

        // RHS is a constant or a select
        ثابت AstConst* ثابت rconstp = VN_CAST(nodep->rhsp(), Const);
        ثابت Ast_گرہ_Sel* ثابت rselp = VN_CAST(nodep->rhsp(), گرہ_Sel);
        ثابت Ast_گرہ_Var_Ref* rvarrefp = خالی_سوچک;
        uint32_t rindex = lindex;
        إذا (rconstp) {  // Ok
        } وإلا إذا (rselp) {
            ثابت AstConst* ثابت rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), گرہ_Var_Ref);
            إذا (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                ملاؤ_سرا();
                أرجع;
            }
            rindex = rbitp->منزل_UInt();
        } وإلا {
            ملاؤ_سرا();
            أرجع;
        }

        إذا (m_mgSelLp) {  // Old merge
            إذا (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_اشاریہ_Lo - 1 || lindex == m_mg_اشاریہ_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mg_وتھ)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                إذا (lindex == m_mg_اشاریہ_Lo - 1) {
                    m_mg_اشاریہ_Lo = lindex;
                } وإلا إذا (lindex == m_mg_اشاریہ_Hi + 1) {
                    m_mg_اشاریہ_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_اشاریہ_Hi << ":" << m_mg_اشاریہ_Lo
                                             << " " << nodep);
                m_mgAssignps.ڈالو_پٹھ(nodep);
                m_mgNextp = nodep->nextp();
                أرجع;
            } وإلا {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_اشاریہ_Hi << ":" << m_mg_اشاریہ_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                ملاؤ_سرا();
            }
        }

        // Merge start
        m_mgAssignps.ڈالو_پٹھ(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_وتھ = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_اشاریہ_Lo = lindex;
        m_mg_اشاریہ_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_وتھ << nodep);
    }
    فراغ ویکھو(Ast_اظہاریہ_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    فراغ ویکھو(AstVar*) override {}  // Accelerate
    فراغ ویکھو(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::جوڑو_Stat("Optimizations, Reloop iterations", m_stat_Re_اشیاء);
    }
};

//######################################################################
// Reloop class functions

فراغ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("reloop", 0, dump_درخت_Either_Level() >= 6);
}
