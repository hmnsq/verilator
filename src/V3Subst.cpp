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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Subst.h"

#समावेश "V3Const.h"
#समावेश "V3Stats.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

श्रेणी SubstVarEntry final {
    मित्र श्रेणी SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    काष्ठा Record final {
        // MEMBERS
        Ast_गाँठ_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        bool m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        व्योम invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = false;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* स्थिर m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<माप_प्रकार>(m_varp->width_शब्दगण()), Record{}};

    // METHDOS
    व्योम deleteAssignmentIfUnused(Record& record, माप_प्रकार& nAssignDeleted) {
        अगर (!record.m_assignp) वापस;
        अगर (record.m_used) वापस;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_वृक्ष(), record.m_assignp);
    }

    Ast_गाँठ_व्यंजक* substRecord(Record& record);

खुला:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = शेष;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    व्योम assignWhole(Ast_गाँठ_Assign* assp, uint32_t step) {
        // Invalidate all word records
        क्रम (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = false;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    व्योम assignWord(Ast_गाँठ_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = false;
    }

    // Mark the whole variable as used (value consumed)
    व्योम usedWhole() {
        m_wholeRecord.m_used = true;
        क्रम (Record& wordRecord : m_wordRecords) wordRecord.m_used = true;
    }

    // Mark the specific word as used (value consumed)
    व्योम usedWord(uint32_t word) {
        m_wholeRecord.m_used = true;
        m_wordRecords[word].m_used = true;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_गाँठ_व्यंजक* substWhole() { वापस substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_गाँठ_व्यंजक* substWord(uint32_t word) { वापस substRecord(m_wordRecords[word]); }

    व्योम deleteUnusedAssignments(माप_प्रकार& nWordAssignDeleted, माप_प्रकार& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        अगर (!m_varp->is_Statement_अस्थायी()) वापस;
        क्रम (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

श्रेणी SubstValidVisitor final : खुला VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    स्थिर uint32_t m_step;  // Step number where assignment was recorded
    bool m_valid = true;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        वापस *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    व्योम visit(AstWordSel* nodep) override {
        अगर (!m_valid) वापस;

        अगर (AstVarRef* स्थिर refp = VN_CAST(nodep->fromp(), VarRef)) {
            अगर (AstConst* स्थिर idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                अगर (m_step < entry.m_wholeRecord.m_step) m_valid = false;
                अगर (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = false;
                वापस;
            }
        }
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstVarRef* nodep) override {
        अगर (!m_valid) वापस;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        स्थिर SubstVarEntry& entry = getEntry(nodep);
        अगर (m_step < entry.m_wholeRecord.m_step) {
            m_valid = false;
            वापस;
        }
        क्रम (स्थिर SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            अगर (m_step < wordRecord.m_step) {
                m_valid = false;
                वापस;
            }
        }
    }

    व्योम visit(AstConst*) override {}  // Accelerate
    व्योम visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {
        अगर (!m_valid) वापस;
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = शेष;

खुला:
    अटल bool valid(SubstVarEntry::Record& record) {
        अगर (!record.m_assignp) वापस false;
        वापस SubstValidVisitor{record}.m_valid;
    }
};

Ast_गाँठ_व्यंजक* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    अगर (!SubstValidVisitor::valid(record)) वापस nullptr;
    Ast_गाँठ_व्यंजक* स्थिर rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    वापस rhsp;
}

//######################################################################
// Substitution visitor

श्रेणी SubstVisitor final : खुला VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    स्थिर AstCFunc* m_funcp = nullptr;  // Current function we are under
    माप_प्रकार m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    माप_प्रकार m_nWordSubstituted = 0;  // Number of words substituted
    माप_प्रकार m_nWholeSubstituted = 0;  // Number of whole variables substituted
    माप_प्रकार m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    माप_प्रकार m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    माप_प्रकार m_n_Const_शब्दगण_Reinlined = 0;  // Number of constant words substituted

    अटल constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    अटल constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* स्थिर varp = nodep->varp();
        अगर (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        वापस *varp->user1u().to<SubstVarEntry*>();
    }

    व्योम simplify(Ast_गाँठ_व्यंजक* exprp) {
        // Often constant, so short circuit
        अगर (VN_IS(exprp, Const)) वापस;
        // Iterate expression, then constant fold it if anything changed
        स्थिर माप_प्रकार nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), गाँठ_व्यंजक);
        अगर (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    अटल bool isSubstitutable(AstVar* nodep) {
        वापस nodep->is_Statement_अस्थायी() && !nodep->noSubst();
    }

    व्योम substitute(Ast_गाँठ* nodep, Ast_गाँठ_व्यंजक* substp) {
        Ast_गाँठ_व्यंजक* newp = substp->backp() ? substp->clone_वृक्ष_Pure(true) : substp;
        अगर (!nodep->isQuad() && newp->isQuad()) {
            newp = नया AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    व्योम visit(AstNetlist* nodep) override { iterate_And_अग्र_Null(nodep->modulesp()); }

    व्योम visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            स्थिर VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            क्रम (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.सफा();
        }

        // Constant fold here, as Ast size can likely be reduced
        अगर (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (!m_funcp) वापस;

        स्थिर uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            वापस VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        स्थिर स्वतः getAssignp = [&](AstVarRef* refp) -> Ast_गाँठ_Assign* {
            // If too complex, don't substitute
            अगर (ops > SUBST_MAX_OPS_SUBST) वापस nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            अगर (VN_IS(nodep->rhsp(), CvtPackedToArray)) वापस nullptr;
            // If non-const but want const subtitutions only
            अगर (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) वापस nullptr;
            // Otherwise can substitute based on the assignment
            वापस nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        अगर (AstVarRef* स्थिर refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            वापस;
        }

        // If LHS is a known word reference, track the word
        अगर (स्थिर AstWordSel* स्थिर selp = VN_CAST(nodep->lhsp(), WordSel)) {
            अगर (AstVarRef* स्थिर refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                अगर (स्थिर AstConst* स्थिर idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    वापस;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    व्योम visit(AstWordSel* nodep) override {
        अगर (!m_funcp) वापस;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        अगर (AstVarRef* स्थिर refp = VN_CAST(nodep->fromp(), VarRef)) {
            अगर (स्थिर AstConst* स्थिर idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                स्थिर uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                अगर (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    वापस;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* स्थिर varp = refp->varp();
                अगर (varp->constPoolEntry()) {
                    AstConst* स्थिर constp = VN_AS(varp->valuep(), Const);
                    स्थिर uint32_t मूल्य = constp->अंक().edataWord(word);
                    File_पंक्ति* स्थिर flp = nodep->fileline();
                    ++m_n_Const_शब्दगण_Reinlined;
                    substitute(nodep, नया AstConst{flp, AstConst::SizedEData{}, मूल्य});
                    वापस;
                }

                // Substitute other variables if possible
                अगर (isSubstitutable(refp->varp())) {
                    अगर (Ast_गाँठ_व्यंजक* स्थिर substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        वापस;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                वापस;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    व्योम visit(AstVarRef* nodep) override {
        अगर (!m_funcp) वापस;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        अगर (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            वापस;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        अगर (isSubstitutable(nodep->varp())) {
            अगर (Ast_गाँठ_व्यंजक* स्थिर substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                अगर (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    वापस;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {
        अगर (!m_funcp) वापस;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        अगर (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    व्योम visit(AstVar*) override {}  // Accelerate
    व्योम visit(AstConst*) override {}  // Accelerate

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_शब्दगण_Reinlined);
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

व्योम V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("subst", 0, dump_वृक्ष_Either_Level() >= 3);
}
