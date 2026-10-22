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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Reloop.h"

#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ ReloopVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_ବସ୍ତୁଗୁଡ଼ିକ;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_ଗଣ୍ଠି_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    ଅଚଳ Ast_ଗଣ୍ଠି* m_mgNextp = nullptr;  // Next node
    ଅଚଳ Ast_ଗଣ୍ଠି_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    ଅଚଳ Ast_ଗଣ୍ଠି_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    ଅଚଳ AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    ସ୍ଥିର AstVar* createVarTemp(File_ଧାଡ଼ି* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ଅଚଳ string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* ଅଚଳ varp
            = ନୂତନ AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        ଫେରନ୍ତୁ varp;
    }
    ଶୂନ୍ୟ mergeEnd() {
        ଯଦି (!m_mgAssignps.empty()) {
            ଅଚଳ uint32_t ବସ୍ତୁଗୁଡ଼ିକ = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << ବସ୍ତୁଗୁଡ଼ିକ << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            ଯଦି (ବସ୍ତୁଗୁଡ଼ିକ >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << ବସ୍ତୁଗୁଡ଼ିକ << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_ବସ୍ତୁଗୁଡ଼ିକ += ବସ୍ତୁଗୁଡ଼ିକ;

                // Transform first assign into for loop body
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_ଧାଡ଼ି* ଅଚଳ fl = bodyp->fileline();
                AstVar* ଅଚଳ itp = createVarTemp(fl, m_mgCfuncp);

                ଯଦି (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_ଗଣ୍ଠି* ଅଚଳ initp = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, itp, VAccess::WRITE},
                                                     ନୂତନ AstConst{fl, m_mgIndexLo}};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstLte{fl, ନୂତନ AstVarRef{fl, itp, VAccess::READ},
                                                      ନୂତନ AstConst{fl, m_mgIndexHi}};
                Ast_ଗଣ୍ଠି* ଅଚଳ incp = ନୂତନ AstAssign{
                    fl, ନୂତନ AstVarRef{fl, itp, VAccess::WRITE},
                    ନୂତନ AstAdd{fl, ନୂତନ AstConst{fl, 1}, ନୂତନ AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{fl};
                loopp->addStmtsp(ନୂତନ AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ offsetp
                    = m_mgOffset == 0 ? nullptr : ନୂତନ AstConst(fl, std::abs(m_mgOffset));
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lbitp = m_mgSelLp->bitp();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lvrefp = ନୂତନ AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? ନୂତନ AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_ବୃକ୍ଷ(), lbitp);
                ଯଦି (m_mgSelRp) {  // else constant and no replace
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rbitp = m_mgSelRp->bitp();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rvrefp = ନୂତନ AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? ନୂତନ AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_ବୃକ୍ଷ(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                ପାଇଁ (Ast_ଗଣ୍ଠି_Assign* assp : m_mgAssignps) {
                    ଯଦି (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_ବୃକ୍ଷ(), assp);
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
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (!m_cfuncp) ଫେରନ୍ତୁ;

        // Left select WordSel or ArraySel
        Ast_ଗଣ୍ଠି_Sel* ଅଚଳ lselp = VN_CAST(nodep->lhsp(), ଗଣ୍ଠି_Sel);
        ଯଦି (!lselp) {  // Not ever merged
            mergeEnd();
            ଫେରନ୍ତୁ;
        }
        // Of a constant index
        ଅଚଳ AstConst* ଅଚଳ lbitp = VN_CAST(lselp->bitp(), Const);
        ଯଦି (!lbitp) {
            mergeEnd();
            ଫେରନ୍ତୁ;
        }
        ଯଦି (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ uint32_t lindex = lbitp->toUInt();
        // Of variable
        ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ lvarrefp = VN_CAST(lselp->fromp(), ଗଣ୍ଠି_Var_Ref);
        ଯଦି (!lvarrefp) {
            mergeEnd();
            ଫେରନ୍ତୁ;
        }

        // RHS is a constant or a select
        ଅଚଳ AstConst* ଅଚଳ rconstp = VN_CAST(nodep->rhsp(), Const);
        ଅଚଳ Ast_ଗଣ୍ଠି_Sel* ଅଚଳ rselp = VN_CAST(nodep->rhsp(), ଗଣ୍ଠି_Sel);
        ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        ଯଦି (rconstp) {  // Ok
        } ଅନ୍ୟଥା ଯଦି (rselp) {
            ଅଚଳ AstConst* ଅଚଳ rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), ଗଣ୍ଠି_Var_Ref);
            ଯଦି (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                ଫେରନ୍ତୁ;
            }
            rindex = rbitp->toUInt();
        } ଅନ୍ୟଥା {
            mergeEnd();
            ଫେରନ୍ତୁ;
        }

        ଯଦି (m_mgSelLp) {  // Old merge
            ଯଦି (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                ଯଦି (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } ଅନ୍ୟଥା ଯଦି (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.ଠେଲ_back(nodep);
                m_mgNextp = nodep->nextp();
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.ଠେଲ_back(nodep);
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
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    ଶୂନ୍ୟ visit(AstVar*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_ବସ୍ତୁଗୁଡ଼ିକ);
    }
};

//######################################################################
// Reloop class functions

ଶୂନ୍ୟ V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("reloop", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
