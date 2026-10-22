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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Reloop.h"

#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग ReloopVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    ध्रुव VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_वस्तुएँ;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_गाँठ_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    ध्रुव Ast_गाँठ* m_mgNextp = nullptr;  // Next node
    ध्रुव Ast_गाँठ_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    ध्रुव Ast_गाँठ_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    ध्रुव Ast_गाँठ_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    ध्रुव Ast_गाँठ_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    ध्रुव AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    स्थिर AstVar* createVarTemp(File_पंक्ति* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ध्रुव string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* ध्रुव varp
            = नूतन AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        प्रत्यागम varp;
    }
    शून्य mergeEnd() {
        यदि (!m_mgAssignps.empty()) {
            ध्रुव uint32_t वस्तुएँ = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << वस्तुएँ << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            यदि (वस्तुएँ >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << वस्तुएँ << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_वस्तुएँ += वस्तुएँ;

                // Transform first assign into for loop body
                Ast_गाँठ_Assign* ध्रुव bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_पंक्ति* ध्रुव fl = bodyp->fileline();
                AstVar* ध्रुव itp = createVarTemp(fl, m_mgCfuncp);

                यदि (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_गाँठ* ध्रुव initp = नूतन AstAssign{fl, नूतन AstVarRef{fl, itp, VAccess::WRITE},
                                                     नूतन AstConst{fl, m_mgIndexLo}};
                Ast_गाँठ_व्यंजक* ध्रुव condp = नूतन AstLte{fl, नूतन AstVarRef{fl, itp, VAccess::READ},
                                                      नूतन AstConst{fl, m_mgIndexHi}};
                Ast_गाँठ* ध्रुव incp = नूतन AstAssign{
                    fl, नूतन AstVarRef{fl, itp, VAccess::WRITE},
                    नूतन AstAdd{fl, नूतन AstConst{fl, 1}, नूतन AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
                loopp->addStmtsp(नूतन AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_गाँठ_व्यंजक* ध्रुव offsetp
                    = m_mgOffset == 0 ? nullptr : नूतन AstConst(fl, std::abs(m_mgOffset));
                Ast_गाँठ_व्यंजक* ध्रुव lbitp = m_mgSelLp->bitp();
                Ast_गाँठ_व्यंजक* ध्रुव lvrefp = नूतन AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? नूतन AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_वृक्ष(), lbitp);
                यदि (m_mgSelRp) {  // else constant and no replace
                    Ast_गाँठ_व्यंजक* ध्रुव rbitp = m_mgSelRp->bitp();
                    Ast_गाँठ_व्यंजक* ध्रुव rvrefp = नूतन AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? नूतन AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_वृक्ष(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                प्रति (Ast_गाँठ_Assign* assp : m_mgAssignps) {
                    यदि (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_वृक्ष(), assp);
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
    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    शून्य visit(Ast_गाँठ_Assign* nodep) override {
        यदि (!m_cfuncp) प्रत्यागम;

        // Left select WordSel or ArraySel
        Ast_गाँठ_Sel* ध्रुव lselp = VN_CAST(nodep->lhsp(), गाँठ_Sel);
        यदि (!lselp) {  // Not ever merged
            mergeEnd();
            प्रत्यागम;
        }
        // Of a constant index
        ध्रुव AstConst* ध्रुव lbitp = VN_CAST(lselp->bitp(), Const);
        यदि (!lbitp) {
            mergeEnd();
            प्रत्यागम;
        }
        यदि (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            प्रत्यागम;
        }
        ध्रुव uint32_t lindex = lbitp->toUInt();
        // Of variable
        ध्रुव Ast_गाँठ_Var_Ref* ध्रुव lvarrefp = VN_CAST(lselp->fromp(), गाँठ_Var_Ref);
        यदि (!lvarrefp) {
            mergeEnd();
            प्रत्यागम;
        }

        // RHS is a constant or a select
        ध्रुव AstConst* ध्रुव rconstp = VN_CAST(nodep->rhsp(), Const);
        ध्रुव Ast_गाँठ_Sel* ध्रुव rselp = VN_CAST(nodep->rhsp(), गाँठ_Sel);
        ध्रुव Ast_गाँठ_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        यदि (rconstp) {  // Ok
        } अन्यथा यदि (rselp) {
            ध्रुव AstConst* ध्रुव rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), गाँठ_Var_Ref);
            यदि (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                प्रत्यागम;
            }
            rindex = rbitp->toUInt();
        } अन्यथा {
            mergeEnd();
            प्रत्यागम;
        }

        यदि (m_mgSelLp) {  // Old merge
            यदि (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                यदि (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } अन्यथा यदि (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.धकेलो_back(nodep);
                m_mgNextp = nodep->nextp();
                प्रत्यागम;
            } अन्यथा {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.धकेलो_back(nodep);
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
    शून्य visit(Ast_व्यंजक_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(Ast_गाँठ_व्यंजक*) override {}  // Accelerate
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_वस्तुएँ);
    }
};

//######################################################################
// Reloop class functions

शून्य V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("reloop", 0, dump_वृक्ष_Either_Level() >= 6);
}
