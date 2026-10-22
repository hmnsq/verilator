// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Generic optimizations on a per function basis
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
//
// - Split assignments to wide locations with Concat/Extend on the RHS
//   at word boundaries:
//    foo = {l, r};
//   becomes (recursively):
//    foo[_:_] = r;
//    foo[_:_] = l;
//
// - Balance concatenation trees, e.g.:
//    {a, {b, {c, d}}
//   becomes:
//    {{a, b}, {c, d}}
//   Reality is more complex here, see the code.
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3FuncOpt.h"

#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ Balance_Concat_વૃક્ષ final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    સંરચના Term final {
        Ast_ગાંઠ_પદાવલિ* exprp = nullptr;
        size_t offset = 0;

        Term() = મૂળભૂત;
        Term(Ast_ગાંઠ_પદાવલિ* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    સ્થિર શૂન્ય gatherTermsRecursive(Ast_ગાંઠ_પદાવલિ* exprp, std::vector<Ast_ગાંઠ_પદાવલિ*>& terms) {
        જો (AstConcat* અચલ catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            પાછા;
        }
        જો (AstExtend* અચલ extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            પાછા;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    સ્થિર std::vector<Ast_ગાંઠ_પદાવલિ*> gatherTerms(Ast_ગાંઠ_પદાવલિ* rootp) {
        std::vector<Ast_ગાંઠ_પદાવલિ*> terms;
        જો (AstConcat* અચલ catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } નહીંતર જો (AstExtend* અચલ extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } નહીંતર {
            rootp->v3fatalSrc("Unexpected node type");
        }
        પાછા terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    સ્થિર Ast_ગાંઠ_પદાવલિ* construct(અચલ std::vector<Term>& terms, અચલ size_t begin,
                                  અચલ size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        જો (end == begin + 1) પાછા terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        અચલ size_t width = terms[end].offset - terms[begin].offset;
        અચલ size_t midOffset = width / 2 + terms[begin].offset;
        અચલ auto beginIt = terms.begin() + begin;
        અચલ auto endIt = terms.begin() + end;
        અચલ auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](અચલ Term& term, size_t કિંમત) {  //
                                                પાછા term.offset < કિંમત;
                                            });
        અચલ size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = construct(terms, begin, mid);
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_ગાંઠ_પદાવલિ* newp = નવું AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(સાચું);  // Must not attempt to balance again.
        પાછા newp;
    }

    // Returns replacement node, or nullptr if no change
    સ્થિર AstConcat* balance(Ast_ગાંઠ_પદાવલિ* અચલ rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        અચલ std::vector<Ast_ગાંઠ_પદાવલિ*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        જો (exprps.size() <= 3) પાછા nullptr;
        // Don't do it if any of the terms are impure
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ exprp : exprps) {
            જો (!exprp->isPure()) પાછા nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        માટે (size_t i = 0; i < exprps.size(); ++i) {
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                જો (AstExtend* અચલ extp = VN_CAST(exprps[i], Extend)) {
                    અચલ પૂર્ણાંક width = extp->width() - extp->lhsp()->width();
                    પાછા નવું AstConst{extp->fileline(), AstConst::Widthed_કિંમત{}, width, 0};
                }
                પાછા exprps[i]->clone_વૃક્ષ_Pure(ખોટું);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].offset = terms[i].offset + exprp->width();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::vector<Term> terms2;
        {
            terms2.reserve(terms.size());

            size_t begin = 0;  // Start of current range considered
            size_t end = 0;  // End of current range considered
            size_t offset = 0;  // Offset of current range considered

            // Create a term from the current range
            અચલ auto makeTerm = [&]() {
                Ast_ગાંઠ_પદાવલિ* અચલ exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            જ્યારે (++end < terms.size() - 1) {
                જો (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            makeTerm();
            // Sentinel term
            terms2.emplace_back(nullptr, offset);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.back().offset == terms.back().offset, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        પાછા VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

જાહેર:
    સ્થિર Ast_ગાંઠ_પદાવલિ* apply(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (!v3Global.opt.fFuncBalanceCat()) પાછા nullptr;
        જો (nodep->user1()) પાછા nullptr;  // Created by us, don't try to balance again
        જો (VN_IS(nodep->backp(), Concat)) પાછા nullptr;  // Not root of tree
        જો (VN_IS(nodep->backp(), Extend)) પાછા nullptr;  // Not root of tree
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = balance(nodep);
        જો (exprp) exprp->user1(સાચું);  // Must not attempt again.
        પાછા exprp;
    }
};

સંરચના FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = મૂળભૂત;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

વર્ગ FuncOptVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    સ્થિર તાર્કિક readsLhs(Ast_ગાંઠ_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<અચલ AstVar*> lhsWrVarps;
        std::unordered_set<અચલ AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](અચલ AstVarRef* refp) {
            જો (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            જો (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        જો (lhsWrVarps.size() == 1) {
            અચલ AstVar* અચલ lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            અચલ તાર્કિક rhsReadsWritten = nodep->rhsp()->exists([=](અચલ AstVarRef* refp) {  //
                પાછા refp->varp() == lhsWrVarp;
            });
            જો (rhsReadsWritten) પાછા સાચું;
            // Check Lhs doesn't read the written var
            પાછા lhsRdVarps.ગણતરી(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        અચલ તાર્કિક rhsReadsWritten = nodep->rhsp()->exists([&](અચલ AstVarRef* refp) {  //
            પાછા lhsWrVarps.ગણતરી(refp->varp());
        });
        જો (rhsReadsWritten) પાછા સાચું;
        // Check Lhs doesn't read the written vars
        માટે (અચલ AstVar* અચલ lhsWrVarp : lhsWrVarps) {
            જો (lhsRdVarps.ગણતરી(lhsWrVarp)) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    તાર્કિક splitConcat(Ast_ગાંઠ_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        જો (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) પાછા ખોટું;
        // Will need the LHS
        Ast_ગાંઠ_પદાવલિ* lhsp = nodep->lhsp();
        જો (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        જો (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) પાછા ખોટું;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        જો (lhsp->exists([](AstVarRef* refp) { પાછા refp->varp()->isSc(); })) પાછા ખોટું;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        જો (AstSel* અચલ selp = VN_CAST(lhsp, Sel)) {
            જો (AstConst* અચલ lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } નહીંતર {
                // Don't optimize if it's a variable select
                પાછા ખોટું;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        જો (lhsp->width() <= VL_QUADSIZE) પાછા ખોટું;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        અચલ પૂર્ણાંક lsbWord = lsb / VL_EDATASIZE;
        અચલ પૂર્ણાંક msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        જો (lsbWord == msbWord) પાછા ખોટું;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        જો (!nodep->user1() && readsLhs(nodep)) પાછા ખોટું;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_ગાંઠ_પદાવલિ* અચલ rrp = [rhsp]() -> Ast_ગાંઠ_પદાવલિ* {
            જો (AstConcat* અચલ catp = VN_CAST(rhsp, Concat)) {
                પાછા catp->rhsp()->unlinkFrBack();
            }
            પાછા VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_ગાંઠ_પદાવલિ* અચલ rlp = [rhsp, rrp]() -> Ast_ગાંઠ_પદાવલિ* {
            જો (AstConcat* અચલ catp = VN_CAST(rhsp, Concat)) {
                પાછા catp->lhsp()->unlinkFrBack();
            }
            અચલ પૂર્ણાંક lWidth = rhsp->width() - rrp->width();
            પાછા નવું AstConst{rhsp->fileline(), AstConst::Widthed_કિંમત{}, lWidth, 0};
        }();
        અચલ પૂર્ણાંક rLsb = lsb;
        અચલ પૂર્ણાંક lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* અચલ arp = નવું AstAssign{
            nodep->fileline(),
            નવું AstSel{lhsp->fileline(), lhsp->clone_વૃક્ષ_Pure(ખોટું), rLsb, rrp->width()}, rrp};
        AstAssign* અચલ alp = નવું AstAssign{
            nodep->fileline(),
            નવું AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(સાચું);
        alp->user1(સાચું);
        // Nuke what is left
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        પાછા સાચું;
    }

    // VISIT
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        જો (v3Global.opt.fFuncSplitCat()) {
            જો (splitConcat(nodep)) પાછા;  // Must return here, in case more code is added below
        }
    }

    શૂન્ય visit(AstConcat* nodep) override {
        જો (Ast_ગાંઠ_પદાવલિ* અચલ newp = Balance_Concat_વૃક્ષ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstExtend* nodep) override {
        જો (Ast_ગાંઠ_પદાવલિ* અચલ newp = Balance_Concat_વૃક્ષ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

જાહેર:
    સ્થિર શૂન્ય apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

શૂન્ય V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        અચલ VNUser1InUse user1InUse;
        FuncOptStats stats;
        માટે (Ast_ગાંઠ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
            માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                જો (AstCFunc* અચલ cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_વૃક્ષ("funcopt", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
