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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3FuncOpt.h"

#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ Balance_Concat_বৃক্ষ final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    সংরচনা Term final {
        Ast_গ্রন্থি_রাশিমালা* exprp = nullptr;
        size_t offset = 0;

        Term() = স্বভাব;
        Term(Ast_গ্রন্থি_রাশিমালা* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    স্থির শূন্য gatherTermsRecursive(Ast_গ্রন্থি_রাশিমালা* exprp, std::vector<Ast_গ্রন্থি_রাশিমালা*>& terms) {
        যদি (AstConcat* ধ্রুব catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            প্রত্যাগম;
        }
        যদি (AstExtend* ধ্রুব extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            প্রত্যাগম;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    স্থির std::vector<Ast_গ্রন্থি_রাশিমালা*> gatherTerms(Ast_গ্রন্থি_রাশিমালা* rootp) {
        std::vector<Ast_গ্রন্থি_রাশিমালা*> terms;
        যদি (AstConcat* ধ্রুব catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } অন্যথা যদি (AstExtend* ধ্রুব extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } অন্যথা {
            rootp->v3fatalSrc("Unexpected node type");
        }
        প্রত্যাগম terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    স্থির Ast_গ্রন্থি_রাশিমালা* construct(ধ্রুব std::vector<Term>& terms, ধ্রুব size_t begin,
                                  ধ্রুব size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        যদি (end == begin + 1) প্রত্যাগম terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ধ্রুব size_t width = terms[end].offset - terms[begin].offset;
        ধ্রুব size_t midOffset = width / 2 + terms[begin].offset;
        ধ্রুব auto beginIt = terms.begin() + begin;
        ধ্রুব auto endIt = terms.begin() + end;
        ধ্রুব auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](ধ্রুব Term& term, size_t মান) {  //
                                                প্রত্যাগম term.offset < মান;
                                            });
        ধ্রুব size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = construct(terms, begin, mid);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_গ্রন্থি_রাশিমালা* newp = নূতন AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(সত্য);  // Must not attempt to balance again.
        প্রত্যাগম newp;
    }

    // Returns replacement node, or nullptr if no change
    স্থির AstConcat* balance(Ast_গ্রন্থি_রাশিমালা* ধ্রুব rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ধ্রুব std::vector<Ast_গ্রন্থি_রাশিমালা*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        যদি (exprps.size() <= 3) প্রত্যাগম nullptr;
        // Don't do it if any of the terms are impure
        প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp : exprps) {
            যদি (!exprp->isPure()) প্রত্যাগম nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        প্রতি (size_t i = 0; i < exprps.size(); ++i) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
                যদি (AstExtend* ধ্রুব extp = VN_CAST(exprps[i], Extend)) {
                    ধ্রুব পূর্ণাঙ্ক width = extp->width() - extp->lhsp()->width();
                    প্রত্যাগম নূতন AstConst{extp->fileline(), AstConst::Widthed_মান{}, width, 0};
                }
                প্রত্যাগম exprps[i]->clone_বৃক্ষ_Pure(অসত্য);
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
            ধ্রুব auto makeTerm = [&]() {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            যাবত্ (++end < terms.size() - 1) {
                যদি (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        প্রত্যাগম VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

সার্বজনিক:
    স্থির Ast_গ্রন্থি_রাশিমালা* apply(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (!v3Global.opt.fFuncBalanceCat()) প্রত্যাগম nullptr;
        যদি (nodep->user1()) প্রত্যাগম nullptr;  // Created by us, don't try to balance again
        যদি (VN_IS(nodep->backp(), Concat)) প্রত্যাগম nullptr;  // Not root of tree
        যদি (VN_IS(nodep->backp(), Extend)) প্রত্যাগম nullptr;  // Not root of tree
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = balance(nodep);
        যদি (exprp) exprp->user1(সত্য);  // Must not attempt again.
        প্রত্যাগম exprp;
    }
};

সংরচনা FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = স্বভাব;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

বর্গ FuncOptVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    স্থির তার্কিক readsLhs(Ast_গ্রন্থি_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<ধ্রুব AstVar*> lhsWrVarps;
        std::unordered_set<ধ্রুব AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ধ্রুব AstVarRef* refp) {
            যদি (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            যদি (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        যদি (lhsWrVarps.size() == 1) {
            ধ্রুব AstVar* ধ্রুব lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ধ্রুব তার্কিক rhsReadsWritten = nodep->rhsp()->exists([=](ধ্রুব AstVarRef* refp) {  //
                প্রত্যাগম refp->varp() == lhsWrVarp;
            });
            যদি (rhsReadsWritten) প্রত্যাগম সত্য;
            // Check Lhs doesn't read the written var
            প্রত্যাগম lhsRdVarps.গুনতি(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ধ্রুব তার্কিক rhsReadsWritten = nodep->rhsp()->exists([&](ধ্রুব AstVarRef* refp) {  //
            প্রত্যাগম lhsWrVarps.গুনতি(refp->varp());
        });
        যদি (rhsReadsWritten) প্রত্যাগম সত্য;
        // Check Lhs doesn't read the written vars
        প্রতি (ধ্রুব AstVar* ধ্রুব lhsWrVarp : lhsWrVarps) {
            যদি (lhsRdVarps.গুনতি(lhsWrVarp)) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    তার্কিক splitConcat(Ast_গ্রন্থি_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        যদি (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) প্রত্যাগম অসত্য;
        // Will need the LHS
        Ast_গ্রন্থি_রাশিমালা* lhsp = nodep->lhsp();
        যদি (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        যদি (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) প্রত্যাগম অসত্য;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        যদি (lhsp->exists([](AstVarRef* refp) { প্রত্যাগম refp->varp()->isSc(); })) প্রত্যাগম অসত্য;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        যদি (AstSel* ধ্রুব selp = VN_CAST(lhsp, Sel)) {
            যদি (AstConst* ধ্রুব lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } অন্যথা {
                // Don't optimize if it's a variable select
                প্রত্যাগম অসত্য;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        যদি (lhsp->width() <= VL_QUADSIZE) প্রত্যাগম অসত্য;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ধ্রুব পূর্ণাঙ্ক lsbWord = lsb / VL_EDATASIZE;
        ধ্রুব পূর্ণাঙ্ক msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        যদি (lsbWord == msbWord) প্রত্যাগম অসত্য;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        যদি (!nodep->user1() && readsLhs(nodep)) প্রত্যাগম অসত্য;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rrp = [rhsp]() -> Ast_গ্রন্থি_রাশিমালা* {
            যদি (AstConcat* ধ্রুব catp = VN_CAST(rhsp, Concat)) {
                প্রত্যাগম catp->rhsp()->unlinkFrBack();
            }
            প্রত্যাগম VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rlp = [rhsp, rrp]() -> Ast_গ্রন্থি_রাশিমালা* {
            যদি (AstConcat* ধ্রুব catp = VN_CAST(rhsp, Concat)) {
                প্রত্যাগম catp->lhsp()->unlinkFrBack();
            }
            ধ্রুব পূর্ণাঙ্ক lWidth = rhsp->width() - rrp->width();
            প্রত্যাগম নূতন AstConst{rhsp->fileline(), AstConst::Widthed_মান{}, lWidth, 0};
        }();
        ধ্রুব পূর্ণাঙ্ক rLsb = lsb;
        ধ্রুব পূর্ণাঙ্ক lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* ধ্রুব arp = নূতন AstAssign{
            nodep->fileline(),
            নূতন AstSel{lhsp->fileline(), lhsp->clone_বৃক্ষ_Pure(অসত্য), rLsb, rrp->width()}, rrp};
        AstAssign* ধ্রুব alp = নূতন AstAssign{
            nodep->fileline(),
            নূতন AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(সত্য);
        alp->user1(সত্য);
        // Nuke what is left
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        প্রত্যাগম সত্য;
    }

    // VISIT
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        যদি (v3Global.opt.fFuncSplitCat()) {
            যদি (splitConcat(nodep)) প্রত্যাগম;  // Must return here, in case more code is added below
        }
    }

    শূন্য visit(AstConcat* nodep) override {
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব newp = Balance_Concat_বৃক্ষ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstExtend* nodep) override {
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব newp = Balance_Concat_বৃক্ষ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

সার্বজনিক:
    স্থির শূন্য apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

শূন্য V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        ধ্রুব VNUser1InUse user1InUse;
        FuncOptStats stats;
        প্রতি (Ast_গ্রন্থি_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
            প্রতি (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (AstCFunc* ধ্রুব cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_বৃক্ষ("funcopt", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
