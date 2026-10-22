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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Subst.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

വർഗ്ഗം SubstVarEntry final {
    friend വർഗ്ഗം SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    ഘടന Record final {
        // MEMBERS
        Ast_കണ്ണി_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        തർക്കം m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        ശൂന്യം invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = അസത്യം;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* സ്ഥിരം m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<size_t>(m_varp->width_വാക്കുകൾ()), Record{}};

    // METHDOS
    ശൂന്യം deleteAssignmentIfUnused(Record& record, size_t& nAssignDeleted) {
        എങ്കിൽ (!record.m_assignp) തിരികെ;
        എങ്കിൽ (record.m_used) തിരികെ;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_വൃക്ഷം(), record.m_assignp);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* substRecord(Record& record);

പൊതു:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = സ്ഥിരസ്ഥിതി;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    ശൂന്യം assignWhole(Ast_കണ്ണി_Assign* assp, uint32_t step) {
        // Invalidate all word records
        വേണ്ടി (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = അസത്യം;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    ശൂന്യം assignWord(Ast_കണ്ണി_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = അസത്യം;
    }

    // Mark the whole variable as used (value consumed)
    ശൂന്യം usedWhole() {
        m_wholeRecord.m_used = സത്യം;
        വേണ്ടി (Record& wordRecord : m_wordRecords) wordRecord.m_used = സത്യം;
    }

    // Mark the specific word as used (value consumed)
    ശൂന്യം usedWord(uint32_t word) {
        m_wholeRecord.m_used = സത്യം;
        m_wordRecords[word].m_used = സത്യം;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_കണ്ണി_വ്യഞ്ജകം* substWhole() { തിരികെ substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_കണ്ണി_വ്യഞ്ജകം* substWord(uint32_t word) { തിരികെ substRecord(m_wordRecords[word]); }

    ശൂന്യം deleteUnusedAssignments(size_t& nWordAssignDeleted, size_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        എങ്കിൽ (!m_varp->isStatementTemp()) തിരികെ;
        വേണ്ടി (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

വർഗ്ഗം SubstValidVisitor final : പൊതു VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    സ്ഥിരം uint32_t m_step;  // Step number where assignment was recorded
    തർക്കം m_valid = സത്യം;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        തിരികെ *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    ശൂന്യം visit(AstWordSel* nodep) override {
        എങ്കിൽ (!m_valid) തിരികെ;

        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->fromp(), VarRef)) {
            എങ്കിൽ (AstConst* സ്ഥിരം idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                എങ്കിൽ (m_step < entry.m_wholeRecord.m_step) m_valid = അസത്യം;
                എങ്കിൽ (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = അസത്യം;
                തിരികെ;
            }
        }
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (!m_valid) തിരികെ;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        സ്ഥിരം SubstVarEntry& entry = getEntry(nodep);
        എങ്കിൽ (m_step < entry.m_wholeRecord.m_step) {
            m_valid = അസത്യം;
            തിരികെ;
        }
        വേണ്ടി (സ്ഥിരം SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            എങ്കിൽ (m_step < wordRecord.m_step) {
                m_valid = അസത്യം;
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(AstConst*) override {}  // Accelerate
    ശൂന്യം visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (!m_valid) തിരികെ;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    സ്ഥിതം തർക്കം valid(SubstVarEntry::Record& record) {
        എങ്കിൽ (!record.m_assignp) തിരികെ അസത്യം;
        തിരികെ SubstValidVisitor{record}.m_valid;
    }
};

Ast_കണ്ണി_വ്യഞ്ജകം* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    എങ്കിൽ (!SubstValidVisitor::valid(record)) തിരികെ nullptr;
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    തിരികെ rhsp;
}

//######################################################################
// Substitution visitor

വർഗ്ഗം SubstVisitor final : പൊതു VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    സ്ഥിരം AstCFunc* m_funcp = nullptr;  // Current function we are under
    size_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    size_t m_nWordSubstituted = 0;  // Number of words substituted
    size_t m_nWholeSubstituted = 0;  // Number of whole variables substituted
    size_t m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    size_t m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    size_t m_n_Const_വാക്കുകൾ_Reinlined = 0;  // Number of constant words substituted

    സ്ഥിതം constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    സ്ഥിതം constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        തിരികെ *varp->user1u().to<SubstVarEntry*>();
    }

    ശൂന്യം simplify(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        // Often constant, so short circuit
        എങ്കിൽ (VN_IS(exprp, Const)) തിരികെ;
        // Iterate expression, then constant fold it if anything changed
        സ്ഥിരം size_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), കണ്ണി_വ്യഞ്ജകം);
        എങ്കിൽ (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    സ്ഥിതം തർക്കം isSubstitutable(AstVar* nodep) {
        തിരികെ nodep->isStatementTemp() && !nodep->noSubst();
    }

    ശൂന്യം substitute(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* substp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = substp->backp() ? substp->clone_വൃക്ഷം_Pure(സത്യം) : substp;
        എങ്കിൽ (!nodep->isQuad() && newp->isQuad()) {
            newp = പുതിയ AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    ശൂന്യം visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    ശൂന്യം visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            സ്ഥിരം VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            വേണ്ടി (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        എങ്കിൽ (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (!m_funcp) തിരികെ;

        സ്ഥിരം uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            തിരികെ VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        സ്ഥിരം auto getAssignp = [&](AstVarRef* refp) -> Ast_കണ്ണി_Assign* {
            // If too complex, don't substitute
            എങ്കിൽ (ops > SUBST_MAX_OPS_SUBST) തിരികെ nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            എങ്കിൽ (VN_IS(nodep->rhsp(), CvtPackedToArray)) തിരികെ nullptr;
            // If non-const but want const subtitutions only
            എങ്കിൽ (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) തിരികെ nullptr;
            // Otherwise can substitute based on the assignment
            തിരികെ nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            തിരികെ;
        }

        // If LHS is a known word reference, track the word
        എങ്കിൽ (സ്ഥിരം AstWordSel* സ്ഥിരം selp = VN_CAST(nodep->lhsp(), WordSel)) {
            എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    തിരികെ;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    ശൂന്യം visit(AstWordSel* nodep) override {
        എങ്കിൽ (!m_funcp) തിരികെ;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->fromp(), VarRef)) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                സ്ഥിരം uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                എങ്കിൽ (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    തിരികെ;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* സ്ഥിരം varp = refp->varp();
                എങ്കിൽ (varp->constPoolEntry()) {
                    AstConst* സ്ഥിരം constp = VN_AS(varp->valuep(), Const);
                    സ്ഥിരം uint32_t മൂല്യം = constp->അക്കം().edataWord(word);
                    File_വരി* സ്ഥിരം flp = nodep->fileline();
                    ++m_n_Const_വാക്കുകൾ_Reinlined;
                    substitute(nodep, പുതിയ AstConst{flp, AstConst::SizedEData{}, മൂല്യം});
                    തിരികെ;
                }

                // Substitute other variables if possible
                എങ്കിൽ (isSubstitutable(refp->varp())) {
                    എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        തിരികെ;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                തിരികെ;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (!m_funcp) തിരികെ;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        എങ്കിൽ (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            തിരികെ;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        എങ്കിൽ (isSubstitutable(nodep->varp())) {
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                എങ്കിൽ (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    തിരികെ;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (!m_funcp) തിരികെ;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        എങ്കിൽ (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    ശൂന്യം visit(AstVar*) override {}  // Accelerate
    ശൂന്യം visit(AstConst*) override {}  // Accelerate

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_വാക്കുകൾ_Reinlined);
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

ശൂന്യം V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("subst", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
