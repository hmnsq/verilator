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

#شامل "V3PchAstMT.h"

#شامل "V3FuncOpt.h"

#شامل "V3Global.h"
#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت Balance_Concat_درخت final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    ساخت رکن_2 final {
        Ast_گرہ_اظہاریہ* exprp = خالی_اشاریہ;
        سائز_t انحراف = 0;

        رکن_2() = طےشدہ;
        رکن_2(Ast_گرہ_اظہاریہ* exprp, سائز_t انحراف)
            : exprp{exprp}
            , انحراف{انحراف} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ساکن خالی gatherTermsRecursive(Ast_گرہ_اظہاریہ* exprp, std::سمتیہ<Ast_گرہ_اظہاریہ*>& terms) {
        اگر (AstConcat* ثابت catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            واپس;
        }
        اگر (AstExtend* ثابت extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_عقب(extp);
            واپس;
        }

        // Base case: different operation
        terms.emplace_عقب(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ساکن std::سمتیہ<Ast_گرہ_اظہاریہ*> gatherTerms(Ast_گرہ_اظہاریہ* rootp) {
        std::سمتیہ<Ast_گرہ_اظہاریہ*> terms;
        اگر (AstConcat* ثابت catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } ورنہ اگر (AstExtend* ثابت extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_عقب(extp);
        } ورنہ {
            rootp->v3fatalSrc("Unexpected node type");
        }
        واپس terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ساکن Ast_گرہ_اظہاریہ* construct(ثابت std::سمتیہ<رکن_2>& terms, ثابت سائز_t begin,
                                  ثابت سائز_t انتہا) {
        UASSERT(انتہا < terms.سائز(), "Invalid end");
        UASSERT(begin < انتہا, "Invalid range");
        // Base case: just return the term
        اگر (انتہا == begin + 1) واپس terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ثابت سائز_t چوڑائی = terms[انتہا].انحراف - terms[begin].انحراف;
        ثابت سائز_t mid_انحراف = چوڑائی / 2 + terms[begin].انحراف;
        ثابت خودکار beginIt = terms.begin() + begin;
        ثابت خودکار انتہا_It = terms.begin() + انتہا;
        ثابت خودکار midIt = std::زیریں_bound(beginIt + 1, انتہا_It - 1, mid_انحراف,  //
                                            [&](ثابت رکن_2& رکن, سائز_t قدر) {  //
                                                واپس رکن.انحراف < قدر;
                                            });
        ثابت سائز_t mid = begin + std::فاصلہ(beginIt, midIt);
        UASSERT(begin < mid && mid < انتہا, "Must make some progress");
        // Construct the subtrees
        Ast_گرہ_اظہاریہ* ثابت rhsp = construct(terms, begin, mid);
        Ast_گرہ_اظہاریہ* ثابت lhsp = construct(terms, mid, انتہا);
        // Construct new node
        Ast_گرہ_اظہاریہ* newp = نیا AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(سچ);  // Must not attempt to balance again.
        واپس newp;
    }

    // Returns replacement node, or nullptr if no change
    ساکن AstConcat* balance(Ast_گرہ_اظہاریہ* ثابت rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        اگر (exprps.سائز() <= 3) واپس خالی_اشاریہ;
        // Don't do it if any of the terms are impure
        برائے (Ast_گرہ_اظہاریہ* ثابت exprp : exprps) {
            اگر (!exprp->is_خالص()) واپس خالی_اشاریہ;
        }

        // Construct the terms Vector that we are going to do processing on
        std::سمتیہ<رکن_2> terms(exprps.سائز() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].انحراف = 0;
        terms[exprps.سائز()].exprp = خالی_اشاریہ;
        برائے (سائز_t i = 0; i < exprps.سائز(); ++i) {
            Ast_گرہ_اظہاریہ* ثابت exprp = [&]() -> Ast_گرہ_اظہاریہ* {
                اگر (AstExtend* ثابت extp = VN_CAST(exprps[i], Extend)) {
                    ثابت صحیح چوڑائی = extp->چوڑائی() - extp->lhsp()->چوڑائی();
                    واپس نیا AstConst{extp->fileline(), AstConst::Widthed_قدر{}, چوڑائی, 0};
                }
                واپس exprps[i]->clone_درخت_خالص(جھوٹ);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].انحراف = terms[i].انحراف + exprp->چوڑائی();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::سمتیہ<رکن_2> terms2;
        {
            terms2.reserve(terms.سائز());

            سائز_t begin = 0;  // Start of current range considered
            سائز_t انتہا = 0;  // End of current range considered
            سائز_t انحراف = 0;  // Offset of current range considered

            // Create a term from the current range
            ثابت خودکار make_رکن = [&]() {
                Ast_گرہ_اظہاریہ* ثابت exprp = construct(terms, begin, انتہا);
                terms2.emplace_عقب(exprp, انحراف);
                انحراف += exprp->چوڑائی();
                begin = انتہا;
            };

            // Create all terms ending on a boundary.
            جب (++انتہا < terms.سائز() - 1) {
                اگر (terms[انتہا].انحراف % VL_EDATASIZE == 0) make_رکن();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_رکن();
            // Sentinel term
            terms2.emplace_عقب(خالی_اشاریہ, انحراف);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.عقب().انحراف == terms.عقب().انحراف, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        واپس VN_AS(construct(terms2, 0, terms2.سائز() - 1), Concat);
    }

عوامی:
    ساکن Ast_گرہ_اظہاریہ* apply(Ast_گرہ_اظہاریہ* nodep) {
        اگر (!v3Global.opt.fFuncBalanceCat()) واپس خالی_اشاریہ;
        اگر (nodep->user1()) واپس خالی_اشاریہ;  // Created by us, don't try to balance again
        اگر (VN_IS(nodep->backp(), Concat)) واپس خالی_اشاریہ;  // Not root of tree
        اگر (VN_IS(nodep->backp(), Extend)) واپس خالی_اشاریہ;  // Not root of tree
        Ast_گرہ_اظہاریہ* ثابت exprp = balance(nodep);
        اگر (exprp) exprp->user1(سچ);  // Must not attempt again.
        واپس exprp;
    }
};

ساخت FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_متوازن_Concats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = طےشدہ;
    ~FuncOptStats() {
        V3Stats::جوڑو_Stat("Optimizations, FuncOpt concat trees balanced", m_متوازن_Concats);
        V3Stats::جوڑو_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

جماعت FuncOptVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ساکن منطقی readsLhs(Ast_گرہ_تفویض* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_سیٹ<ثابت AstVar*> lhsWrVarps;
        std::unordered_سیٹ<ثابت AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
            اگر (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            اگر (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        اگر (lhsWrVarps.سائز() == 1) {
            ثابت AstVar* ثابت lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ثابت منطقی rhsReadsWritten = nodep->rhsp()->exists([=](ثابت AstVarRef* refp) {  //
                واپس refp->varp() == lhsWrVarp;
            });
            اگر (rhsReadsWritten) واپس سچ;
            // Check Lhs doesn't read the written var
            واپس lhsRdVarps.گنتی(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ثابت منطقی rhsReadsWritten = nodep->rhsp()->exists([&](ثابت AstVarRef* refp) {  //
            واپس lhsWrVarps.گنتی(refp->varp());
        });
        اگر (rhsReadsWritten) واپس سچ;
        // Check Lhs doesn't read the written vars
        برائے (ثابت AstVar* ثابت lhsWrVarp : lhsWrVarps) {
            اگر (lhsRdVarps.گنتی(lhsWrVarp)) واپس سچ;
        }
        واپس جھوٹ;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    منطقی بانٹو_Concat(Ast_گرہ_تفویض* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        اگر (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) واپس جھوٹ;
        // Will need the LHS
        Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp();
        اگر (!VN_IS(lhsp->dtypep()->skipRefp(), قطار_DType))
            UASSERT_OBJ(lhsp->چوڑائی() == rhsp->چوڑائی(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        اگر (!nodep->user1() && (!lhsp->is_خالص() || !rhsp->is_خالص())) واپس جھوٹ;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        اگر (lhsp->exists([](AstVarRef* refp) { واپس refp->varp()->isSc(); })) واپس جھوٹ;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        اگر (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            اگر (AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->مقصد_UInt();
            } ورنہ {
                // Don't optimize if it's a variable select
                واپس جھوٹ;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        اگر (lhsp->چوڑائی() <= VL_QUADSIZE) واپس جھوٹ;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ثابت صحیح lsb_لفظ = lsb / VL_EDATASIZE;
        ثابت صحیح msb_لفظ = (lsb + rhsp->چوڑائی() - 1) / VL_EDATASIZE;
        اگر (lsb_لفظ == msb_لفظ) واپس جھوٹ;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        اگر (!nodep->user1() && readsLhs(nodep)) واپس جھوٹ;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_گرہ_اظہاریہ* ثابت rrp = [rhsp]() -> Ast_گرہ_اظہاریہ* {
            اگر (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                واپس catp->rhsp()->unlink_Fr_عقب();
            }
            واپس VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_عقب();
        }();
        Ast_گرہ_اظہاریہ* ثابت rlp = [rhsp, rrp]() -> Ast_گرہ_اظہاریہ* {
            اگر (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                واپس catp->lhsp()->unlink_Fr_عقب();
            }
            ثابت صحیح l_چوڑائی = rhsp->چوڑائی() - rrp->چوڑائی();
            واپس نیا AstConst{rhsp->fileline(), AstConst::Widthed_قدر{}, l_چوڑائی, 0};
        }();
        ثابت صحیح rLsb = lsb;
        ثابت صحیح lLsb = lsb + rrp->چوڑائی();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_تفویض* ثابت arp = نیا Ast_تفویض{
            nodep->fileline(),
            نیا AstSel{lhsp->fileline(), lhsp->clone_درخت_خالص(جھوٹ), rLsb, rrp->چوڑائی()}, rrp};
        Ast_تفویض* ثابت alp = نیا Ast_تفویض{
            nodep->fileline(),
            نیا AstSel{lhsp->fileline(), lhsp->unlink_Fr_عقب(), lLsb, rlp->چوڑائی()}, rlp};
        nodep->جوڑو_اگلا_یہاں(arp);
        arp->جوڑو_اگلا_یہاں(alp);
        // Safe to split these.
        arp->user1(سچ);
        alp->user1(سچ);
        // Nuke what is left
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        واپس سچ;
    }

    //دورہ
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        اگر (v3Global.opt.f_Func_بانٹو_Cat()) {
            اگر (بانٹو_Concat(nodep)) واپس;  // Must return here, in case more code is added below
        }
    }

    خالی دورہ(AstConcat* nodep) override {
        اگر (Ast_گرہ_اظہاریہ* ثابت newp = Balance_Concat_درخت::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    خالی دورہ(AstExtend* nodep) override {
        اگر (Ast_گرہ_اظہاریہ* ثابت newp = Balance_Concat_درخت::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    صریح FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

عوامی:
    ساکن خالی apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

خالی V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        ثابت VNUser1_اندر_Use user1_اندر_Use;
        FuncOptStats stats;
        برائے (Ast_گرہ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), گرہ_Module)) {
            برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (AstCFunc* ثابت cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_درخت("funcopt", 0, dump_درخت_Either_Level() >= 3);
}
