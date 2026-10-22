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

वर्ग Balance_Concat_वृक्षः final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    संरचना पदांशः_2 final {
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = शून्यसूचकः;
        आकार_t विस्थापनम् = 0;

        पदांशः_2() = स्वभाव;
        पदांशः_2(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, आकार_t विस्थापनम्)
            : exprp{exprp}
            , विस्थापनम्{विस्थापनम्} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    स्थिर शून्य gatherTermsRecursive(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& terms) {
        यदि (AstConcat* ध्रुव catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            प्रत्यागम;
        }
        यदि (AstExtend* ध्रुव extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_पृष्ठभागः(extp);
            प्रत्यागम;
        }

        // Base case: different operation
        terms.emplace_पृष्ठभागः(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    स्थिर std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> gatherTerms(Ast_ग्रन्थिः_व्यञ्जकम्* rootp) {
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> terms;
        यदि (AstConcat* ध्रुव catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } अन्यथा यदि (AstExtend* ध्रुव extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_पृष्ठभागः(extp);
        } अन्यथा {
            rootp->v3fatalSrc("Unexpected node type");
        }
        प्रत्यागम terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* construct(ध्रुव std::सदिशम्<पदांशः_2>& terms, ध्रुव आकार_t begin,
                                  ध्रुव आकार_t अन्तः_2) {
        UASSERT(अन्तः_2 < terms.आकार(), "Invalid end");
        UASSERT(begin < अन्तः_2, "Invalid range");
        // Base case: just return the term
        यदि (अन्तः_2 == begin + 1) प्रत्यागम terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        ध्रुव आकार_t विस्तारः = terms[अन्तः_2].विस्थापनम् - terms[begin].विस्थापनम्;
        ध्रुव आकार_t mid_विस्थापनम् = विस्तारः / 2 + terms[begin].विस्थापनम्;
        ध्रुव स्वचालित beginIt = terms.begin() + begin;
        ध्रुव स्वचालित अन्तः_It = terms.begin() + अन्तः_2;
        ध्रुव स्वचालित midIt = std::अधस्तनम्_bound(beginIt + 1, अन्तः_It - 1, mid_विस्थापनम्,  //
                                            [&](ध्रुव पदांशः_2& पदांशः, आकार_t मानम्) {  //
                                                प्रत्यागम पदांशः.विस्थापनम् < मानम्;
                                            });
        ध्रुव आकार_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < अन्तः_2, "Must make some progress");
        // Construct the subtrees
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = construct(terms, begin, mid);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = construct(terms, mid, अन्तः_2);
        // Construct new node
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = नूतन AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(सत्य);  // Must not attempt to balance again.
        प्रत्यागम newp;
    }

    // Returns replacement node, or nullptr if no change
    स्थिर AstConcat* balance(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        ध्रुव std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        यदि (exprps.आकार() <= 3) प्रत्यागम शून्यसूचकः;
        // Don't do it if any of the terms are impure
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp : exprps) {
            यदि (!exprp->is_विशुद्धम्()) प्रत्यागम शून्यसूचकः;
        }

        // Construct the terms Vector that we are going to do processing on
        std::सदिशम्<पदांशः_2> terms(exprps.आकार() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].विस्थापनम् = 0;
        terms[exprps.आकार()].exprp = शून्यसूचकः;
        प्रति (आकार_t i = 0; i < exprps.आकार(); ++i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                यदि (AstExtend* ध्रुव extp = VN_CAST(exprps[i], Extend)) {
                    ध्रुव पूर्णाङ्क विस्तारः = extp->विस्तारः() - extp->lhsp()->विस्तारः();
                    प्रत्यागम नूतन AstConst{extp->fileline(), AstConst::Widthed_मानम्{}, विस्तारः, 0};
                }
                प्रत्यागम exprps[i]->clone_वृक्षः_विशुद्धम्(असत्य);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].विस्थापनम् = terms[i].विस्थापनम् + exprp->विस्तारः();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::सदिशम्<पदांशः_2> terms2;
        {
            terms2.reserve(terms.आकार());

            आकार_t begin = 0;  // Start of current range considered
            आकार_t अन्तः_2 = 0;  // End of current range considered
            आकार_t विस्थापनम् = 0;  // Offset of current range considered

            // Create a term from the current range
            ध्रुव स्वचालित make_पदांशः = [&]() {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = construct(terms, begin, अन्तः_2);
                terms2.emplace_पृष्ठभागः(exprp, विस्थापनम्);
                विस्थापनम् += exprp->विस्तारः();
                begin = अन्तः_2;
            };

            // Create all terms ending on a boundary.
            यावत् (++अन्तः_2 < terms.आकार() - 1) {
                यदि (terms[अन्तः_2].विस्थापनम् % VL_EDATASIZE == 0) make_पदांशः();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_पदांशः();
            // Sentinel term
            terms2.emplace_पृष्ठभागः(शून्यसूचकः, विस्थापनम्);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.पृष्ठभागः().विस्थापनम् == terms.पृष्ठभागः().विस्थापनम्, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        प्रत्यागम VN_AS(construct(terms2, 0, terms2.आकार() - 1), Concat);
    }

सार्वजनिक:
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* apply(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (!v3Global.opt.fFuncBalanceCat()) प्रत्यागम शून्यसूचकः;
        यदि (nodep->user1()) प्रत्यागम शून्यसूचकः;  // Created by us, don't try to balance again
        यदि (VN_IS(nodep->backp(), Concat)) प्रत्यागम शून्यसूचकः;  // Not root of tree
        यदि (VN_IS(nodep->backp(), Extend)) प्रत्यागम शून्यसूचकः;  // Not root of tree
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = balance(nodep);
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
        V3Stats::सङ्कलय_Stat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::सङ्कलय_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

वर्ग FuncOptVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    स्थिर तार्किक readsLhs(Ast_ग्रन्थिः_नियोजय* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_समुच्चयः<ध्रुव AstVar*> lhsWrVarps;
        std::unordered_समुच्चयः<ध्रुव AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](ध्रुव AstVarRef* refp) {
            यदि (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            यदि (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        यदि (lhsWrVarps.आकार() == 1) {
            ध्रुव AstVar* ध्रुव lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            ध्रुव तार्किक rhsReadsWritten = nodep->rhsp()->exists([=](ध्रुव AstVarRef* refp) {  //
                प्रत्यागम refp->varp() == lhsWrVarp;
            });
            यदि (rhsReadsWritten) प्रत्यागम सत्य;
            // Check Lhs doesn't read the written var
            प्रत्यागम lhsRdVarps.गणनम्(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        ध्रुव तार्किक rhsReadsWritten = nodep->rhsp()->exists([&](ध्रुव AstVarRef* refp) {  //
            प्रत्यागम lhsWrVarps.गणनम्(refp->varp());
        });
        यदि (rhsReadsWritten) प्रत्यागम सत्य;
        // Check Lhs doesn't read the written vars
        प्रति (ध्रुव AstVar* ध्रुव lhsWrVarp : lhsWrVarps) {
            यदि (lhsRdVarps.गणनम्(lhsWrVarp)) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    तार्किक विभज_Concat(Ast_ग्रन्थिः_नियोजय* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        यदि (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) प्रत्यागम असत्य;
        // Will need the LHS
        Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp();
        यदि (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->विस्तारः() == rhsp->विस्तारः(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        यदि (!nodep->user1() && (!lhsp->is_विशुद्धम्() || !rhsp->is_विशुद्धम्())) प्रत्यागम असत्य;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        यदि (lhsp->exists([](AstVarRef* refp) { प्रत्यागम refp->varp()->isSc(); })) प्रत्यागम असत्य;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        यदि (AstSel* ध्रुव selp = VN_CAST(lhsp, Sel)) {
            यदि (AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->गन्तव्यम्_UInt();
            } अन्यथा {
                // Don't optimize if it's a variable select
                प्रत्यागम असत्य;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        यदि (lhsp->विस्तारः() <= VL_QUADSIZE) प्रत्यागम असत्य;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        ध्रुव पूर्णाङ्क lsbWord = lsb / VL_EDATASIZE;
        ध्रुव पूर्णाङ्क msbWord = (lsb + rhsp->विस्तारः() - 1) / VL_EDATASIZE;
        यदि (lsbWord == msbWord) प्रत्यागम असत्य;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        यदि (!nodep->user1() && readsLhs(nodep)) प्रत्यागम असत्य;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rrp = [rhsp]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            यदि (AstConcat* ध्रुव catp = VN_CAST(rhsp, Concat)) {
                प्रत्यागम catp->rhsp()->unlink_Fr_पृष्ठभागः();
            }
            प्रत्यागम VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_पृष्ठभागः();
        }();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rlp = [rhsp, rrp]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            यदि (AstConcat* ध्रुव catp = VN_CAST(rhsp, Concat)) {
                प्रत्यागम catp->lhsp()->unlink_Fr_पृष्ठभागः();
            }
            ध्रुव पूर्णाङ्क l_विस्तारः = rhsp->विस्तारः() - rrp->विस्तारः();
            प्रत्यागम नूतन AstConst{rhsp->fileline(), AstConst::Widthed_मानम्{}, l_विस्तारः, 0};
        }();
        ध्रुव पूर्णाङ्क rLsb = lsb;
        ध्रुव पूर्णाङ्क lLsb = lsb + rrp->विस्तारः();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_नियोजय* ध्रुव arp = नूतन Ast_नियोजय{
            nodep->fileline(),
            नूतन AstSel{lhsp->fileline(), lhsp->clone_वृक्षः_विशुद्धम्(असत्य), rLsb, rrp->विस्तारः()}, rrp};
        Ast_नियोजय* ध्रुव alp = नूतन Ast_नियोजय{
            nodep->fileline(),
            नूतन AstSel{lhsp->fileline(), lhsp->unlink_Fr_पृष्ठभागः(), lLsb, rlp->विस्तारः()}, rlp};
        nodep->सङ्कलय_अग्र_अत्र(arp);
        arp->सङ्कलय_अग्र_अत्र(alp);
        // Safe to split these.
        arp->user1(सत्य);
        alp->user1(सत्य);
        // Nuke what is left
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        प्रत्यागम सत्य;
    }

    //दर्शय
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        यदि (v3Global.opt.f_Func_विभज_Cat()) {
            यदि (विभज_Concat(nodep)) प्रत्यागम;  // Must return here, in case more code is added below
        }
    }

    शून्य दर्शय(AstConcat* nodep) override {
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = Balance_Concat_वृक्षः::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstExtend* nodep) override {
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = Balance_Concat_वृक्षः::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    स्पष्ट FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
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
        ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;
        FuncOptStats stats;
        प्रति (Ast_ग्रन्थिः_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
            प्रति (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (AstCFunc* ध्रुव cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_वृक्षः("funcopt", 0, dump_वृक्षः_Either_Level() >= 3);
}
