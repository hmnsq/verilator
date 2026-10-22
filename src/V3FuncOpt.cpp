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

رده Balance_Concat_درخت final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    ساختار Term final {
        Ast_گره_عبارت* exprp = nullptr;
        size_t offset = 0;

        Term() = پیشفرض;
        Term(Ast_گره_عبارت* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ایستا تهی gatherTermsRecursive(Ast_گره_عبارت* exprp, std::vector<Ast_گره_عبارت*>& terms) {
        اگر (AstConcat* ثابت catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            بازگشت;
        }
        اگر (AstExtend* ثابت extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            بازگشت;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ایستا std::vector<Ast_گره_عبارت*> gatherTerms(Ast_گره_عبارت* rootp) {
        std::vector<Ast_گره_عبارت*> terms;
        اگر (AstConcat* ثابت catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } وگرنه اگر (AstExtend* ثابت extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } وگرنه {
            rootp->v3fatalSrc("Unexpected node type");
        }
        بازگشت terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ایستا Ast_گره_عبارت* construct(ثابت std::vector<Term>& terms, ثابت size_t begin,
                                  ثابت size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        اگر (end == begin + 1) بازگشت terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ثابت size_t width = terms[end].offset - terms[begin].offset;
        ثابت size_t midOffset = width / 2 + terms[begin].offset;
        ثابت auto beginIt = terms.begin() + begin;
        ثابت auto endIt = terms.begin() + end;
        ثابت auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](ثابت Term& term, size_t مقدار) {  //
                                                بازگشت term.offset < مقدار;
                                            });
        ثابت size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_گره_عبارت* ثابت rhsp = construct(terms, begin, mid);
        Ast_گره_عبارت* ثابت lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_گره_عبارت* newp = جدید AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(درست);  // Must not attempt to balance again.
        بازگشت newp;
    }

    // Returns replacement node, or nullptr if no change
    ایستا AstConcat* balance(Ast_گره_عبارت* ثابت rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ثابت std::vector<Ast_گره_عبارت*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        اگر (exprps.size() <= 3) بازگشت nullptr;
        // Don't do it if any of the terms are impure
        برای (Ast_گره_عبارت* ثابت exprp : exprps) {
            اگر (!exprp->isPure()) بازگشت nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        برای (size_t i = 0; i < exprps.size(); ++i) {
            Ast_گره_عبارت* ثابت exprp = [&]() -> Ast_گره_عبارت* {
                اگر (AstExtend* ثابت extp = VN_CAST(exprps[i], Extend)) {
                    ثابت صحیح width = extp->width() - extp->lhsp()->width();
                    بازگشت جدید AstConst{extp->fileline(), AstConst::Widthed_مقدار{}, width, 0};
                }
                بازگشت exprps[i]->clone_درخت_Pure(نادرست);
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
            ثابت auto makeTerm = [&]() {
                Ast_گره_عبارت* ثابت exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            وقتی (++end < terms.size() - 1) {
                اگر (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        بازگشت VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

عمومی:
    ایستا Ast_گره_عبارت* apply(Ast_گره_عبارت* nodep) {
        اگر (!v3Global.opt.fFuncBalanceCat()) بازگشت nullptr;
        اگر (nodep->user1()) بازگشت nullptr;  // Created by us, don't try to balance again
        اگر (VN_IS(nodep->backp(), Concat)) بازگشت nullptr;  // Not root of tree
        اگر (VN_IS(nodep->backp(), Extend)) بازگشت nullptr;  // Not root of tree
        Ast_گره_عبارت* ثابت exprp = balance(nodep);
        اگر (exprp) exprp->user1(درست);  // Must not attempt again.
        بازگشت exprp;
    }
};

ساختار FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = پیشفرض;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

رده FuncOptVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ایستا منطقی readsLhs(Ast_گره_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<ثابت AstVar*> lhsWrVarps;
        std::unordered_set<ثابت AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
            اگر (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            اگر (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        اگر (lhsWrVarps.size() == 1) {
            ثابت AstVar* ثابت lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ثابت منطقی rhsReadsWritten = nodep->rhsp()->exists([=](ثابت AstVarRef* refp) {  //
                بازگشت refp->varp() == lhsWrVarp;
            });
            اگر (rhsReadsWritten) بازگشت درست;
            // Check Lhs doesn't read the written var
            بازگشت lhsRdVarps.شمار(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ثابت منطقی rhsReadsWritten = nodep->rhsp()->exists([&](ثابت AstVarRef* refp) {  //
            بازگشت lhsWrVarps.شمار(refp->varp());
        });
        اگر (rhsReadsWritten) بازگشت درست;
        // Check Lhs doesn't read the written vars
        برای (ثابت AstVar* ثابت lhsWrVarp : lhsWrVarps) {
            اگر (lhsRdVarps.شمار(lhsWrVarp)) بازگشت درست;
        }
        بازگشت نادرست;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    منطقی splitConcat(Ast_گره_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        اگر (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) بازگشت نادرست;
        // Will need the LHS
        Ast_گره_عبارت* lhsp = nodep->lhsp();
        اگر (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        اگر (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) بازگشت نادرست;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        اگر (lhsp->exists([](AstVarRef* refp) { بازگشت refp->varp()->isSc(); })) بازگشت نادرست;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        اگر (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            اگر (AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } وگرنه {
                // Don't optimize if it's a variable select
                بازگشت نادرست;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        اگر (lhsp->width() <= VL_QUADSIZE) بازگشت نادرست;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ثابت صحیح lsbWord = lsb / VL_EDATASIZE;
        ثابت صحیح msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        اگر (lsbWord == msbWord) بازگشت نادرست;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        اگر (!nodep->user1() && readsLhs(nodep)) بازگشت نادرست;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_گره_عبارت* ثابت rrp = [rhsp]() -> Ast_گره_عبارت* {
            اگر (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                بازگشت catp->rhsp()->unlinkFrBack();
            }
            بازگشت VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_گره_عبارت* ثابت rlp = [rhsp, rrp]() -> Ast_گره_عبارت* {
            اگر (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                بازگشت catp->lhsp()->unlinkFrBack();
            }
            ثابت صحیح lWidth = rhsp->width() - rrp->width();
            بازگشت جدید AstConst{rhsp->fileline(), AstConst::Widthed_مقدار{}, lWidth, 0};
        }();
        ثابت صحیح rLsb = lsb;
        ثابت صحیح lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* ثابت arp = جدید AstAssign{
            nodep->fileline(),
            جدید AstSel{lhsp->fileline(), lhsp->clone_درخت_Pure(نادرست), rLsb, rrp->width()}, rrp};
        AstAssign* ثابت alp = جدید AstAssign{
            nodep->fileline(),
            جدید AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(درست);
        alp->user1(درست);
        // Nuke what is left
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        بازگشت درست;
    }

    // VISIT
    تهی visit(Ast_گره_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        اگر (v3Global.opt.fFuncSplitCat()) {
            اگر (splitConcat(nodep)) بازگشت;  // Must return here, in case more code is added below
        }
    }

    تهی visit(AstConcat* nodep) override {
        اگر (Ast_گره_عبارت* ثابت newp = Balance_Concat_درخت::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    تهی visit(AstExtend* nodep) override {
        اگر (Ast_گره_عبارت* ثابت newp = Balance_Concat_درخت::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

عمومی:
    ایستا تهی apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

تهی V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        ثابت VNUser1InUse user1InUse;
        FuncOptStats stats;
        برای (Ast_گره_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), گره_Module)) {
            برای (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (AstCFunc* ثابت cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_درخت("funcopt", 0, dump_درخت_Either_Level() >= 3);
}
