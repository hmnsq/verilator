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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_वस्तूनि;  // Statistic tracking
    AstCFunc* m_cfuncp = शून्यसूचकः;  // Current block

    std::सदिशम्<Ast_ग्रन्थिः_नियोजय*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = शून्यसूचकः;  // Parent C function
    ध्रुव Ast_ग्रन्थिः* m_mgNextp = शून्यसूचकः;  // Next node
    ध्रुव Ast_ग्रन्थिः_Sel* m_mgSelLp = शून्यसूचकः;  // Parent select, nullptr = idle
    ध्रुव Ast_ग्रन्थिः_Sel* m_mgSelRp = शून्यसूचकः;  // Parent select, nullptr = constant
    ध्रुव Ast_ग्रन्थिः_Var_Ref* m_mgVarrefLp = शून्यसूचकः;  // Parent varref
    ध्रुव Ast_ग्रन्थिः_Var_Ref* m_mgVarrefRp = शून्यसूचकः;  // Parent varref, nullptr = constant
    int64_t m_mg_विस्थापनम् = 0;  // Index offset
    ध्रुव AstConst* m_mgConstRp = शून्यसूचकः;  // Parent RHS constant, nullptr = sel
    uint32_t m_mg_सूचकांक_Lo = 0;  // Merge range
    uint32_t m_mg_सूचकांक_Hi = 0;  // Merge range

    // METHODS

    स्थिर AstVar* create_Var_अस्थायी(File_पङ्क्तिः* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        ध्रुव string newvarname{"__Vilp" + std::गन्तव्यम्_string(cfuncp->user1Inc() + 1)};
        AstVar* ध्रुव varp
            = नूतन AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->सङ्कलय_Varsp(varp);
        प्रत्यागम varp;
    }
    शून्य merge_अन्तः() {
        यदि (!m_mgAssignps.रिक्तानि()) {
            ध्रुव uint32_t वस्तूनि = m_mg_सूचकांक_Hi - m_mg_सूचकांक_Lo + 1;
            UINFO(9, "End merge iter=" << वस्तूनि << " " << m_mg_सूचकांक_Hi << ":" << m_mg_सूचकांक_Lo << " "
                                       << m_mg_विस्थापनम् << " " << m_mgAssignps[0]);
            यदि (वस्तूनि >= static_cast<uint32_t>(v3Global.opt.reloop_सीमा())) {
                UINFO(6, "Reloop merging items=" << वस्तूनि << " " << m_mg_सूचकांक_Hi << ":"
                                                 << m_mg_सूचकांक_Lo << " " << m_mg_विस्थापनम् << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_वस्तूनि += वस्तूनि;

                // Transform first assign into for loop body
                Ast_ग्रन्थिः_नियोजय* ध्रुव bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_पङ्क्तिः* ध्रुव fl = bodyp->fileline();
                AstVar* ध्रुव itp = create_Var_अस्थायी(fl, m_mgCfuncp);

                यदि (m_mg_विस्थापनम् > 0) {
                    UASSERT_OBJ(m_mg_सूचकांक_Lo >= m_mg_विस्थापनम्, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mg_सूचकांक_Lo -= m_mg_विस्थापनम्;
                    m_mg_सूचकांक_Hi -= m_mg_विस्थापनम्;
                }

                Ast_ग्रन्थिः* ध्रुव initp = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, itp, VAccess::WRITE},
                                                     नूतन AstConst{fl, m_mg_सूचकांक_Lo}};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstLte{fl, नूतन AstVarRef{fl, itp, VAccess::READ},
                                                      नूतन AstConst{fl, m_mg_सूचकांक_Hi}};
                Ast_ग्रन्थिः* ध्रुव incp = नूतन Ast_नियोजय{
                    fl, नूतन AstVarRef{fl, itp, VAccess::WRITE},
                    नूतन Ast_सङ्कलय{fl, नूतन AstConst{fl, 1}, नूतन AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
                loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{fl, loopp, condp});
                initp->सङ्कलय_अग्र(loopp);
                bodyp->replaceWith(initp);
                loopp->सङ्कलय_Stmtsp(bodyp);
                loopp->सङ्कलय_Stmtsp(incp);

                // Replace constant index with new loop index
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव offsetp
                    = m_mg_विस्थापनम् == 0 ? शून्यसूचकः : नूतन AstConst(fl, std::निरपेक्षम्(m_mg_विस्थापनम्));
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lbitp = m_mgSelLp->bitp();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lvrefp = नूतन AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mg_विस्थापनम् > 0 ? नूतन Ast_सङ्कलय{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_वृक्षः(), lbitp);
                यदि (m_mgSelRp) {  // else constant and no replace
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rbitp = m_mgSelRp->bitp();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rvrefp = नूतन AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mg_विस्थापनम् < 0 ? नूतन Ast_सङ्कलय{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_वृक्षः(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                प्रति (Ast_ग्रन्थिः_नियोजय* assp : m_mgAssignps) {
                    यदि (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = शून्यसूचकः;
            m_mgSelRp = शून्यसूचकः;
            m_mgVarrefLp = शून्यसूचकः;
            m_mgVarrefRp = शून्यसूचकः;
            m_mg_विस्थापनम् = 0;
            m_mgConstRp = शून्यसूचकः;
        }
    }

    // VISITORS
    शून्य दर्शय(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        merge_अन्तः();  // Finish last pending merge, if any
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (!m_cfuncp) प्रत्यागम;

        // Left select WordSel or ArraySel
        Ast_ग्रन्थिः_Sel* ध्रुव lselp = VN_CAST(nodep->lhsp(), ग्रन्थिः_Sel);
        यदि (!lselp) {  // Not ever merged
            merge_अन्तः();
            प्रत्यागम;
        }
        // Of a constant index
        ध्रुव AstConst* ध्रुव lbitp = VN_CAST(lselp->bitp(), Const);
        यदि (!lbitp) {
            merge_अन्तः();
            प्रत्यागम;
        }
        यदि (lbitp->विस्तारः() > 32) {  // Assoc arrays can do this
            merge_अन्तः();
            प्रत्यागम;
        }
        ध्रुव uint32_t lindex = lbitp->गन्तव्यम्_UInt();
        // Of variable
        ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव lvarrefp = VN_CAST(lselp->fromp(), ग्रन्थिः_Var_Ref);
        यदि (!lvarrefp) {
            merge_अन्तः();
            प्रत्यागम;
        }

        // RHS is a constant or a select
        ध्रुव AstConst* ध्रुव rconstp = VN_CAST(nodep->rhsp(), Const);
        ध्रुव Ast_ग्रन्थिः_Sel* ध्रुव rselp = VN_CAST(nodep->rhsp(), ग्रन्थिः_Sel);
        ध्रुव Ast_ग्रन्थिः_Var_Ref* rvarrefp = शून्यसूचकः;
        uint32_t rindex = lindex;
        यदि (rconstp) {  // Ok
        } अन्यथा यदि (rselp) {
            ध्रुव AstConst* ध्रुव rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), ग्रन्थिः_Var_Ref);
            यदि (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                merge_अन्तः();
                प्रत्यागम;
            }
            rindex = rbitp->गन्तव्यम्_UInt();
        } अन्यथा {
            merge_अन्तः();
            प्रत्यागम;
        }

        यदि (m_mgSelLp) {  // Old merge
            यदि (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mg_सूचकांक_Lo - 1 || lindex == m_mg_सूचकांक_Hi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mg_विस्थापनम्)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                यदि (lindex == m_mg_सूचकांक_Lo - 1) {
                    m_mg_सूचकांक_Lo = lindex;
                } अन्यथा यदि (lindex == m_mg_सूचकांक_Hi + 1) {
                    m_mg_सूचकांक_Hi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mg_सूचकांक_Hi << ":" << m_mg_सूचकांक_Lo
                                             << " " << nodep);
                m_mgAssignps.प्रक्षिप_पृष्ठभागः(nodep);
                m_mgNextp = nodep->nextp();
                प्रत्यागम;
            } अन्यथा {
                UINFO(9, "End merge i=" << lindex << " " << m_mg_सूचकांक_Hi << ":" << m_mg_सूचकांक_Lo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                merge_अन्तः();
            }
        }

        // Merge start
        m_mgAssignps.प्रक्षिप_पृष्ठभागः(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mg_विस्थापनम् = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mg_सूचकांक_Lo = lindex;
        m_mg_सूचकांक_Hi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mg_विस्थापनम् << nodep);
    }
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    शून्य दर्शय(AstVar*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::सङ्कलय_Stat("Optimizations, Reloops", m_statReloops);
        V3Stats::सङ्कलय_Stat("Optimizations, Reloop iterations", m_stat_Re_वस्तूनि);
    }
};

//######################################################################
// Reloop class functions

शून्य V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("reloop", 0, dump_वृक्षः_Either_Level() >= 6);
}
