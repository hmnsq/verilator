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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Subst.h"

#شامل "V3Const.h"
#شامل "V3Stats.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

جماعت Subst_Var_اندراج final {
    دوست جماعت SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    ساخت ریکارڈ_2 final {
        // MEMBERS
        Ast_گرہ_تفویض* m_assignp;  // Last assignment to this part
        uint32_t m_قدم;  // Step number of last assignment. 0 means invalid entry.
        منطقی m_مستعمل;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        ریکارڈ_2() { invalidate(); }
        // METHODS
        خالی invalidate() {
            m_assignp = خالی_اشاریہ;
            m_قدم = 0;
            m_مستعمل = جھوٹ;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ثابت m_varp;
    // The recrod for whole variable tracking
    ریکارڈ_2 m_پورا_ریکارڈ;
    // A record for each word in the variable
    std::سمتیہ<ریکارڈ_2> m_لفظ_اندراجات{static_cast<سائز_t>(m_varp->چوڑائی_الفاظ()), ریکارڈ_2{}};

    // METHDOS
    خالی delete_Assignment_If_غیر_مستعمل(ریکارڈ_2& ریکارڈ, سائز_t& n_تفویض_Deleted) {
        اگر (!ریکارڈ.m_assignp) واپس;
        اگر (ریکارڈ.m_مستعمل) واپس;
        ++n_تفویض_Deleted;
        VL_DO_DANGLING(ریکارڈ.m_assignp->unlink_Fr_عقب()->delete_درخت(), ریکارڈ.m_assignp);
    }

    Ast_گرہ_اظہاریہ* subst_ریکارڈ(ریکارڈ_2& ریکارڈ);

عوامی:
    // CONSTRUCTORS
    صریح Subst_Var_اندراج(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_اندراج() = طےشدہ;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    خالی تفویض_پورا(Ast_گرہ_تفویض* assp, uint32_t قدم) {
        // Invalidate all word records
        برائے (ریکارڈ_2& لفظ_ریکارڈ : m_لفظ_اندراجات) لفظ_ریکارڈ.invalidate();
        // Set whole record
        m_پورا_ریکارڈ.m_assignp = assp;
        m_پورا_ریکارڈ.m_قدم = قدم;
        m_پورا_ریکارڈ.m_مستعمل = جھوٹ;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    خالی تفویض_لفظ(Ast_گرہ_تفویض* assp, uint32_t قدم, uint32_t لفظ) {
        // Invalidate whole record
        m_پورا_ریکارڈ.invalidate();
        // Set word record
        ریکارڈ_2& لفظ_ریکارڈ = m_لفظ_اندراجات[لفظ];
        لفظ_ریکارڈ.m_assignp = assp;
        لفظ_ریکارڈ.m_قدم = قدم;
        لفظ_ریکارڈ.m_مستعمل = جھوٹ;
    }

    // Mark the whole variable as used (value consumed)
    خالی مستعمل_پورا() {
        m_پورا_ریکارڈ.m_مستعمل = سچ;
        برائے (ریکارڈ_2& لفظ_ریکارڈ : m_لفظ_اندراجات) لفظ_ریکارڈ.m_مستعمل = سچ;
    }

    // Mark the specific word as used (value consumed)
    خالی مستعمل_لفظ(uint32_t لفظ) {
        m_پورا_ریکارڈ.m_مستعمل = سچ;
        m_لفظ_اندراجات[لفظ].m_مستعمل = سچ;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_گرہ_اظہاریہ* subst_پورا() { واپس subst_ریکارڈ(m_پورا_ریکارڈ); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_گرہ_اظہاریہ* subst_لفظ(uint32_t لفظ) { واپس subst_ریکارڈ(m_لفظ_اندراجات[لفظ]); }

    خالی delete_غیر_مستعمل_Assignments(سائز_t& n_لفظ_تفویض_Deleted, سائز_t& n_پورا_تفویض_Deleted) {
        // Delete assignments to temporaries if they are not used
        اگر (!m_varp->is_Statement_عارضی()) واپس;
        برائے (ریکارڈ_2& لفظ_ریکارڈ : m_لفظ_اندراجات)
            delete_Assignment_If_غیر_مستعمل(لفظ_ریکارڈ, n_لفظ_تفویض_Deleted);
        delete_Assignment_If_غیر_مستعمل(m_پورا_ریکارڈ, n_پورا_تفویض_Deleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

جماعت SubstValidVisitor final : عوامی VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ثابت uint32_t m_قدم;  // Step number where assignment was recorded
    منطقی m_valid = سچ;  // Is the expression we are considering to substitute valid

    // METHODS
    Subst_Var_اندراج& get_اندراج(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        واپس *nodep->varp()->user1u().مقصد<Subst_Var_اندراج*>();
    }

    // VISITORS
    خالی دورہ(Ast_لفظ_Sel* nodep) override {
        اگر (!m_valid) واپس;

        اگر (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            اگر (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_اندراج& اندراج_2 = get_اندراج(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                اگر (m_قدم < اندراج_2.m_پورا_ریکارڈ.m_قدم) m_valid = جھوٹ;
                اگر (m_قدم < اندراج_2.m_لفظ_اندراجات[idxp->مقصد_UInt()].m_قدم) m_valid = جھوٹ;
                واپس;
            }
        }
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstVarRef* nodep) override {
        اگر (!m_valid) واپس;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ثابت Subst_Var_اندراج& اندراج_2 = get_اندراج(nodep);
        اگر (m_قدم < اندراج_2.m_پورا_ریکارڈ.m_قدم) {
            m_valid = جھوٹ;
            واپس;
        }
        برائے (ثابت Subst_Var_اندراج::ریکارڈ_2& لفظ_ریکارڈ : اندراج_2.m_لفظ_اندراجات) {
            اگر (m_قدم < لفظ_ریکارڈ.m_قدم) {
                m_valid = جھوٹ;
                واپس;
            }
        }
    }

    خالی دورہ(AstConst*) override {}  // Accelerate
    خالی دورہ(Ast_متن*) override {}  // CExpr/CStmt literal text has no variable dependencies

    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {
        اگر (!m_valid) واپس;
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    صریح SubstValidVisitor(Subst_Var_اندراج::ریکارڈ_2& ریکارڈ)
        : m_قدم{ریکارڈ.m_قدم} {
        iterateConst(ریکارڈ.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = طےشدہ;

عوامی:
    ساکن منطقی valid(Subst_Var_اندراج::ریکارڈ_2& ریکارڈ) {
        اگر (!ریکارڈ.m_assignp) واپس جھوٹ;
        واپس SubstValidVisitor{ریکارڈ}.m_valid;
    }
};

Ast_گرہ_اظہاریہ* Subst_Var_اندراج::subst_ریکارڈ(Subst_Var_اندراج::ریکارڈ_2& ریکارڈ) {
    اگر (!SubstValidVisitor::valid(ریکارڈ)) واپس خالی_اشاریہ;
    Ast_گرہ_اظہاریہ* ثابت rhsp = ریکارڈ.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_خالص(), ریکارڈ.m_assignp, "Substituting impure expression"););
    واپس rhsp;
}

//######################################################################
// Substitution visitor

جماعت SubstVisitor final : عوامی VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_اندراج> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_تفویض_قدم = 0;  // Assignment number to determine variable lifetimes
    ثابت AstCFunc* m_funcp = خالی_اشاریہ;  // Current function we are under
    سائز_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    سائز_t m_n_لفظ_Substituted = 0;  // Number of words substituted
    سائز_t m_n_پورا_Substituted = 0;  // Number of whole variables substituted
    سائز_t m_n_لفظ_تفویض_Deleted = 0;  // Number of word assignments deleted
    سائز_t m_n_پورا_تفویض_Deleted = 0;  // Number of whole variable assignments deleted
    سائز_t m_n_Const_الفاظ_Reinlined = 0;  // Number of constant words substituted

    ساکن constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ساکن constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_اندراج& get_اندراج(AstVarRef* nodep) {
        AstVar* ثابت varp = nodep->varp();
        اگر (!varp->user1p()) {
            m_entries.emplace_عقب(varp);
            varp->user1p(&m_entries.عقب());
        }
        واپس *varp->user1u().مقصد<Subst_Var_اندراج*>();
    }

    خالی simplify(Ast_گرہ_اظہاریہ* exprp) {
        // Often constant, so short circuit
        اگر (VN_IS(exprp, Const)) واپس;
        // Iterate expression, then constant fold it if anything changed
        ثابت سائز_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), گرہ_اظہاریہ);
        اگر (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ساکن منطقی isSubstitutable(AstVar* nodep) {
        واپس nodep->is_Statement_عارضی() && !nodep->noSubst();
    }

    خالی substitute(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* substp) {
        Ast_گرہ_اظہاریہ* newp = substp->backp() ? substp->clone_درخت_خالص(سچ) : substp;
        اگر (!nodep->isQuad() && newp->isQuad()) {
            newp = نیا AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    خالی دورہ(AstNetlist* nodep) override { iterate_And_اگلا_Null(nodep->modulesp()); }

    خالی دورہ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.خالی_خانے(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ثابت VNUser1_اندر_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            برائے (Subst_Var_اندراج& اندراج_2 : m_entries) {
                اندراج_2.delete_غیر_مستعمل_Assignments(m_n_لفظ_تفویض_Deleted, m_n_پورا_تفویض_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        اگر (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (!m_funcp) واپس;

        ثابت uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            واپس VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ثابت خودکار getAssignp = [&](AstVarRef* refp) -> Ast_گرہ_تفویض* {
            // If too complex, don't substitute
            اگر (ops > SUBST_MAX_OPS_SUBST) واپس خالی_اشاریہ;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            اگر (VN_IS(nodep->rhsp(), Cvt_Packed_مقصد_Array)) واپس خالی_اشاریہ;
            // If non-const but want const subtitutions only
            اگر (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) واپس خالی_اشاریہ;
            // Otherwise can substitute based on the assignment
            واپس nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        اگر (AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_اندراج(refp).تفویض_پورا(getAssignp(refp), ++m_تفویض_قدم);
            واپس;
        }

        // If LHS is a known word reference, track the word
        اگر (ثابت Ast_لفظ_Sel* ثابت selp = VN_CAST(nodep->lhsp(), لفظ_Sel)) {
            اگر (AstVarRef* ثابت refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                اگر (ثابت AstConst* ثابت idxp = VN_CAST(selp->bitp(), Const)) {
                    get_اندراج(refp).تفویض_لفظ(getAssignp(refp), ++m_تفویض_قدم, idxp->مقصد_UInt());
                    واپس;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    خالی دورہ(Ast_لفظ_Sel* nodep) override {
        اگر (!m_funcp) واپس;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        اگر (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            اگر (ثابت AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_اندراج& اندراج_2 = get_اندراج(refp);
                ثابت uint32_t لفظ = idxp->مقصد_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                اگر (refp->access().isWriteOrRW()) {
                    اندراج_2.تفویض_لفظ(خالی_اشاریہ, ++m_تفویض_قدم, لفظ);
                    واپس;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ثابت varp = refp->varp();
                اگر (varp->const_Pool_اندراج()) {
                    AstConst* ثابت constp = VN_AS(varp->valuep(), Const);
                    ثابت uint32_t قدر = constp->عدد().edata_لفظ(لفظ);
                    File_سطر* ثابت flp = nodep->fileline();
                    ++m_n_Const_الفاظ_Reinlined;
                    substitute(nodep, نیا AstConst{flp, AstConst::SizedEData{}, قدر});
                    واپس;
                }

                // Substitute other variables if possible
                اگر (isSubstitutable(refp->varp())) {
                    اگر (Ast_گرہ_اظہاریہ* ثابت substp = اندراج_2.subst_لفظ(لفظ)) {
                        ++m_n_لفظ_Substituted;
                        substitute(nodep, substp);
                        واپس;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                اندراج_2.مستعمل_لفظ(لفظ);
                واپس;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    خالی دورہ(AstVarRef* nodep) override {
        اگر (!m_funcp) واپس;

        Subst_Var_اندراج& اندراج_2 = get_اندراج(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        اگر (nodep->access().isWriteOrRW()) {
            اندراج_2.تفویض_پورا(خالی_اشاریہ, ++m_تفویض_قدم);
            واپس;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        اگر (isSubstitutable(nodep->varp())) {
            اگر (Ast_گرہ_اظہاریہ* ثابت substp = اندراج_2.subst_پورا()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                اگر (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_پورا_Substituted;
                    substitute(nodep, substp);
                    واپس;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        اندراج_2.مستعمل_پورا();
    }

    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {
        اگر (!m_funcp) واپس;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        اگر (!nodep->is_خالص()) m_ops = SUBST_MAX_OPS_NA;
    }

    خالی دورہ(AstVar*) override {}  // Accelerate
    خالی دورہ(AstConst*) override {}  // Accelerate

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_الفاظ_Reinlined);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_پورا_تفویض_Deleted);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Whole variables substituted", m_n_پورا_Substituted);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Word assignments deleted", m_n_لفظ_تفویض_Deleted);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Words substituted", m_n_لفظ_Substituted);
        UASSERT(m_entries.خالی_خانے(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

خالی V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("subst", 0, dump_درخت_Either_Level() >= 3);
}
