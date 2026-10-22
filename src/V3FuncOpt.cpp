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

वर्ग Balance_Concat_वृक्ष final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    संरचना Term final {
        Ast_गाँठ_व्यंजक* exprp = nullptr;
        माप_प्रकार offset = 0;

        Term() = स्वभाव;
        Term(Ast_गाँठ_व्यंजक* exprp, माप_प्रकार offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    स्थिर शून्य gatherTermsRecursive(Ast_गाँठ_व्यंजक* exprp, std::vector<Ast_गाँठ_व्यंजक*>& terms) {
        यदि (AstConcat* ध्रुव catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            प्रत्यागम;
        }
        यदि (AstExtend* ध्रुव extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            प्रत्यागम;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    स्थिर std::vector<Ast_गाँठ_व्यंजक*> gatherTerms(Ast_गाँठ_व्यंजक* rootp) {
        std::vector<Ast_गाँठ_व्यंजक*> terms;
        यदि (AstConcat* ध्रुव catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } अन्यथा यदि (AstExtend* ध्रुव extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } अन्यथा {
            rootp->v3fatalSrc("Unexpected node type");
        }
        प्रत्यागम terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    स्थिर Ast_गाँठ_व्यंजक* construct(ध्रुव std::vector<Term>& terms, ध्रुव माप_प्रकार begin,
                                  ध्रुव माप_प्रकार end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        यदि (end == begin + 1) प्रत्यागम terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ध्रुव माप_प्रकार width = terms[end].offset - terms[begin].offset;
        ध्रुव माप_प्रकार midOffset = width / 2 + terms[begin].offset;
        ध्रुव स्वतः beginIt = terms.begin() + begin;
        ध्रुव स्वतः endIt = terms.begin() + end;
        ध्रुव स्वतः midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](ध्रुव Term& term, माप_प्रकार मूल्य) {  //
                                                प्रत्यागम term.offset < मूल्य;
                                            });
        ध्रुव माप_प्रकार mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_गाँठ_व्यंजक* ध्रुव rhsp = construct(terms, begin, mid);
        Ast_गाँठ_व्यंजक* ध्रुव lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_गाँठ_व्यंजक* newp = नूतन AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(सत्य);  // Must not attempt to balance again.
        प्रत्यागम newp;
    }

    // Returns replacement node, or nullptr if no change
    स्थिर AstConcat* balance(Ast_गाँठ_व्यंजक* ध्रुव rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ध्रुव std::vector<Ast_गाँठ_व्यंजक*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        यदि (exprps.size() <= 3) प्रत्यागम nullptr;
        // Don't do it if any of the terms are impure
        प्रति (Ast_गाँठ_व्यंजक* ध्रुव exprp : exprps) {
            यदि (!exprp->isPure()) प्रत्यागम nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        प्रति (माप_प्रकार i = 0; i < exprps.size(); ++i) {
            Ast_गाँठ_व्यंजक* ध्रुव exprp = [&]() -> Ast_गाँठ_व्यंजक* {
                यदि (AstExtend* ध्रुव extp = VN_CAST(exprps[i], Extend)) {
                    ध्रुव पूर्णाङ्क width = extp->width() - extp->lhsp()->width();
                    प्रत्यागम नूतन AstConst{extp->fileline(), AstConst::Widthed_मूल्य{}, width, 0};
                }
                प्रत्यागम exprps[i]->clone_वृक्ष_Pure(असत्य);
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

            माप_प्रकार begin = 0;  // Start of current range considered
            माप_प्रकार end = 0;  // End of current range considered
            माप_प्रकार offset = 0;  // Offset of current range considered

            // Create a term from the current range
            ध्रुव स्वतः makeTerm = [&]() {
                Ast_गाँठ_व्यंजक* ध्रुव exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            यावत् (++end < terms.size() - 1) {
                यदि (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        प्रत्यागम VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

सार्वजनिक:
    स्थिर Ast_गाँठ_व्यंजक* apply(Ast_गाँठ_व्यंजक* nodep) {
        यदि (!v3Global.opt.fFuncBalanceCat()) प्रत्यागम nullptr;
        यदि (nodep->user1()) प्रत्यागम nullptr;  // Created by us, don't try to balance again
        यदि (VN_IS(nodep->backp(), Concat)) प्रत्यागम nullptr;  // Not root of tree
        यदि (VN_IS(nodep->backp(), Extend)) प्रत्यागम nullptr;  // Not root of tree
        Ast_गाँठ_व्यंजक* ध्रुव exprp = balance(nodep);
        यदि (exprp) exprp->user1(सत्य);  // Must not attempt again.
        प्रत्यागम exprp;
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

वर्ग FuncOptVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    स्थिर तार्किक readsLhs(Ast_गाँठ_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<ध्रुव AstVar*> lhsWrVarps;
        std::unordered_set<ध्रुव AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ध्रुव AstVarRef* refp) {
            यदि (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            यदि (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        यदि (lhsWrVarps.size() == 1) {
            ध्रुव AstVar* ध्रुव lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ध्रुव तार्किक rhsReadsWritten = nodep->rhsp()->exists([=](ध्रुव AstVarRef* refp) {  //
                प्रत्यागम refp->varp() == lhsWrVarp;
            });
            यदि (rhsReadsWritten) प्रत्यागम सत्य;
            // Check Lhs doesn't read the written var
            प्रत्यागम lhsRdVarps.गिनती(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ध्रुव तार्किक rhsReadsWritten = nodep->rhsp()->exists([&](ध्रुव AstVarRef* refp) {  //
            प्रत्यागम lhsWrVarps.गिनती(refp->varp());
        });
        यदि (rhsReadsWritten) प्रत्यागम सत्य;
        // Check Lhs doesn't read the written vars
        प्रति (ध्रुव AstVar* ध्रुव lhsWrVarp : lhsWrVarps) {
            यदि (lhsRdVarps.गिनती(lhsWrVarp)) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    तार्किक splitConcat(Ast_गाँठ_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_गाँठ_व्यंजक* ध्रुव rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        यदि (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) प्रत्यागम असत्य;
        // Will need the LHS
        Ast_गाँठ_व्यंजक* lhsp = nodep->lhsp();
        यदि (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        यदि (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) प्रत्यागम असत्य;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        यदि (lhsp->exists([](AstVarRef* refp) { प्रत्यागम refp->varp()->isSc(); })) प्रत्यागम असत्य;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        यदि (AstSel* ध्रुव selp = VN_CAST(lhsp, Sel)) {
            यदि (AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } अन्यथा {
                // Don't optimize if it's a variable select
                प्रत्यागम असत्य;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        यदि (lhsp->width() <= VL_QUADSIZE) प्रत्यागम असत्य;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ध्रुव पूर्णाङ्क lsbWord = lsb / VL_EDATASIZE;
        ध्रुव पूर्णाङ्क msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        यदि (lsbWord == msbWord) प्रत्यागम असत्य;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        यदि (!nodep->user1() && readsLhs(nodep)) प्रत्यागम असत्य;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_गाँठ_व्यंजक* ध्रुव rrp = [rhsp]() -> Ast_गाँठ_व्यंजक* {
            यदि (AstConcat* ध्रुव catp = VN_CAST(rhsp, Concat)) {
                प्रत्यागम catp->rhsp()->unlinkFrBack();
            }
            प्रत्यागम VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_गाँठ_व्यंजक* ध्रुव rlp = [rhsp, rrp]() -> Ast_गाँठ_व्यंजक* {
            यदि (AstConcat* ध्रुव catp = VN_CAST(rhsp, Concat)) {
                प्रत्यागम catp->lhsp()->unlinkFrBack();
            }
            ध्रुव पूर्णाङ्क lWidth = rhsp->width() - rrp->width();
            प्रत्यागम नूतन AstConst{rhsp->fileline(), AstConst::Widthed_मूल्य{}, lWidth, 0};
        }();
        ध्रुव पूर्णाङ्क rLsb = lsb;
        ध्रुव पूर्णाङ्क lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* ध्रुव arp = नूतन AstAssign{
            nodep->fileline(),
            नूतन AstSel{lhsp->fileline(), lhsp->clone_वृक्ष_Pure(असत्य), rLsb, rrp->width()}, rrp};
        AstAssign* ध्रुव alp = नूतन AstAssign{
            nodep->fileline(),
            नूतन AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(सत्य);
        alp->user1(सत्य);
        // Nuke what is left
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        प्रत्यागम सत्य;
    }

    // VISIT
    शून्य visit(Ast_गाँठ_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        यदि (v3Global.opt.fFuncSplitCat()) {
            यदि (splitConcat(nodep)) प्रत्यागम;  // Must return here, in case more code is added below
        }
    }

    शून्य visit(AstConcat* nodep) override {
        यदि (Ast_गाँठ_व्यंजक* ध्रुव newp = Balance_Concat_वृक्ष::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            प्रत्यागम;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    शून्य visit(AstExtend* nodep) override {
        यदि (Ast_गाँठ_व्यंजक* ध्रुव newp = Balance_Concat_वृक्ष::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            प्रत्यागम;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

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
        प्रति (Ast_गाँठ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), गाँठ_Module)) {
            प्रति (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (AstCFunc* ध्रुव cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_वृक्ष("funcopt", 0, dump_वृक्ष_Either_Level() >= 3);
}
