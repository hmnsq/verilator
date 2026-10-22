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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3FuncOpt.h"

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

वग्ग Balance_Concat_वृक्षः final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    संरचना Term final {
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nullptr;
        size_t offset = 0;

        Term() = स्वभाव;
        Term(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    स्थिर शून्य gatherTermsRecursive(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>& terms) {
        सचे (AstConcat* ध्रुव catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            पटिनिवत्त;
        }
        सचे (AstExtend* ध्रुव extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            पटिनिवत्त;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    स्थिर std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> gatherTerms(Ast_ग्रन्थिः_व्यञ्जकम्* rootp) {
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> terms;
        सचे (AstConcat* ध्रुव catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } अञ्ञथा सचे (AstExtend* ध्रुव extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } अञ्ञथा {
            rootp->v3fatalSrc("Unexpected node type");
        }
        पटिनिवत्त terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* construct(ध्रुव std::vector<Term>& terms, ध्रुव size_t begin,
                                  ध्रुव size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        सचे (end == begin + 1) पटिनिवत्त terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ध्रुव size_t width = terms[end].offset - terms[begin].offset;
        ध्रुव size_t midOffset = width / 2 + terms[begin].offset;
        ध्रुव auto beginIt = terms.begin() + begin;
        ध्रुव auto endIt = terms.begin() + end;
        ध्रुव auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](ध्रुव Term& term, size_t मानम्) {  //
                                                पटिनिवत्त term.offset < मानम्;
                                            });
        ध्रुव size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = construct(terms, begin, mid);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = नूतन AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(सच्च);  // Must not attempt to balance again.
        पटिनिवत्त newp;
    }

    // Returns replacement node, or nullptr if no change
    स्थिर AstConcat* balance(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ध्रुव std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        सचे (exprps.size() <= 3) पटिनिवत्त nullptr;
        // Don't do it if any of the terms are impure
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp : exprps) {
            सचे (!exprp->isPure()) पटिनिवत्त nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        अत्थाय (size_t i = 0; i < exprps.size(); ++i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                सचे (AstExtend* ध्रुव extp = VN_CAST(exprps[i], Extend)) {
                    ध्रुव पूर्णाङ्क width = extp->width() - extp->lhsp()->width();
                    पटिनिवत्त नूतन AstConst{extp->fileline(), AstConst::Widthed_मानम्{}, width, 0};
                }
                पटिनिवत्त exprps[i]->clone_वृक्षः_Pure(मिच्छा);
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
            ध्रुव auto makeTerm = [&]() {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            याव (++end < terms.size() - 1) {
                सचे (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        पटिनिवत्त VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

सार्वजनिक:
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* apply(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (!v3Global.opt.fFuncBalanceCat()) पटिनिवत्त nullptr;
        सचे (nodep->user1()) पटिनिवत्त nullptr;  // Created by us, don't try to balance again
        सचे (VN_IS(nodep->backp(), Concat)) पटिनिवत्त nullptr;  // Not root of tree
        सचे (VN_IS(nodep->backp(), Extend)) पटिनिवत्त nullptr;  // Not root of tree
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = balance(nodep);
        सचे (exprp) exprp->user1(सच्च);  // Must not attempt again.
        पटिनिवत्त exprp;
    }
};

संरचना FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = स्वभाव;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

वग्ग FuncOptVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    स्थिर तार्किक readsLhs(Ast_ग्रन्थिः_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<ध्रुव AstVar*> lhsWrVarps;
        std::unordered_set<ध्रुव AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ध्रुव AstVarRef* refp) {
            सचे (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            सचे (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        सचे (lhsWrVarps.size() == 1) {
            ध्रुव AstVar* ध्रुव lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ध्रुव तार्किक rhsReadsWritten = nodep->rhsp()->exists([=](ध्रुव AstVarRef* refp) {  //
                पटिनिवत्त refp->varp() == lhsWrVarp;
            });
            सचे (rhsReadsWritten) पटिनिवत्त सच्च;
            // Check Lhs doesn't read the written var
            पटिनिवत्त lhsRdVarps.गणनम्(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ध्रुव तार्किक rhsReadsWritten = nodep->rhsp()->exists([&](ध्रुव AstVarRef* refp) {  //
            पटिनिवत्त lhsWrVarps.गणनम्(refp->varp());
        });
        सचे (rhsReadsWritten) पटिनिवत्त सच्च;
        // Check Lhs doesn't read the written vars
        अत्थाय (ध्रुव AstVar* ध्रुव lhsWrVarp : lhsWrVarps) {
            सचे (lhsRdVarps.गणनम्(lhsWrVarp)) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    तार्किक splitConcat(Ast_ग्रन्थिः_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        सचे (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) पटिनिवत्त मिच्छा;
        // Will need the LHS
        Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp();
        सचे (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        सचे (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) पटिनिवत्त मिच्छा;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        सचे (lhsp->exists([](AstVarRef* refp) { पटिनिवत्त refp->varp()->isSc(); })) पटिनिवत्त मिच्छा;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        सचे (AstSel* ध्रुव selp = VN_CAST(lhsp, Sel)) {
            सचे (AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } अञ्ञथा {
                // Don't optimize if it's a variable select
                पटिनिवत्त मिच्छा;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        सचे (lhsp->width() <= VL_QUADSIZE) पटिनिवत्त मिच्छा;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ध्रुव पूर्णाङ्क lsbWord = lsb / VL_EDATASIZE;
        ध्रुव पूर्णाङ्क msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        सचे (lsbWord == msbWord) पटिनिवत्त मिच्छा;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        सचे (!nodep->user1() && readsLhs(nodep)) पटिनिवत्त मिच्छा;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rrp = [rhsp]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            सचे (AstConcat* ध्रुव catp = VN_CAST(rhsp, Concat)) {
                पटिनिवत्त catp->rhsp()->unlinkFrBack();
            }
            पटिनिवत्त VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rlp = [rhsp, rrp]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            सचे (AstConcat* ध्रुव catp = VN_CAST(rhsp, Concat)) {
                पटिनिवत्त catp->lhsp()->unlinkFrBack();
            }
            ध्रुव पूर्णाङ्क lWidth = rhsp->width() - rrp->width();
            पटिनिवत्त नूतन AstConst{rhsp->fileline(), AstConst::Widthed_मानम्{}, lWidth, 0};
        }();
        ध्रुव पूर्णाङ्क rLsb = lsb;
        ध्रुव पूर्णाङ्क lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* ध्रुव arp = नूतन AstAssign{
            nodep->fileline(),
            नूतन AstSel{lhsp->fileline(), lhsp->clone_वृक्षः_Pure(मिच्छा), rLsb, rrp->width()}, rrp};
        AstAssign* ध्रुव alp = नूतन AstAssign{
            nodep->fileline(),
            नूतन AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(सच्च);
        alp->user1(सच्च);
        // Nuke what is left
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
        पटिनिवत्त सच्च;
    }

    // VISIT
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        सचे (v3Global.opt.fFuncSplitCat()) {
            सचे (splitConcat(nodep)) पटिनिवत्त;  // Must return here, in case more code is added below
        }
    }

    शून्य visit(AstConcat* nodep) override {
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = Balance_Concat_वृक्षः::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    शून्य visit(AstExtend* nodep) override {
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = Balance_Concat_वृक्षः::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

सार्वजनिक:
    स्थिर शून्य apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

शून्य V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        ध्रुव VNUser1InUse user1InUse;
        FuncOptStats stats;
        अत्थाय (Ast_ग्रन्थिः_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
            अत्थाय (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                सचे (AstCFunc* ध्रुव cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_वृक्षः("funcopt", 0, dump_वृक्षः_Either_Level() >= 3);
}
