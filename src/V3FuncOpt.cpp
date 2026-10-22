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

#समावेश "V3PchAstMT.h"

#समावेश "V3FuncOpt.h"

#समावेश "V3Global.h"
#समावेश "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी Balance_Concat_वृक्ष final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    काष्ठा Term final {
        Ast_गाँठ_व्यंजक* exprp = nullptr;
        माप_प्रकार offset = 0;

        Term() = शेष;
        Term(Ast_गाँठ_व्यंजक* exprp, माप_प्रकार offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    अटल व्योम gatherTermsRecursive(Ast_गाँठ_व्यंजक* exprp, std::vector<Ast_गाँठ_व्यंजक*>& terms) {
        अगर (AstConcat* स्थिर catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            वापस;
        }
        अगर (AstExtend* स्थिर extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            वापस;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    अटल std::vector<Ast_गाँठ_व्यंजक*> gatherTerms(Ast_गाँठ_व्यंजक* rootp) {
        std::vector<Ast_गाँठ_व्यंजक*> terms;
        अगर (AstConcat* स्थिर catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } अन्यथा अगर (AstExtend* स्थिर extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } अन्यथा {
            rootp->v3fatalSrc("Unexpected node type");
        }
        वापस terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    अटल Ast_गाँठ_व्यंजक* construct(स्थिर std::vector<Term>& terms, स्थिर माप_प्रकार begin,
                                  स्थिर माप_प्रकार end) {
        UASSERT(end < terms.आकार(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        अगर (end == begin + 1) वापस terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        स्थिर माप_प्रकार width = terms[end].offset - terms[begin].offset;
        स्थिर माप_प्रकार midOffset = width / 2 + terms[begin].offset;
        स्थिर स्वतः beginIt = terms.begin() + begin;
        स्थिर स्वतः endIt = terms.begin() + end;
        स्थिर स्वतः midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](स्थिर Term& term, माप_प्रकार मूल्य) {  //
                                                वापस term.offset < मूल्य;
                                            });
        स्थिर माप_प्रकार mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_गाँठ_व्यंजक* स्थिर rhsp = construct(terms, begin, mid);
        Ast_गाँठ_व्यंजक* स्थिर lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_गाँठ_व्यंजक* newp = नया AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(true);  // Must not attempt to balance again.
        वापस newp;
    }

    // Returns replacement node, or nullptr if no change
    अटल AstConcat* balance(Ast_गाँठ_व्यंजक* स्थिर rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        स्थिर std::vector<Ast_गाँठ_व्यंजक*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        अगर (exprps.आकार() <= 3) वापस nullptr;
        // Don't do it if any of the terms are impure
        क्रम (Ast_गाँठ_व्यंजक* स्थिर exprp : exprps) {
            अगर (!exprp->isPure()) वापस nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.आकार() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.आकार()].exprp = nullptr;
        क्रम (माप_प्रकार i = 0; i < exprps.आकार(); ++i) {
            Ast_गाँठ_व्यंजक* स्थिर exprp = [&]() -> Ast_गाँठ_व्यंजक* {
                अगर (AstExtend* स्थिर extp = VN_CAST(exprps[i], Extend)) {
                    स्थिर पूर्णांक width = extp->width() - extp->lhsp()->width();
                    वापस नया AstConst{extp->fileline(), AstConst::Widthed_मूल्य{}, width, 0};
                }
                वापस exprps[i]->clone_वृक्ष_Pure(false);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].offset = terms[i].offset + exprp->width();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::vector<Term> terms2;
        {
            terms2.reserve(terms.आकार());

            माप_प्रकार begin = 0;  // Start of current range considered
            माप_प्रकार end = 0;  // End of current range considered
            माप_प्रकार offset = 0;  // Offset of current range considered

            // Create a term from the current range
            स्थिर स्वतः makeTerm = [&]() {
                Ast_गाँठ_व्यंजक* स्थिर exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            जबतक (++end < terms.आकार() - 1) {
                अगर (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        वापस VN_AS(construct(terms2, 0, terms2.आकार() - 1), Concat);
    }

खुला:
    अटल Ast_गाँठ_व्यंजक* apply(Ast_गाँठ_व्यंजक* nodep) {
        अगर (!v3Global.opt.fFuncBalanceCat()) वापस nullptr;
        अगर (nodep->user1()) वापस nullptr;  // Created by us, don't try to balance again
        अगर (VN_IS(nodep->backp(), Concat)) वापस nullptr;  // Not root of tree
        अगर (VN_IS(nodep->backp(), Extend)) वापस nullptr;  // Not root of tree
        Ast_गाँठ_व्यंजक* स्थिर exprp = balance(nodep);
        अगर (exprp) exprp->user1(true);  // Must not attempt again.
        वापस exprp;
    }
};

काष्ठा FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = शेष;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

श्रेणी FuncOptVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    अटल bool readsLhs(Ast_गाँठ_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<स्थिर AstVar*> lhsWrVarps;
        std::unordered_set<स्थिर AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](स्थिर AstVarRef* refp) {
            अगर (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            अगर (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        अगर (lhsWrVarps.आकार() == 1) {
            स्थिर AstVar* स्थिर lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            स्थिर bool rhsReadsWritten = nodep->rhsp()->exists([=](स्थिर AstVarRef* refp) {  //
                वापस refp->varp() == lhsWrVarp;
            });
            अगर (rhsReadsWritten) वापस true;
            // Check Lhs doesn't read the written var
            वापस lhsRdVarps.गिनती(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        स्थिर bool rhsReadsWritten = nodep->rhsp()->exists([&](स्थिर AstVarRef* refp) {  //
            वापस lhsWrVarps.गिनती(refp->varp());
        });
        अगर (rhsReadsWritten) वापस true;
        // Check Lhs doesn't read the written vars
        क्रम (स्थिर AstVar* स्थिर lhsWrVarp : lhsWrVarps) {
            अगर (lhsRdVarps.गिनती(lhsWrVarp)) वापस true;
        }
        वापस false;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    bool splitConcat(Ast_गाँठ_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        अगर (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) वापस false;
        // Will need the LHS
        Ast_गाँठ_व्यंजक* lhsp = nodep->lhsp();
        अगर (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        अगर (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) वापस false;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        अगर (lhsp->exists([](AstVarRef* refp) { वापस refp->varp()->isSc(); })) वापस false;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        अगर (AstSel* स्थिर selp = VN_CAST(lhsp, Sel)) {
            अगर (AstConst* स्थिर lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } अन्यथा {
                // Don't optimize if it's a variable select
                वापस false;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        अगर (lhsp->width() <= VL_QUADSIZE) वापस false;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        स्थिर पूर्णांक lsbWord = lsb / VL_EDATASIZE;
        स्थिर पूर्णांक msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        अगर (lsbWord == msbWord) वापस false;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        अगर (!nodep->user1() && readsLhs(nodep)) वापस false;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_गाँठ_व्यंजक* स्थिर rrp = [rhsp]() -> Ast_गाँठ_व्यंजक* {
            अगर (AstConcat* स्थिर catp = VN_CAST(rhsp, Concat)) {
                वापस catp->rhsp()->unlinkFrBack();
            }
            वापस VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_गाँठ_व्यंजक* स्थिर rlp = [rhsp, rrp]() -> Ast_गाँठ_व्यंजक* {
            अगर (AstConcat* स्थिर catp = VN_CAST(rhsp, Concat)) {
                वापस catp->lhsp()->unlinkFrBack();
            }
            स्थिर पूर्णांक lWidth = rhsp->width() - rrp->width();
            वापस नया AstConst{rhsp->fileline(), AstConst::Widthed_मूल्य{}, lWidth, 0};
        }();
        स्थिर पूर्णांक rLsb = lsb;
        स्थिर पूर्णांक lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* स्थिर arp = नया AstAssign{
            nodep->fileline(),
            नया AstSel{lhsp->fileline(), lhsp->clone_वृक्ष_Pure(false), rLsb, rrp->width()}, rrp};
        AstAssign* स्थिर alp = नया AstAssign{
            nodep->fileline(),
            नया AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->add_अग्र_Here(arp);
        arp->add_अग्र_Here(alp);
        // Safe to split these.
        arp->user1(true);
        alp->user1(true);
        // Nuke what is left
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        वापस true;
    }

    // VISIT
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        अगर (v3Global.opt.fFuncSplitCat()) {
            अगर (splitConcat(nodep)) वापस;  // Must return here, in case more code is added below
        }
    }

    व्योम visit(AstConcat* nodep) override {
        अगर (Ast_गाँठ_व्यंजक* स्थिर newp = Balance_Concat_वृक्ष::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    व्योम visit(AstExtend* nodep) override {
        अगर (Ast_गाँठ_व्यंजक* स्थिर newp = Balance_Concat_वृक्ष::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

खुला:
    अटल व्योम apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

व्योम V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        स्थिर VNUser1InUse user1InUse;
        FuncOptStats stats;
        क्रम (Ast_गाँठ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), गाँठ_Module)) {
            क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                अगर (AstCFunc* स्थिर cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_वृक्ष("funcopt", 0, dump_वृक्ष_Either_Level() >= 3);
}
