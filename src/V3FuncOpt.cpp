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

صنف Balance_Concat_درخت final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    بنية رکن_2 final {
        Ast_گرہ_اظہاریہ* exprp = خالی_سوچک;
        ناپ_t وتھ = 0;

        رکن_2() = افتراضي;
        رکن_2(Ast_گرہ_اظہاریہ* exprp, ناپ_t وتھ)
            : exprp{exprp}
            , وتھ{وتھ} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ساكن فراغ gatherTermsRecursive(Ast_گرہ_اظہاریہ* exprp, std::سمتیہ<Ast_گرہ_اظہاریہ*>& terms) {
        إذا (AstConcat* ثابت catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            أرجع;
        }
        إذا (AstExtend* ثابت extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_پٹھ(extp);
            أرجع;
        }

        // Base case: different operation
        terms.emplace_پٹھ(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ساكن std::سمتیہ<Ast_گرہ_اظہاریہ*> gatherTerms(Ast_گرہ_اظہاریہ* rootp) {
        std::سمتیہ<Ast_گرہ_اظہاریہ*> terms;
        إذا (AstConcat* ثابت catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } وإلا إذا (AstExtend* ثابت extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_پٹھ(extp);
        } وإلا {
            rootp->v3fatalSrc("Unexpected node type");
        }
        أرجع terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ساكن Ast_گرہ_اظہاریہ* construct(ثابت std::سمتیہ<رکن_2>& terms, ثابت ناپ_t begin,
                                  ثابت ناپ_t سرا) {
        UASSERT(سرا < terms.ناپ(), "Invalid end");
        UASSERT(begin < سرا, "Invalid range");
        // Base case: just return the term
        إذا (سرا == begin + 1) أرجع terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ثابت ناپ_t چوڑائی = terms[سرا].وتھ - terms[begin].وتھ;
        ثابت ناپ_t mid_وتھ = چوڑائی / 2 + terms[begin].وتھ;
        ثابت خودکار beginIt = terms.begin() + begin;
        ثابت خودکار سرا_It = terms.begin() + سرا;
        ثابت خودکار midIt = std::ہیٹھلا_bound(beginIt + 1, سرا_It - 1, mid_وتھ,  //
                                            [&](ثابت رکن_2& رکن, ناپ_t قدر) {  //
                                                أرجع رکن.وتھ < قدر;
                                            });
        ثابت ناپ_t mid = begin + std::فاصلہ(beginIt, midIt);
        UASSERT(begin < mid && mid < سرا, "Must make some progress");
        // Construct the subtrees
        Ast_گرہ_اظہاریہ* ثابت rhsp = construct(terms, begin, mid);
        Ast_گرہ_اظہاریہ* ثابت lhsp = construct(terms, mid, سرا);
        // Construct new node
        Ast_گرہ_اظہاریہ* newp = جديد AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(سچ);  // Must not attempt to balance again.
        أرجع newp;
    }

    // Returns replacement node, or nullptr if no change
    ساكن AstConcat* balance(Ast_گرہ_اظہاریہ* ثابت rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        إذا (exprps.ناپ() <= 3) أرجع خالی_سوچک;
        // Don't do it if any of the terms are impure
        لكل (Ast_گرہ_اظہاریہ* ثابت exprp : exprps) {
            إذا (!exprp->is_خالص()) أرجع خالی_سوچک;
        }

        // Construct the terms Vector that we are going to do processing on
        std::سمتیہ<رکن_2> terms(exprps.ناپ() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].وتھ = 0;
        terms[exprps.ناپ()].exprp = خالی_سوچک;
        لكل (ناپ_t i = 0; i < exprps.ناپ(); ++i) {
            Ast_گرہ_اظہاریہ* ثابت exprp = [&]() -> Ast_گرہ_اظہاریہ* {
                إذا (AstExtend* ثابت extp = VN_CAST(exprps[i], Extend)) {
                    ثابت صحيح چوڑائی = extp->چوڑائی() - extp->lhsp()->چوڑائی();
                    أرجع جديد AstConst{extp->fileline(), AstConst::Widthed_قدر{}, چوڑائی, 0};
                }
                أرجع exprps[i]->clone_درخت_خالص(خطأ);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].وتھ = terms[i].وتھ + exprp->چوڑائی();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::سمتیہ<رکن_2> terms2;
        {
            terms2.reserve(terms.ناپ());

            ناپ_t begin = 0;  // Start of current range considered
            ناپ_t سرا = 0;  // End of current range considered
            ناپ_t وتھ = 0;  // Offset of current range considered

            // Create a term from the current range
            ثابت خودکار make_رکن = [&]() {
                Ast_گرہ_اظہاریہ* ثابت exprp = construct(terms, begin, سرا);
                terms2.emplace_پٹھ(exprp, وتھ);
                وتھ += exprp->چوڑائی();
                begin = سرا;
            };

            // Create all terms ending on a boundary.
            طالما (++سرا < terms.ناپ() - 1) {
                إذا (terms[سرا].وتھ % VL_EDATASIZE == 0) make_رکن();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_رکن();
            // Sentinel term
            terms2.emplace_پٹھ(خالی_سوچک, وتھ);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.پٹھ().وتھ == terms.پٹھ().وتھ, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        أرجع VN_AS(construct(terms2, 0, terms2.ناپ() - 1), Concat);
    }

عام:
    ساكن Ast_گرہ_اظہاریہ* apply(Ast_گرہ_اظہاریہ* nodep) {
        إذا (!v3Global.opt.fFuncBalanceCat()) أرجع خالی_سوچک;
        إذا (nodep->user1()) أرجع خالی_سوچک;  // Created by us, don't try to balance again
        إذا (VN_IS(nodep->backp(), Concat)) أرجع خالی_سوچک;  // Not root of tree
        إذا (VN_IS(nodep->backp(), Extend)) أرجع خالی_سوچک;  // Not root of tree
        Ast_گرہ_اظہاریہ* ثابت exprp = balance(nodep);
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
        V3Stats::جوڑو_Stat("Optimizations, FuncOpt concat trees balanced", m_متوازن_Concats);
        V3Stats::جوڑو_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

صنف FuncOptVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ساكن منطقي readsLhs(Ast_گرہ_سونپو* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_سیٹ<ثابت AstVar*> lhsWrVarps;
        std::unordered_سیٹ<ثابت AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            إذا (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        إذا (lhsWrVarps.ناپ() == 1) {
            ثابت AstVar* ثابت lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([=](ثابت AstVarRef* refp) {  //
                أرجع refp->varp() == lhsWrVarp;
            });
            إذا (rhsReadsWritten) أرجع سچ;
            // Check Lhs doesn't read the written var
            أرجع lhsRdVarps.گنتی(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([&](ثابت AstVarRef* refp) {  //
            أرجع lhsWrVarps.گنتی(refp->varp());
        });
        إذا (rhsReadsWritten) أرجع سچ;
        // Check Lhs doesn't read the written vars
        لكل (ثابت AstVar* ثابت lhsWrVarp : lhsWrVarps) {
            إذا (lhsRdVarps.گنتی(lhsWrVarp)) أرجع سچ;
        }
        أرجع خطأ;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    منطقي ونڈو_Concat(Ast_گرہ_سونپو* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        إذا (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) أرجع خطأ;
        // Will need the LHS
        Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp();
        إذا (!VN_IS(lhsp->dtypep()->skipRefp(), قطار_DType))
            UASSERT_OBJ(lhsp->چوڑائی() == rhsp->چوڑائی(), nodep, "Inconsistent assignment");
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
        إذا (lhsp->چوڑائی() <= VL_QUADSIZE) أرجع خطأ;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ثابت صحيح lsb_لفظ = lsb / VL_EDATASIZE;
        ثابت صحيح msb_لفظ = (lsb + rhsp->چوڑائی() - 1) / VL_EDATASIZE;
        إذا (lsb_لفظ == msb_لفظ) أرجع خطأ;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        إذا (!nodep->user1() && readsLhs(nodep)) أرجع خطأ;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_گرہ_اظہاریہ* ثابت rrp = [rhsp]() -> Ast_گرہ_اظہاریہ* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->rhsp()->unlink_Fr_پٹھ();
            }
            أرجع VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_پٹھ();
        }();
        Ast_گرہ_اظہاریہ* ثابت rlp = [rhsp, rrp]() -> Ast_گرہ_اظہاریہ* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->lhsp()->unlink_Fr_پٹھ();
            }
            ثابت صحيح l_چوڑائی = rhsp->چوڑائی() - rrp->چوڑائی();
            أرجع جديد AstConst{rhsp->fileline(), AstConst::Widthed_قدر{}, l_چوڑائی, 0};
        }();
        ثابت صحيح rLsb = lsb;
        ثابت صحيح lLsb = lsb + rrp->چوڑائی();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_سونپو* ثابت arp = جديد Ast_سونپو{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->clone_درخت_خالص(خطأ), rLsb, rrp->چوڑائی()}, rrp};
        Ast_سونپو* ثابت alp = جديد Ast_سونپو{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->unlink_Fr_پٹھ(), lLsb, rlp->چوڑائی()}, rlp};
        nodep->جوڑو_اگلا_ایتھے(arp);
        arp->جوڑو_اگلا_ایتھے(alp);
        // Safe to split these.
        arp->user1(سچ);
        alp->user1(سچ);
        // Nuke what is left
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
        أرجع سچ;
    }

    //ویکھو
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        إذا (v3Global.opt.f_Func_ونڈو_Cat()) {
            إذا (ونڈو_Concat(nodep)) أرجع;  // Must return here, in case more code is added below
        }
    }

    فراغ ویکھو(AstConcat* nodep) override {
        إذا (Ast_گرہ_اظہاریہ* ثابت newp = Balance_Concat_درخت::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstExtend* nodep) override {
        إذا (Ast_گرہ_اظہاریہ* ثابت newp = Balance_Concat_درخت::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

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
        ثابت VNUser1_وچ_Use user1_وچ_Use;
        FuncOptStats stats;
        لكل (Ast_گرہ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), گرہ_Module)) {
            لكل (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstCFunc* ثابت cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_درخت("funcopt", 0, dump_درخت_Either_Level() >= 3);
}
