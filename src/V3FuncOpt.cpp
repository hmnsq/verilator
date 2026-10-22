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
    ರಚನೆ ಪದ_2 final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾತ್ರ_t ಸ್ಥಾನಾಂತರ = 0;

        ಪದ_2() = ಪೂರ್ವನಿಯೋಜಿತ;
        ಪದ_2(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಗಾತ್ರ_t ಸ್ಥಾನಾಂತರ)
            : exprp{exprp}
            , ಸ್ಥಾನಾಂತರ{ಸ್ಥಾನಾಂತರ} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ಸ್ಥಿರ ಶೂನ್ಯ gatherTermsRecursive(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& terms) {
        ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (AstExtend* ಸ್ಥಿರಾಂಕ extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_ಹಿಂಭಾಗ(extp);
            ಹಿಂತಿರುಗಿ;
        }

        // Base case: different operation
        terms.emplace_ಹಿಂಭಾಗ(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ಸ್ಥಿರ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> gatherTerms(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rootp) {
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> terms;
        ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstExtend* ಸ್ಥಿರಾಂಕ extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_ಹಿಂಭಾಗ(extp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            rootp->v3fatalSrc("Unexpected node type");
        }
        ಹಿಂತಿರುಗಿ terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* construct(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪದ_2>& terms, ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t begin,
                                  ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಕೊನೆ) {
        UASSERT(ಕೊನೆ < terms.ಗಾತ್ರ_2(), "Invalid end");
        UASSERT(begin < ಕೊನೆ, "Invalid range");
        // Base case: just return the term
        ಆದರೆ (ಕೊನೆ == begin + 1) ಹಿಂತಿರುಗಿ terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಅಗಲ = terms[ಕೊನೆ].ಸ್ಥಾನಾಂತರ - terms[begin].ಸ್ಥಾನಾಂತರ;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t mid_ಸ್ಥಾನಾಂತರ = ಅಗಲ / 2 + terms[begin].ಸ್ಥಾನಾಂತರ;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ beginIt = terms.begin() + begin;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೊನೆ_It = terms.begin() + ಕೊನೆ;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ midIt = std::ಅಧೋ_bound(beginIt + 1, ಕೊನೆ_It - 1, mid_ಸ್ಥಾನಾಂತರ,  //
                                            [&](ಸ್ಥಿರಾಂಕ ಪದ_2& ಪದ, ಗಾತ್ರ_t ಮೌಲ್ಯ) {  //
                                                ಹಿಂತಿರುಗಿ ಪದ.ಸ್ಥಾನಾಂತರ < ಮೌಲ್ಯ;
                                            });
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t mid = begin + std::ದೂರ(beginIt, midIt);
        UASSERT(begin < mid && mid < ಕೊನೆ, "Must make some progress");
        // Construct the subtrees
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = construct(terms, begin, mid);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = construct(terms, mid, ಕೊನೆ);
        // Construct new node
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಹೊಸ AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(ನಿಜ);  // Must not attempt to balance again.
        ಹಿಂತಿರುಗಿ newp;
    }

    // Returns replacement node, or nullptr if no change
    ಸ್ಥಿರ AstConcat* balance(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        ಆದರೆ (exprps.ಗಾತ್ರ_2() <= 3) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // Don't do it if any of the terms are impure
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp : exprps) {
            ಆದರೆ (!exprp->is_ಶುದ್ಧ()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        // Construct the terms Vector that we are going to do processing on
        std::ಸದಿಶ<ಪದ_2> terms(exprps.ಗಾತ್ರ_2() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].ಸ್ಥಾನಾಂತರ = 0;
        terms[exprps.ಗಾತ್ರ_2()].exprp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < exprps.ಗಾತ್ರ_2(); ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                ಆದರೆ (AstExtend* ಸ್ಥಿರಾಂಕ extp = VN_CAST(exprps[i], Extend)) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = extp->ಅಗಲ() - extp->lhsp()->ಅಗಲ();
                    ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{extp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, ಅಗಲ, 0};
                }
                ಹಿಂತಿರುಗಿ exprps[i]->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].ಸ್ಥಾನಾಂತರ = terms[i].ಸ್ಥಾನಾಂತರ + exprp->ಅಗಲ();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::ಸದಿಶ<ಪದ_2> terms2;
        {
            terms2.reserve(terms.ಗಾತ್ರ_2());

            ಗಾತ್ರ_t begin = 0;  // Start of current range considered
            ಗಾತ್ರ_t ಕೊನೆ = 0;  // End of current range considered
            ಗಾತ್ರ_t ಸ್ಥಾನಾಂತರ = 0;  // Offset of current range considered

            // Create a term from the current range
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ make_ಪದ = [&]() {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = construct(terms, begin, ಕೊನೆ);
                terms2.emplace_ಹಿಂಭಾಗ(exprp, ಸ್ಥಾನಾಂತರ);
                ಸ್ಥಾನಾಂತರ += exprp->ಅಗಲ();
                begin = ಕೊನೆ;
            };

            // Create all terms ending on a boundary.
            ಆಗ (++ಕೊನೆ < terms.ಗಾತ್ರ_2() - 1) {
                ಆದರೆ (terms[ಕೊನೆ].ಸ್ಥಾನಾಂತರ % VL_EDATASIZE == 0) make_ಪದ();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_ಪದ();
            // Sentinel term
            terms2.emplace_ಹಿಂಭಾಗ(ಶೂನ್ಯಸೂಚಕ, ಸ್ಥಾನಾಂತರ);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.ಹಿಂಭಾಗ().ಸ್ಥಾನಾಂತರ == terms.ಹಿಂಭಾಗ().ಸ್ಥಾನಾಂತರ, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        ಹಿಂತಿರುಗಿ VN_AS(construct(terms2, 0, terms2.ಗಾತ್ರ_2() - 1), Concat);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* apply(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (!v3Global.opt.fFuncBalanceCat()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->user1()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Created by us, don't try to balance again
        ಆದರೆ (VN_IS(nodep->backp(), Concat)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Not root of tree
        ಆದರೆ (VN_IS(nodep->backp(), Extend)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Not root of tree
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = balance(nodep);
        ಆದರೆ (exprp) exprp->user1(ನಿಜ);  // Must not attempt again.
        ಹಿಂತಿರುಗಿ exprp;
    }
};

ರಚನೆ FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_ಸಮತೋಲಿತ_Concats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~FuncOptStats() {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, FuncOpt concat trees balanced", m_ಸಮತೋಲಿತ_Concats);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

ವರ್ಗ FuncOptVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ಸ್ಥಿರ ತಾರ್ಕಿಕ readsLhs(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> lhsWrVarps;
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
            ಆದರೆ (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            ಆದರೆ (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        ಆದರೆ (lhsWrVarps.ಗಾತ್ರ_2() == 1) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsReadsWritten = nodep->rhsp()->exists([=](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {  //
                ಹಿಂತಿರುಗಿ refp->varp() == lhsWrVarp;
            });
            ಆದರೆ (rhsReadsWritten) ಹಿಂತಿರುಗಿ ನಿಜ;
            // Check Lhs doesn't read the written var
            ಹಿಂತಿರುಗಿ lhsRdVarps.ಎಣಿಕೆ_2(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsReadsWritten = nodep->rhsp()->exists([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {  //
            ಹಿಂತಿರುಗಿ lhsWrVarps.ಎಣಿಕೆ_2(refp->varp());
        });
        ಆದರೆ (rhsReadsWritten) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Check Lhs doesn't read the written vars
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ lhsWrVarp : lhsWrVarps) {
            ಆದರೆ (lhsRdVarps.ಎಣಿಕೆ_2(lhsWrVarp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    ತಾರ್ಕಿಕ ವಿಭಜಿಸು_Concat(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        ಆದರೆ (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Will need the LHS
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp();
        ಆದರೆ (!VN_IS(lhsp->dtypep()->skipRefp(), ಸರತಿ_DType))
            UASSERT_OBJ(lhsp->ಅಗಲ() == rhsp->ಅಗಲ(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        ಆದರೆ (!nodep->user1() && (!lhsp->is_ಶುದ್ಧ() || !rhsp->is_ಶುದ್ಧ())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        ಆದರೆ (lhsp->exists([](AstVarRef* refp) { ಹಿಂತಿರುಗಿ refp->varp()->isSc(); })) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(lhsp, Sel)) {
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->ಗಮ್ಯ_UInt();
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Don't optimize if it's a variable select
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        ಆದರೆ (lhsp->ಅಗಲ() <= VL_QUADSIZE) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb_ಶಬ್ದ = lsb / VL_EDATASIZE;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb_ಶಬ್ದ = (lsb + rhsp->ಅಗಲ() - 1) / VL_EDATASIZE;
        ಆದರೆ (lsb_ಶಬ್ದ == msb_ಶಬ್ದ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        ಆದರೆ (!nodep->user1() && readsLhs(nodep)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rrp = [rhsp]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ catp = VN_CAST(rhsp, Concat)) {
                ಹಿಂತಿರುಗಿ catp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            }
            ಹಿಂತಿರುಗಿ VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        }();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rlp = [rhsp, rrp]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ catp = VN_CAST(rhsp, Concat)) {
                ಹಿಂತಿರುಗಿ catp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ l_ಅಗಲ = rhsp->ಅಗಲ() - rrp->ಅಗಲ();
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{rhsp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, l_ಅಗಲ, 0};
        }();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rLsb = lsb;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lLsb = lsb + rrp->ಅಗಲ();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ arp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(),
            ಹೊಸ AstSel{lhsp->fileline(), lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), rLsb, rrp->ಅಗಲ()}, rrp};
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ alp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(),
            ಹೊಸ AstSel{lhsp->fileline(), lhsp->unlink_Fr_ಹಿಂಭಾಗ(), lLsb, rlp->ಅಗಲ()}, rlp};
        nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(arp);
        arp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(alp);
        // Safe to split these.
        arp->user1(ನಿಜ);
        alp->user1(ನಿಜ);
        // Nuke what is left
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    //ಭೇಟಿ
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        ಆದರೆ (v3Global.opt.f_Func_ವಿಭಜಿಸು_Cat()) {
            ಆದರೆ (ವಿಭಜಿಸು_Concat(nodep)) ಹಿಂತಿರುಗಿ;  // Must return here, in case more code is added below
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstConcat* nodep) override {
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = Balance_Concat_ವೃಕ್ಷ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_ಸಮತೋಲಿತ_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstExtend* nodep) override {
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = Balance_Concat_ವೃಕ್ಷ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_ಸಮತೋಲಿತ_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
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
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
        FuncOptStats stats;
        ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಆದರೆ (AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("funcopt", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
