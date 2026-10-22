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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3FuncOpt.h"

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு Balance_Concat_மரம் final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    கட்டமைப்பு Term final {
        Ast_முனை_கோவை* exprp = nullptr;
        size_t offset = 0;

        Term() = இயல்பு;
        Term(Ast_முனை_கோவை* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    நிலையான வெறுமை gatherTermsRecursive(Ast_முனை_கோவை* exprp, std::vector<Ast_முனை_கோவை*>& terms) {
        எனில் (AstConcat* மாறிலி catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            திரும்பு;
        }
        எனில் (AstExtend* மாறிலி extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            திரும்பு;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    நிலையான std::vector<Ast_முனை_கோவை*> gatherTerms(Ast_முனை_கோவை* rootp) {
        std::vector<Ast_முனை_கோவை*> terms;
        எனில் (AstConcat* மாறிலி catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } இல்லையெனில் எனில் (AstExtend* மாறிலி extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } இல்லையெனில் {
            rootp->v3fatalSrc("Unexpected node type");
        }
        திரும்பு terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    நிலையான Ast_முனை_கோவை* construct(மாறிலி std::vector<Term>& terms, மாறிலி size_t begin,
                                  மாறிலி size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        எனில் (end == begin + 1) திரும்பு terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        மாறிலி size_t width = terms[end].offset - terms[begin].offset;
        மாறிலி size_t midOffset = width / 2 + terms[begin].offset;
        மாறிலி auto beginIt = terms.begin() + begin;
        மாறிலி auto endIt = terms.begin() + end;
        மாறிலி auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](மாறிலி Term& term, size_t மதிப்பு) {  //
                                                திரும்பு term.offset < மதிப்பு;
                                            });
        மாறிலி size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_முனை_கோவை* மாறிலி rhsp = construct(terms, begin, mid);
        Ast_முனை_கோவை* மாறிலி lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_முனை_கோவை* newp = புதிய AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(உண்மை);  // Must not attempt to balance again.
        திரும்பு newp;
    }

    // Returns replacement node, or nullptr if no change
    நிலையான AstConcat* balance(Ast_முனை_கோவை* மாறிலி rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        மாறிலி std::vector<Ast_முனை_கோவை*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        எனில் (exprps.size() <= 3) திரும்பு nullptr;
        // Don't do it if any of the terms are impure
        க்கு (Ast_முனை_கோவை* மாறிலி exprp : exprps) {
            எனில் (!exprp->isPure()) திரும்பு nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        க்கு (size_t i = 0; i < exprps.size(); ++i) {
            Ast_முனை_கோவை* மாறிலி exprp = [&]() -> Ast_முனை_கோவை* {
                எனில் (AstExtend* மாறிலி extp = VN_CAST(exprps[i], Extend)) {
                    மாறிலி முழு width = extp->width() - extp->lhsp()->width();
                    திரும்பு புதிய AstConst{extp->fileline(), AstConst::Widthed_மதிப்பு{}, width, 0};
                }
                திரும்பு exprps[i]->clone_மரம்_Pure(பொய்);
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
            மாறிலி auto makeTerm = [&]() {
                Ast_முனை_கோவை* மாறிலி exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            போது (++end < terms.size() - 1) {
                எனில் (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        திரும்பு VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

பொது:
    நிலையான Ast_முனை_கோவை* apply(Ast_முனை_கோவை* nodep) {
        எனில் (!v3Global.opt.fFuncBalanceCat()) திரும்பு nullptr;
        எனில் (nodep->user1()) திரும்பு nullptr;  // Created by us, don't try to balance again
        எனில் (VN_IS(nodep->backp(), Concat)) திரும்பு nullptr;  // Not root of tree
        எனில் (VN_IS(nodep->backp(), Extend)) திரும்பு nullptr;  // Not root of tree
        Ast_முனை_கோவை* மாறிலி exprp = balance(nodep);
        எனில் (exprp) exprp->user1(உண்மை);  // Must not attempt again.
        திரும்பு exprp;
    }
};

கட்டமைப்பு FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = இயல்பு;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

வகுப்பு FuncOptVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    நிலையான தருக்க readsLhs(Ast_முனை_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<மாறிலி AstVar*> lhsWrVarps;
        std::unordered_set<மாறிலி AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](மாறிலி AstVarRef* refp) {
            எனில் (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            எனில் (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        எனில் (lhsWrVarps.size() == 1) {
            மாறிலி AstVar* மாறிலி lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            மாறிலி தருக்க rhsReadsWritten = nodep->rhsp()->exists([=](மாறிலி AstVarRef* refp) {  //
                திரும்பு refp->varp() == lhsWrVarp;
            });
            எனில் (rhsReadsWritten) திரும்பு உண்மை;
            // Check Lhs doesn't read the written var
            திரும்பு lhsRdVarps.மொத்தம்(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        மாறிலி தருக்க rhsReadsWritten = nodep->rhsp()->exists([&](மாறிலி AstVarRef* refp) {  //
            திரும்பு lhsWrVarps.மொத்தம்(refp->varp());
        });
        எனில் (rhsReadsWritten) திரும்பு உண்மை;
        // Check Lhs doesn't read the written vars
        க்கு (மாறிலி AstVar* மாறிலி lhsWrVarp : lhsWrVarps) {
            எனில் (lhsRdVarps.மொத்தம்(lhsWrVarp)) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    தருக்க splitConcat(Ast_முனை_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        எனில் (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) திரும்பு பொய்;
        // Will need the LHS
        Ast_முனை_கோவை* lhsp = nodep->lhsp();
        எனில் (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        எனில் (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) திரும்பு பொய்;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        எனில் (lhsp->exists([](AstVarRef* refp) { திரும்பு refp->varp()->isSc(); })) திரும்பு பொய்;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        எனில் (AstSel* மாறிலி selp = VN_CAST(lhsp, Sel)) {
            எனில் (AstConst* மாறிலி lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } இல்லையெனில் {
                // Don't optimize if it's a variable select
                திரும்பு பொய்;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        எனில் (lhsp->width() <= VL_QUADSIZE) திரும்பு பொய்;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        மாறிலி முழு lsbWord = lsb / VL_EDATASIZE;
        மாறிலி முழு msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        எனில் (lsbWord == msbWord) திரும்பு பொய்;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        எனில் (!nodep->user1() && readsLhs(nodep)) திரும்பு பொய்;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_முனை_கோவை* மாறிலி rrp = [rhsp]() -> Ast_முனை_கோவை* {
            எனில் (AstConcat* மாறிலி catp = VN_CAST(rhsp, Concat)) {
                திரும்பு catp->rhsp()->unlinkFrBack();
            }
            திரும்பு VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_முனை_கோவை* மாறிலி rlp = [rhsp, rrp]() -> Ast_முனை_கோவை* {
            எனில் (AstConcat* மாறிலி catp = VN_CAST(rhsp, Concat)) {
                திரும்பு catp->lhsp()->unlinkFrBack();
            }
            மாறிலி முழு lWidth = rhsp->width() - rrp->width();
            திரும்பு புதிய AstConst{rhsp->fileline(), AstConst::Widthed_மதிப்பு{}, lWidth, 0};
        }();
        மாறிலி முழு rLsb = lsb;
        மாறிலி முழு lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* மாறிலி arp = புதிய AstAssign{
            nodep->fileline(),
            புதிய AstSel{lhsp->fileline(), lhsp->clone_மரம்_Pure(பொய்), rLsb, rrp->width()}, rrp};
        AstAssign* மாறிலி alp = புதிய AstAssign{
            nodep->fileline(),
            புதிய AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(உண்மை);
        alp->user1(உண்மை);
        // Nuke what is left
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        திரும்பு உண்மை;
    }

    // VISIT
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        எனில் (v3Global.opt.fFuncSplitCat()) {
            எனில் (splitConcat(nodep)) திரும்பு;  // Must return here, in case more code is added below
        }
    }

    வெறுமை visit(AstConcat* nodep) override {
        எனில் (Ast_முனை_கோவை* மாறிலி newp = Balance_Concat_மரம்::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(AstExtend* nodep) override {
        எனில் (Ast_முனை_கோவை* மாறிலி newp = Balance_Concat_மரம்::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

பொது:
    நிலையான வெறுமை apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

வெறுமை V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        மாறிலி VNUser1InUse user1InUse;
        FuncOptStats stats;
        க்கு (Ast_முனை_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), முனை_Module)) {
            க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                எனில் (AstCFunc* மாறிலி cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_மரம்("funcopt", 0, dump_மரம்_Either_Level() >= 3);
}
