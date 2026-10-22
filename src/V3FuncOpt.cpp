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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3FuncOpt.h"

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ Balance_Concat_ବୃକ୍ଷ final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    ସଂରଚନା Term final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nullptr;
        size_t offset = 0;

        Term() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        Term(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    ସ୍ଥିର ଶୂନ୍ୟ gatherTermsRecursive(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& terms) {
        ଯଦି (AstConcat* ଅଚଳ catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (AstExtend* ଅଚଳ extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            ଫେରନ୍ତୁ;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    ସ୍ଥିର std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> gatherTerms(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rootp) {
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> terms;
        ଯଦି (AstConcat* ଅଚଳ catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } ଅନ୍ୟଥା ଯଦି (AstExtend* ଅଚଳ extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } ଅନ୍ୟଥା {
            rootp->v3fatalSrc("Unexpected node type");
        }
        ଫେରନ୍ତୁ terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* construct(ଅଚଳ std::vector<Term>& terms, ଅଚଳ size_t begin,
                                  ଅଚଳ size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        ଯଦି (end == begin + 1) ଫେରନ୍ତୁ terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ଅଚଳ size_t width = terms[end].offset - terms[begin].offset;
        ଅଚଳ size_t midOffset = width / 2 + terms[begin].offset;
        ଅଚଳ auto beginIt = terms.begin() + begin;
        ଅଚଳ auto endIt = terms.begin() + end;
        ଅଚଳ auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](ଅଚଳ Term& term, size_t ମୂଲ୍ୟ) {  //
                                                ଫେରନ୍ତୁ term.offset < ମୂଲ୍ୟ;
                                            });
        ଅଚଳ size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = construct(terms, begin, mid);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = ନୂତନ AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(ସତ୍ୟ);  // Must not attempt to balance again.
        ଫେରନ୍ତୁ newp;
    }

    // Returns replacement node, or nullptr if no change
    ସ୍ଥିର AstConcat* balance(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        ଯଦି (exprps.size() <= 3) ଫେରନ୍ତୁ nullptr;
        // Don't do it if any of the terms are impure
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp : exprps) {
            ଯଦି (!exprp->isPure()) ଫେରନ୍ତୁ nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        ପାଇଁ (size_t i = 0; i < exprps.size(); ++i) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                ଯଦି (AstExtend* ଅଚଳ extp = VN_CAST(exprps[i], Extend)) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = extp->width() - extp->lhsp()->width();
                    ଫେରନ୍ତୁ ନୂତନ AstConst{extp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, width, 0};
                }
                ଫେରନ୍ତୁ exprps[i]->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
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
            ଅଚଳ auto makeTerm = [&]() {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            ଯେତେବେଳେ (++end < terms.size() - 1) {
                ଯଦି (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        ଫେରନ୍ତୁ VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* apply(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (!v3Global.opt.fFuncBalanceCat()) ଫେରନ୍ତୁ nullptr;
        ଯଦି (nodep->user1()) ଫେରନ୍ତୁ nullptr;  // Created by us, don't try to balance again
        ଯଦି (VN_IS(nodep->backp(), Concat)) ଫେରନ୍ତୁ nullptr;  // Not root of tree
        ଯଦି (VN_IS(nodep->backp(), Extend)) ଫେରନ୍ତୁ nullptr;  // Not root of tree
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = balance(nodep);
        ଯଦି (exprp) exprp->user1(ସତ୍ୟ);  // Must not attempt again.
        ଫେରନ୍ତୁ exprp;
    }
};

ସଂରଚନା FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

ଶ୍ରେଣୀ FuncOptVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    ସ୍ଥିର ତାର୍କିକ readsLhs(Ast_ଗଣ୍ଠି_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<ଅଚଳ AstVar*> lhsWrVarps;
        std::unordered_set<ଅଚଳ AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ଅଚଳ AstVarRef* refp) {
            ଯଦି (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            ଯଦି (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        ଯଦି (lhsWrVarps.size() == 1) {
            ଅଚଳ AstVar* ଅଚଳ lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ଅଚଳ ତାର୍କିକ rhsReadsWritten = nodep->rhsp()->exists([=](ଅଚଳ AstVarRef* refp) {  //
                ଫେରନ୍ତୁ refp->varp() == lhsWrVarp;
            });
            ଯଦି (rhsReadsWritten) ଫେରନ୍ତୁ ସତ୍ୟ;
            // Check Lhs doesn't read the written var
            ଫେରନ୍ତୁ lhsRdVarps.ଗଣନା_2(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ଅଚଳ ତାର୍କିକ rhsReadsWritten = nodep->rhsp()->exists([&](ଅଚଳ AstVarRef* refp) {  //
            ଫେରନ୍ତୁ lhsWrVarps.ଗଣନା_2(refp->varp());
        });
        ଯଦି (rhsReadsWritten) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Check Lhs doesn't read the written vars
        ପାଇଁ (ଅଚଳ AstVar* ଅଚଳ lhsWrVarp : lhsWrVarps) {
            ଯଦି (lhsRdVarps.ଗଣନା_2(lhsWrVarp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    ତାର୍କିକ splitConcat(Ast_ଗଣ୍ଠି_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        ଯଦି (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Will need the LHS
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nodep->lhsp();
        ଯଦି (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        ଯଦି (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        ଯଦି (lhsp->exists([](AstVarRef* refp) { ଫେରନ୍ତୁ refp->varp()->isSc(); })) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(lhsp, Sel)) {
            ଯଦି (AstConst* ଅଚଳ lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } ଅନ୍ୟଥା {
                // Don't optimize if it's a variable select
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        ଯଦି (lhsp->width() <= VL_QUADSIZE) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsbWord = lsb / VL_EDATASIZE;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        ଯଦି (lsbWord == msbWord) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        ଯଦି (!nodep->user1() && readsLhs(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rrp = [rhsp]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            ଯଦି (AstConcat* ଅଚଳ catp = VN_CAST(rhsp, Concat)) {
                ଫେରନ୍ତୁ catp->rhsp()->unlinkFrBack();
            }
            ଫେରନ୍ତୁ VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rlp = [rhsp, rrp]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            ଯଦି (AstConcat* ଅଚଳ catp = VN_CAST(rhsp, Concat)) {
                ଫେରନ୍ତୁ catp->lhsp()->unlinkFrBack();
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lWidth = rhsp->width() - rrp->width();
            ଫେରନ୍ତୁ ନୂତନ AstConst{rhsp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, lWidth, 0};
        }();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rLsb = lsb;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* ଅଚଳ arp = ନୂତନ AstAssign{
            nodep->fileline(),
            ନୂତନ AstSel{lhsp->fileline(), lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), rLsb, rrp->width()}, rrp};
        AstAssign* ଅଚଳ alp = ନୂତନ AstAssign{
            nodep->fileline(),
            ନୂତନ AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(ସତ୍ୟ);
        alp->user1(ସତ୍ୟ);
        // Nuke what is left
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // VISIT
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        ଯଦି (v3Global.opt.fFuncSplitCat()) {
            ଯଦି (splitConcat(nodep)) ଫେରନ୍ତୁ;  // Must return here, in case more code is added below
        }
    }

    ଶୂନ୍ୟ visit(AstConcat* nodep) override {
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = Balance_Concat_ବୃକ୍ଷ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstExtend* nodep) override {
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = Balance_Concat_ବୃକ୍ଷ::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

ଶୂନ୍ୟ V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        ଅଚଳ VNUser1InUse user1InUse;
        FuncOptStats stats;
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ଯଦି (AstCFunc* ଅଚଳ cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("funcopt", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
