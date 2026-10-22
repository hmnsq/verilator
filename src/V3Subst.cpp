// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Substitute constants and expressions in expr temp's
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Subst's Transformations:
//
// Each module:
//      Search all ASSIGN(WORDSEL(...)) and build what it's assigned to
//      Later usages of that word may then be replaced as long as
//      the RHS hasn't changed value.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Subst.h"

#ضمن "V3Const.h"
#ضمن "V3Stats.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

صنف Subst_Var_ثبت final {
    ملګری صنف SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    بنية ریکارډ_2 final {
        // MEMBERS
        Ast_غوټه_ټاکل* m_assignp;  // Last assignment to this part
        uint32_t m_ګام;  // Step number of last assignment. 0 means invalid entry.
        منطقي m_کارول_شوی;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        ریکارډ_2() { invalidate(); }
        // METHODS
        فراغ invalidate() {
            m_assignp = تش_اشاره;
            m_ګام = 0;
            m_کارول_شوی = خطأ;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ثابت m_varp;
    // The recrod for whole variable tracking
    ریکارډ_2 m_ټولیز_ریکارډ;
    // A record for each word in the variable
    std::ویکټور<ریکارډ_2> m_کلمه_ریکارډونه{static_cast<اندازه_t>(m_varp->پلنوالی_ټکي()), ریکارډ_2{}};

    // METHDOS
    فراغ delete_Assignment_If_ناکارول_شوی(ریکارډ_2& ریکارډ, اندازه_t& n_ټاکل_Deleted) {
        إذا (!ریکارډ.m_assignp) أرجع;
        إذا (ریکارډ.m_کارول_شوی) أرجع;
        ++n_ټاکل_Deleted;
        VL_DO_DANGLING(ریکارډ.m_assignp->unlink_Fr_شا()->delete_ونه(), ریکارډ.m_assignp);
    }

    Ast_غوټه_بیان* subst_ریکارډ(ریکارډ_2& ریکارډ);

عام:
    // CONSTRUCTORS
    څرګند Subst_Var_ثبت(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_ثبت() = افتراضي;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    فراغ ټاکل_ټولیز(Ast_غوټه_ټاکل* assp, uint32_t ګام) {
        // Invalidate all word records
        لكل (ریکارډ_2& کلمه_ریکارډ : m_کلمه_ریکارډونه) کلمه_ریکارډ.invalidate();
        // Set whole record
        m_ټولیز_ریکارډ.m_assignp = assp;
        m_ټولیز_ریکارډ.m_ګام = ګام;
        m_ټولیز_ریکارډ.m_کارول_شوی = خطأ;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    فراغ ټاکل_کلمه(Ast_غوټه_ټاکل* assp, uint32_t ګام, uint32_t کلمه) {
        // Invalidate whole record
        m_ټولیز_ریکارډ.invalidate();
        // Set word record
        ریکارډ_2& کلمه_ریکارډ = m_کلمه_ریکارډونه[کلمه];
        کلمه_ریکارډ.m_assignp = assp;
        کلمه_ریکارډ.m_ګام = ګام;
        کلمه_ریکارډ.m_کارول_شوی = خطأ;
    }

    // Mark the whole variable as used (value consumed)
    فراغ کارول_شوی_ټولیز() {
        m_ټولیز_ریکارډ.m_کارول_شوی = ریښتیا;
        لكل (ریکارډ_2& کلمه_ریکارډ : m_کلمه_ریکارډونه) کلمه_ریکارډ.m_کارول_شوی = ریښتیا;
    }

    // Mark the specific word as used (value consumed)
    فراغ کارول_شوی_کلمه(uint32_t کلمه) {
        m_ټولیز_ریکارډ.m_کارول_شوی = ریښتیا;
        m_کلمه_ریکارډونه[کلمه].m_کارول_شوی = ریښتیا;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_غوټه_بیان* subst_ټولیز() { أرجع subst_ریکارډ(m_ټولیز_ریکارډ); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_غوټه_بیان* subst_کلمه(uint32_t کلمه) { أرجع subst_ریکارډ(m_کلمه_ریکارډونه[کلمه]); }

    فراغ delete_ناکارول_شوی_Assignments(اندازه_t& n_کلمه_ټاکل_Deleted, اندازه_t& n_ټولیز_ټاکل_Deleted) {
        // Delete assignments to temporaries if they are not used
        إذا (!m_varp->is_Statement_لنډمهاله()) أرجع;
        لكل (ریکارډ_2& کلمه_ریکارډ : m_کلمه_ریکارډونه)
            delete_Assignment_If_ناکارول_شوی(کلمه_ریکارډ, n_کلمه_ټاکل_Deleted);
        delete_Assignment_If_ناکارول_شوی(m_ټولیز_ریکارډ, n_ټولیز_ټاکل_Deleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

صنف SubstValidVisitor final : عام VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ثابت uint32_t m_ګام;  // Step number where assignment was recorded
    منطقي m_valid = ریښتیا;  // Is the expression we are considering to substitute valid

    // METHODS
    Subst_Var_ثبت& get_ثبت(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        أرجع *nodep->varp()->user1u().منزل<Subst_Var_ثبت*>();
    }

    // VISITORS
    فراغ لیدنه(Ast_کلمه_Sel* nodep) override {
        إذا (!m_valid) أرجع;

        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            إذا (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_ثبت& ثبت = get_ثبت(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                إذا (m_ګام < ثبت.m_ټولیز_ریکارډ.m_ګام) m_valid = خطأ;
                إذا (m_ګام < ثبت.m_کلمه_ریکارډونه[idxp->منزل_UInt()].m_ګام) m_valid = خطأ;
                أرجع;
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (!m_valid) أرجع;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ثابت Subst_Var_ثبت& ثبت = get_ثبت(nodep);
        إذا (m_ګام < ثبت.m_ټولیز_ریکارډ.m_ګام) {
            m_valid = خطأ;
            أرجع;
        }
        لكل (ثابت Subst_Var_ثبت::ریکارډ_2& کلمه_ریکارډ : ثبت.m_کلمه_ریکارډونه) {
            إذا (m_ګام < کلمه_ریکارډ.m_ګام) {
                m_valid = خطأ;
                أرجع;
            }
        }
    }

    فراغ لیدنه(AstConst*) override {}  // Accelerate
    فراغ لیدنه(Ast_متن*) override {}  // CExpr/CStmt literal text has no variable dependencies

    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {
        إذا (!m_valid) أرجع;
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    څرګند SubstValidVisitor(Subst_Var_ثبت::ریکارډ_2& ریکارډ)
        : m_ګام{ریکارډ.m_ګام} {
        iterateConst(ریکارډ.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = افتراضي;

عام:
    ساكن منطقي valid(Subst_Var_ثبت::ریکارډ_2& ریکارډ) {
        إذا (!ریکارډ.m_assignp) أرجع خطأ;
        أرجع SubstValidVisitor{ریکارډ}.m_valid;
    }
};

Ast_غوټه_بیان* Subst_Var_ثبت::subst_ریکارډ(Subst_Var_ثبت::ریکارډ_2& ریکارډ) {
    إذا (!SubstValidVisitor::valid(ریکارډ)) أرجع تش_اشاره;
    Ast_غوټه_بیان* ثابت rhsp = ریکارډ.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_خالص(), ریکارډ.m_assignp, "Substituting impure expression"););
    أرجع rhsp;
}

//######################################################################
// Substitution visitor

صنف SubstVisitor final : عام VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_ثبت> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_ټاکل_ګام = 0;  // Assignment number to determine variable lifetimes
    ثابت AstCFunc* m_funcp = تش_اشاره;  // Current function we are under
    اندازه_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    اندازه_t m_n_کلمه_Substituted = 0;  // Number of words substituted
    اندازه_t m_n_ټولیز_Substituted = 0;  // Number of whole variables substituted
    اندازه_t m_n_کلمه_ټاکل_Deleted = 0;  // Number of word assignments deleted
    اندازه_t m_n_ټولیز_ټاکل_Deleted = 0;  // Number of whole variable assignments deleted
    اندازه_t m_n_Const_ټکي_Reinlined = 0;  // Number of constant words substituted

    ساكن constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ساكن constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_ثبت& get_ثبت(AstVarRef* nodep) {
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp->user1p()) {
            m_entries.emplace_شا(varp);
            varp->user1p(&m_entries.شا());
        }
        أرجع *varp->user1u().منزل<Subst_Var_ثبت*>();
    }

    فراغ simplify(Ast_غوټه_بیان* exprp) {
        // Often constant, so short circuit
        إذا (VN_IS(exprp, Const)) أرجع;
        // Iterate expression, then constant fold it if anything changed
        ثابت اندازه_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), غوټه_بیان);
        إذا (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ساكن منطقي isSubstitutable(AstVar* nodep) {
        أرجع nodep->is_Statement_لنډمهاله() && !nodep->noSubst();
    }

    فراغ substitute(Ast_غوټه* nodep, Ast_غوټه_بیان* substp) {
        Ast_غوټه_بیان* newp = substp->backp() ? substp->clone_ونه_خالص(ریښتیا) : substp;
        إذا (!nodep->isQuad() && newp->isQuad()) {
            newp = جديد AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    فراغ لیدنه(AstNetlist* nodep) override { iterate_And_بل_Null(nodep->modulesp()); }

    فراغ لیدنه(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.تشه(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ثابت VNUser1_دننه_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            لكل (Subst_Var_ثبت& ثبت : m_entries) {
                ثبت.delete_ناکارول_شوی_Assignments(m_n_کلمه_ټاکل_Deleted, m_n_ټولیز_ټاکل_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        إذا (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        إذا (!m_funcp) أرجع;

        ثابت uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            أرجع VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ثابت خپلکاری getAssignp = [&](AstVarRef* refp) -> Ast_غوټه_ټاکل* {
            // If too complex, don't substitute
            إذا (ops > SUBST_MAX_OPS_SUBST) أرجع تش_اشاره;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            إذا (VN_IS(nodep->rhsp(), Cvt_Packed_منزل_Array)) أرجع تش_اشاره;
            // If non-const but want const subtitutions only
            إذا (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) أرجع تش_اشاره;
            // Otherwise can substitute based on the assignment
            أرجع nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_ثبت(refp).ټاکل_ټولیز(getAssignp(refp), ++m_ټاکل_ګام);
            أرجع;
        }

        // If LHS is a known word reference, track the word
        إذا (ثابت Ast_کلمه_Sel* ثابت selp = VN_CAST(nodep->lhsp(), کلمه_Sel)) {
            إذا (AstVarRef* ثابت refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                إذا (ثابت AstConst* ثابت idxp = VN_CAST(selp->bitp(), Const)) {
                    get_ثبت(refp).ټاکل_کلمه(getAssignp(refp), ++m_ټاکل_ګام, idxp->منزل_UInt());
                    أرجع;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    فراغ لیدنه(Ast_کلمه_Sel* nodep) override {
        إذا (!m_funcp) أرجع;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            إذا (ثابت AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_ثبت& ثبت = get_ثبت(refp);
                ثابت uint32_t کلمه = idxp->منزل_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                إذا (refp->access().isWriteOrRW()) {
                    ثبت.ټاکل_کلمه(تش_اشاره, ++m_ټاکل_ګام, کلمه);
                    أرجع;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ثابت varp = refp->varp();
                إذا (varp->const_Pool_ثبت()) {
                    AstConst* ثابت constp = VN_AS(varp->valuep(), Const);
                    ثابت uint32_t ارزښت = constp->عدد().edata_کلمه(کلمه);
                    File_کرښه* ثابت flp = nodep->fileline();
                    ++m_n_Const_ټکي_Reinlined;
                    substitute(nodep, جديد AstConst{flp, AstConst::SizedEData{}, ارزښت});
                    أرجع;
                }

                // Substitute other variables if possible
                إذا (isSubstitutable(refp->varp())) {
                    إذا (Ast_غوټه_بیان* ثابت substp = ثبت.subst_کلمه(کلمه)) {
                        ++m_n_کلمه_Substituted;
                        substitute(nodep, substp);
                        أرجع;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                ثبت.کارول_شوی_کلمه(کلمه);
                أرجع;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (!m_funcp) أرجع;

        Subst_Var_ثبت& ثبت = get_ثبت(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        إذا (nodep->access().isWriteOrRW()) {
            ثبت.ټاکل_ټولیز(تش_اشاره, ++m_ټاکل_ګام);
            أرجع;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        إذا (isSubstitutable(nodep->varp())) {
            إذا (Ast_غوټه_بیان* ثابت substp = ثبت.subst_ټولیز()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                إذا (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_ټولیز_Substituted;
                    substitute(nodep, substp);
                    أرجع;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        ثبت.کارول_شوی_ټولیز();
    }

    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {
        إذا (!m_funcp) أرجع;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        إذا (!nodep->is_خالص()) m_ops = SUBST_MAX_OPS_NA;
    }

    فراغ لیدنه(AstVar*) override {}  // Accelerate
    فراغ لیدنه(AstConst*) override {}  // Accelerate

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::جمع_کړه_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::جمع_کړه_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_ټکي_Reinlined);
        V3Stats::جمع_کړه_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_ټولیز_ټاکل_Deleted);
        V3Stats::جمع_کړه_Stat("Optimizations, Subst, Whole variables substituted", m_n_ټولیز_Substituted);
        V3Stats::جمع_کړه_Stat("Optimizations, Subst, Word assignments deleted", m_n_کلمه_ټاکل_Deleted);
        V3Stats::جمع_کړه_Stat("Optimizations, Subst, Words substituted", m_n_کلمه_Substituted);
        UASSERT(m_entries.تشه(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

فراغ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("subst", 0, dump_ونه_Either_Level() >= 3);
}
