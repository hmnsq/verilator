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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Reloop.h"

#समावेश "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी ReloopVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    स्थिर VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_वस्तुएँ;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_गाँठ_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    स्थिर Ast_गाँठ* m_mgNextp = nullptr;  // Next node
    स्थिर Ast_गाँठ_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    स्थिर Ast_गाँठ_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    स्थिर Ast_गाँठ_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    स्थिर Ast_गाँठ_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    स्थिर AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_सूचकांक_Lo = 0;  // Merge range
    uint32_t m_mg_सूचकांक_Hi = 0;  // Merge range

    // METHODS

    अटल AstVar* create_Var_अस्थायी(File_पंक्ति* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        स्थिर string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* स्थिर varp
            = नया AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        वापस varp;
    }
    व्योम mergeEnd() {
        अगर (!m_mgAssignps.empty()) {
            स्थिर uint32_t वस्तुएँ = m_mg_सूचकांक_Hi - m_mg_सूचकांक_Lo + 1;
            UINFO(9, "End merge iter=" << वस्तुएँ << " " << m_mg_सूचकांक_Hi << ":" << m_mg_सूचकांक_Lo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            अगर (वस्तुएँ >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << वस्तुएँ << " " << m_mg_सूचकांक_Hi << ":"
                                                 << m_mg_सूचकांक_Lo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_वस्तुएँ += वस्तुएँ;

                // Transform first assign into for loop body
                Ast_गाँठ_Assign* स्थिर bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_पंक्ति* स्थिर fl = bodyp->fileline();
                AstVar* स्थिर itp = create_Var_अस्थायी(fl, m_mgCfuncp);

                अगर (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mg_सूचकांक_Lo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_सूचकांक_Lo -= m_mgOffset;
                    m_mg_सूचकांक_Hi -= m_mgOffset;
                }

                Ast_गाँठ* स्थिर initp = नया AstAssign{fl, नया AstVarRef{fl, itp, VAccess::WRITE},
                                                     नया AstConst{fl, m_mg_सूचकांक_Lo}};
                Ast_गाँठ_व्यंजक* स्थिर condp = नया AstLte{fl, नया AstVarRef{fl, itp, VAccess::READ},
                                                      नया AstConst{fl, m_mg_सूचकांक_Hi}};
                Ast_गाँठ* स्थिर incp = नया AstAssign{
                    fl, नया AstVarRef{fl, itp, VAccess::WRITE},
                    नया AstAdd{fl, नया AstConst{fl, 1}, नया AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* स्थिर loopp = नया AstLoop{fl};
                loopp->addStmtsp(नया AstLoopTest{fl, loopp, condp});
                initp->add_अग्र(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_गाँठ_व्यंजक* स्थिर offsetp
                    = m_mgOffset == 0 ? nullptr : नया AstConst(fl, std::असल(m_mgOffset));
                Ast_गाँठ_व्यंजक* स्थिर lbitp = m_mgSelLp->bitp();
                Ast_गाँठ_व्यंजक* स्थिर lvrefp = नया AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? नया AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_वृक्ष(), lbitp);
                अगर (m_mgSelRp) {  // else constant and no replace
                    Ast_गाँठ_व्यंजक* स्थिर rbitp = m_mgSelRp->bitp();
                    Ast_गाँठ_व्यंजक* स्थिर rvrefp = नया AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? नया AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_वृक्ष(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                क्रम (Ast_गाँठ_Assign* assp : m_mgAssignps) {
                    अगर (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_वृक्ष(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.सफा();
            m_mgSelLp = nullptr;
            m_mgSelRp = nullptr;
            m_mgVarrefLp = nullptr;
            m_mgVarrefRp = nullptr;
            m_mgOffset = 0;
            m_mgConstRp = nullptr;
        }
    }

    // VISITORS
    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (!m_cfuncp) वापस;

        // Left select WordSel or ArraySel
        Ast_गाँठ_Sel* स्थिर lselp = VN_CAST(nodep->lhsp(), गाँठ_Sel);
        अगर (!lselp) {  // Not ever merged
            mergeEnd();
            वापस;
        }
        // Of a constant index
        स्थिर AstConst* स्थिर lbitp = VN_CAST(lselp->bitp(), Const);
        अगर (!lbitp) {
            mergeEnd();
            वापस;
        }
        अगर (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            वापस;
        }
        स्थिर uint32_t lindex = lbitp->toUInt();
        // Of variable
        स्थिर Ast_गाँठ_Var_Ref* स्थिर lvarrefp = VN_CAST(lselp->fromp(), गाँठ_Var_Ref);
        अगर (!lvarrefp) {
            mergeEnd();
            वापस;
        }

        // RHS is a constant or a select
        स्थिर AstConst* स्थिर rconstp = VN_CAST(nodep->rhsp(), Const);
        स्थिर Ast_गाँठ_Sel* स्थिर rselp = VN_CAST(nodep->rhsp(), गाँठ_Sel);
        स्थिर Ast_गाँठ_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        अगर (rconstp) {  // Ok
        } अन्यथा अगर (rselp) {
            स्थिर AstConst* स्थिर rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), गाँठ_Var_Ref);
            अगर (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                वापस;
            }
            rindex = rbitp->toUInt();
        } अन्यथा {
            mergeEnd();
            वापस;
        }

        अगर (m_mgSelLp) {  // Old merge
            अगर (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_सूचकांक_Lo - 1 || lindex == m_mg_सूचकांक_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                अगर (lindex == m_mg_सूचकांक_Lo - 1) {
                    m_mg_सूचकांक_Lo = lindex;
                } अन्यथा अगर (lindex == m_mg_सूचकांक_Hi + 1) {
                    m_mg_सूचकांक_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_सूचकांक_Hi << ":" << m_mg_सूचकांक_Lo
                                             << " " << nodep);
                m_mgAssignps.धकेलो_back(nodep);
                m_mgNextp = nodep->nextp();
                वापस;
            } अन्यथा {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_सूचकांक_Hi << ":" << m_mg_सूचकांक_Lo
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
        m_mg_सूचकांक_Lo = lindex;
        m_mg_सूचकांक_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mgOffset << nodep);
    }
    व्योम visit(Ast_व्यंजक_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    व्योम visit(AstVar*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ_व्यंजक*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_वस्तुएँ);
    }
};

//######################################################################
// Reloop class functions

व्योम V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("reloop", 0, dump_वृक्ष_Either_Level() >= 6);
}
