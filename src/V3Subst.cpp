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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Subst.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

वर्ग Subst_Var_प्रविष्टिः final {
    मित्रम् वर्ग SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    संरचना लेखः_2 final {
        // MEMBERS
        Ast_ग्रन्थिः_नियोजय* m_assignp;  // Last assignment to this part
        uint32_t m_चरणः;  // Step number of last assignment. 0 means invalid entry.
        तार्किक m_प्रयुक्तम्;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        लेखः_2() { invalidate(); }
        // METHODS
        शून्य invalidate() {
            m_assignp = शून्यसूचकः;
            m_चरणः = 0;
            m_प्रयुक्तम् = असत्य;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ध्रुव m_varp;
    // The recrod for whole variable tracking
    लेखः_2 m_सम्पूर्णम्_लेखः;
    // A record for each word in the variable
    std::सदिशम्<लेखः_2> m_word_लेखाः{static_cast<आकार_t>(m_varp->विस्तारः_शब्दाः()), लेखः_2{}};

    // METHDOS
    शून्य delete_Assignment_If_अप्रयुक्तम्(लेखः_2& लेखः, आकार_t& n_नियोजय_Deleted) {
        यदि (!लेखः.m_assignp) प्रत्यागम;
        यदि (लेखः.m_प्रयुक्तम्) प्रत्यागम;
        ++n_नियोजय_Deleted;
        VL_DO_DANGLING(लेखः.m_assignp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), लेखः.m_assignp);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* subst_लेखः(लेखः_2& लेखः);

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट Subst_Var_प्रविष्टिः(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_प्रविष्टिः() = स्वभाव;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    शून्य नियोजय_सम्पूर्णम्(Ast_ग्रन्थिः_नियोजय* assp, uint32_t चरणः) {
        // Invalidate all word records
        प्रति (लेखः_2& word_लेखः : m_word_लेखाः) word_लेखः.invalidate();
        // Set whole record
        m_सम्पूर्णम्_लेखः.m_assignp = assp;
        m_सम्पूर्णम्_लेखः.m_चरणः = चरणः;
        m_सम्पूर्णम्_लेखः.m_प्रयुक्तम् = असत्य;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    शून्य नियोजय_Word(Ast_ग्रन्थिः_नियोजय* assp, uint32_t चरणः, uint32_t word) {
        // Invalidate whole record
        m_सम्पूर्णम्_लेखः.invalidate();
        // Set word record
        लेखः_2& word_लेखः = m_word_लेखाः[word];
        word_लेखः.m_assignp = assp;
        word_लेखः.m_चरणः = चरणः;
        word_लेखः.m_प्रयुक्तम् = असत्य;
    }

    // Mark the whole variable as used (value consumed)
    शून्य प्रयुक्तम्_सम्पूर्णम्() {
        m_सम्पूर्णम्_लेखः.m_प्रयुक्तम् = सत्य;
        प्रति (लेखः_2& word_लेखः : m_word_लेखाः) word_लेखः.m_प्रयुक्तम् = सत्य;
    }

    // Mark the specific word as used (value consumed)
    शून्य प्रयुक्तम्_Word(uint32_t word) {
        m_सम्पूर्णम्_लेखः.m_प्रयुक्तम् = सत्य;
        m_word_लेखाः[word].m_प्रयुक्तम् = सत्य;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ग्रन्थिः_व्यञ्जकम्* subst_सम्पूर्णम्() { प्रत्यागम subst_लेखः(m_सम्पूर्णम्_लेखः); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ग्रन्थिः_व्यञ्जकम्* substWord(uint32_t word) { प्रत्यागम subst_लेखः(m_word_लेखाः[word]); }

    शून्य delete_अप्रयुक्तम्_Assignments(आकार_t& n_Word_नियोजय_Deleted, आकार_t& n_सम्पूर्णम्_नियोजय_Deleted) {
        // Delete assignments to temporaries if they are not used
        यदि (!m_varp->is_Statement_अस्थायी()) प्रत्यागम;
        प्रति (लेखः_2& word_लेखः : m_word_लेखाः)
            delete_Assignment_If_अप्रयुक्तम्(word_लेखः, n_Word_नियोजय_Deleted);
        delete_Assignment_If_अप्रयुक्तम्(m_सम्पूर्णम्_लेखः, n_सम्पूर्णम्_नियोजय_Deleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

वर्ग SubstValidVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ध्रुव uint32_t m_चरणः;  // Step number where assignment was recorded
    तार्किक m_valid = सत्य;  // Is the expression we are considering to substitute valid

    // METHODS
    Subst_Var_प्रविष्टिः& get_प्रविष्टिः(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        प्रत्यागम *nodep->varp()->user1u().गन्तव्यम्<Subst_Var_प्रविष्टिः*>();
    }

    // VISITORS
    शून्य दर्शय(AstWordSel* nodep) override {
        यदि (!m_valid) प्रत्यागम;

        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef)) {
            यदि (AstConst* ध्रुव idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_प्रविष्टिः& प्रविष्टिः = get_प्रविष्टिः(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                यदि (m_चरणः < प्रविष्टिः.m_सम्पूर्णम्_लेखः.m_चरणः) m_valid = असत्य;
                यदि (m_चरणः < प्रविष्टिः.m_word_लेखाः[idxp->गन्तव्यम्_UInt()].m_चरणः) m_valid = असत्य;
                प्रत्यागम;
            }
        }
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (!m_valid) प्रत्यागम;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ध्रुव Subst_Var_प्रविष्टिः& प्रविष्टिः = get_प्रविष्टिः(nodep);
        यदि (m_चरणः < प्रविष्टिः.m_सम्पूर्णम्_लेखः.m_चरणः) {
            m_valid = असत्य;
            प्रत्यागम;
        }
        प्रति (ध्रुव Subst_Var_प्रविष्टिः::लेखः_2& word_लेखः : प्रविष्टिः.m_word_लेखाः) {
            यदि (m_चरणः < word_लेखः.m_चरणः) {
                m_valid = असत्य;
                प्रत्यागम;
            }
        }
    }

    शून्य दर्शय(AstConst*) override {}  // Accelerate
    शून्य दर्शय(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        यदि (!m_valid) प्रत्यागम;
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    स्पष्ट SubstValidVisitor(Subst_Var_प्रविष्टिः::लेखः_2& लेखः)
        : m_चरणः{लेखः.m_चरणः} {
        iterateConst(लेखः.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = स्वभाव;

सार्वजनिक:
    स्थिर तार्किक valid(Subst_Var_प्रविष्टिः::लेखः_2& लेखः) {
        यदि (!लेखः.m_assignp) प्रत्यागम असत्य;
        प्रत्यागम SubstValidVisitor{लेखः}.m_valid;
    }
};

Ast_ग्रन्थिः_व्यञ्जकम्* Subst_Var_प्रविष्टिः::subst_लेखः(Subst_Var_प्रविष्टिः::लेखः_2& लेखः) {
    यदि (!SubstValidVisitor::valid(लेखः)) प्रत्यागम शून्यसूचकः;
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = लेखः.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_विशुद्धम्(), लेखः.m_assignp, "Substituting impure expression"););
    प्रत्यागम rhsp;
}

//######################################################################
// Substitution visitor

वर्ग SubstVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_प्रविष्टिः> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_नियोजय_चरणः = 0;  // Assignment number to determine variable lifetimes
    ध्रुव AstCFunc* m_funcp = शून्यसूचकः;  // Current function we are under
    आकार_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    आकार_t m_nWordSubstituted = 0;  // Number of words substituted
    आकार_t m_n_सम्पूर्णम्_Substituted = 0;  // Number of whole variables substituted
    आकार_t m_n_Word_नियोजय_Deleted = 0;  // Number of word assignments deleted
    आकार_t m_n_सम्पूर्णम्_नियोजय_Deleted = 0;  // Number of whole variable assignments deleted
    आकार_t m_n_Const_शब्दाः_Reinlined = 0;  // Number of constant words substituted

    स्थिर constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    स्थिर constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_प्रविष्टिः& get_प्रविष्टिः(AstVarRef* nodep) {
        AstVar* ध्रुव varp = nodep->varp();
        यदि (!varp->user1p()) {
            m_entries.emplace_पृष्ठभागः(varp);
            varp->user1p(&m_entries.पृष्ठभागः());
        }
        प्रत्यागम *varp->user1u().गन्तव्यम्<Subst_Var_प्रविष्टिः*>();
    }

    शून्य simplify(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // Often constant, so short circuit
        यदि (VN_IS(exprp, Const)) प्रत्यागम;
        // Iterate expression, then constant fold it if anything changed
        ध्रुव आकार_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), ग्रन्थिः_व्यञ्जकम्);
        यदि (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    स्थिर तार्किक isSubstitutable(AstVar* nodep) {
        प्रत्यागम nodep->is_Statement_अस्थायी() && !nodep->noSubst();
    }

    शून्य substitute(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* substp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = substp->backp() ? substp->clone_वृक्षः_विशुद्धम्(सत्य) : substp;
        यदि (!nodep->isQuad() && newp->isQuad()) {
            newp = नूतन AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    शून्य दर्शय(AstNetlist* nodep) override { iterate_And_अग्र_Null(nodep->modulesp()); }

    शून्य दर्शय(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.रिक्तानि(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ध्रुव VNUser1_अन्तर्_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            प्रति (Subst_Var_प्रविष्टिः& प्रविष्टिः : m_entries) {
                प्रविष्टिः.delete_अप्रयुक्तम्_Assignments(m_n_Word_नियोजय_Deleted, m_n_सम्पूर्णम्_नियोजय_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        यदि (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (!m_funcp) प्रत्यागम;

        ध्रुव uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            प्रत्यागम VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ध्रुव स्वचालित getAssignp = [&](AstVarRef* refp) -> Ast_ग्रन्थिः_नियोजय* {
            // If too complex, don't substitute
            यदि (ops > SUBST_MAX_OPS_SUBST) प्रत्यागम शून्यसूचकः;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            यदि (VN_IS(nodep->rhsp(), Cvt_Packed_गन्तव्यम्_Array)) प्रत्यागम शून्यसूचकः;
            // If non-const but want const subtitutions only
            यदि (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) प्रत्यागम शून्यसूचकः;
            // Otherwise can substitute based on the assignment
            प्रत्यागम nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_प्रविष्टिः(refp).नियोजय_सम्पूर्णम्(getAssignp(refp), ++m_नियोजय_चरणः);
            प्रत्यागम;
        }

        // If LHS is a known word reference, track the word
        यदि (ध्रुव AstWordSel* ध्रुव selp = VN_CAST(nodep->lhsp(), WordSel)) {
            यदि (AstVarRef* ध्रुव refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                यदि (ध्रुव AstConst* ध्रुव idxp = VN_CAST(selp->bitp(), Const)) {
                    get_प्रविष्टिः(refp).नियोजय_Word(getAssignp(refp), ++m_नियोजय_चरणः, idxp->गन्तव्यम्_UInt());
                    प्रत्यागम;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    शून्य दर्शय(AstWordSel* nodep) override {
        यदि (!m_funcp) प्रत्यागम;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef)) {
            यदि (ध्रुव AstConst* ध्रुव idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_प्रविष्टिः& प्रविष्टिः = get_प्रविष्टिः(refp);
                ध्रुव uint32_t word = idxp->गन्तव्यम्_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                यदि (refp->access().isWriteOrRW()) {
                    प्रविष्टिः.नियोजय_Word(शून्यसूचकः, ++m_नियोजय_चरणः, word);
                    प्रत्यागम;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ध्रुव varp = refp->varp();
                यदि (varp->const_Pool_प्रविष्टिः()) {
                    AstConst* ध्रुव constp = VN_AS(varp->valuep(), Const);
                    ध्रुव uint32_t मानम् = constp->अङ्कः().edataWord(word);
                    File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
                    ++m_n_Const_शब्दाः_Reinlined;
                    substitute(nodep, नूतन AstConst{flp, AstConst::SizedEData{}, मानम्});
                    प्रत्यागम;
                }

                // Substitute other variables if possible
                यदि (isSubstitutable(refp->varp())) {
                    यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव substp = प्रविष्टिः.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        प्रत्यागम;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                प्रविष्टिः.प्रयुक्तम्_Word(word);
                प्रत्यागम;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (!m_funcp) प्रत्यागम;

        Subst_Var_प्रविष्टिः& प्रविष्टिः = get_प्रविष्टिः(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        यदि (nodep->access().isWriteOrRW()) {
            प्रविष्टिः.नियोजय_सम्पूर्णम्(शून्यसूचकः, ++m_नियोजय_चरणः);
            प्रत्यागम;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        यदि (isSubstitutable(nodep->varp())) {
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव substp = प्रविष्टिः.subst_सम्पूर्णम्()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                यदि (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_सम्पूर्णम्_Substituted;
                    substitute(nodep, substp);
                    प्रत्यागम;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        प्रविष्टिः.प्रयुक्तम्_सम्पूर्णम्();
    }

    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        यदि (!m_funcp) प्रत्यागम;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        यदि (!nodep->is_विशुद्धम्()) m_ops = SUBST_MAX_OPS_NA;
    }

    शून्य दर्शय(AstVar*) override {}  // Accelerate
    शून्य दर्शय(AstConst*) override {}  // Accelerate

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::सङ्कलय_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::सङ्कलय_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_शब्दाः_Reinlined);
        V3Stats::सङ्कलय_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_सम्पूर्णम्_नियोजय_Deleted);
        V3Stats::सङ्कलय_Stat("Optimizations, Subst, Whole variables substituted", m_n_सम्पूर्णम्_Substituted);
        V3Stats::सङ्कलय_Stat("Optimizations, Subst, Word assignments deleted", m_n_Word_नियोजय_Deleted);
        V3Stats::सङ्कलय_Stat("Optimizations, Subst, Words substituted", m_nWordSubstituted);
        UASSERT(m_entries.रिक्तानि(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

शून्य V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("subst", 0, dump_वृक्षः_Either_Level() >= 3);
}
