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

رده ReloopVisitor final : عمومی VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    ثابت VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_اقلام;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_گره_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    ثابت Ast_گره* m_mgNextp = nullptr;  // Next node
    ثابت Ast_گره_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    ثابت Ast_گره_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    ثابت Ast_گره_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    ثابت Ast_گره_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    ثابت AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    ایستا AstVar* createVarTemp(File_سطر* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ثابت string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* ثابت varp
            = جدید AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        بازگشت varp;
    }
    تهی mergeEnd() {
        اگر (!m_mgAssignps.empty()) {
            ثابت uint32_t اقلام = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << اقلام << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            اگر (اقلام >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << اقلام << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_اقلام += اقلام;

                // Transform first assign into for loop body
                Ast_گره_Assign* ثابت bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_سطر* ثابت fl = bodyp->fileline();
                AstVar* ثابت itp = createVarTemp(fl, m_mgCfuncp);

                اگر (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_گره* ثابت initp = جدید AstAssign{fl, جدید AstVarRef{fl, itp, VAccess::WRITE},
                                                     جدید AstConst{fl, m_mgIndexLo}};
                Ast_گره_عبارت* ثابت condp = جدید AstLte{fl, جدید AstVarRef{fl, itp, VAccess::READ},
                                                      جدید AstConst{fl, m_mgIndexHi}};
                Ast_گره* ثابت incp = جدید AstAssign{
                    fl, جدید AstVarRef{fl, itp, VAccess::WRITE},
                    جدید AstAdd{fl, جدید AstConst{fl, 1}, جدید AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ثابت loopp = جدید AstLoop{fl};
                loopp->addStmtsp(جدید AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_گره_عبارت* ثابت offsetp
                    = m_mgOffset == 0 ? nullptr : جدید AstConst(fl, std::abs(m_mgOffset));
                Ast_گره_عبارت* ثابت lbitp = m_mgSelLp->bitp();
                Ast_گره_عبارت* ثابت lvrefp = جدید AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? جدید AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_درخت(), lbitp);
                اگر (m_mgSelRp) {  // else constant and no replace
                    Ast_گره_عبارت* ثابت rbitp = m_mgSelRp->bitp();
                    Ast_گره_عبارت* ثابت rvrefp = جدید AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? جدید AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_درخت(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                برای (Ast_گره_Assign* assp : m_mgAssignps) {
                    اگر (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_درخت(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = nullptr;
            m_mgSelRp = nullptr;
            m_mgVarrefLp = nullptr;
            m_mgVarrefRp = nullptr;
            m_mgOffset = 0;
            m_mgConstRp = nullptr;
        }
    }

    // VISITORS
    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (!m_cfuncp) بازگشت;

        // Left select WordSel or ArraySel
        Ast_گره_Sel* ثابت lselp = VN_CAST(nodep->lhsp(), گره_Sel);
        اگر (!lselp) {  // Not ever merged
            mergeEnd();
            بازگشت;
        }
        // Of a constant index
        ثابت AstConst* ثابت lbitp = VN_CAST(lselp->bitp(), Const);
        اگر (!lbitp) {
            mergeEnd();
            بازگشت;
        }
        اگر (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            بازگشت;
        }
        ثابت uint32_t lindex = lbitp->toUInt();
        // Of variable
        ثابت Ast_گره_Var_Ref* ثابت lvarrefp = VN_CAST(lselp->fromp(), گره_Var_Ref);
        اگر (!lvarrefp) {
            mergeEnd();
            بازگشت;
        }

        // RHS is a constant or a select
        ثابت AstConst* ثابت rconstp = VN_CAST(nodep->rhsp(), Const);
        ثابت Ast_گره_Sel* ثابت rselp = VN_CAST(nodep->rhsp(), گره_Sel);
        ثابت Ast_گره_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        اگر (rconstp) {  // Ok
        } وگرنه اگر (rselp) {
            ثابت AstConst* ثابت rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), گره_Var_Ref);
            اگر (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                بازگشت;
            }
            rindex = rbitp->toUInt();
        } وگرنه {
            mergeEnd();
            بازگشت;
        }

        اگر (m_mgSelLp) {  // Old merge
            اگر (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                اگر (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } وگرنه اگر (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.افزودن_back(nodep);
                m_mgNextp = nodep->nextp();
                بازگشت;
            } وگرنه {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.افزودن_back(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mgOffset = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mgIndexLo = lindex;
        m_mgIndexHi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mgOffset << nodep);
    }
    تهی visit(Ast_عبارت_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    تهی visit(AstVar*) override {}  // Accelerate
    تهی visit(Ast_گره_عبارت*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_اقلام);
    }
};

//######################################################################
// Reloop class functions

تهی V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("reloop", 0, dump_درخت_Either_Level() >= 6);
}
