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
    ثابت VNUser1_درون_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_اقلام;  // Statistic tracking
    AstCFunc* m_cfuncp = اشاره_تهی;  // Current block

    std::وکتور<Ast_گره_تخصیص_بده*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = اشاره_تهی;  // Parent C function
    ثابت Ast_گره* m_mgNextp = اشاره_تهی;  // Next node
    ثابت Ast_گره_Sel* m_mgSelLp = اشاره_تهی;  // Parent select, nullptr = idle
    ثابت Ast_گره_Sel* m_mgSelRp = اشاره_تهی;  // Parent select, nullptr = constant
    ثابت Ast_گره_Var_Ref* m_mgVarrefLp = اشاره_تهی;  // Parent varref
    ثابت Ast_گره_Var_Ref* m_mgVarrefRp = اشاره_تهی;  // Parent varref, nullptr = constant
    int64_t m_mg_جابه_جایی = 0;  // Index offset
    ثابت AstConst* m_mgConstRp = اشاره_تهی;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_شاخص_Lo = 0;  // Merge range
    uint32_t m_mg_شاخص_Hi = 0;  // Merge range

    // METHODS

    ساكن AstVar* create_Var_موقت(File_سطر* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ثابت string newvarname{"__Vilp" + std::مقصد_string(cfuncp->user1Inc() + 1)};
        AstVar* ثابت varp
            = جديد AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->جمع_کن_Varsp(varp);
        أرجع varp;
    }
    فراغ ادغام_انتها() {
        إذا (!m_mgAssignps.خالی()) {
            ثابت uint32_t اقلام = m_mg_شاخص_Hi - m_mg_شاخص_Lo + 1;
            UINFO(9, "End merge iter=" << اقلام << " " << m_mg_شاخص_Hi << ":" << m_mg_شاخص_Lo << " "
                                       << m_mg_جابه_جایی << " " << m_mgAssignps[0]);
            إذا (اقلام >= static_cast<uint32_t>(v3Global.opt.reloop_حد())) {
                UINFO(6, "Reloop merging items=" << اقلام << " " << m_mg_شاخص_Hi << ":"
                                                 << m_mg_شاخص_Lo << " " << m_mg_جابه_جایی << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_اقلام += اقلام;

                // Transform first assign into for loop body
                Ast_گره_تخصیص_بده* ثابت bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_سطر* ثابت fl = bodyp->fileline();
                AstVar* ثابت itp = create_Var_موقت(fl, m_mgCfuncp);

                إذا (m_mg_جابه_جایی > 0) {
                    UASSERT_OBJ(m_mg_شاخص_Lo >= m_mg_جابه_جایی, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_شاخص_Lo -= m_mg_جابه_جایی;
                    m_mg_شاخص_Hi -= m_mg_جابه_جایی;
                }

                Ast_گره* ثابت initp = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                                                     جديد AstConst{fl, m_mg_شاخص_Lo}};
                Ast_گره_عبارت* ثابت condp = جديد AstLte{fl, جديد AstVarRef{fl, itp, VAccess::READ},
                                                      جديد AstConst{fl, m_mg_شاخص_Hi}};
                Ast_گره* ثابت incp = جديد Ast_تخصیص_بده{
                    fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                    جديد Ast_جمع_کن{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ثابت loopp = جديد AstLoop{fl};
                loopp->جمع_کن_Stmtsp(جديد AstLoopTest{fl, loopp, condp});
                initp->جمع_کن_بعدی(loopp);
                bodyp->replaceWith(initp);
                loopp->جمع_کن_Stmtsp(bodyp);
                loopp->جمع_کن_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_گره_عبارت* ثابت offsetp
                    = m_mg_جابه_جایی == 0 ? اشاره_تهی : جديد AstConst(fl, std::قدر_مطلق(m_mg_جابه_جایی));
                Ast_گره_عبارت* ثابت lbitp = m_mgSelLp->bitp();
                Ast_گره_عبارت* ثابت lvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_جابه_جایی > 0 ? جديد Ast_جمع_کن{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_درخت(), lbitp);
                إذا (m_mgSelRp) {  // else constant and no replace
                    Ast_گره_عبارت* ثابت rbitp = m_mgSelRp->bitp();
                    Ast_گره_عبارت* ثابت rvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_جابه_جایی < 0 ? جديد Ast_جمع_کن{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_درخت(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                لكل (Ast_گره_تخصیص_بده* assp : m_mgAssignps) {
                    إذا (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_پشت()->delete_درخت(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = اشاره_تهی;
            m_mgSelRp = اشاره_تهی;
            m_mgVarrefLp = اشاره_تهی;
            m_mgVarrefRp = اشاره_تهی;
            m_mg_جابه_جایی = 0;
            m_mgConstRp = اشاره_تهی;
        }
    }

    // VISITORS
    فراغ بازدید(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ادغام_انتها();  // Finish last pending merge, if any
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        إذا (!m_cfuncp) أرجع;

        // Left select WordSel or ArraySel
        Ast_گره_Sel* ثابت lselp = VN_CAST(nodep->lhsp(), گره_Sel);
        إذا (!lselp) {  // Not ever merged
            ادغام_انتها();
            أرجع;
        }
        // Of a constant index
        ثابت AstConst* ثابت lbitp = VN_CAST(lselp->bitp(), Const);
        إذا (!lbitp) {
            ادغام_انتها();
            أرجع;
        }
        إذا (lbitp->عرض() > 32) {  // Assoc arrays can do this
            ادغام_انتها();
            أرجع;
        }
        ثابت uint32_t lindex = lbitp->مقصد_UInt();
        // Of variable
        ثابت Ast_گره_Var_Ref* ثابت lvarrefp = VN_CAST(lselp->fromp(), گره_Var_Ref);
        إذا (!lvarrefp) {
            ادغام_انتها();
            أرجع;
        }

        // RHS is a constant or a select
        ثابت AstConst* ثابت rconstp = VN_CAST(nodep->rhsp(), Const);
        ثابت Ast_گره_Sel* ثابت rselp = VN_CAST(nodep->rhsp(), گره_Sel);
        ثابت Ast_گره_Var_Ref* rvarrefp = اشاره_تهی;
        uint32_t rindex = lindex;
        إذا (rconstp) {  // Ok
        } وإلا إذا (rselp) {
            ثابت AstConst* ثابت rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), گره_Var_Ref);
            إذا (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                ادغام_انتها();
                أرجع;
            }
            rindex = rbitp->مقصد_UInt();
        } وإلا {
            ادغام_انتها();
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
                && (m_mgConstRp || lindex == rindex + m_mg_جابه_جایی)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                إذا (lindex == m_mg_شاخص_Lo - 1) {
                    m_mg_شاخص_Lo = lindex;
                } وإلا إذا (lindex == m_mg_شاخص_Hi + 1) {
                    m_mg_شاخص_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_شاخص_Hi << ":" << m_mg_شاخص_Lo
                                             << " " << nodep);
                m_mgAssignps.افزودن_پشت(nodep);
                m_mgNextp = nodep->nextp();
                أرجع;
            } وإلا {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_شاخص_Hi << ":" << m_mg_شاخص_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                ادغام_انتها();
            }
        }

        // Merge start
        m_mgAssignps.افزودن_پشت(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_جابه_جایی = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_شاخص_Lo = lindex;
        m_mg_شاخص_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_جابه_جایی << nodep);
    }
    فراغ بازدید(Ast_عبارت_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    فراغ بازدید(AstVar*) override {}  // Accelerate
    فراغ بازدید(Ast_گره_عبارت*) override {}  // Accelerate
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::جمع_کن_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::جمع_کن_Stat("Optimizations, Reloop iterations", m_stat_Re_اقلام);
    }
};

//######################################################################
// Reloop class functions

فراغ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("reloop", 0, dump_درخت_Either_Level() >= 6);
}
