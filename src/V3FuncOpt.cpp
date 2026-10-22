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

صنف Balance_Concat_دەرەخ final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    بنية Term final {
        Ast_تۈگۈن_ئىپادە* exprp = nullptr;
        size_t offset = 0;

        Term() = افتراضي;
        Term(Ast_تۈگۈن_ئىپادە* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ساكن فراغ gatherTermsRecursive(Ast_تۈگۈن_ئىپادە* exprp, std::vector<Ast_تۈگۈن_ئىپادە*>& terms) {
        إذا (AstConcat* ثابت catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            أرجع;
        }
        إذا (AstExtend* ثابت extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            أرجع;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ساكن std::vector<Ast_تۈگۈن_ئىپادە*> gatherTerms(Ast_تۈگۈن_ئىپادە* rootp) {
        std::vector<Ast_تۈگۈن_ئىپادە*> terms;
        إذا (AstConcat* ثابت catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } وإلا إذا (AstExtend* ثابت extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } وإلا {
            rootp->v3fatalSrc("Unexpected node type");
        }
        أرجع terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ساكن Ast_تۈگۈن_ئىپادە* construct(ثابت std::vector<Term>& terms, ثابت size_t begin,
                                  ثابت size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        إذا (end == begin + 1) أرجع terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ثابت size_t width = terms[end].offset - terms[begin].offset;
        ثابت size_t midOffset = width / 2 + terms[begin].offset;
        ثابت auto beginIt = terms.begin() + begin;
        ثابت auto endIt = terms.begin() + end;
        ثابت auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](ثابت Term& term, size_t قىممەت) {  //
                                                أرجع term.offset < قىممەت;
                                            });
        ثابت size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_تۈگۈن_ئىپادە* ثابت rhsp = construct(terms, begin, mid);
        Ast_تۈگۈن_ئىپادە* ثابت lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_تۈگۈن_ئىپادە* newp = جديد AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(true);  // Must not attempt to balance again.
        أرجع newp;
    }

    // Returns replacement node, or nullptr if no change
    ساكن AstConcat* balance(Ast_تۈگۈن_ئىپادە* ثابت rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ثابت std::vector<Ast_تۈگۈن_ئىپادە*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        إذا (exprps.size() <= 3) أرجع nullptr;
        // Don't do it if any of the terms are impure
        لكل (Ast_تۈگۈن_ئىپادە* ثابت exprp : exprps) {
            إذا (!exprp->isPure()) أرجع nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        لكل (size_t i = 0; i < exprps.size(); ++i) {
            Ast_تۈگۈن_ئىپادە* ثابت exprp = [&]() -> Ast_تۈگۈن_ئىپادە* {
                إذا (AstExtend* ثابت extp = VN_CAST(exprps[i], Extend)) {
                    ثابت صحيح width = extp->width() - extp->lhsp()->width();
                    أرجع جديد AstConst{extp->fileline(), AstConst::Widthed_قىممەت{}, width, 0};
                }
                أرجع exprps[i]->clone_دەرەخ_Pure(خطأ);
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
                Ast_تۈگۈن_ئىپادە* ثابت exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            طالما (++end < terms.size() - 1) {
                إذا (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        أرجع VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

عام:
    ساكن Ast_تۈگۈن_ئىپادە* apply(Ast_تۈگۈن_ئىپادە* nodep) {
        إذا (!v3Global.opt.fFuncBalanceCat()) أرجع nullptr;
        إذا (nodep->user1()) أرجع nullptr;  // Created by us, don't try to balance again
        إذا (VN_IS(nodep->backp(), Concat)) أرجع nullptr;  // Not root of tree
        إذا (VN_IS(nodep->backp(), Extend)) أرجع nullptr;  // Not root of tree
        Ast_تۈگۈن_ئىپادە* ثابت exprp = balance(nodep);
        إذا (exprp) exprp->user1(true);  // Must not attempt again.
        أرجع exprp;
    }
};

بنية FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = افتراضي;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

صنف FuncOptVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ساكن منطقي readsLhs(Ast_تۈگۈن_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<ثابت AstVar*> lhsWrVarps;
        std::unordered_set<ثابت AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ثابت AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            إذا (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        إذا (lhsWrVarps.size() == 1) {
            ثابت AstVar* ثابت lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([=](ثابت AstVarRef* refp) {  //
                أرجع refp->varp() == lhsWrVarp;
            });
            إذا (rhsReadsWritten) أرجع true;
            // Check Lhs doesn't read the written var
            أرجع lhsRdVarps.سانى(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ثابت منطقي rhsReadsWritten = nodep->rhsp()->exists([&](ثابت AstVarRef* refp) {  //
            أرجع lhsWrVarps.سانى(refp->varp());
        });
        إذا (rhsReadsWritten) أرجع true;
        // Check Lhs doesn't read the written vars
        لكل (ثابت AstVar* ثابت lhsWrVarp : lhsWrVarps) {
            إذا (lhsRdVarps.سانى(lhsWrVarp)) أرجع true;
        }
        أرجع خطأ;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    منطقي splitConcat(Ast_تۈگۈن_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_تۈگۈن_ئىپادە* ثابت rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        إذا (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) أرجع خطأ;
        // Will need the LHS
        Ast_تۈگۈن_ئىپادە* lhsp = nodep->lhsp();
        إذا (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        إذا (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) أرجع خطأ;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        إذا (lhsp->exists([](AstVarRef* refp) { أرجع refp->varp()->isSc(); })) أرجع خطأ;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        إذا (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            إذا (AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } وإلا {
                // Don't optimize if it's a variable select
                أرجع خطأ;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        إذا (lhsp->width() <= VL_QUADSIZE) أرجع خطأ;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ثابت صحيح lsbWord = lsb / VL_EDATASIZE;
        ثابت صحيح msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        إذا (lsbWord == msbWord) أرجع خطأ;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        إذا (!nodep->user1() && readsLhs(nodep)) أرجع خطأ;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_تۈگۈن_ئىپادە* ثابت rrp = [rhsp]() -> Ast_تۈگۈن_ئىپادە* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->rhsp()->unlinkFrBack();
            }
            أرجع VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_تۈگۈن_ئىپادە* ثابت rlp = [rhsp, rrp]() -> Ast_تۈگۈن_ئىپادە* {
            إذا (AstConcat* ثابت catp = VN_CAST(rhsp, Concat)) {
                أرجع catp->lhsp()->unlinkFrBack();
            }
            ثابت صحيح lWidth = rhsp->width() - rrp->width();
            أرجع جديد AstConst{rhsp->fileline(), AstConst::Widthed_قىممەت{}, lWidth, 0};
        }();
        ثابت صحيح rLsb = lsb;
        ثابت صحيح lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* ثابت arp = جديد AstAssign{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->clone_دەرەخ_Pure(خطأ), rLsb, rrp->width()}, rrp};
        AstAssign* ثابت alp = جديد AstAssign{
            nodep->fileline(),
            جديد AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(true);
        alp->user1(true);
        // Nuke what is left
        VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep->unlinkFrBack()), nodep);
        أرجع true;
    }

    // VISIT
    فراغ visit(Ast_تۈگۈن_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        إذا (v3Global.opt.fFuncSplitCat()) {
            إذا (splitConcat(nodep)) أرجع;  // Must return here, in case more code is added below
        }
    }

    فراغ visit(AstConcat* nodep) override {
        إذا (Ast_تۈگۈن_ئىپادە* ثابت newp = Balance_Concat_دەرەخ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstExtend* nodep) override {
        إذا (Ast_تۈگۈن_ئىپادە* ثابت newp = Balance_Concat_دەرەخ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
            أرجع;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
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
        ثابت VNUser1InUse user1InUse;
        FuncOptStats stats;
        لكل (Ast_تۈگۈن_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), تۈگۈن_Module)) {
            لكل (Ast_تۈگۈن* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstCFunc* ثابت cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_دەرەخ("funcopt", 0, dump_دەرەخ_Either_Level() >= 3);
}
