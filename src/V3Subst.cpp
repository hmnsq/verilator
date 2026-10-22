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

वर्ग SubstVarEntry final {
    मित्र वर्ग SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    संरचना Record final {
        // MEMBERS
        Ast_गाँठो_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        तार्किक m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        शून्य invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = असत्य;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* अचर m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<माप_प्रकार>(m_varp->width_शब्दहरू()), Record{}};

    // METHDOS
    शून्य deleteAssignmentIfUnused(Record& record, माप_प्रकार& nAssignDeleted) {
        यदि (!record.m_assignp) फर्काऊ;
        यदि (record.m_used) फर्काऊ;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_रूख(), record.m_assignp);
    }

    Ast_गाँठो_अभिव्यक्ति* substRecord(Record& record);

सार्वजनिक:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = स्वभाव;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    शून्य assignWhole(Ast_गाँठो_Assign* assp, uint32_t step) {
        // Invalidate all word records
        लागि (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = असत्य;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    शून्य assignWord(Ast_गाँठो_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = असत्य;
    }

    // Mark the whole variable as used (value consumed)
    शून्य usedWhole() {
        m_wholeRecord.m_used = सत्य;
        लागि (Record& wordRecord : m_wordRecords) wordRecord.m_used = सत्य;
    }

    // Mark the specific word as used (value consumed)
    शून्य usedWord(uint32_t word) {
        m_wholeRecord.m_used = सत्य;
        m_wordRecords[word].m_used = सत्य;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_गाँठो_अभिव्यक्ति* substWhole() { फर्काऊ substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_गाँठो_अभिव्यक्ति* substWord(uint32_t word) { फर्काऊ substRecord(m_wordRecords[word]); }

    शून्य deleteUnusedAssignments(माप_प्रकार& nWordAssignDeleted, माप_प्रकार& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        यदि (!m_varp->isStatementTemp()) फर्काऊ;
        लागि (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

वर्ग SubstValidVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    अचर uint32_t m_step;  // Step number where assignment was recorded
    तार्किक m_valid = सत्य;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        फर्काऊ *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    शून्य visit(AstWordSel* nodep) override {
        यदि (!m_valid) फर्काऊ;

        यदि (AstVarRef* अचर refp = VN_CAST(nodep->fromp(), VarRef)) {
            यदि (AstConst* अचर idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                यदि (m_step < entry.m_wholeRecord.m_step) m_valid = असत्य;
                यदि (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = असत्य;
                फर्काऊ;
            }
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        यदि (!m_valid) फर्काऊ;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        अचर SubstVarEntry& entry = getEntry(nodep);
        यदि (m_step < entry.m_wholeRecord.m_step) {
            m_valid = असत्य;
            फर्काऊ;
        }
        लागि (अचर SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            यदि (m_step < wordRecord.m_step) {
                m_valid = असत्य;
                फर्काऊ;
            }
        }
    }

    शून्य visit(AstConst*) override {}  // Accelerate
    शून्य visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    शून्य visit(Ast_गाँठो_अभिव्यक्ति* nodep) override {
        यदि (!m_valid) फर्काऊ;
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठो* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = स्वभाव;

सार्वजनिक:
    स्थिर तार्किक valid(SubstVarEntry::Record& record) {
        यदि (!record.m_assignp) फर्काऊ असत्य;
        फर्काऊ SubstValidVisitor{record}.m_valid;
    }
};

Ast_गाँठो_अभिव्यक्ति* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    यदि (!SubstValidVisitor::valid(record)) फर्काऊ nullptr;
    Ast_गाँठो_अभिव्यक्ति* अचर rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    फर्काऊ rhsp;
}

//######################################################################
// Substitution visitor

वर्ग SubstVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    अचर AstCFunc* m_funcp = nullptr;  // Current function we are under
    माप_प्रकार m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    माप_प्रकार m_nWordSubstituted = 0;  // Number of words substituted
    माप_प्रकार m_nWholeSubstituted = 0;  // Number of whole variables substituted
    माप_प्रकार m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    माप_प्रकार m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    माप_प्रकार m_n_Const_शब्दहरू_Reinlined = 0;  // Number of constant words substituted

    स्थिर constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    स्थिर constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* अचर varp = nodep->varp();
        यदि (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        फर्काऊ *varp->user1u().to<SubstVarEntry*>();
    }

    शून्य simplify(Ast_गाँठो_अभिव्यक्ति* exprp) {
        // Often constant, so short circuit
        यदि (VN_IS(exprp, Const)) फर्काऊ;
        // Iterate expression, then constant fold it if anything changed
        अचर माप_प्रकार nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), गाँठो_अभिव्यक्ति);
        यदि (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    स्थिर तार्किक isSubstitutable(AstVar* nodep) {
        फर्काऊ nodep->isStatementTemp() && !nodep->noSubst();
    }

    शून्य substitute(Ast_गाँठो* nodep, Ast_गाँठो_अभिव्यक्ति* substp) {
        Ast_गाँठो_अभिव्यक्ति* newp = substp->backp() ? substp->clone_रूख_Pure(सत्य) : substp;
        यदि (!nodep->isQuad() && newp->isQuad()) {
            newp = नूतन AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    शून्य visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    शून्य visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            अचर VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            लागि (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        यदि (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        यदि (!m_funcp) फर्काऊ;

        अचर uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            फर्काऊ VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        अचर स्वतः getAssignp = [&](AstVarRef* refp) -> Ast_गाँठो_Assign* {
            // If too complex, don't substitute
            यदि (ops > SUBST_MAX_OPS_SUBST) फर्काऊ nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            यदि (VN_IS(nodep->rhsp(), CvtPackedToArray)) फर्काऊ nullptr;
            // If non-const but want const subtitutions only
            यदि (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) फर्काऊ nullptr;
            // Otherwise can substitute based on the assignment
            फर्काऊ nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        यदि (AstVarRef* अचर refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            फर्काऊ;
        }

        // If LHS is a known word reference, track the word
        यदि (अचर AstWordSel* अचर selp = VN_CAST(nodep->lhsp(), WordSel)) {
            यदि (AstVarRef* अचर refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                यदि (अचर AstConst* अचर idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    फर्काऊ;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    शून्य visit(AstWordSel* nodep) override {
        यदि (!m_funcp) फर्काऊ;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        यदि (AstVarRef* अचर refp = VN_CAST(nodep->fromp(), VarRef)) {
            यदि (अचर AstConst* अचर idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                अचर uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                यदि (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    फर्काऊ;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* अचर varp = refp->varp();
                यदि (varp->constPoolEntry()) {
                    AstConst* अचर constp = VN_AS(varp->valuep(), Const);
                    अचर uint32_t मान = constp->अङ्क().edataWord(word);
                    File_पङ्क्ति* अचर flp = nodep->fileline();
                    ++m_n_Const_शब्दहरू_Reinlined;
                    substitute(nodep, नूतन AstConst{flp, AstConst::SizedEData{}, मान});
                    फर्काऊ;
                }

                // Substitute other variables if possible
                यदि (isSubstitutable(refp->varp())) {
                    यदि (Ast_गाँठो_अभिव्यक्ति* अचर substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        फर्काऊ;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                फर्काऊ;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    शून्य visit(AstVarRef* nodep) override {
        यदि (!m_funcp) फर्काऊ;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        यदि (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            फर्काऊ;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        यदि (isSubstitutable(nodep->varp())) {
            यदि (Ast_गाँठो_अभिव्यक्ति* अचर substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                यदि (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    फर्काऊ;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    शून्य visit(Ast_गाँठो_अभिव्यक्ति* nodep) override {
        यदि (!m_funcp) फर्काऊ;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        यदि (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(AstConst*) override {}  // Accelerate

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_शब्दहरू_Reinlined);
        V3Stats::addStat("Optimizations, Subst, Whole variable assignments deleted",
                         m_nWholeAssignDeleted);
        V3Stats::addStat("Optimizations, Subst, Whole variables substituted", m_nWholeSubstituted);
        V3Stats::addStat("Optimizations, Subst, Word assignments deleted", m_nWordAssignDeleted);
        V3Stats::addStat("Optimizations, Subst, Words substituted", m_nWordSubstituted);
        UASSERT(m_entries.empty(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

शून्य V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("subst", 0, dump_रूख_Either_Level() >= 3);
}
