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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_ವಸ್ತುಗಳು;  // Statistic tracking
    AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current block

    std::ಸದಿಶ<Ast_ಗಂಟು_ನಿಯೋಜಿಸು*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = ಶೂನ್ಯಸೂಚಕ;  // Parent C function
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_mgNextp = ಶೂನ್ಯಸೂಚಕ;  // Next node
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Sel* m_mgSelLp = ಶೂನ್ಯಸೂಚಕ;  // Parent select, nullptr = idle
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Sel* m_mgSelRp = ಶೂನ್ಯಸೂಚಕ;  // Parent select, nullptr = constant
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* m_mgVarrefLp = ಶೂನ್ಯಸೂಚಕ;  // Parent varref
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* m_mgVarrefRp = ಶೂನ್ಯಸೂಚಕ;  // Parent varref, nullptr = constant
    int64_t m_mg_ಸ್ಥಾನಾಂತರ = 0;  // Index offset
    ಸ್ಥಿರಾಂಕ AstConst* m_mgConstRp = ಶೂನ್ಯಸೂಚಕ;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_ಸೂಚ್ಯಂಕ_Lo = 0;  // Merge range
    uint32_t m_mg_ಸೂಚ್ಯಂಕ_Hi = 0;  // Merge range

    // METHODS

    ಸ್ಥಿರ AstVar* create_Var_ತಾತ್ಕಾಲಿಕ(File_ಸಾಲು* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ಸ್ಥಿರಾಂಕ string newvarname{"__Vilp" + std::ಗಮ್ಯ_string(cfuncp->user1Inc() + 1)};
        AstVar* ಸ್ಥಿರಾಂಕ varp
            = ಹೊಸ AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->ಕೂಡಿಸು_Varsp(varp);
        ಹಿಂತಿರುಗಿ varp;
    }
    ಶೂನ್ಯ ವಿಲೀನ_ಕೊನೆ() {
        ಆದರೆ (!m_mgAssignps.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ uint32_t ವಸ್ತುಗಳು = m_mg_ಸೂಚ್ಯಂಕ_Hi - m_mg_ಸೂಚ್ಯಂಕ_Lo + 1;
            UINFO(9, "End merge iter=" << ವಸ್ತುಗಳು << " " << m_mg_ಸೂಚ್ಯಂಕ_Hi << ":" << m_mg_ಸೂಚ್ಯಂಕ_Lo << " "
                                       << m_mg_ಸ್ಥಾನಾಂತರ << " " << m_mgAssignps[0]);
            ಆದರೆ (ವಸ್ತುಗಳು >= static_cast<uint32_t>(v3Global.opt.reloop_ಎಲ್ಲೆ())) {
                UINFO(6, "Reloop merging items=" << ವಸ್ತುಗಳು << " " << m_mg_ಸೂಚ್ಯಂಕ_Hi << ":"
                                                 << m_mg_ಸೂಚ್ಯಂಕ_Lo << " " << m_mg_ಸ್ಥಾನಾಂತರ << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_ವಸ್ತುಗಳು += ವಸ್ತುಗಳು;

                // Transform first assign into for loop body
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = bodyp->fileline();
                AstVar* ಸ್ಥಿರಾಂಕ itp = create_Var_ತಾತ್ಕಾಲಿಕ(fl, m_mgCfuncp);

                ಆದರೆ (m_mg_ಸ್ಥಾನಾಂತರ > 0) {
                    UASSERT_OBJ(m_mg_ಸೂಚ್ಯಂಕ_Lo >= m_mg_ಸ್ಥಾನಾಂತರ, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_ಸೂಚ್ಯಂಕ_Lo -= m_mg_ಸ್ಥಾನಾಂತರ;
                    m_mg_ಸೂಚ್ಯಂಕ_Hi -= m_mg_ಸ್ಥಾನಾಂತರ;
                }

                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ initp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, itp, VAccess::WRITE},
                                                     ಹೊಸ AstConst{fl, m_mg_ಸೂಚ್ಯಂಕ_Lo}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstLte{fl, ಹೊಸ AstVarRef{fl, itp, VAccess::READ},
                                                      ಹೊಸ AstConst{fl, m_mg_ಸೂಚ್ಯಂಕ_Hi}};
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ incp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    fl, ಹೊಸ AstVarRef{fl, itp, VAccess::WRITE},
                    ಹೊಸ Ast_ಕೂಡಿಸು{fl, ಹೊಸ AstConst{fl, 1}, ಹೊಸ AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{fl};
                loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{fl, loopp, condp});
                initp->ಕೂಡಿಸು_ಮುಂದಿನ(loopp);
                bodyp->replaceWith(initp);
                loopp->ಕೂಡಿಸು_Stmtsp(bodyp);
                loopp->ಕೂಡಿಸು_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ offsetp
                    = m_mg_ಸ್ಥಾನಾಂತರ == 0 ? ಶೂನ್ಯಸೂಚಕ : ಹೊಸ AstConst(fl, std::ನಿರಪೇಕ್ಷ(m_mg_ಸ್ಥಾನಾಂತರ));
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lbitp = m_mgSelLp->bitp();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lvrefp = ಹೊಸ AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_ಸ್ಥಾನಾಂತರ > 0 ? ಹೊಸ Ast_ಕೂಡಿಸು{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_ವೃಕ್ಷ(), lbitp);
                ಆದರೆ (m_mgSelRp) {  // else constant and no replace
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rbitp = m_mgSelRp->bitp();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rvrefp = ಹೊಸ AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_ಸ್ಥಾನಾಂತರ < 0 ? ಹೊಸ Ast_ಕೂಡಿಸು{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_ವೃಕ್ಷ(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                ಗಾಗಿ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assp : m_mgAssignps) {
                    ಆದರೆ (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = ಶೂನ್ಯಸೂಚಕ;
            m_mgSelRp = ಶೂನ್ಯಸೂಚಕ;
            m_mgVarrefLp = ಶೂನ್ಯಸೂಚಕ;
            m_mgVarrefRp = ಶೂನ್ಯಸೂಚಕ;
            m_mg_ಸ್ಥಾನಾಂತರ = 0;
            m_mgConstRp = ಶೂನ್ಯಸೂಚಕ;
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        ವಿಲೀನ_ಕೊನೆ();  // Finish last pending merge, if any
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (!m_cfuncp) ಹಿಂತಿರುಗಿ;

        // Left select WordSel or ArraySel
        Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ lselp = VN_CAST(nodep->lhsp(), ಗಂಟು_Sel);
        ಆದರೆ (!lselp) {  // Not ever merged
            ವಿಲೀನ_ಕೊನೆ();
            ಹಿಂತಿರುಗಿ;
        }
        // Of a constant index
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lbitp = VN_CAST(lselp->bitp(), Const);
        ಆದರೆ (!lbitp) {
            ವಿಲೀನ_ಕೊನೆ();
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (lbitp->ಅಗಲ() > 32) {  // Assoc arrays can do this
            ವಿಲೀನ_ಕೊನೆ();
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ uint32_t lindex = lbitp->ಗಮ್ಯ_UInt();
        // Of variable
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ lvarrefp = VN_CAST(lselp->fromp(), ಗಂಟು_Var_Ref);
        ಆದರೆ (!lvarrefp) {
            ವಿಲೀನ_ಕೊನೆ();
            ಹಿಂತಿರುಗಿ;
        }

        // RHS is a constant or a select
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ rconstp = VN_CAST(nodep->rhsp(), Const);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ rselp = VN_CAST(nodep->rhsp(), ಗಂಟು_Sel);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* rvarrefp = ಶೂನ್ಯಸೂಚಕ;
        uint32_t rindex = lindex;
        ಆದರೆ (rconstp) {  // Ok
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rselp) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), ಗಂಟು_Var_Ref);
            ಆದರೆ (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                ವಿಲೀನ_ಕೊನೆ();
                ಹಿಂತಿರುಗಿ;
            }
            rindex = rbitp->ಗಮ್ಯ_UInt();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ವಿಲೀನ_ಕೊನೆ();
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (m_mgSelLp) {  // Old merge
            ಆದರೆ (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_ಸೂಚ್ಯಂಕ_Lo - 1 || lindex == m_mg_ಸೂಚ್ಯಂಕ_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mg_ಸ್ಥಾನಾಂತರ)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                ಆದರೆ (lindex == m_mg_ಸೂಚ್ಯಂಕ_Lo - 1) {
                    m_mg_ಸೂಚ್ಯಂಕ_Lo = lindex;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lindex == m_mg_ಸೂಚ್ಯಂಕ_Hi + 1) {
                    m_mg_ಸೂಚ್ಯಂಕ_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_ಸೂಚ್ಯಂಕ_Hi << ":" << m_mg_ಸೂಚ್ಯಂಕ_Lo
                                             << " " << nodep);
                m_mgAssignps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
                m_mgNextp = nodep->nextp();
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_ಸೂಚ್ಯಂಕ_Hi << ":" << m_mg_ಸೂಚ್ಯಂಕ_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                ವಿಲೀನ_ಕೊನೆ();
            }
        }

        // Merge start
        m_mgAssignps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_ಸ್ಥಾನಾಂತರ = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_ಸೂಚ್ಯಂಕ_Lo = lindex;
        m_mg_ಸೂಚ್ಯಂಕ_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_ಸ್ಥಾನಾಂತರ << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Reloop iterations", m_stat_Re_ವಸ್ತುಗಳು);
    }
};

//######################################################################
// Reloop class functions

ಶೂನ್ಯ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("reloop", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
