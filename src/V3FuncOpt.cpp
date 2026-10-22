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

صنف Balance_Concat_ونه final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    بنية غړی_2 final {
        Ast_غوټه_بیان* exprp = تش_اشاره;
        اندازه_t دننه_واټن = 0;

        غړی_2() = افتراضي;
        غړی_2(Ast_غوټه_بیان* exprp, اندازه_t دننه_واټن)
            : exprp{exprp}
            , دننه_واټن{دننه_واټن} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ساكن فراغ gatherTermsRecursive(Ast_غوټه_بیان* exprp, std::ویکټور<Ast_غوټه_بیان*>& terms) {
        إذا (AstConcat* ثابت catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            أرجع;
        }
        إذا (AstExtend* ثابت extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_شا(extp);
            أرجع;
        }

        // Base case: different operation
        terms.emplace_شا(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ساكن std::ویکټور<Ast_غوټه_بیان*> gatherTerms(Ast_غوټه_بیان* rootp) {
        std::ویکټور<Ast_غوټه_بیان*> terms;
        إذا (AstConcat* ثابت catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } وإلا إذا (AstExtend* ثابت extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_شا(extp);
        } وإلا {
            rootp->v3fatalSrc("Unexpected node type");
        }
        أرجع terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ساكن Ast_غوټه_بیان* construct(ثابت std::ویکټور<غړی_2>& terms, ثابت اندازه_t begin,
                                  ثابت اندازه_t پای_ټکی) {
        UASSERT(پای_ټکی < terms.اندازه(), "Invalid end");
        UASSERT(begin < پای_ټکی, "Invalid range");
        // Base case: just return the term
        إذا (پای_ټکی == begin + 1) أرجع terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ثابت اندازه_t پلنوالی = terms[پای_ټکی].دننه_واټن - terms[begin].دننه_واټن;
        ثابت اندازه_t mid_دننه_واټن = پلنوالی / 2 + terms[begin].دننه_واټن;
        ثابت خپلکاری beginIt = terms.begin() + begin;
        ثابت خپلکاری پای_ټکی_It = terms.begin() + پای_ټکی;
        ثابت خپلکاری midIt = std::ښکتنی_bound(beginIt + 1, پای_ټکی_It - 1, mid_دننه_واټن,  //
                                            [&](ثابت غړی_2& غړی, اندازه_t ارزښت) {  //
                                                أرجع غړی.دننه_واټن < ارزښت;
                                            });
        ثابت اندازه_t mid = begin + std::واټن(beginIt, midIt);
        UASSERT(begin < mid && mid < پای_ټکی, "Must make some progress");
        // Construct the subtrees
        Ast_غوټه_بیان* ثابت rhsp = construct(terms, begin, mid);
        Ast_غوټه_بیان* ثابت lhsp = construct(terms, mid, پای_ټکی);
        // Construct new node
        Ast_غوټه_بیان* newp = جديد AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(ریښتیا);  // Must not attempt to balance again.
        أرجع newp;
    }

    // Returns replacement node, or nullptr if no change
    ساكن AstConcat* balance(Ast_غوټه_بیان* ثابت rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ثابت std::ویکټور<Ast_غوټه_بیان*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        إذا (exprps.اندازه() <= 3) أرجع تش_اشاره;
        // Don't do it if any of the terms are impure
        لكل (Ast_غوټه_بیان* ثابت exprp : exprps) {
            إذا (!exprp->is_خالص()) أرجع تش_اشاره;
        }

        // Construct the terms Vector that we are going to do processing on
        std::ویکټور<غړی_2> terms(exprps.اندازه() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].دننه_واټن = 0;
        terms[exprps.اندازه()].exprp = تش_اشاره;
        لكل (اندازه_t i = 0; i < exprps.اندازه(); ++i) {
            Ast_غوټه_بیان* ثابت exprp = [&]() -> Ast_غوټه_بیان* {
                إذا (AstExtend* ثابت extp = VN_CAST(exprps[i], Extend)) {
                    ثابت صحيح پلنوالی = extp->پلنوالی() - extp->lhsp()->پلنوالی();
                    أرجع جديد AstConst{extp->fileline(), AstConst::Widthed_ارزښت{}, پلنوالی, 0};
                }
                أرجع exprps[i]->clone_ونه_خالص(خطأ);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].دننه_واټن = terms[i].دننه_واټن + exprp->پلنوالی();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::ویکټور<غړی_2> terms2;
        {
            terms2.reserve(terms.اندازه());

            اندازه_t begin = 0;  // Start of current range considered
            اندازه_t پای_ټکی = 0;  // End of current range considered
            اندازه_t دننه_واټن = 0;  // Offset of current range considered

            // Create a term from the current range
            ثابت خپلکاری make_غړی = [&]() {
                Ast_غوټه_بیان* ثابت exprp = construct(terms, begin, پای_ټکی);
                terms2.emplace_شا(exprp, دننه_واټن);
                دننه_واټن += exprp->پلنوالی();
                begin = پای_ټکی;
            };

            // Create all terms ending on a boundary.
            طالما (++پای_ټکی < terms.اندازه() - 1) {
                إذا (terms[پای_ټکی].دننه_واټن % VL_EDATASIZE == 0) make_غړی();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_غړی();
            // Sentinel term
            terms2.emplace_شا(تش_اشاره, دننه_واټن);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.شا().دننه_واټن == terms.شا().دننه_واټن, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        أرجع VN_AS(construct(terms2, 0, terms2.اندازه() - 1), Concat);
    }

عام:
    ساكن Ast_غوټه_بیان* apply(Ast_غوټه_بیان* nodep) {
        إذا (!v3Global.opt.fFuncBalanceCat()) أرجع تش_اشاره;
        إذا (nodep->user1()) أرجع تش_اشاره;  // Created by us, don't try to balance again
        إذا (VN_IS(nodep->backp(), Concat)) أرجع تش_اشاره;  // Not root of tree
        إذا (VN_IS(nodep->backp(), Extend)) أرجع تش_اشاره;  // Not root of tree
        Ast_غوټه_بیان* ثابت exprp = balance(nodep);
        إذا (exprp) exprp->user1(ریښتیا);  // Must not attempt again.
        أرجع exprp;
    }
};

بنية FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_متوازن_Concats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = افتراضي;
    ~FuncOptStats() {
        V3Stats::جمع_کړه_Stat("Optimizations, FuncOpt concat trees balanced", m_متوازن_Concats);
        V3Stats::جمع_کړه_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

صنف FuncOptVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ساكن منطقي readsLhs(Ast_غوټه_ټاکل* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_سېټ<ثابت AstVar*> lhsWrVarps;
        std::unordered_سېټ<ثابت AstVar*> lhsRdVarps;
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
            إذا (rhsReadsWritten) أرجع ریښتیا;
            // Check Lhs doesn't read the written var
            أرجع lhsRdVarps.شمېر(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([&](ثابت AstVarRef* refp) {  //
            أرجع lhsWrVarps.شمېر(refp->varp());
        });
        إذا (rhsReadsWritten) أرجع ریښتیا;
        // Check Lhs doesn't read the written vars
        لكل (ثابت AstVar* ثابت lhsWrVarp : lhsWrVarps) {
            إذا (lhsRdVarps.شمېر(lhsWrVarp)) أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    منطقي وېشل_Concat(Ast_غوټه_ټاکل* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        إذا (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) أرجع خطأ;
        // Will need the LHS
        Ast_غوټه_بیان* lhsp = nodep->lhsp();
        إذا (!VN_IS(lhsp->dtypep()->skipRefp(), کتار_DType))
            UASSERT_OBJ(lhsp->پلنوالی() == rhsp->پلنوالی(), nodep, "Inconsistent assignment");
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
        إذا (lhsp->پلنوالی() <= VL_QUADSIZE) أرجع خطأ;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ثابت صحيح lsb_کلمه = lsb / VL_EDATASIZE;
        ثابت صحيح msb_کلمه = (lsb + rhsp->پلنوالی() - 1) / VL_EDATASIZE;
        إذا (lsb_کلمه == msb_کلمه) أرجع خطأ;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        إذا (!nodep->user1() && readsLhs(nodep)) أرجع خطأ;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_غوټه_بیان* ثابت rrp = [rhsp]() -> Ast_غوټه_بیان* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->rhsp()->unlink_Fr_شا();
            }
            أرجع VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_شا();
        }();
        Ast_غوټه_بیان* ثابت rlp = [rhsp, rrp]() -> Ast_غوټه_بیان* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->lhsp()->unlink_Fr_شا();
            }
            ثابت صحيح l_پلنوالی = rhsp->پلنوالی() - rrp->پلنوالی();
            أرجع جديد AstConst{rhsp->fileline(), AstConst::Widthed_ارزښت{}, l_پلنوالی, 0};
        }();
        ثابت صحيح rLsb = lsb;
        ثابت صحيح lLsb = lsb + rrp->پلنوالی();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_ټاکل* ثابت arp = جديد Ast_ټاکل{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->clone_ونه_خالص(خطأ), rLsb, rrp->پلنوالی()}, rrp};
        Ast_ټاکل* ثابت alp = جديد Ast_ټاکل{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->unlink_Fr_شا(), lLsb, rlp->پلنوالی()}, rlp};
        nodep->جمع_کړه_بل_دلته(arp);
        arp->جمع_کړه_بل_دلته(alp);
        // Safe to split these.
        arp->user1(ریښتیا);
        alp->user1(ریښتیا);
        // Nuke what is left
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        أرجع ریښتیا;
    }

    //لیدنه
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        إذا (v3Global.opt.f_Func_وېشل_Cat()) {
            إذا (وېشل_Concat(nodep)) أرجع;  // Must return here, in case more code is added below
        }
    }

    فراغ لیدنه(AstConcat* nodep) override {
        إذا (Ast_غوټه_بیان* ثابت newp = Balance_Concat_ونه::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstExtend* nodep) override {
        إذا (Ast_غوټه_بیان* ثابت newp = Balance_Concat_ونه::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_متوازن_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    څرګند FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
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
        ثابت VNUser1_دننه_Use user1_دننه_Use;
        FuncOptStats stats;
        لكل (Ast_غوټه_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), غوټه_Module)) {
            لكل (Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstCFunc* ثابت cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_ونه("funcopt", 0, dump_ونه_Either_Level() >= 3);
}
