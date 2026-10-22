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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3FuncOpt.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ Balance_Concat_ਰੁੱਖ final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    ਬਣਤਰ ਪਦ_2 final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = ਖਾਲੀ_ਸੂਚਕ;
        ਮਾਪ_t ਵਿੱਥ = 0;

        ਪਦ_2() = ਮੂਲ;
        ਪਦ_2(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਮਾਪ_t ਵਿੱਥ)
            : exprp{exprp}
            , ਵਿੱਥ{ਵਿੱਥ} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ਸਥਿਰ ਖਾਲੀ gatherTermsRecursive(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& terms) {
        ਜੇ (AstConcat* ਅਚਲ catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            ਵਾਪਸ;
        }
        ਜੇ (AstExtend* ਅਚਲ extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_ਪਿੱਠ(extp);
            ਵਾਪਸ;
        }

        // Base case: different operation
        terms.emplace_ਪਿੱਠ(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ਸਥਿਰ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> gatherTerms(Ast_ਗੰਢ_ਵਿਅੰਜਕ* rootp) {
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> terms;
        ਜੇ (AstConcat* ਅਚਲ catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } ਨਹੀਂਤਾਂ ਜੇ (AstExtend* ਅਚਲ extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_ਪਿੱਠ(extp);
        } ਨਹੀਂਤਾਂ {
            rootp->v3fatalSrc("Unexpected node type");
        }
        ਵਾਪਸ terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* construct(ਅਚਲ std::ਸਦਿਸ਼<ਪਦ_2>& terms, ਅਚਲ ਮਾਪ_t begin,
                                  ਅਚਲ ਮਾਪ_t ਸਿਰਾ) {
        UASSERT(ਸਿਰਾ < terms.ਮਾਪ(), "Invalid end");
        UASSERT(begin < ਸਿਰਾ, "Invalid range");
        // Base case: just return the term
        ਜੇ (ਸਿਰਾ == begin + 1) ਵਾਪਸ terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ਅਚਲ ਮਾਪ_t ਚੌੜਾਈ = terms[ਸਿਰਾ].ਵਿੱਥ - terms[begin].ਵਿੱਥ;
        ਅਚਲ ਮਾਪ_t mid_ਵਿੱਥ = ਚੌੜਾਈ / 2 + terms[begin].ਵਿੱਥ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ beginIt = terms.begin() + begin;
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਸਿਰਾ_It = terms.begin() + ਸਿਰਾ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ midIt = std::ਹੇਠਲਾ_bound(beginIt + 1, ਸਿਰਾ_It - 1, mid_ਵਿੱਥ,  //
                                            [&](ਅਚਲ ਪਦ_2& ਪਦ, ਮਾਪ_t ਮੁੱਲ) {  //
                                                ਵਾਪਸ ਪਦ.ਵਿੱਥ < ਮੁੱਲ;
                                            });
        ਅਚਲ ਮਾਪ_t mid = begin + std::ਦੂਰੀ(beginIt, midIt);
        UASSERT(begin < mid && mid < ਸਿਰਾ, "Must make some progress");
        // Construct the subtrees
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = construct(terms, begin, mid);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = construct(terms, mid, ਸਿਰਾ);
        // Construct new node
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਨਵਾਂ AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(ਸੱਚ);  // Must not attempt to balance again.
        ਵਾਪਸ newp;
    }

    // Returns replacement node, or nullptr if no change
    ਸਥਿਰ AstConcat* balance(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        ਜੇ (exprps.ਮਾਪ() <= 3) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // Don't do it if any of the terms are impure
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp : exprps) {
            ਜੇ (!exprp->is_ਸ਼ੁੱਧ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        // Construct the terms Vector that we are going to do processing on
        std::ਸਦਿਸ਼<ਪਦ_2> terms(exprps.ਮਾਪ() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].ਵਿੱਥ = 0;
        terms[exprps.ਮਾਪ()].exprp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਮਾਪ_t i = 0; i < exprps.ਮਾਪ(); ++i) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                ਜੇ (AstExtend* ਅਚਲ extp = VN_CAST(exprps[i], Extend)) {
                    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = extp->ਚੌੜਾਈ() - extp->lhsp()->ਚੌੜਾਈ();
                    ਵਾਪਸ ਨਵਾਂ AstConst{extp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, ਚੌੜਾਈ, 0};
                }
                ਵਾਪਸ exprps[i]->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].ਵਿੱਥ = terms[i].ਵਿੱਥ + exprp->ਚੌੜਾਈ();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::ਸਦਿਸ਼<ਪਦ_2> terms2;
        {
            terms2.reserve(terms.ਮਾਪ());

            ਮਾਪ_t begin = 0;  // Start of current range considered
            ਮਾਪ_t ਸਿਰਾ = 0;  // End of current range considered
            ਮਾਪ_t ਵਿੱਥ = 0;  // Offset of current range considered

            // Create a term from the current range
            ਅਚਲ ਸਵੈਚਾਲਿਤ make_ਪਦ = [&]() {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = construct(terms, begin, ਸਿਰਾ);
                terms2.emplace_ਪਿੱਠ(exprp, ਵਿੱਥ);
                ਵਿੱਥ += exprp->ਚੌੜਾਈ();
                begin = ਸਿਰਾ;
            };

            // Create all terms ending on a boundary.
            ਜਦੋਂ (++ਸਿਰਾ < terms.ਮਾਪ() - 1) {
                ਜੇ (terms[ਸਿਰਾ].ਵਿੱਥ % VL_EDATASIZE == 0) make_ਪਦ();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_ਪਦ();
            // Sentinel term
            terms2.emplace_ਪਿੱਠ(ਖਾਲੀ_ਸੂਚਕ, ਵਿੱਥ);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.ਪਿੱਠ().ਵਿੱਥ == terms.ਪਿੱਠ().ਵਿੱਥ, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        ਵਾਪਸ VN_AS(construct(terms2, 0, terms2.ਮਾਪ() - 1), Concat);
    }

ਜਨਤਕ:
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* apply(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (!v3Global.opt.fFuncBalanceCat()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->user1()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Created by us, don't try to balance again
        ਜੇ (VN_IS(nodep->backp(), Concat)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Not root of tree
        ਜੇ (VN_IS(nodep->backp(), Extend)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Not root of tree
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = balance(nodep);
        ਜੇ (exprp) exprp->user1(ਸੱਚ);  // Must not attempt again.
        ਵਾਪਸ exprp;
    }
};

ਬਣਤਰ FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_ਸੰਤੁਲਿਤ_Concats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = ਮੂਲ;
    ~FuncOptStats() {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, FuncOpt concat trees balanced", m_ਸੰਤੁਲਿਤ_Concats);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

ਵਰਗ FuncOptVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ਸਥਿਰ ਤਾਰਕਿਕ readsLhs(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_ਸੈੱਟ<ਅਚਲ AstVar*> lhsWrVarps;
        std::unordered_ਸੈੱਟ<ਅਚਲ AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ਅਚਲ AstVarRef* refp) {
            ਜੇ (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            ਜੇ (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        ਜੇ (lhsWrVarps.ਮਾਪ() == 1) {
            ਅਚਲ AstVar* ਅਚਲ lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ਅਚਲ ਤਾਰਕਿਕ rhsReadsWritten = nodep->rhsp()->exists([=](ਅਚਲ AstVarRef* refp) {  //
                ਵਾਪਸ refp->varp() == lhsWrVarp;
            });
            ਜੇ (rhsReadsWritten) ਵਾਪਸ ਸੱਚ;
            // Check Lhs doesn't read the written var
            ਵਾਪਸ lhsRdVarps.ਗਿਣਤੀ_2(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ਅਚਲ ਤਾਰਕਿਕ rhsReadsWritten = nodep->rhsp()->exists([&](ਅਚਲ AstVarRef* refp) {  //
            ਵਾਪਸ lhsWrVarps.ਗਿਣਤੀ_2(refp->varp());
        });
        ਜੇ (rhsReadsWritten) ਵਾਪਸ ਸੱਚ;
        // Check Lhs doesn't read the written vars
        ਲਈ (ਅਚਲ AstVar* ਅਚਲ lhsWrVarp : lhsWrVarps) {
            ਜੇ (lhsRdVarps.ਗਿਣਤੀ_2(lhsWrVarp)) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    ਤਾਰਕਿਕ ਵੰਡੋ_Concat(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        ਜੇ (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) ਵਾਪਸ ਝੂਠ;
        // Will need the LHS
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nodep->lhsp();
        ਜੇ (!VN_IS(lhsp->dtypep()->skipRefp(), ਕਤਾਰ_DType))
            UASSERT_OBJ(lhsp->ਚੌੜਾਈ() == rhsp->ਚੌੜਾਈ(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        ਜੇ (!nodep->user1() && (!lhsp->is_ਸ਼ੁੱਧ() || !rhsp->is_ਸ਼ੁੱਧ())) ਵਾਪਸ ਝੂਠ;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        ਜੇ (lhsp->exists([](AstVarRef* refp) { ਵਾਪਸ refp->varp()->isSc(); })) ਵਾਪਸ ਝੂਠ;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(lhsp, Sel)) {
            ਜੇ (AstConst* ਅਚਲ lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->ਮੰਜ਼ਿਲ_UInt();
            } ਨਹੀਂਤਾਂ {
                // Don't optimize if it's a variable select
                ਵਾਪਸ ਝੂਠ;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        ਜੇ (lhsp->ਚੌੜਾਈ() <= VL_QUADSIZE) ਵਾਪਸ ਝੂਠ;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ਅਚਲ ਪੂਰਨ lsb_ਸ਼ਬਦ = lsb / VL_EDATASIZE;
        ਅਚਲ ਪੂਰਨ msb_ਸ਼ਬਦ = (lsb + rhsp->ਚੌੜਾਈ() - 1) / VL_EDATASIZE;
        ਜੇ (lsb_ਸ਼ਬਦ == msb_ਸ਼ਬਦ) ਵਾਪਸ ਝੂਠ;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        ਜੇ (!nodep->user1() && readsLhs(nodep)) ਵਾਪਸ ਝੂਠ;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rrp = [rhsp]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            ਜੇ (AstConcat* ਅਚਲ catp = VN_CAST(rhsp, Concat)) {
                ਵਾਪਸ catp->rhsp()->unlink_Fr_ਪਿੱਠ();
            }
            ਵਾਪਸ VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_ਪਿੱਠ();
        }();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rlp = [rhsp, rrp]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            ਜੇ (AstConcat* ਅਚਲ catp = VN_CAST(rhsp, Concat)) {
                ਵਾਪਸ catp->lhsp()->unlink_Fr_ਪਿੱਠ();
            }
            ਅਚਲ ਪੂਰਨ l_ਚੌੜਾਈ = rhsp->ਚੌੜਾਈ() - rrp->ਚੌੜਾਈ();
            ਵਾਪਸ ਨਵਾਂ AstConst{rhsp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, l_ਚੌੜਾਈ, 0};
        }();
        ਅਚਲ ਪੂਰਨ rLsb = lsb;
        ਅਚਲ ਪੂਰਨ lLsb = lsb + rrp->ਚੌੜਾਈ();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_ਸੌਂਪੋ* ਅਚਲ arp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(),
            ਨਵਾਂ AstSel{lhsp->fileline(), lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), rLsb, rrp->ਚੌੜਾਈ()}, rrp};
        Ast_ਸੌਂਪੋ* ਅਚਲ alp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(),
            ਨਵਾਂ AstSel{lhsp->fileline(), lhsp->unlink_Fr_ਪਿੱਠ(), lLsb, rlp->ਚੌੜਾਈ()}, rlp};
        nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(arp);
        arp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(alp);
        // Safe to split these.
        arp->user1(ਸੱਚ);
        alp->user1(ਸੱਚ);
        // Nuke what is left
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        ਵਾਪਸ ਸੱਚ;
    }

    //ਦੇਖੋ
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        ਜੇ (v3Global.opt.f_Func_ਵੰਡੋ_Cat()) {
            ਜੇ (ਵੰਡੋ_Concat(nodep)) ਵਾਪਸ;  // Must return here, in case more code is added below
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstConcat* nodep) override {
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = Balance_Concat_ਰੁੱਖ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_ਸੰਤੁਲਿਤ_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstExtend* nodep) override {
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = Balance_Concat_ਰੁੱਖ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_ਸੰਤੁਲਿਤ_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

ਖਾਲੀ V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
        FuncOptStats stats;
        ਲਈ (Ast_ਗੰਢ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
            ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ਜੇ (AstCFunc* ਅਚਲ cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_ਰੁੱਖ("funcopt", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
