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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Reloop.h"

#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ ReloopVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_ವಸ್ತುಗಳು;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_ಗಂಟು_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    ಧ್ರುವ Ast_ಗಂಟು* m_mgNextp = nullptr;  // Next node
    ಧ್ರುವ Ast_ಗಂಟು_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    ಧ್ರುವ Ast_ಗಂಟು_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    ಧ್ರುವ AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    ಸ್ಥಿರ AstVar* createVarTemp(File_ಸಾಲು* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ಧ್ರುವ string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* ಧ್ರುವ varp
            = ನೂತನ AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        ಪ್ರತ್ಯಾಗಮ varp;
    }
    ಶೂನ್ಯ mergeEnd() {
        ಯದಿ (!m_mgAssignps.empty()) {
            ಧ್ರುವ uint32_t ವಸ್ತುಗಳು = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << ವಸ್ತುಗಳು << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            ಯದಿ (ವಸ್ತುಗಳು >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << ವಸ್ತುಗಳು << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_ವಸ್ತುಗಳು += ವಸ್ತುಗಳು;

                // Transform first assign into for loop body
                Ast_ಗಂಟು_Assign* ಧ್ರುವ bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_ಸಾಲು* ಧ್ರುವ fl = bodyp->fileline();
                AstVar* ಧ್ರುವ itp = createVarTemp(fl, m_mgCfuncp);

                ಯದಿ (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_ಗಂಟು* ಧ್ರುವ initp = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, itp, VAccess::WRITE},
                                                     ನೂತನ AstConst{fl, m_mgIndexLo}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstLte{fl, ನೂತನ AstVarRef{fl, itp, VAccess::READ},
                                                      ನೂತನ AstConst{fl, m_mgIndexHi}};
                Ast_ಗಂಟು* ಧ್ರುವ incp = ನೂತನ AstAssign{
                    fl, ನೂತನ AstVarRef{fl, itp, VAccess::WRITE},
                    ನೂತನ AstAdd{fl, ನೂತನ AstConst{fl, 1}, ನೂತನ AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{fl};
                loopp->addStmtsp(ನೂತನ AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ offsetp
                    = m_mgOffset == 0 ? nullptr : ನೂತನ AstConst(fl, std::abs(m_mgOffset));
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lbitp = m_mgSelLp->bitp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lvrefp = ನೂತನ AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? ನೂತನ AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_ವೃಕ್ಷ(), lbitp);
                ಯದಿ (m_mgSelRp) {  // else constant and no replace
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rbitp = m_mgSelRp->bitp();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rvrefp = ನೂತನ AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? ನೂತನ AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_ವೃಕ್ಷ(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                ಪ್ರತಿ (Ast_ಗಂಟು_Assign* assp : m_mgAssignps) {
                    ಯದಿ (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_ವೃಕ್ಷ(), assp);
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
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (!m_cfuncp) ಪ್ರತ್ಯಾಗಮ;

        // Left select WordSel or ArraySel
        Ast_ಗಂಟು_Sel* ಧ್ರುವ lselp = VN_CAST(nodep->lhsp(), ಗಂಟು_Sel);
        ಯದಿ (!lselp) {  // Not ever merged
            mergeEnd();
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Of a constant index
        ಧ್ರುವ AstConst* ಧ್ರುವ lbitp = VN_CAST(lselp->bitp(), Const);
        ಯದಿ (!lbitp) {
            mergeEnd();
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ uint32_t lindex = lbitp->toUInt();
        // Of variable
        ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ lvarrefp = VN_CAST(lselp->fromp(), ಗಂಟು_Var_Ref);
        ಯದಿ (!lvarrefp) {
            mergeEnd();
            ಪ್ರತ್ಯಾಗಮ;
        }

        // RHS is a constant or a select
        ಧ್ರುವ AstConst* ಧ್ರುವ rconstp = VN_CAST(nodep->rhsp(), Const);
        ಧ್ರುವ Ast_ಗಂಟು_Sel* ಧ್ರುವ rselp = VN_CAST(nodep->rhsp(), ಗಂಟು_Sel);
        ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        ಯದಿ (rconstp) {  // Ok
        } ಅನ್ಯಥಾ ಯದಿ (rselp) {
            ಧ್ರುವ AstConst* ಧ್ರುವ rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), ಗಂಟು_Var_Ref);
            ಯದಿ (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                ಪ್ರತ್ಯಾಗಮ;
            }
            rindex = rbitp->toUInt();
        } ಅನ್ಯಥಾ {
            mergeEnd();
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (m_mgSelLp) {  // Old merge
            ಯದಿ (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                ಯದಿ (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } ಅನ್ಯಥಾ ಯದಿ (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.ತಳ್ಳು_back(nodep);
                m_mgNextp = nodep->nextp();
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.ತಳ್ಳು_back(nodep);
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
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    ಶೂನ್ಯ visit(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_ವಸ್ತುಗಳು);
    }
};

//######################################################################
// Reloop class functions

ಶೂನ್ಯ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("reloop", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
