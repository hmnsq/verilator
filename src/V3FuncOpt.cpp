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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3FuncOpt.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ Balance_Concat_ವೃಕ್ಷ final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    ಸಂರಚನಾ Term final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nullptr;
        size_t offset = 0;

        Term() = ಸ್ವಭಾವ;
        Term(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ಸ್ಥಿರ ಶೂನ್ಯ gatherTermsRecursive(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& terms) {
        ಯದಿ (AstConcat* ಧ್ರುವ catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (AstExtend* ಧ್ರುವ extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ಸ್ಥಿರ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> gatherTerms(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rootp) {
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> terms;
        ಯದಿ (AstConcat* ಧ್ರುವ catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } ಅನ್ಯಥಾ ಯದಿ (AstExtend* ಧ್ರುವ extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } ಅನ್ಯಥಾ {
            rootp->v3fatalSrc("Unexpected node type");
        }
        ಪ್ರತ್ಯಾಗಮ terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* construct(ಧ್ರುವ std::vector<Term>& terms, ಧ್ರುವ size_t begin,
                                  ಧ್ರುವ size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        ಯದಿ (end == begin + 1) ಪ್ರತ್ಯಾಗಮ terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ಧ್ರುವ size_t width = terms[end].offset - terms[begin].offset;
        ಧ್ರುವ size_t midOffset = width / 2 + terms[begin].offset;
        ಧ್ರುವ auto beginIt = terms.begin() + begin;
        ಧ್ರುವ auto endIt = terms.begin() + end;
        ಧ್ರುವ auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](ಧ್ರುವ Term& term, size_t ಮೌಲ್ಯ) {  //
                                                ಪ್ರತ್ಯಾಗಮ term.offset < ಮೌಲ್ಯ;
                                            });
        ಧ್ರುವ size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = construct(terms, begin, mid);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ನೂತನ AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(ಸತ್ಯ);  // Must not attempt to balance again.
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    // Returns replacement node, or nullptr if no change
    ಸ್ಥಿರ AstConcat* balance(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ಧ್ರುವ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        ಯದಿ (exprps.size() <= 3) ಪ್ರತ್ಯಾಗಮ nullptr;
        // Don't do it if any of the terms are impure
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp : exprps) {
            ಯದಿ (!exprp->isPure()) ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        ಪ್ರತಿ (size_t i = 0; i < exprps.size(); ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                ಯದಿ (AstExtend* ಧ್ರುವ extp = VN_CAST(exprps[i], Extend)) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = extp->width() - extp->lhsp()->width();
                    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{extp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, width, 0};
                }
                ಪ್ರತ್ಯಾಗಮ exprps[i]->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
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
            ಧ್ರುವ auto makeTerm = [&]() {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            ಯಾವತ್ (++end < terms.size() - 1) {
                ಯದಿ (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        ಪ್ರತ್ಯಾಗಮ VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* apply(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (!v3Global.opt.fFuncBalanceCat()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (nodep->user1()) ಪ್ರತ್ಯಾಗಮ nullptr;  // Created by us, don't try to balance again
        ಯದಿ (VN_IS(nodep->backp(), Concat)) ಪ್ರತ್ಯಾಗಮ nullptr;  // Not root of tree
        ಯದಿ (VN_IS(nodep->backp(), Extend)) ಪ್ರತ್ಯಾಗಮ nullptr;  // Not root of tree
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = balance(nodep);
        ಯದಿ (exprp) exprp->user1(ಸತ್ಯ);  // Must not attempt again.
        ಪ್ರತ್ಯಾಗಮ exprp;
    }
};

ಸಂರಚನಾ FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = ಸ್ವಭಾವ;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

ವರ್ಗ FuncOptVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ಸ್ಥಿರ ತಾರ್ಕಿಕ readsLhs(Ast_ಗಂಟು_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<ಧ್ರುವ AstVar*> lhsWrVarps;
        std::unordered_set<ಧ್ರುವ AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ಧ್ರುವ AstVarRef* refp) {
            ಯದಿ (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            ಯದಿ (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        ಯದಿ (lhsWrVarps.size() == 1) {
            ಧ್ರುವ AstVar* ಧ್ರುವ lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ಧ್ರುವ ತಾರ್ಕಿಕ rhsReadsWritten = nodep->rhsp()->exists([=](ಧ್ರುವ AstVarRef* refp) {  //
                ಪ್ರತ್ಯಾಗಮ refp->varp() == lhsWrVarp;
            });
            ಯದಿ (rhsReadsWritten) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // Check Lhs doesn't read the written var
            ಪ್ರತ್ಯಾಗಮ lhsRdVarps.ಎಣಿಕೆ(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ಧ್ರುವ ತಾರ್ಕಿಕ rhsReadsWritten = nodep->rhsp()->exists([&](ಧ್ರುವ AstVarRef* refp) {  //
            ಪ್ರತ್ಯಾಗಮ lhsWrVarps.ಎಣಿಕೆ(refp->varp());
        });
        ಯದಿ (rhsReadsWritten) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Check Lhs doesn't read the written vars
        ಪ್ರತಿ (ಧ್ರುವ AstVar* ಧ್ರುವ lhsWrVarp : lhsWrVarps) {
            ಯದಿ (lhsRdVarps.ಎಣಿಕೆ(lhsWrVarp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    ತಾರ್ಕಿಕ splitConcat(Ast_ಗಂಟು_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        ಯದಿ (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Will need the LHS
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp();
        ಯದಿ (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        ಯದಿ (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        ಯದಿ (lhsp->exists([](AstVarRef* refp) { ಪ್ರತ್ಯಾಗಮ refp->varp()->isSc(); })) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(lhsp, Sel)) {
            ಯದಿ (AstConst* ಧ್ರುವ lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } ಅನ್ಯಥಾ {
                // Don't optimize if it's a variable select
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        ಯದಿ (lhsp->width() <= VL_QUADSIZE) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsbWord = lsb / VL_EDATASIZE;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        ಯದಿ (lsbWord == msbWord) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        ಯದಿ (!nodep->user1() && readsLhs(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rrp = [rhsp]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಯದಿ (AstConcat* ಧ್ರುವ catp = VN_CAST(rhsp, Concat)) {
                ಪ್ರತ್ಯಾಗಮ catp->rhsp()->unlinkFrBack();
            }
            ಪ್ರತ್ಯಾಗಮ VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rlp = [rhsp, rrp]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಯದಿ (AstConcat* ಧ್ರುವ catp = VN_CAST(rhsp, Concat)) {
                ಪ್ರತ್ಯಾಗಮ catp->lhsp()->unlinkFrBack();
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lWidth = rhsp->width() - rrp->width();
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{rhsp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, lWidth, 0};
        }();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rLsb = lsb;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* ಧ್ರುವ arp = ನೂತನ AstAssign{
            nodep->fileline(),
            ನೂತನ AstSel{lhsp->fileline(), lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), rLsb, rrp->width()}, rrp};
        AstAssign* ಧ್ರುವ alp = ನೂತನ AstAssign{
            nodep->fileline(),
            ನೂತನ AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(ಸತ್ಯ);
        alp->user1(ಸತ್ಯ);
        // Nuke what is left
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // VISIT
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        ಯದಿ (v3Global.opt.fFuncSplitCat()) {
            ಯದಿ (splitConcat(nodep)) ಪ್ರತ್ಯಾಗಮ;  // Must return here, in case more code is added below
        }
    }

    ಶೂನ್ಯ visit(AstConcat* nodep) override {
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = Balance_Concat_ವೃಕ್ಷ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstExtend* nodep) override {
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = Balance_Concat_ವೃಕ್ಷ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

ಶೂನ್ಯ V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        ಧ್ರುವ VNUser1InUse user1InUse;
        FuncOptStats stats;
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಯದಿ (AstCFunc* ಧ್ರುವ cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("funcopt", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
