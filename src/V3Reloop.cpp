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
    ثابت VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_عناصر;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_عقدة_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    ثابت Ast_عقدة* m_mgNextp = nullptr;  // Next node
    ثابت Ast_عقدة_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    ثابت Ast_عقدة_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    ثابت Ast_عقدة_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    ثابت Ast_عقدة_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    ثابت AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    ساكن AstVar* createVarTemp(File_سطر* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ثابت string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* ثابت varp
            = جديد AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        إرجاع varp;
    }
    فراغ mergeEnd() {
        إذا (!m_mgAssignps.empty()) {
            ثابت uint32_t عناصر = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << عناصر << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            إذا (عناصر >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << عناصر << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_عناصر += عناصر;

                // Transform first assign into for loop body
                Ast_عقدة_Assign* ثابت bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_سطر* ثابت fl = bodyp->fileline();
                AstVar* ثابت itp = createVarTemp(fl, m_mgCfuncp);

                إذا (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_عقدة* ثابت initp = جديد AstAssign{fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                                                     جديد AstConst{fl, m_mgIndexLo}};
                Ast_عقدة_تعبير* ثابت condp = جديد AstLte{fl, جديد AstVarRef{fl, itp, VAccess::READ},
                                                      جديد AstConst{fl, m_mgIndexHi}};
                Ast_عقدة* ثابت incp = جديد AstAssign{
                    fl, جديد AstVarRef{fl, itp, VAccess::WRITE},
                    جديد AstAdd{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ثابت loopp = جديد AstLoop{fl};
                loopp->addStmtsp(جديد AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_عقدة_تعبير* ثابت offsetp
                    = m_mgOffset == 0 ? nullptr : جديد AstConst(fl, std::abs(m_mgOffset));
                Ast_عقدة_تعبير* ثابت lbitp = m_mgSelLp->bitp();
                Ast_عقدة_تعبير* ثابت lvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? جديد AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_شجرة(), lbitp);
                إذا (m_mgSelRp) {  // else constant and no replace
                    Ast_عقدة_تعبير* ثابت rbitp = m_mgSelRp->bitp();
                    Ast_عقدة_تعبير* ثابت rvrefp = جديد AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? جديد AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_شجرة(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                لكل (Ast_عقدة_Assign* assp : m_mgAssignps) {
                    إذا (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_شجرة(), assp);
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
    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    فراغ visit(Ast_عقدة_Assign* nodep) override {
        إذا (!m_cfuncp) إرجاع;

        // Left select WordSel or ArraySel
        Ast_عقدة_Sel* ثابت lselp = VN_CAST(nodep->lhsp(), عقدة_Sel);
        إذا (!lselp) {  // Not ever merged
            mergeEnd();
            إرجاع;
        }
        // Of a constant index
        ثابت AstConst* ثابت lbitp = VN_CAST(lselp->bitp(), Const);
        إذا (!lbitp) {
            mergeEnd();
            إرجاع;
        }
        إذا (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            إرجاع;
        }
        ثابت uint32_t lindex = lbitp->toUInt();
        // Of variable
        ثابت Ast_عقدة_Var_Ref* ثابت lvarrefp = VN_CAST(lselp->fromp(), عقدة_Var_Ref);
        إذا (!lvarrefp) {
            mergeEnd();
            إرجاع;
        }

        // RHS is a constant or a select
        ثابت AstConst* ثابت rconstp = VN_CAST(nodep->rhsp(), Const);
        ثابت Ast_عقدة_Sel* ثابت rselp = VN_CAST(nodep->rhsp(), عقدة_Sel);
        ثابت Ast_عقدة_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        إذا (rconstp) {  // Ok
        } وإلا إذا (rselp) {
            ثابت AstConst* ثابت rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), عقدة_Var_Ref);
            إذا (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                إرجاع;
            }
            rindex = rbitp->toUInt();
        } وإلا {
            mergeEnd();
            إرجاع;
        }

        إذا (m_mgSelLp) {  // Old merge
            إذا (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                إذا (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } وإلا إذا (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.ادفع_back(nodep);
                m_mgNextp = nodep->nextp();
                إرجاع;
            } وإلا {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.ادفع_back(nodep);
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
    فراغ visit(Ast_تعبير_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    فراغ visit(AstVar*) override {}  // Accelerate
    فراغ visit(Ast_عقدة_تعبير*) override {}  // Accelerate
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_عناصر);
    }
};

//######################################################################
// Reloop class functions

فراغ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("reloop", 0, dump_شجرة_Either_Level() >= 6);
}
