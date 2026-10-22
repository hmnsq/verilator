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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Reloop.h"

#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

বর্গ ReloopVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    ধ্রুব VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_বস্তুসমূহ;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_গ্রন্থি_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    ধ্রুব Ast_গ্রন্থি* m_mgNextp = nullptr;  // Next node
    ধ্রুব Ast_গ্রন্থি_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    ধ্রুব Ast_গ্রন্থি_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    ধ্রুব Ast_গ্রন্থি_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    ধ্রুব Ast_গ্রন্থি_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    ধ্রুব AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    স্থির AstVar* createVarTemp(File_সারি* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ধ্রুব string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* ধ্রুব varp
            = নূতন AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        প্রত্যাগম varp;
    }
    শূন্য mergeEnd() {
        যদি (!m_mgAssignps.empty()) {
            ধ্রুব uint32_t বস্তুসমূহ = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << বস্তুসমূহ << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            যদি (বস্তুসমূহ >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << বস্তুসমূহ << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_বস্তুসমূহ += বস্তুসমূহ;

                // Transform first assign into for loop body
                Ast_গ্রন্থি_Assign* ধ্রুব bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_সারি* ধ্রুব fl = bodyp->fileline();
                AstVar* ধ্রুব itp = createVarTemp(fl, m_mgCfuncp);

                যদি (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_গ্রন্থি* ধ্রুব initp = নূতন AstAssign{fl, নূতন AstVarRef{fl, itp, VAccess::WRITE},
                                                     নূতন AstConst{fl, m_mgIndexLo}};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = নূতন AstLte{fl, নূতন AstVarRef{fl, itp, VAccess::READ},
                                                      নূতন AstConst{fl, m_mgIndexHi}};
                Ast_গ্রন্থি* ধ্রুব incp = নূতন AstAssign{
                    fl, নূতন AstVarRef{fl, itp, VAccess::WRITE},
                    নূতন AstAdd{fl, নূতন AstConst{fl, 1}, নূতন AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ধ্রুব loopp = নূতন AstLoop{fl};
                loopp->addStmtsp(নূতন AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব offsetp
                    = m_mgOffset == 0 ? nullptr : নূতন AstConst(fl, std::abs(m_mgOffset));
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব lbitp = m_mgSelLp->bitp();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব lvrefp = নূতন AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? নূতন AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_বৃক্ষ(), lbitp);
                যদি (m_mgSelRp) {  // else constant and no replace
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব rbitp = m_mgSelRp->bitp();
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব rvrefp = নূতন AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? নূতন AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_বৃক্ষ(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                প্রতি (Ast_গ্রন্থি_Assign* assp : m_mgAssignps) {
                    যদি (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_বৃক্ষ(), assp);
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
    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (!m_cfuncp) প্রত্যাগম;

        // Left select WordSel or ArraySel
        Ast_গ্রন্থি_Sel* ধ্রুব lselp = VN_CAST(nodep->lhsp(), গ্রন্থি_Sel);
        যদি (!lselp) {  // Not ever merged
            mergeEnd();
            প্রত্যাগম;
        }
        // Of a constant index
        ধ্রুব AstConst* ধ্রুব lbitp = VN_CAST(lselp->bitp(), Const);
        যদি (!lbitp) {
            mergeEnd();
            প্রত্যাগম;
        }
        যদি (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            প্রত্যাগম;
        }
        ধ্রুব uint32_t lindex = lbitp->toUInt();
        // Of variable
        ধ্রুব Ast_গ্রন্থি_Var_Ref* ধ্রুব lvarrefp = VN_CAST(lselp->fromp(), গ্রন্থি_Var_Ref);
        যদি (!lvarrefp) {
            mergeEnd();
            প্রত্যাগম;
        }

        // RHS is a constant or a select
        ধ্রুব AstConst* ধ্রুব rconstp = VN_CAST(nodep->rhsp(), Const);
        ধ্রুব Ast_গ্রন্থি_Sel* ধ্রুব rselp = VN_CAST(nodep->rhsp(), গ্রন্থি_Sel);
        ধ্রুব Ast_গ্রন্থি_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        যদি (rconstp) {  // Ok
        } অন্যথা যদি (rselp) {
            ধ্রুব AstConst* ধ্রুব rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), গ্রন্থি_Var_Ref);
            যদি (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                প্রত্যাগম;
            }
            rindex = rbitp->toUInt();
        } অন্যথা {
            mergeEnd();
            প্রত্যাগম;
        }

        যদি (m_mgSelLp) {  // Old merge
            যদি (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                যদি (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } অন্যথা যদি (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.ঠেলো_back(nodep);
                m_mgNextp = nodep->nextp();
                প্রত্যাগম;
            } অন্যথা {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.ঠেলো_back(nodep);
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
    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    শূন্য visit(AstVar*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_বস্তুসমূহ);
    }
};

//######################################################################
// Reloop class functions

শূন্য V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("reloop", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
