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

صنف Subst_Var_مدخل final {
    دوست صنف SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    بنية رکورد_2 final {
        // MEMBERS
        Ast_گره_تخصیص_بده* m_assignp;  // Last assignment to this part
        uint32_t m_گام;  // Step number of last assignment. 0 means invalid entry.
        منطقي m_استفاده_شده;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        رکورد_2() { invalidate(); }
        // METHODS
        فراغ invalidate() {
            m_assignp = اشاره_تهی;
            m_گام = 0;
            m_استفاده_شده = خطأ;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ثابت m_varp;
    // The recrod for whole variable tracking
    رکورد_2 m_کامل_رکورد;
    // A record for each word in the variable
    std::وکتور<رکورد_2> m_کلمه_رکوردها{static_cast<اندازه_t>(m_varp->عرض_واژگان()), رکورد_2{}};

    // METHDOS
    فراغ delete_Assignment_If_استفادهنشده(رکورد_2& رکورد, اندازه_t& n_تخصیص_بده_Deleted) {
        إذا (!رکورد.m_assignp) أرجع;
        إذا (رکورد.m_استفاده_شده) أرجع;
        ++n_تخصیص_بده_Deleted;
        VL_DO_DANGLING(رکورد.m_assignp->unlink_Fr_پشت()->delete_درخت(), رکورد.m_assignp);
    }

    Ast_گره_عبارت* subst_رکورد(رکورد_2& رکورد);

عام:
    // CONSTRUCTORS
    صریح Subst_Var_مدخل(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_مدخل() = افتراضي;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    فراغ تخصیص_بده_کامل(Ast_گره_تخصیص_بده* assp, uint32_t گام) {
        // Invalidate all word records
        لكل (رکورد_2& کلمه_رکورد : m_کلمه_رکوردها) کلمه_رکورد.invalidate();
        // Set whole record
        m_کامل_رکورد.m_assignp = assp;
        m_کامل_رکورد.m_گام = گام;
        m_کامل_رکورد.m_استفاده_شده = خطأ;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    فراغ تخصیص_بده_کلمه(Ast_گره_تخصیص_بده* assp, uint32_t گام, uint32_t کلمه) {
        // Invalidate whole record
        m_کامل_رکورد.invalidate();
        // Set word record
        رکورد_2& کلمه_رکورد = m_کلمه_رکوردها[کلمه];
        کلمه_رکورد.m_assignp = assp;
        کلمه_رکورد.m_گام = گام;
        کلمه_رکورد.m_استفاده_شده = خطأ;
    }

    // Mark the whole variable as used (value consumed)
    فراغ استفاده_شده_کامل() {
        m_کامل_رکورد.m_استفاده_شده = درست;
        لكل (رکورد_2& کلمه_رکورد : m_کلمه_رکوردها) کلمه_رکورد.m_استفاده_شده = درست;
    }

    // Mark the specific word as used (value consumed)
    فراغ استفاده_شده_کلمه(uint32_t کلمه) {
        m_کامل_رکورد.m_استفاده_شده = درست;
        m_کلمه_رکوردها[کلمه].m_استفاده_شده = درست;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_گره_عبارت* subst_کامل() { أرجع subst_رکورد(m_کامل_رکورد); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_گره_عبارت* subst_کلمه(uint32_t کلمه) { أرجع subst_رکورد(m_کلمه_رکوردها[کلمه]); }

    فراغ delete_استفادهنشده_Assignments(اندازه_t& n_کلمه_تخصیص_بده_Deleted, اندازه_t& n_کامل_تخصیص_بده_Deleted) {
        // Delete assignments to temporaries if they are not used
        إذا (!m_varp->is_Statement_موقت()) أرجع;
        لكل (رکورد_2& کلمه_رکورد : m_کلمه_رکوردها)
            delete_Assignment_If_استفادهنشده(کلمه_رکورد, n_کلمه_تخصیص_بده_Deleted);
        delete_Assignment_If_استفادهنشده(m_کامل_رکورد, n_کامل_تخصیص_بده_Deleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

صنف SubstValidVisitor final : عام VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ثابت uint32_t m_گام;  // Step number where assignment was recorded
    منطقي m_valid = درست;  // Is the expression we are considering to substitute valid

    // METHODS
    Subst_Var_مدخل& get_مدخل(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        أرجع *nodep->varp()->user1u().مقصد<Subst_Var_مدخل*>();
    }

    // VISITORS
    فراغ بازدید(Ast_کلمه_Sel* nodep) override {
        إذا (!m_valid) أرجع;

        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            إذا (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_مدخل& مدخل = get_مدخل(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                إذا (m_گام < مدخل.m_کامل_رکورد.m_گام) m_valid = خطأ;
                إذا (m_گام < مدخل.m_کلمه_رکوردها[idxp->مقصد_UInt()].m_گام) m_valid = خطأ;
                أرجع;
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(AstVarRef* nodep) override {
        إذا (!m_valid) أرجع;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ثابت Subst_Var_مدخل& مدخل = get_مدخل(nodep);
        إذا (m_گام < مدخل.m_کامل_رکورد.m_گام) {
            m_valid = خطأ;
            أرجع;
        }
        لكل (ثابت Subst_Var_مدخل::رکورد_2& کلمه_رکورد : مدخل.m_کلمه_رکوردها) {
            إذا (m_گام < کلمه_رکورد.m_گام) {
                m_valid = خطأ;
                أرجع;
            }
        }
    }

    فراغ بازدید(AstConst*) override {}  // Accelerate
    فراغ بازدید(Ast_متن*) override {}  // CExpr/CStmt literal text has no variable dependencies

    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        إذا (!m_valid) أرجع;
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    صریح SubstValidVisitor(Subst_Var_مدخل::رکورد_2& رکورد)
        : m_گام{رکورد.m_گام} {
        iterateConst(رکورد.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = افتراضي;

عام:
    ساكن منطقي valid(Subst_Var_مدخل::رکورد_2& رکورد) {
        إذا (!رکورد.m_assignp) أرجع خطأ;
        أرجع SubstValidVisitor{رکورد}.m_valid;
    }
};

Ast_گره_عبارت* Subst_Var_مدخل::subst_رکورد(Subst_Var_مدخل::رکورد_2& رکورد) {
    إذا (!SubstValidVisitor::valid(رکورد)) أرجع اشاره_تهی;
    Ast_گره_عبارت* ثابت rhsp = رکورد.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_خالص(), رکورد.m_assignp, "Substituting impure expression"););
    أرجع rhsp;
}

//######################################################################
// Substitution visitor

صنف SubstVisitor final : عام VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_مدخل> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_تخصیص_بده_گام = 0;  // Assignment number to determine variable lifetimes
    ثابت AstCFunc* m_funcp = اشاره_تهی;  // Current function we are under
    اندازه_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    اندازه_t m_n_کلمه_Substituted = 0;  // Number of words substituted
    اندازه_t m_n_کامل_Substituted = 0;  // Number of whole variables substituted
    اندازه_t m_n_کلمه_تخصیص_بده_Deleted = 0;  // Number of word assignments deleted
    اندازه_t m_n_کامل_تخصیص_بده_Deleted = 0;  // Number of whole variable assignments deleted
    اندازه_t m_n_Const_واژگان_Reinlined = 0;  // Number of constant words substituted

    ساكن constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ساكن constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_مدخل& get_مدخل(AstVarRef* nodep) {
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp->user1p()) {
            m_entries.emplace_پشت(varp);
            varp->user1p(&m_entries.پشت());
        }
        أرجع *varp->user1u().مقصد<Subst_Var_مدخل*>();
    }

    فراغ simplify(Ast_گره_عبارت* exprp) {
        // Often constant, so short circuit
        إذا (VN_IS(exprp, Const)) أرجع;
        // Iterate expression, then constant fold it if anything changed
        ثابت اندازه_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), گره_عبارت);
        إذا (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ساكن منطقي isSubstitutable(AstVar* nodep) {
        أرجع nodep->is_Statement_موقت() && !nodep->noSubst();
    }

    فراغ substitute(Ast_گره* nodep, Ast_گره_عبارت* substp) {
        Ast_گره_عبارت* newp = substp->backp() ? substp->clone_درخت_خالص(درست) : substp;
        إذا (!nodep->isQuad() && newp->isQuad()) {
            newp = جديد AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    فراغ بازدید(AstNetlist* nodep) override { iterate_And_بعدی_Null(nodep->modulesp()); }

    فراغ بازدید(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.خالی(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ثابت VNUser1_درون_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            لكل (Subst_Var_مدخل& مدخل : m_entries) {
                مدخل.delete_استفادهنشده_Assignments(m_n_کلمه_تخصیص_بده_Deleted, m_n_کامل_تخصیص_بده_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        إذا (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
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
        ثابت خودکار getAssignp = [&](AstVarRef* refp) -> Ast_گره_تخصیص_بده* {
            // If too complex, don't substitute
            إذا (ops > SUBST_MAX_OPS_SUBST) أرجع اشاره_تهی;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            إذا (VN_IS(nodep->rhsp(), Cvt_Packed_مقصد_Array)) أرجع اشاره_تهی;
            // If non-const but want const subtitutions only
            إذا (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) أرجع اشاره_تهی;
            // Otherwise can substitute based on the assignment
            أرجع nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_مدخل(refp).تخصیص_بده_کامل(getAssignp(refp), ++m_تخصیص_بده_گام);
            أرجع;
        }

        // If LHS is a known word reference, track the word
        إذا (ثابت Ast_کلمه_Sel* ثابت selp = VN_CAST(nodep->lhsp(), کلمه_Sel)) {
            إذا (AstVarRef* ثابت refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                إذا (ثابت AstConst* ثابت idxp = VN_CAST(selp->bitp(), Const)) {
                    get_مدخل(refp).تخصیص_بده_کلمه(getAssignp(refp), ++m_تخصیص_بده_گام, idxp->مقصد_UInt());
                    أرجع;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    فراغ بازدید(Ast_کلمه_Sel* nodep) override {
        إذا (!m_funcp) أرجع;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            إذا (ثابت AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_مدخل& مدخل = get_مدخل(refp);
                ثابت uint32_t کلمه = idxp->مقصد_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                إذا (refp->access().isWriteOrRW()) {
                    مدخل.تخصیص_بده_کلمه(اشاره_تهی, ++m_تخصیص_بده_گام, کلمه);
                    أرجع;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ثابت varp = refp->varp();
                إذا (varp->const_Pool_مدخل()) {
                    AstConst* ثابت constp = VN_AS(varp->valuep(), Const);
                    ثابت uint32_t مقدار = constp->رقم().edata_کلمه(کلمه);
                    File_سطر* ثابت flp = nodep->fileline();
                    ++m_n_Const_واژگان_Reinlined;
                    substitute(nodep, جديد AstConst{flp, AstConst::SizedEData{}, مقدار});
                    أرجع;
                }

                // Substitute other variables if possible
                إذا (isSubstitutable(refp->varp())) {
                    إذا (Ast_گره_عبارت* ثابت substp = مدخل.subst_کلمه(کلمه)) {
                        ++m_n_کلمه_Substituted;
                        substitute(nodep, substp);
                        أرجع;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                مدخل.استفاده_شده_کلمه(کلمه);
                أرجع;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    فراغ بازدید(AstVarRef* nodep) override {
        إذا (!m_funcp) أرجع;

        Subst_Var_مدخل& مدخل = get_مدخل(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        إذا (nodep->access().isWriteOrRW()) {
            مدخل.تخصیص_بده_کامل(اشاره_تهی, ++m_تخصیص_بده_گام);
            أرجع;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        إذا (isSubstitutable(nodep->varp())) {
            إذا (Ast_گره_عبارت* ثابت substp = مدخل.subst_کامل()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                إذا (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_کامل_Substituted;
                    substitute(nodep, substp);
                    أرجع;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        مدخل.استفاده_شده_کامل();
    }

    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        إذا (!m_funcp) أرجع;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        إذا (!nodep->is_خالص()) m_ops = SUBST_MAX_OPS_NA;
    }

    فراغ بازدید(AstVar*) override {}  // Accelerate
    فراغ بازدید(AstConst*) override {}  // Accelerate

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::جمع_کن_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::جمع_کن_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_واژگان_Reinlined);
        V3Stats::جمع_کن_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_کامل_تخصیص_بده_Deleted);
        V3Stats::جمع_کن_Stat("Optimizations, Subst, Whole variables substituted", m_n_کامل_Substituted);
        V3Stats::جمع_کن_Stat("Optimizations, Subst, Word assignments deleted", m_n_کلمه_تخصیص_بده_Deleted);
        V3Stats::جمع_کن_Stat("Optimizations, Subst, Words substituted", m_n_کلمه_Substituted);
        UASSERT(m_entries.خالی(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

فراغ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("subst", 0, dump_درخت_Either_Level() >= 3);
}
