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

#ضمن "V3PchAstMT.h"

#ضمن "V3FuncOpt.h"

#ضمن "V3Global.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

صنف Balance_Concat_وڻ final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    بنية رڪن_2 final {
        Ast_ڳنڍ_اظهار* exprp = خالي_اشارو;
        سائيز_t وٿي = 0;

        رڪن_2() = افتراضي;
        رڪن_2(Ast_ڳنڍ_اظهار* exprp, سائيز_t وٿي)
            : exprp{exprp}
            , وٿي{وٿي} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ساكن فراغ gatherTermsRecursive(Ast_ڳنڍ_اظهار* exprp, std::ويڪٽر<Ast_ڳنڍ_اظهار*>& terms) {
        إذا (AstConcat* ثابت catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            أرجع;
        }
        إذا (AstExtend* ثابت extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_پٺ(extp);
            أرجع;
        }

        // Base case: different operation
        terms.emplace_پٺ(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ساكن std::ويڪٽر<Ast_ڳنڍ_اظهار*> gatherTerms(Ast_ڳنڍ_اظهار* rootp) {
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> terms;
        إذا (AstConcat* ثابت catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } وإلا إذا (AstExtend* ثابت extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_پٺ(extp);
        } وإلا {
            rootp->v3fatalSrc("Unexpected node type");
        }
        أرجع terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ساكن Ast_ڳنڍ_اظهار* construct(ثابت std::ويڪٽر<رڪن_2>& terms, ثابت سائيز_t begin,
                                  ثابت سائيز_t پڇاڙي) {
        UASSERT(پڇاڙي < terms.سائيز(), "Invalid end");
        UASSERT(begin < پڇاڙي, "Invalid range");
        // Base case: just return the term
        إذا (پڇاڙي == begin + 1) أرجع terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ثابت سائيز_t ويڪر = terms[پڇاڙي].وٿي - terms[begin].وٿي;
        ثابت سائيز_t mid_وٿي = ويڪر / 2 + terms[begin].وٿي;
        ثابت خودڪار beginIt = terms.begin() + begin;
        ثابت خودڪار پڇاڙي_It = terms.begin() + پڇاڙي;
        ثابت خودڪار midIt = std::هيٺاهون_bound(beginIt + 1, پڇاڙي_It - 1, mid_وٿي,  //
                                            [&](ثابت رڪن_2& رڪن, سائيز_t قدر) {  //
                                                أرجع رڪن.وٿي < قدر;
                                            });
        ثابت سائيز_t mid = begin + std::مفاصلو(beginIt, midIt);
        UASSERT(begin < mid && mid < پڇاڙي, "Must make some progress");
        // Construct the subtrees
        Ast_ڳنڍ_اظهار* ثابت rhsp = construct(terms, begin, mid);
        Ast_ڳنڍ_اظهار* ثابت lhsp = construct(terms, mid, پڇاڙي);
        // Construct new node
        Ast_ڳنڍ_اظهار* newp = جديد AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(سچ);  // Must not attempt to balance again.
        أرجع newp;
    }

    // Returns replacement node, or nullptr if no change
    ساكن AstConcat* balance(Ast_ڳنڍ_اظهار* ثابت rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ثابت std::ويڪٽر<Ast_ڳنڍ_اظهار*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        إذا (exprps.سائيز() <= 3) أرجع خالي_اشارو;
        // Don't do it if any of the terms are impure
        لكل (Ast_ڳنڍ_اظهار* ثابت exprp : exprps) {
            إذا (!exprp->is_خالص()) أرجع خالي_اشارو;
        }

        // Construct the terms Vector that we are going to do processing on
        std::ويڪٽر<رڪن_2> terms(exprps.سائيز() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].وٿي = 0;
        terms[exprps.سائيز()].exprp = خالي_اشارو;
        لكل (سائيز_t i = 0; i < exprps.سائيز(); ++i) {
            Ast_ڳنڍ_اظهار* ثابت exprp = [&]() -> Ast_ڳنڍ_اظهار* {
                إذا (AstExtend* ثابت extp = VN_CAST(exprps[i], Extend)) {
                    ثابت صحيح ويڪر = extp->ويڪر() - extp->lhsp()->ويڪر();
                    أرجع جديد AstConst{extp->fileline(), AstConst::Widthed_قدر{}, ويڪر, 0};
                }
                أرجع exprps[i]->clone_وڻ_خالص(خطأ);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].وٿي = terms[i].وٿي + exprp->ويڪر();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::ويڪٽر<رڪن_2> terms2;
        {
            terms2.reserve(terms.سائيز());

            سائيز_t begin = 0;  // Start of current range considered
            سائيز_t پڇاڙي = 0;  // End of current range considered
            سائيز_t وٿي = 0;  // Offset of current range considered

            // Create a term from the current range
            ثابت خودڪار make_رڪن = [&]() {
                Ast_ڳنڍ_اظهار* ثابت exprp = construct(terms, begin, پڇاڙي);
                terms2.emplace_پٺ(exprp, وٿي);
                وٿي += exprp->ويڪر();
                begin = پڇاڙي;
            };

            // Create all terms ending on a boundary.
            طالما (++پڇاڙي < terms.سائيز() - 1) {
                إذا (terms[پڇاڙي].وٿي % VL_EDATASIZE == 0) make_رڪن();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_رڪن();
            // Sentinel term
            terms2.emplace_پٺ(خالي_اشارو, وٿي);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.پٺ().وٿي == terms.پٺ().وٿي, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        أرجع VN_AS(construct(terms2, 0, terms2.سائيز() - 1), Concat);
    }

عام:
    ساكن Ast_ڳنڍ_اظهار* apply(Ast_ڳنڍ_اظهار* nodep) {
        إذا (!v3Global.opt.fFuncBalanceCat()) أرجع خالي_اشارو;
        إذا (nodep->user1()) أرجع خالي_اشارو;  // Created by us, don't try to balance again
        إذا (VN_IS(nodep->backp(), Concat)) أرجع خالي_اشارو;  // Not root of tree
        إذا (VN_IS(nodep->backp(), Extend)) أرجع خالي_اشارو;  // Not root of tree
        Ast_ڳنڍ_اظهار* ثابت exprp = balance(nodep);
        إذا (exprp) exprp->user1(سچ);  // Must not attempt again.
        أرجع exprp;
    }
};

بنية FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_متوازن_Concats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = افتراضي;
    ~FuncOptStats() {
        V3Stats::جوڙيو_Stat("Optimizations, FuncOpt concat trees balanced", m_متوازن_Concats);
        V3Stats::جوڙيو_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

صنف FuncOptVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ساكن منطقي readsLhs(Ast_ڳنڍ_سونپيو* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_سيٽ<ثابت AstVar*> lhsWrVarps;
        std::unordered_سيٽ<ثابت AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            إذا (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        إذا (lhsWrVarps.سائيز() == 1) {
            ثابت AstVar* ثابت lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([=](ثابت AstVarRef* refp) {  //
                أرجع refp->varp() == lhsWrVarp;
            });
            إذا (rhsReadsWritten) أرجع سچ;
            // Check Lhs doesn't read the written var
            أرجع lhsRdVarps.ڳڻپ(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([&](ثابت AstVarRef* refp) {  //
            أرجع lhsWrVarps.ڳڻپ(refp->varp());
        });
        إذا (rhsReadsWritten) أرجع سچ;
        // Check Lhs doesn't read the written vars
        لكل (ثابت AstVar* ثابت lhsWrVarp : lhsWrVarps) {
            إذا (lhsRdVarps.ڳڻپ(lhsWrVarp)) أرجع سچ;
        }
        أرجع خطأ;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    منطقي ورهايو_Concat(Ast_ڳنڍ_سونپيو* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        إذا (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) أرجع خطأ;
        // Will need the LHS
        Ast_ڳنڍ_اظهار* lhsp = nodep->lhsp();
        إذا (!VN_IS(lhsp->dtypep()->skipRefp(), قطار_DType))
            UASSERT_OBJ(lhsp->ويڪر() == rhsp->ويڪر(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        إذا (!nodep->user1() && (!lhsp->is_خالص() || !rhsp->is_خالص())) أرجع خطأ;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        إذا (lhsp->exists([](AstVarRef* refp) { أرجع refp->varp()->isSc(); })) أرجع خطأ;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        إذا (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            إذا (AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->منزل_UInt();
            } وإلا {
                // Don't optimize if it's a variable select
                أرجع خطأ;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        إذا (lhsp->ويڪر() <= VL_QUADSIZE) أرجع خطأ;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ثابت صحيح lsb_لفظ = lsb / VL_EDATASIZE;
        ثابت صحيح msb_لفظ = (lsb + rhsp->ويڪر() - 1) / VL_EDATASIZE;
        إذا (lsb_لفظ == msb_لفظ) أرجع خطأ;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        إذا (!nodep->user1() && readsLhs(nodep)) أرجع خطأ;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_ڳنڍ_اظهار* ثابت rrp = [rhsp]() -> Ast_ڳنڍ_اظهار* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->rhsp()->unlink_Fr_پٺ();
            }
            أرجع VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_پٺ();
        }();
        Ast_ڳنڍ_اظهار* ثابت rlp = [rhsp, rrp]() -> Ast_ڳنڍ_اظهار* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->lhsp()->unlink_Fr_پٺ();
            }
            ثابت صحيح l_ويڪر = rhsp->ويڪر() - rrp->ويڪر();
            أرجع جديد AstConst{rhsp->fileline(), AstConst::Widthed_قدر{}, l_ويڪر, 0};
        }();
        ثابت صحيح rLsb = lsb;
        ثابت صحيح lLsb = lsb + rrp->ويڪر();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_سونپيو* ثابت arp = جديد Ast_سونپيو{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->clone_وڻ_خالص(خطأ), rLsb, rrp->ويڪر()}, rrp};
        Ast_سونپيو* ثابت alp = جديد Ast_سونپيو{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->unlink_Fr_پٺ(), lLsb, rlp->ويڪر()}, rlp};
        nodep->جوڙيو_ايندڙ_هتي(arp);
        arp->جوڙيو_ايندڙ_هتي(alp);
        // Safe to split these.
        arp->user1(سچ);
        alp->user1(سچ);
        // Nuke what is left
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        أرجع سچ;
    }

    //گهمو
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        إذا (v3Global.opt.f_Func_ورهايو_Cat()) {
            إذا (ورهايو_Concat(nodep)) أرجع;  // Must return here, in case more code is added below
        }
    }

    فراغ گهمو(AstConcat* nodep) override {
        إذا (Ast_ڳنڍ_اظهار* ثابت newp = Balance_Concat_وڻ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ گهمو(AstExtend* nodep) override {
        إذا (Ast_ڳنڍ_اظهار* ثابت newp = Balance_Concat_وڻ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    واضح FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

عام:
    ساكن فراغ apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

فراغ V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        ثابت VNUser1_اندريون_Use user1_اندريون_Use;
        FuncOptStats stats;
        لكل (Ast_ڳنڍ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ڳنڍ_Module)) {
            لكل (Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstCFunc* ثابت cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_وڻ("funcopt", 0, dump_وڻ_Either_Level() >= 3);
}
