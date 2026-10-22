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

صنف Subst_Var_داخلا final {
    دوست صنف SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    بنية رڪارڊ_2 final {
        // MEMBERS
        Ast_ڳنڍ_سونپيو* m_assignp;  // Last assignment to this part
        uint32_t m_قدم;  // Step number of last assignment. 0 means invalid entry.
        منطقي m_استعمال_ٿيل;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        رڪارڊ_2() { invalidate(); }
        // METHODS
        فراغ invalidate() {
            m_assignp = خالي_اشارو;
            m_قدم = 0;
            m_استعمال_ٿيل = خطأ;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ثابت m_varp;
    // The recrod for whole variable tracking
    رڪارڊ_2 m_سڄو_رڪارڊ;
    // A record for each word in the variable
    std::ويڪٽر<رڪارڊ_2> m_لفظ_رڪارڊون{static_cast<سائيز_t>(m_varp->ويڪر_لفظ()), رڪارڊ_2{}};

    // METHDOS
    فراغ delete_Assignment_If_اڻ_استعمال_ٿيل(رڪارڊ_2& رڪارڊ, سائيز_t& n_سونپيو_Deleted) {
        إذا (!رڪارڊ.m_assignp) أرجع;
        إذا (رڪارڊ.m_استعمال_ٿيل) أرجع;
        ++n_سونپيو_Deleted;
        VL_DO_DANGLING(رڪارڊ.m_assignp->unlink_Fr_پٺ()->delete_وڻ(), رڪارڊ.m_assignp);
    }

    Ast_ڳنڍ_اظهار* subst_رڪارڊ(رڪارڊ_2& رڪارڊ);

عام:
    // CONSTRUCTORS
    واضح Subst_Var_داخلا(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_داخلا() = افتراضي;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    فراغ سونپيو_سڄو(Ast_ڳنڍ_سونپيو* assp, uint32_t قدم) {
        // Invalidate all word records
        لكل (رڪارڊ_2& لفظ_رڪارڊ : m_لفظ_رڪارڊون) لفظ_رڪارڊ.invalidate();
        // Set whole record
        m_سڄو_رڪارڊ.m_assignp = assp;
        m_سڄو_رڪارڊ.m_قدم = قدم;
        m_سڄو_رڪارڊ.m_استعمال_ٿيل = خطأ;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    فراغ سونپيو_لفظ(Ast_ڳنڍ_سونپيو* assp, uint32_t قدم, uint32_t لفظ) {
        // Invalidate whole record
        m_سڄو_رڪارڊ.invalidate();
        // Set word record
        رڪارڊ_2& لفظ_رڪارڊ = m_لفظ_رڪارڊون[لفظ];
        لفظ_رڪارڊ.m_assignp = assp;
        لفظ_رڪارڊ.m_قدم = قدم;
        لفظ_رڪارڊ.m_استعمال_ٿيل = خطأ;
    }

    // Mark the whole variable as used (value consumed)
    فراغ استعمال_ٿيل_سڄو() {
        m_سڄو_رڪارڊ.m_استعمال_ٿيل = سچ;
        لكل (رڪارڊ_2& لفظ_رڪارڊ : m_لفظ_رڪارڊون) لفظ_رڪارڊ.m_استعمال_ٿيل = سچ;
    }

    // Mark the specific word as used (value consumed)
    فراغ استعمال_ٿيل_لفظ(uint32_t لفظ) {
        m_سڄو_رڪارڊ.m_استعمال_ٿيل = سچ;
        m_لفظ_رڪارڊون[لفظ].m_استعمال_ٿيل = سچ;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ڳنڍ_اظهار* subst_سڄو() { أرجع subst_رڪارڊ(m_سڄو_رڪارڊ); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ڳنڍ_اظهار* subst_لفظ(uint32_t لفظ) { أرجع subst_رڪارڊ(m_لفظ_رڪارڊون[لفظ]); }

    فراغ delete_اڻ_استعمال_ٿيل_Assignments(سائيز_t& n_لفظ_سونپيو_Deleted, سائيز_t& n_سڄو_سونپيو_Deleted) {
        // Delete assignments to temporaries if they are not used
        إذا (!m_varp->is_Statement_عارضي()) أرجع;
        لكل (رڪارڊ_2& لفظ_رڪارڊ : m_لفظ_رڪارڊون)
            delete_Assignment_If_اڻ_استعمال_ٿيل(لفظ_رڪارڊ, n_لفظ_سونپيو_Deleted);
        delete_Assignment_If_اڻ_استعمال_ٿيل(m_سڄو_رڪارڊ, n_سڄو_سونپيو_Deleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

صنف SubstValidVisitor final : عام VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ثابت uint32_t m_قدم;  // Step number where assignment was recorded
    منطقي m_valid = سچ;  // Is the expression we are considering to substitute valid

    // METHODS
    Subst_Var_داخلا& get_داخلا(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        أرجع *nodep->varp()->user1u().منزل<Subst_Var_داخلا*>();
    }

    // VISITORS
    فراغ گهمو(Ast_لفظ_Sel* nodep) override {
        إذا (!m_valid) أرجع;

        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            إذا (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_داخلا& داخلا_2 = get_داخلا(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                إذا (m_قدم < داخلا_2.m_سڄو_رڪارڊ.m_قدم) m_valid = خطأ;
                إذا (m_قدم < داخلا_2.m_لفظ_رڪارڊون[idxp->منزل_UInt()].m_قدم) m_valid = خطأ;
                أرجع;
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(AstVarRef* nodep) override {
        إذا (!m_valid) أرجع;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ثابت Subst_Var_داخلا& داخلا_2 = get_داخلا(nodep);
        إذا (m_قدم < داخلا_2.m_سڄو_رڪارڊ.m_قدم) {
            m_valid = خطأ;
            أرجع;
        }
        لكل (ثابت Subst_Var_داخلا::رڪارڊ_2& لفظ_رڪارڊ : داخلا_2.m_لفظ_رڪارڊون) {
            إذا (m_قدم < لفظ_رڪارڊ.m_قدم) {
                m_valid = خطأ;
                أرجع;
            }
        }
    }

    فراغ گهمو(AstConst*) override {}  // Accelerate
    فراغ گهمو(Ast_متن*) override {}  // CExpr/CStmt literal text has no variable dependencies

    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        إذا (!m_valid) أرجع;
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    واضح SubstValidVisitor(Subst_Var_داخلا::رڪارڊ_2& رڪارڊ)
        : m_قدم{رڪارڊ.m_قدم} {
        iterateConst(رڪارڊ.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = افتراضي;

عام:
    ساكن منطقي valid(Subst_Var_داخلا::رڪارڊ_2& رڪارڊ) {
        إذا (!رڪارڊ.m_assignp) أرجع خطأ;
        أرجع SubstValidVisitor{رڪارڊ}.m_valid;
    }
};

Ast_ڳنڍ_اظهار* Subst_Var_داخلا::subst_رڪارڊ(Subst_Var_داخلا::رڪارڊ_2& رڪارڊ) {
    إذا (!SubstValidVisitor::valid(رڪارڊ)) أرجع خالي_اشارو;
    Ast_ڳنڍ_اظهار* ثابت rhsp = رڪارڊ.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_خالص(), رڪارڊ.m_assignp, "Substituting impure expression"););
    أرجع rhsp;
}

//######################################################################
// Substitution visitor

صنف SubstVisitor final : عام VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_داخلا> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_سونپيو_قدم = 0;  // Assignment number to determine variable lifetimes
    ثابت AstCFunc* m_funcp = خالي_اشارو;  // Current function we are under
    سائيز_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    سائيز_t m_n_لفظ_Substituted = 0;  // Number of words substituted
    سائيز_t m_n_سڄو_Substituted = 0;  // Number of whole variables substituted
    سائيز_t m_n_لفظ_سونپيو_Deleted = 0;  // Number of word assignments deleted
    سائيز_t m_n_سڄو_سونپيو_Deleted = 0;  // Number of whole variable assignments deleted
    سائيز_t m_n_Const_لفظ_Reinlined = 0;  // Number of constant words substituted

    ساكن constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ساكن constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_داخلا& get_داخلا(AstVarRef* nodep) {
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp->user1p()) {
            m_entries.emplace_پٺ(varp);
            varp->user1p(&m_entries.پٺ());
        }
        أرجع *varp->user1u().منزل<Subst_Var_داخلا*>();
    }

    فراغ simplify(Ast_ڳنڍ_اظهار* exprp) {
        // Often constant, so short circuit
        إذا (VN_IS(exprp, Const)) أرجع;
        // Iterate expression, then constant fold it if anything changed
        ثابت سائيز_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), ڳنڍ_اظهار);
        إذا (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ساكن منطقي isSubstitutable(AstVar* nodep) {
        أرجع nodep->is_Statement_عارضي() && !nodep->noSubst();
    }

    فراغ substitute(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* substp) {
        Ast_ڳنڍ_اظهار* newp = substp->backp() ? substp->clone_وڻ_خالص(سچ) : substp;
        إذا (!nodep->isQuad() && newp->isQuad()) {
            newp = جديد AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    فراغ گهمو(AstNetlist* nodep) override { iterate_And_ايندڙ_Null(nodep->modulesp()); }

    فراغ گهمو(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.خالي_جايون(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ثابت VNUser1_اندريون_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            لكل (Subst_Var_داخلا& داخلا_2 : m_entries) {
                داخلا_2.delete_اڻ_استعمال_ٿيل_Assignments(m_n_لفظ_سونپيو_Deleted, m_n_سڄو_سونپيو_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        إذا (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
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
        ثابت خودڪار getAssignp = [&](AstVarRef* refp) -> Ast_ڳنڍ_سونپيو* {
            // If too complex, don't substitute
            إذا (ops > SUBST_MAX_OPS_SUBST) أرجع خالي_اشارو;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            إذا (VN_IS(nodep->rhsp(), Cvt_Packed_منزل_Array)) أرجع خالي_اشارو;
            // If non-const but want const subtitutions only
            إذا (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) أرجع خالي_اشارو;
            // Otherwise can substitute based on the assignment
            أرجع nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_داخلا(refp).سونپيو_سڄو(getAssignp(refp), ++m_سونپيو_قدم);
            أرجع;
        }

        // If LHS is a known word reference, track the word
        إذا (ثابت Ast_لفظ_Sel* ثابت selp = VN_CAST(nodep->lhsp(), لفظ_Sel)) {
            إذا (AstVarRef* ثابت refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                إذا (ثابت AstConst* ثابت idxp = VN_CAST(selp->bitp(), Const)) {
                    get_داخلا(refp).سونپيو_لفظ(getAssignp(refp), ++m_سونپيو_قدم, idxp->منزل_UInt());
                    أرجع;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    فراغ گهمو(Ast_لفظ_Sel* nodep) override {
        إذا (!m_funcp) أرجع;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            إذا (ثابت AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_داخلا& داخلا_2 = get_داخلا(refp);
                ثابت uint32_t لفظ = idxp->منزل_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                إذا (refp->access().isWriteOrRW()) {
                    داخلا_2.سونپيو_لفظ(خالي_اشارو, ++m_سونپيو_قدم, لفظ);
                    أرجع;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ثابت varp = refp->varp();
                إذا (varp->const_Pool_داخلا()) {
                    AstConst* ثابت constp = VN_AS(varp->valuep(), Const);
                    ثابت uint32_t قدر = constp->انگ().edata_لفظ(لفظ);
                    File_سٽ* ثابت flp = nodep->fileline();
                    ++m_n_Const_لفظ_Reinlined;
                    substitute(nodep, جديد AstConst{flp, AstConst::SizedEData{}, قدر});
                    أرجع;
                }

                // Substitute other variables if possible
                إذا (isSubstitutable(refp->varp())) {
                    إذا (Ast_ڳنڍ_اظهار* ثابت substp = داخلا_2.subst_لفظ(لفظ)) {
                        ++m_n_لفظ_Substituted;
                        substitute(nodep, substp);
                        أرجع;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                داخلا_2.استعمال_ٿيل_لفظ(لفظ);
                أرجع;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    فراغ گهمو(AstVarRef* nodep) override {
        إذا (!m_funcp) أرجع;

        Subst_Var_داخلا& داخلا_2 = get_داخلا(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        إذا (nodep->access().isWriteOrRW()) {
            داخلا_2.سونپيو_سڄو(خالي_اشارو, ++m_سونپيو_قدم);
            أرجع;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        إذا (isSubstitutable(nodep->varp())) {
            إذا (Ast_ڳنڍ_اظهار* ثابت substp = داخلا_2.subst_سڄو()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                إذا (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_سڄو_Substituted;
                    substitute(nodep, substp);
                    أرجع;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        داخلا_2.استعمال_ٿيل_سڄو();
    }

    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        إذا (!m_funcp) أرجع;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        إذا (!nodep->is_خالص()) m_ops = SUBST_MAX_OPS_NA;
    }

    فراغ گهمو(AstVar*) override {}  // Accelerate
    فراغ گهمو(AstConst*) override {}  // Accelerate

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::جوڙيو_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::جوڙيو_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_لفظ_Reinlined);
        V3Stats::جوڙيو_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_سڄو_سونپيو_Deleted);
        V3Stats::جوڙيو_Stat("Optimizations, Subst, Whole variables substituted", m_n_سڄو_Substituted);
        V3Stats::جوڙيو_Stat("Optimizations, Subst, Word assignments deleted", m_n_لفظ_سونپيو_Deleted);
        V3Stats::جوڙيو_Stat("Optimizations, Subst, Words substituted", m_n_لفظ_Substituted);
        UASSERT(m_entries.خالي_جايون(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

فراغ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("subst", 0, dump_وڻ_Either_Level() >= 3);
}
