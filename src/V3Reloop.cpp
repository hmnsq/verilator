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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Reloop.h"

#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ ReloopVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    અચલ VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_વસ્તુઓ;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_ગાંઠ_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    અચલ Ast_ગાંઠ* m_mgNextp = nullptr;  // Next node
    અચલ Ast_ગાંઠ_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    અચલ Ast_ગાંઠ_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    અચલ Ast_ગાંઠ_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    અચલ Ast_ગાંઠ_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    અચલ AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    સ્થિર AstVar* createVarTemp(File_લીટી* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        અચલ string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* અચલ varp
            = નવું AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        પાછા varp;
    }
    શૂન્ય mergeEnd() {
        જો (!m_mgAssignps.empty()) {
            અચલ uint32_t વસ્તુઓ = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << વસ્તુઓ << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            જો (વસ્તુઓ >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << વસ્તુઓ << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_વસ્તુઓ += વસ્તુઓ;

                // Transform first assign into for loop body
                Ast_ગાંઠ_Assign* અચલ bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_લીટી* અચલ fl = bodyp->fileline();
                AstVar* અચલ itp = createVarTemp(fl, m_mgCfuncp);

                જો (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_ગાંઠ* અચલ initp = નવું AstAssign{fl, નવું AstVarRef{fl, itp, VAccess::WRITE},
                                                     નવું AstConst{fl, m_mgIndexLo}};
                Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstLte{fl, નવું AstVarRef{fl, itp, VAccess::READ},
                                                      નવું AstConst{fl, m_mgIndexHi}};
                Ast_ગાંઠ* અચલ incp = નવું AstAssign{
                    fl, નવું AstVarRef{fl, itp, VAccess::WRITE},
                    નવું AstAdd{fl, નવું AstConst{fl, 1}, નવું AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* અચલ loopp = નવું AstLoop{fl};
                loopp->addStmtsp(નવું AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_ગાંઠ_પદાવલિ* અચલ offsetp
                    = m_mgOffset == 0 ? nullptr : નવું AstConst(fl, std::abs(m_mgOffset));
                Ast_ગાંઠ_પદાવલિ* અચલ lbitp = m_mgSelLp->bitp();
                Ast_ગાંઠ_પદાવલિ* અચલ lvrefp = નવું AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? નવું AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_વૃક્ષ(), lbitp);
                જો (m_mgSelRp) {  // else constant and no replace
                    Ast_ગાંઠ_પદાવલિ* અચલ rbitp = m_mgSelRp->bitp();
                    Ast_ગાંઠ_પદાવલિ* અચલ rvrefp = નવું AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? નવું AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_વૃક્ષ(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                માટે (Ast_ગાંઠ_Assign* assp : m_mgAssignps) {
                    જો (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_વૃક્ષ(), assp);
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
    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (!m_cfuncp) પાછા;

        // Left select WordSel or ArraySel
        Ast_ગાંઠ_Sel* અચલ lselp = VN_CAST(nodep->lhsp(), ગાંઠ_Sel);
        જો (!lselp) {  // Not ever merged
            mergeEnd();
            પાછા;
        }
        // Of a constant index
        અચલ AstConst* અચલ lbitp = VN_CAST(lselp->bitp(), Const);
        જો (!lbitp) {
            mergeEnd();
            પાછા;
        }
        જો (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            પાછા;
        }
        અચલ uint32_t lindex = lbitp->toUInt();
        // Of variable
        અચલ Ast_ગાંઠ_Var_Ref* અચલ lvarrefp = VN_CAST(lselp->fromp(), ગાંઠ_Var_Ref);
        જો (!lvarrefp) {
            mergeEnd();
            પાછા;
        }

        // RHS is a constant or a select
        અચલ AstConst* અચલ rconstp = VN_CAST(nodep->rhsp(), Const);
        અચલ Ast_ગાંઠ_Sel* અચલ rselp = VN_CAST(nodep->rhsp(), ગાંઠ_Sel);
        અચલ Ast_ગાંઠ_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        જો (rconstp) {  // Ok
        } નહીંતર જો (rselp) {
            અચલ AstConst* અચલ rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), ગાંઠ_Var_Ref);
            જો (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                પાછા;
            }
            rindex = rbitp->toUInt();
        } નહીંતર {
            mergeEnd();
            પાછા;
        }

        જો (m_mgSelLp) {  // Old merge
            જો (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                જો (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } નહીંતર જો (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.ધકેલો_back(nodep);
                m_mgNextp = nodep->nextp();
                પાછા;
            } નહીંતર {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.ધકેલો_back(nodep);
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
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    શૂન્ય visit(AstVar*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_વસ્તુઓ);
    }
};

//######################################################################
// Reloop class functions

શૂન્ય V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("reloop", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
