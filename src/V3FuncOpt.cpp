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
    بنية جمله_2 final {
        Ast_گره_عبارت* exprp = اشاره_تهی;
        اندازه_t جابه_جایی = 0;

        جمله_2() = افتراضي;
        جمله_2(Ast_گره_عبارت* exprp, اندازه_t جابه_جایی)
            : exprp{exprp}
            , جابه_جایی{جابه_جایی} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ساكن فراغ gatherTermsRecursive(Ast_گره_عبارت* exprp, std::وکتور<Ast_گره_عبارت*>& terms) {
        إذا (AstConcat* ثابت catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            أرجع;
        }
        إذا (AstExtend* ثابت extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_پشت(extp);
            أرجع;
        }

        // Base case: different operation
        terms.emplace_پشت(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ساكن std::وکتور<Ast_گره_عبارت*> gatherTerms(Ast_گره_عبارت* rootp) {
        std::وکتور<Ast_گره_عبارت*> terms;
        إذا (AstConcat* ثابت catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } وإلا إذا (AstExtend* ثابت extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_پشت(extp);
        } وإلا {
            rootp->v3fatalSrc("Unexpected node type");
        }
        أرجع terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ساكن Ast_گره_عبارت* construct(ثابت std::وکتور<جمله_2>& terms, ثابت اندازه_t begin,
                                  ثابت اندازه_t انتها) {
        UASSERT(انتها < terms.اندازه(), "Invalid end");
        UASSERT(begin < انتها, "Invalid range");
        // Base case: just return the term
        إذا (انتها == begin + 1) أرجع terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ثابت اندازه_t عرض = terms[انتها].جابه_جایی - terms[begin].جابه_جایی;
        ثابت اندازه_t mid_جابه_جایی = عرض / 2 + terms[begin].جابه_جایی;
        ثابت خودکار beginIt = terms.begin() + begin;
        ثابت خودکار انتها_It = terms.begin() + انتها;
        ثابت خودکار midIt = std::پایینی_bound(beginIt + 1, انتها_It - 1, mid_جابه_جایی,  //
                                            [&](ثابت جمله_2& جمله, اندازه_t مقدار) {  //
                                                أرجع جمله.جابه_جایی < مقدار;
                                            });
        ثابت اندازه_t mid = begin + std::فاصله(beginIt, midIt);
        UASSERT(begin < mid && mid < انتها, "Must make some progress");
        // Construct the subtrees
        Ast_گره_عبارت* ثابت rhsp = construct(terms, begin, mid);
        Ast_گره_عبارت* ثابت lhsp = construct(terms, mid, انتها);
        // Construct new node
        Ast_گره_عبارت* newp = جديد AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(درست);  // Must not attempt to balance again.
        أرجع newp;
    }

    // Returns replacement node, or nullptr if no change
    ساكن AstConcat* balance(Ast_گره_عبارت* ثابت rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ثابت std::وکتور<Ast_گره_عبارت*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        إذا (exprps.اندازه() <= 3) أرجع اشاره_تهی;
        // Don't do it if any of the terms are impure
        لكل (Ast_گره_عبارت* ثابت exprp : exprps) {
            إذا (!exprp->is_خالص()) أرجع اشاره_تهی;
        }

        // Construct the terms Vector that we are going to do processing on
        std::وکتور<جمله_2> terms(exprps.اندازه() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].جابه_جایی = 0;
        terms[exprps.اندازه()].exprp = اشاره_تهی;
        لكل (اندازه_t i = 0; i < exprps.اندازه(); ++i) {
            Ast_گره_عبارت* ثابت exprp = [&]() -> Ast_گره_عبارت* {
                إذا (AstExtend* ثابت extp = VN_CAST(exprps[i], Extend)) {
                    ثابت صحيح عرض = extp->عرض() - extp->lhsp()->عرض();
                    أرجع جديد AstConst{extp->fileline(), AstConst::Widthed_مقدار{}, عرض, 0};
                }
                أرجع exprps[i]->clone_درخت_خالص(خطأ);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].جابه_جایی = terms[i].جابه_جایی + exprp->عرض();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::وکتور<جمله_2> terms2;
        {
            terms2.reserve(terms.اندازه());

            اندازه_t begin = 0;  // Start of current range considered
            اندازه_t انتها = 0;  // End of current range considered
            اندازه_t جابه_جایی = 0;  // Offset of current range considered

            // Create a term from the current range
            ثابت خودکار make_جمله = [&]() {
                Ast_گره_عبارت* ثابت exprp = construct(terms, begin, انتها);
                terms2.emplace_پشت(exprp, جابه_جایی);
                جابه_جایی += exprp->عرض();
                begin = انتها;
            };

            // Create all terms ending on a boundary.
            طالما (++انتها < terms.اندازه() - 1) {
                إذا (terms[انتها].جابه_جایی % VL_EDATASIZE == 0) make_جمله();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_جمله();
            // Sentinel term
            terms2.emplace_پشت(اشاره_تهی, جابه_جایی);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.پشت().جابه_جایی == terms.پشت().جابه_جایی, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        أرجع VN_AS(construct(terms2, 0, terms2.اندازه() - 1), Concat);
    }

عام:
    ساكن Ast_گره_عبارت* apply(Ast_گره_عبارت* nodep) {
        إذا (!v3Global.opt.fFuncBalanceCat()) أرجع اشاره_تهی;
        إذا (nodep->user1()) أرجع اشاره_تهی;  // Created by us, don't try to balance again
        إذا (VN_IS(nodep->backp(), Concat)) أرجع اشاره_تهی;  // Not root of tree
        إذا (VN_IS(nodep->backp(), Extend)) أرجع اشاره_تهی;  // Not root of tree
        Ast_گره_عبارت* ثابت exprp = balance(nodep);
        إذا (exprp) exprp->user1(درست);  // Must not attempt again.
        أرجع exprp;
    }
};

بنية FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_متوازن_Concats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = افتراضي;
    ~FuncOptStats() {
        V3Stats::جمع_کن_Stat("Optimizations, FuncOpt concat trees balanced", m_متوازن_Concats);
        V3Stats::جمع_کن_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

صنف FuncOptVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ساكن منطقي readsLhs(Ast_گره_تخصیص_بده* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_مجموعه<ثابت AstVar*> lhsWrVarps;
        std::unordered_مجموعه<ثابت AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            إذا (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        إذا (lhsWrVarps.اندازه() == 1) {
            ثابت AstVar* ثابت lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([=](ثابت AstVarRef* refp) {  //
                أرجع refp->varp() == lhsWrVarp;
            });
            إذا (rhsReadsWritten) أرجع درست;
            // Check Lhs doesn't read the written var
            أرجع lhsRdVarps.شمار(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([&](ثابت AstVarRef* refp) {  //
            أرجع lhsWrVarps.شمار(refp->varp());
        });
        إذا (rhsReadsWritten) أرجع درست;
        // Check Lhs doesn't read the written vars
        لكل (ثابت AstVar* ثابت lhsWrVarp : lhsWrVarps) {
            إذا (lhsRdVarps.شمار(lhsWrVarp)) أرجع درست;
        }
        أرجع خطأ;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    منطقي تقسیم_Concat(Ast_گره_تخصیص_بده* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        إذا (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) أرجع خطأ;
        // Will need the LHS
        Ast_گره_عبارت* lhsp = nodep->lhsp();
        إذا (!VN_IS(lhsp->dtypep()->skipRefp(), صف_DType))
            UASSERT_OBJ(lhsp->عرض() == rhsp->عرض(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        إذا (!nodep->user1() && (!lhsp->is_خالص() || !rhsp->is_خالص())) أرجع خطأ;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        إذا (lhsp->exists([](AstVarRef* refp) { أرجع refp->varp()->isSc(); })) أرجع خطأ;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        إذا (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            إذا (AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->مقصد_UInt();
            } وإلا {
                // Don't optimize if it's a variable select
                أرجع خطأ;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        إذا (lhsp->عرض() <= VL_QUADSIZE) أرجع خطأ;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ثابت صحيح lsb_کلمه = lsb / VL_EDATASIZE;
        ثابت صحيح msb_کلمه = (lsb + rhsp->عرض() - 1) / VL_EDATASIZE;
        إذا (lsb_کلمه == msb_کلمه) أرجع خطأ;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        إذا (!nodep->user1() && readsLhs(nodep)) أرجع خطأ;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_گره_عبارت* ثابت rrp = [rhsp]() -> Ast_گره_عبارت* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->rhsp()->unlink_Fr_پشت();
            }
            أرجع VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_پشت();
        }();
        Ast_گره_عبارت* ثابت rlp = [rhsp, rrp]() -> Ast_گره_عبارت* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->lhsp()->unlink_Fr_پشت();
            }
            ثابت صحيح l_عرض = rhsp->عرض() - rrp->عرض();
            أرجع جديد AstConst{rhsp->fileline(), AstConst::Widthed_مقدار{}, l_عرض, 0};
        }();
        ثابت صحيح rLsb = lsb;
        ثابت صحيح lLsb = lsb + rrp->عرض();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_تخصیص_بده* ثابت arp = جديد Ast_تخصیص_بده{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->clone_درخت_خالص(خطأ), rLsb, rrp->عرض()}, rrp};
        Ast_تخصیص_بده* ثابت alp = جديد Ast_تخصیص_بده{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->unlink_Fr_پشت(), lLsb, rlp->عرض()}, rlp};
        nodep->جمع_کن_بعدی_اینجا(arp);
        arp->جمع_کن_بعدی_اینجا(alp);
        // Safe to split these.
        arp->user1(درست);
        alp->user1(درست);
        // Nuke what is left
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
        أرجع درست;
    }

    //بازدید
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        إذا (v3Global.opt.f_Func_تقسیم_Cat()) {
            إذا (تقسیم_Concat(nodep)) أرجع;  // Must return here, in case more code is added below
        }
    }

    فراغ بازدید(AstConcat* nodep) override {
        إذا (Ast_گره_عبارت* ثابت newp = Balance_Concat_درخت::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ بازدید(AstExtend* nodep) override {
        إذا (Ast_گره_عبارت* ثابت newp = Balance_Concat_درخت::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    صریح FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
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
        ثابت VNUser1_درون_Use user1_درون_Use;
        FuncOptStats stats;
        لكل (Ast_گره_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), گره_Module)) {
            لكل (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstCFunc* ثابت cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_درخت("funcopt", 0, dump_درخت_Either_Level() >= 3);
}
