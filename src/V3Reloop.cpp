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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Reloop.h"

#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت ReloopVisitor final : عوامی VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_اشیاء;  // Statistic tracking
    AstCFunc* m_cfuncp = خالی_اشاریہ;  // Current block

    std::سمتیہ<Ast_گرہ_تفویض*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = خالی_اشاریہ;  // Parent C function
    ثابت Ast_گرہ* m_mgNextp = خالی_اشاریہ;  // Next node
    ثابت Ast_گرہ_Sel* m_mgSelLp = خالی_اشاریہ;  // Parent select, nullptr = idle
    ثابت Ast_گرہ_Sel* m_mgSelRp = خالی_اشاریہ;  // Parent select, nullptr = constant
    ثابت Ast_گرہ_Var_Ref* m_mgVarrefLp = خالی_اشاریہ;  // Parent varref
    ثابت Ast_گرہ_Var_Ref* m_mgVarrefRp = خالی_اشاریہ;  // Parent varref, nullptr = constant
    int64_t m_mg_انحراف = 0;  // Index offset
    ثابت AstConst* m_mgConstRp = خالی_اشاریہ;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_اشاریہ_Lo = 0;  // Merge range
    uint32_t m_mg_اشاریہ_Hi = 0;  // Merge range

    // METHODS

    ساکن AstVar* create_Var_عارضی(File_سطر* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ثابت string newvarname{"__Vilp" + std::مقصد_string(cfuncp->user1Inc() + 1)};
        AstVar* ثابت varp
            = نیا AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->جوڑو_Varsp(varp);
        واپس varp;
    }
    خالی ملاؤ_انتہا() {
        اگر (!m_mgAssignps.خالی_خانے()) {
            ثابت uint32_t اشیاء = m_mg_اشاریہ_Hi - m_mg_اشاریہ_Lo + 1;
            UINFO(9, "End merge iter=" << اشیاء << " " << m_mg_اشاریہ_Hi << ":" << m_mg_اشاریہ_Lo << " "
                                       << m_mg_انحراف << " " << m_mgAssignps[0]);
            اگر (اشیاء >= static_cast<uint32_t>(v3Global.opt.reloop_حد_بندی())) {
                UINFO(6, "Reloop merging items=" << اشیاء << " " << m_mg_اشاریہ_Hi << ":"
                                                 << m_mg_اشاریہ_Lo << " " << m_mg_انحراف << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_اشیاء += اشیاء;

                // Transform first assign into for loop body
                Ast_گرہ_تفویض* ثابت bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_سطر* ثابت fl = bodyp->fileline();
                AstVar* ثابت itp = create_Var_عارضی(fl, m_mgCfuncp);

                اگر (m_mg_انحراف > 0) {
                    UASSERT_OBJ(m_mg_اشاریہ_Lo >= m_mg_انحراف, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_اشاریہ_Lo -= m_mg_انحراف;
                    m_mg_اشاریہ_Hi -= m_mg_انحراف;
                }

                Ast_گرہ* ثابت initp = نیا Ast_تفویض{fl, نیا AstVarRef{fl, itp, VAccess::WRITE},
                                                     نیا AstConst{fl, m_mg_اشاریہ_Lo}};
                Ast_گرہ_اظہاریہ* ثابت condp = نیا AstLte{fl, نیا AstVarRef{fl, itp, VAccess::READ},
                                                      نیا AstConst{fl, m_mg_اشاریہ_Hi}};
                Ast_گرہ* ثابت incp = نیا Ast_تفویض{
                    fl, نیا AstVarRef{fl, itp, VAccess::WRITE},
                    نیا Ast_جوڑو{fl, نیا AstConst{fl, 1}, نیا AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ثابت loopp = نیا AstLoop{fl};
                loopp->جوڑو_Stmtsp(نیا AstLoopTest{fl, loopp, condp});
                initp->جوڑو_اگلا(loopp);
                bodyp->replaceWith(initp);
                loopp->جوڑو_Stmtsp(bodyp);
                loopp->جوڑو_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_گرہ_اظہاریہ* ثابت offsetp
                    = m_mg_انحراف == 0 ? خالی_اشاریہ : نیا AstConst(fl, std::مطلق_قدر(m_mg_انحراف));
                Ast_گرہ_اظہاریہ* ثابت lbitp = m_mgSelLp->bitp();
                Ast_گرہ_اظہاریہ* ثابت lvrefp = نیا AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_انحراف > 0 ? نیا Ast_جوڑو{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_درخت(), lbitp);
                اگر (m_mgSelRp) {  // else constant and no replace
                    Ast_گرہ_اظہاریہ* ثابت rbitp = m_mgSelRp->bitp();
                    Ast_گرہ_اظہاریہ* ثابت rvrefp = نیا AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_انحراف < 0 ? نیا Ast_جوڑو{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_درخت(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                برائے (Ast_گرہ_تفویض* assp : m_mgAssignps) {
                    اگر (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_عقب()->delete_درخت(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = خالی_اشاریہ;
            m_mgSelRp = خالی_اشاریہ;
            m_mgVarrefLp = خالی_اشاریہ;
            m_mgVarrefRp = خالی_اشاریہ;
            m_mg_انحراف = 0;
            m_mgConstRp = خالی_اشاریہ;
        }
    }

    // VISITORS
    خالی دورہ(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ملاؤ_انتہا();  // Finish last pending merge, if any
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (!m_cfuncp) واپس;

        // Left select WordSel or ArraySel
        Ast_گرہ_Sel* ثابت lselp = VN_CAST(nodep->lhsp(), گرہ_Sel);
        اگر (!lselp) {  // Not ever merged
            ملاؤ_انتہا();
            واپس;
        }
        // Of a constant index
        ثابت AstConst* ثابت lbitp = VN_CAST(lselp->bitp(), Const);
        اگر (!lbitp) {
            ملاؤ_انتہا();
            واپس;
        }
        اگر (lbitp->چوڑائی() > 32) {  // Assoc arrays can do this
            ملاؤ_انتہا();
            واپس;
        }
        ثابت uint32_t lindex = lbitp->مقصد_UInt();
        // Of variable
        ثابت Ast_گرہ_Var_Ref* ثابت lvarrefp = VN_CAST(lselp->fromp(), گرہ_Var_Ref);
        اگر (!lvarrefp) {
            ملاؤ_انتہا();
            واپس;
        }

        // RHS is a constant or a select
        ثابت AstConst* ثابت rconstp = VN_CAST(nodep->rhsp(), Const);
        ثابت Ast_گرہ_Sel* ثابت rselp = VN_CAST(nodep->rhsp(), گرہ_Sel);
        ثابت Ast_گرہ_Var_Ref* rvarrefp = خالی_اشاریہ;
        uint32_t rindex = lindex;
        اگر (rconstp) {  // Ok
        } ورنہ اگر (rselp) {
            ثابت AstConst* ثابت rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), گرہ_Var_Ref);
            اگر (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                ملاؤ_انتہا();
                واپس;
            }
            rindex = rbitp->مقصد_UInt();
        } ورنہ {
            ملاؤ_انتہا();
            واپس;
        }

        اگر (m_mgSelLp) {  // Old merge
            اگر (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_اشاریہ_Lo - 1 || lindex == m_mg_اشاریہ_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mg_انحراف)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                اگر (lindex == m_mg_اشاریہ_Lo - 1) {
                    m_mg_اشاریہ_Lo = lindex;
                } ورنہ اگر (lindex == m_mg_اشاریہ_Hi + 1) {
                    m_mg_اشاریہ_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_اشاریہ_Hi << ":" << m_mg_اشاریہ_Lo
                                             << " " << nodep);
                m_mgAssignps.ڈالو_عقب(nodep);
                m_mgNextp = nodep->nextp();
                واپس;
            } ورنہ {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_اشاریہ_Hi << ":" << m_mg_اشاریہ_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                ملاؤ_انتہا();
            }
        }

        // Merge start
        m_mgAssignps.ڈالو_عقب(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_انحراف = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_اشاریہ_Lo = lindex;
        m_mg_اشاریہ_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_انحراف << nodep);
    }
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    خالی دورہ(AstVar*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::جوڑو_Stat("Optimizations, Reloop iterations", m_stat_Re_اشیاء);
    }
};

//######################################################################
// Reloop class functions

خالی V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("reloop", 0, dump_درخت_Either_Level() >= 6);
}
