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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Reloop.h"

#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு ReloopVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    மாறிலி VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_பொருட்கள்;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_முனை_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    மாறிலி Ast_முனை* m_mgNextp = nullptr;  // Next node
    மாறிலி Ast_முனை_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    மாறிலி Ast_முனை_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    மாறிலி Ast_முனை_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    மாறிலி Ast_முனை_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    மாறிலி AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    நிலையான AstVar* createVarTemp(File_வரி* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        மாறிலி string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* மாறிலி varp
            = புதிய AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        திரும்பு varp;
    }
    வெறுமை mergeEnd() {
        எனில் (!m_mgAssignps.empty()) {
            மாறிலி uint32_t பொருட்கள் = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << பொருட்கள் << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            எனில் (பொருட்கள் >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << பொருட்கள் << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_பொருட்கள் += பொருட்கள்;

                // Transform first assign into for loop body
                Ast_முனை_Assign* மாறிலி bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_வரி* மாறிலி fl = bodyp->fileline();
                AstVar* மாறிலி itp = createVarTemp(fl, m_mgCfuncp);

                எனில் (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_முனை* மாறிலி initp = புதிய AstAssign{fl, புதிய AstVarRef{fl, itp, VAccess::WRITE},
                                                     புதிய AstConst{fl, m_mgIndexLo}};
                Ast_முனை_கோவை* மாறிலி condp = புதிய AstLte{fl, புதிய AstVarRef{fl, itp, VAccess::READ},
                                                      புதிய AstConst{fl, m_mgIndexHi}};
                Ast_முனை* மாறிலி incp = புதிய AstAssign{
                    fl, புதிய AstVarRef{fl, itp, VAccess::WRITE},
                    புதிய AstAdd{fl, புதிய AstConst{fl, 1}, புதிய AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* மாறிலி loopp = புதிய AstLoop{fl};
                loopp->addStmtsp(புதிய AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_முனை_கோவை* மாறிலி offsetp
                    = m_mgOffset == 0 ? nullptr : புதிய AstConst(fl, std::abs(m_mgOffset));
                Ast_முனை_கோவை* மாறிலி lbitp = m_mgSelLp->bitp();
                Ast_முனை_கோவை* மாறிலி lvrefp = புதிய AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? புதிய AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_மரம்(), lbitp);
                எனில் (m_mgSelRp) {  // else constant and no replace
                    Ast_முனை_கோவை* மாறிலி rbitp = m_mgSelRp->bitp();
                    Ast_முனை_கோவை* மாறிலி rvrefp = புதிய AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? புதிய AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_மரம்(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                க்கு (Ast_முனை_Assign* assp : m_mgAssignps) {
                    எனில் (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_மரம்(), assp);
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
    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (!m_cfuncp) திரும்பு;

        // Left select WordSel or ArraySel
        Ast_முனை_Sel* மாறிலி lselp = VN_CAST(nodep->lhsp(), முனை_Sel);
        எனில் (!lselp) {  // Not ever merged
            mergeEnd();
            திரும்பு;
        }
        // Of a constant index
        மாறிலி AstConst* மாறிலி lbitp = VN_CAST(lselp->bitp(), Const);
        எனில் (!lbitp) {
            mergeEnd();
            திரும்பு;
        }
        எனில் (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            திரும்பு;
        }
        மாறிலி uint32_t lindex = lbitp->toUInt();
        // Of variable
        மாறிலி Ast_முனை_Var_Ref* மாறிலி lvarrefp = VN_CAST(lselp->fromp(), முனை_Var_Ref);
        எனில் (!lvarrefp) {
            mergeEnd();
            திரும்பு;
        }

        // RHS is a constant or a select
        மாறிலி AstConst* மாறிலி rconstp = VN_CAST(nodep->rhsp(), Const);
        மாறிலி Ast_முனை_Sel* மாறிலி rselp = VN_CAST(nodep->rhsp(), முனை_Sel);
        மாறிலி Ast_முனை_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        எனில் (rconstp) {  // Ok
        } இல்லையெனில் எனில் (rselp) {
            மாறிலி AstConst* மாறிலி rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), முனை_Var_Ref);
            எனில் (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                திரும்பு;
            }
            rindex = rbitp->toUInt();
        } இல்லையெனில் {
            mergeEnd();
            திரும்பு;
        }

        எனில் (m_mgSelLp) {  // Old merge
            எனில் (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                எனில் (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } இல்லையெனில் எனில் (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.தள்ளு_back(nodep);
                m_mgNextp = nodep->nextp();
                திரும்பு;
            } இல்லையெனில் {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.தள்ளு_back(nodep);
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
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    வெறுமை visit(AstVar*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை_கோவை*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_பொருட்கள்);
    }
};

//######################################################################
// Reloop class functions

வெறுமை V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("reloop", 0, dump_மரம்_Either_Level() >= 6);
}
