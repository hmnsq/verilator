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

صنف Subst_Var_اندراج final {
    یار صنف SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    بنية ریکارڈ_2 final {
        // MEMBERS
        Ast_گرہ_سونپو* m_assignp;  // Last assignment to this part
        uint32_t m_قدم;  // Step number of last assignment. 0 means invalid entry.
        منطقي m_ورتیا;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        ریکارڈ_2() { invalidate(); }
        // METHODS
        فراغ invalidate() {
            m_assignp = خالی_سوچک;
            m_قدم = 0;
            m_ورتیا = خطأ;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ثابت m_varp;
    // The recrod for whole variable tracking
    ریکارڈ_2 m_سارا_ریکارڈ;
    // A record for each word in the variable
    std::سمتیہ<ریکارڈ_2> m_لفظ_ریکارڈاں{static_cast<ناپ_t>(m_varp->چوڑائی_الفاظ()), ریکارڈ_2{}};

    // METHDOS
    فراغ delete_Assignment_If_بن_ورتیا(ریکارڈ_2& ریکارڈ, ناپ_t& n_سونپو_Deleted) {
        إذا (!ریکارڈ.m_assignp) أرجع;
        إذا (ریکارڈ.m_ورتیا) أرجع;
        ++n_سونپو_Deleted;
        VL_DO_DANGLING(ریکارڈ.m_assignp->unlink_Fr_پٹھ()->delete_درخت(), ریکارڈ.m_assignp);
    }

    Ast_گرہ_اظہاریہ* subst_ریکارڈ(ریکارڈ_2& ریکارڈ);

عام:
    // CONSTRUCTORS
    واضح Subst_Var_اندراج(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_اندراج() = افتراضي;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    فراغ سونپو_سارا(Ast_گرہ_سونپو* assp, uint32_t قدم) {
        // Invalidate all word records
        لكل (ریکارڈ_2& لفظ_ریکارڈ : m_لفظ_ریکارڈاں) لفظ_ریکارڈ.invalidate();
        // Set whole record
        m_سارا_ریکارڈ.m_assignp = assp;
        m_سارا_ریکارڈ.m_قدم = قدم;
        m_سارا_ریکارڈ.m_ورتیا = خطأ;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    فراغ سونپو_لفظ(Ast_گرہ_سونپو* assp, uint32_t قدم, uint32_t لفظ) {
        // Invalidate whole record
        m_سارا_ریکارڈ.invalidate();
        // Set word record
        ریکارڈ_2& لفظ_ریکارڈ = m_لفظ_ریکارڈاں[لفظ];
        لفظ_ریکارڈ.m_assignp = assp;
        لفظ_ریکارڈ.m_قدم = قدم;
        لفظ_ریکارڈ.m_ورتیا = خطأ;
    }

    // Mark the whole variable as used (value consumed)
    فراغ ورتیا_سارا() {
        m_سارا_ریکارڈ.m_ورتیا = سچ;
        لكل (ریکارڈ_2& لفظ_ریکارڈ : m_لفظ_ریکارڈاں) لفظ_ریکارڈ.m_ورتیا = سچ;
    }

    // Mark the specific word as used (value consumed)
    فراغ ورتیا_لفظ(uint32_t لفظ) {
        m_سارا_ریکارڈ.m_ورتیا = سچ;
        m_لفظ_ریکارڈاں[لفظ].m_ورتیا = سچ;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_گرہ_اظہاریہ* subst_سارا() { أرجع subst_ریکارڈ(m_سارا_ریکارڈ); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_گرہ_اظہاریہ* subst_لفظ(uint32_t لفظ) { أرجع subst_ریکارڈ(m_لفظ_ریکارڈاں[لفظ]); }

    فراغ delete_بن_ورتیا_Assignments(ناپ_t& n_لفظ_سونپو_Deleted, ناپ_t& n_سارا_سونپو_Deleted) {
        // Delete assignments to temporaries if they are not used
        إذا (!m_varp->is_Statement_عارضی()) أرجع;
        لكل (ریکارڈ_2& لفظ_ریکارڈ : m_لفظ_ریکارڈاں)
            delete_Assignment_If_بن_ورتیا(لفظ_ریکارڈ, n_لفظ_سونپو_Deleted);
        delete_Assignment_If_بن_ورتیا(m_سارا_ریکارڈ, n_سارا_سونپو_Deleted);
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
    Subst_Var_اندراج& get_اندراج(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        أرجع *nodep->varp()->user1u().منزل<Subst_Var_اندراج*>();
    }

    // VISITORS
    فراغ ویکھو(Ast_لفظ_Sel* nodep) override {
        إذا (!m_valid) أرجع;

        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            إذا (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_اندراج& اندراج = get_اندراج(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                إذا (m_قدم < اندراج.m_سارا_ریکارڈ.m_قدم) m_valid = خطأ;
                إذا (m_قدم < اندراج.m_لفظ_ریکارڈاں[idxp->منزل_UInt()].m_قدم) m_valid = خطأ;
                أرجع;
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(AstVarRef* nodep) override {
        إذا (!m_valid) أرجع;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ثابت Subst_Var_اندراج& اندراج = get_اندراج(nodep);
        إذا (m_قدم < اندراج.m_سارا_ریکارڈ.m_قدم) {
            m_valid = خطأ;
            أرجع;
        }
        لكل (ثابت Subst_Var_اندراج::ریکارڈ_2& لفظ_ریکارڈ : اندراج.m_لفظ_ریکارڈاں) {
            إذا (m_قدم < لفظ_ریکارڈ.m_قدم) {
                m_valid = خطأ;
                أرجع;
            }
        }
    }

    فراغ ویکھو(AstConst*) override {}  // Accelerate
    فراغ ویکھو(Ast_متن*) override {}  // CExpr/CStmt literal text has no variable dependencies

    فراغ ویکھو(Ast_گرہ_اظہاریہ* nodep) override {
        إذا (!m_valid) أرجع;
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    واضح SubstValidVisitor(Subst_Var_اندراج::ریکارڈ_2& ریکارڈ)
        : m_قدم{ریکارڈ.m_قدم} {
        iterateConst(ریکارڈ.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = افتراضي;

عام:
    ساكن منطقي valid(Subst_Var_اندراج::ریکارڈ_2& ریکارڈ) {
        إذا (!ریکارڈ.m_assignp) أرجع خطأ;
        أرجع SubstValidVisitor{ریکارڈ}.m_valid;
    }
};

Ast_گرہ_اظہاریہ* Subst_Var_اندراج::subst_ریکارڈ(Subst_Var_اندراج::ریکارڈ_2& ریکارڈ) {
    إذا (!SubstValidVisitor::valid(ریکارڈ)) أرجع خالی_سوچک;
    Ast_گرہ_اظہاریہ* ثابت rhsp = ریکارڈ.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_خالص(), ریکارڈ.m_assignp, "Substituting impure expression"););
    أرجع rhsp;
}

//######################################################################
// Substitution visitor

صنف SubstVisitor final : عام VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_اندراج> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_سونپو_قدم = 0;  // Assignment number to determine variable lifetimes
    ثابت AstCFunc* m_funcp = خالی_سوچک;  // Current function we are under
    ناپ_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    ناپ_t m_n_لفظ_Substituted = 0;  // Number of words substituted
    ناپ_t m_n_سارا_Substituted = 0;  // Number of whole variables substituted
    ناپ_t m_n_لفظ_سونپو_Deleted = 0;  // Number of word assignments deleted
    ناپ_t m_n_سارا_سونپو_Deleted = 0;  // Number of whole variable assignments deleted
    ناپ_t m_n_Const_الفاظ_Reinlined = 0;  // Number of constant words substituted

    ساكن constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ساكن constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_اندراج& get_اندراج(AstVarRef* nodep) {
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp->user1p()) {
            m_entries.emplace_پٹھ(varp);
            varp->user1p(&m_entries.پٹھ());
        }
        أرجع *varp->user1u().منزل<Subst_Var_اندراج*>();
    }

    فراغ simplify(Ast_گرہ_اظہاریہ* exprp) {
        // Often constant, so short circuit
        إذا (VN_IS(exprp, Const)) أرجع;
        // Iterate expression, then constant fold it if anything changed
        ثابت ناپ_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), گرہ_اظہاریہ);
        إذا (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ساكن منطقي isSubstitutable(AstVar* nodep) {
        أرجع nodep->is_Statement_عارضی() && !nodep->noSubst();
    }

    فراغ substitute(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* substp) {
        Ast_گرہ_اظہاریہ* newp = substp->backp() ? substp->clone_درخت_خالص(سچ) : substp;
        إذا (!nodep->isQuad() && newp->isQuad()) {
            newp = جديد AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    فراغ ویکھو(AstNetlist* nodep) override { iterate_And_اگلا_Null(nodep->modulesp()); }

    فراغ ویکھو(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.خالی_تھاواں(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ثابت VNUser1_وچ_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            لكل (Subst_Var_اندراج& اندراج : m_entries) {
                اندراج.delete_بن_ورتیا_Assignments(m_n_لفظ_سونپو_Deleted, m_n_سارا_سونپو_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        إذا (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
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
        ثابت خودکار getAssignp = [&](AstVarRef* refp) -> Ast_گرہ_سونپو* {
            // If too complex, don't substitute
            إذا (ops > SUBST_MAX_OPS_SUBST) أرجع خالی_سوچک;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            إذا (VN_IS(nodep->rhsp(), Cvt_Packed_منزل_Array)) أرجع خالی_سوچک;
            // If non-const but want const subtitutions only
            إذا (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) أرجع خالی_سوچک;
            // Otherwise can substitute based on the assignment
            أرجع nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_اندراج(refp).سونپو_سارا(getAssignp(refp), ++m_سونپو_قدم);
            أرجع;
        }

        // If LHS is a known word reference, track the word
        إذا (ثابت Ast_لفظ_Sel* ثابت selp = VN_CAST(nodep->lhsp(), لفظ_Sel)) {
            إذا (AstVarRef* ثابت refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                إذا (ثابت AstConst* ثابت idxp = VN_CAST(selp->bitp(), Const)) {
                    get_اندراج(refp).سونپو_لفظ(getAssignp(refp), ++m_سونپو_قدم, idxp->منزل_UInt());
                    أرجع;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    فراغ ویکھو(Ast_لفظ_Sel* nodep) override {
        إذا (!m_funcp) أرجع;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            إذا (ثابت AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_اندراج& اندراج = get_اندراج(refp);
                ثابت uint32_t لفظ = idxp->منزل_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                إذا (refp->access().isWriteOrRW()) {
                    اندراج.سونپو_لفظ(خالی_سوچک, ++m_سونپو_قدم, لفظ);
                    أرجع;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ثابت varp = refp->varp();
                إذا (varp->const_Pool_اندراج()) {
                    AstConst* ثابت constp = VN_AS(varp->valuep(), Const);
                    ثابت uint32_t قدر = constp->عدد().edata_لفظ(لفظ);
                    File_سطر* ثابت flp = nodep->fileline();
                    ++m_n_Const_الفاظ_Reinlined;
                    substitute(nodep, جديد AstConst{flp, AstConst::SizedEData{}, قدر});
                    أرجع;
                }

                // Substitute other variables if possible
                إذا (isSubstitutable(refp->varp())) {
                    إذا (Ast_گرہ_اظہاریہ* ثابت substp = اندراج.subst_لفظ(لفظ)) {
                        ++m_n_لفظ_Substituted;
                        substitute(nodep, substp);
                        أرجع;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                اندراج.ورتیا_لفظ(لفظ);
                أرجع;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    فراغ ویکھو(AstVarRef* nodep) override {
        إذا (!m_funcp) أرجع;

        Subst_Var_اندراج& اندراج = get_اندراج(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        إذا (nodep->access().isWriteOrRW()) {
            اندراج.سونپو_سارا(خالی_سوچک, ++m_سونپو_قدم);
            أرجع;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        إذا (isSubstitutable(nodep->varp())) {
            إذا (Ast_گرہ_اظہاریہ* ثابت substp = اندراج.subst_سارا()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                إذا (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_سارا_Substituted;
                    substitute(nodep, substp);
                    أرجع;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        اندراج.ورتیا_سارا();
    }

    فراغ ویکھو(Ast_گرہ_اظہاریہ* nodep) override {
        إذا (!m_funcp) أرجع;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        إذا (!nodep->is_خالص()) m_ops = SUBST_MAX_OPS_NA;
    }

    فراغ ویکھو(AstVar*) override {}  // Accelerate
    فراغ ویکھو(AstConst*) override {}  // Accelerate

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_الفاظ_Reinlined);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_سارا_سونپو_Deleted);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Whole variables substituted", m_n_سارا_Substituted);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Word assignments deleted", m_n_لفظ_سونپو_Deleted);
        V3Stats::جوڑو_Stat("Optimizations, Subst, Words substituted", m_n_لفظ_Substituted);
        UASSERT(m_entries.خالی_تھاواں(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

فراغ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("subst", 0, dump_درخت_Either_Level() >= 3);
}
