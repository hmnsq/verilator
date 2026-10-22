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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3FuncOpt.h"

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి Balance_Concat_వృక్షం final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    నిర్మాణం పదం_2 final {
        Ast_కణుపు_సమాసం* exprp = శూన్యసూచిక;
        కొలత_t స్థానభ్రంశం = 0;

        పదం_2() = అప్రమేయ;
        పదం_2(Ast_కణుపు_సమాసం* exprp, కొలత_t స్థానభ్రంశం)
            : exprp{exprp}
            , స్థానభ్రంశం{స్థానభ్రంశం} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    స్థిర శూన్య gatherTermsRecursive(Ast_కణుపు_సమాసం* exprp, std::సదిశ<Ast_కణుపు_సమాసం*>& terms) {
        ఒకవేళ (AstConcat* స్థిరరాశి catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            తిరిగి;
        }
        ఒకవేళ (AstExtend* స్థిరరాశి extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_వెనుక(extp);
            తిరిగి;
        }

        // Base case: different operation
        terms.emplace_వెనుక(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    స్థిర std::సదిశ<Ast_కణుపు_సమాసం*> gatherTerms(Ast_కణుపు_సమాసం* rootp) {
        std::సదిశ<Ast_కణుపు_సమాసం*> terms;
        ఒకవేళ (AstConcat* స్థిరరాశి catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } లేకపోతే ఒకవేళ (AstExtend* స్థిరరాశి extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_వెనుక(extp);
        } లేకపోతే {
            rootp->v3fatalSrc("Unexpected node type");
        }
        తిరిగి terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    స్థిర Ast_కణుపు_సమాసం* construct(స్థిరరాశి std::సదిశ<పదం_2>& terms, స్థిరరాశి కొలత_t begin,
                                  స్థిరరాశి కొలత_t చివర) {
        UASSERT(చివర < terms.కొలత(), "Invalid end");
        UASSERT(begin < చివర, "Invalid range");
        // Base case: just return the term
        ఒకవేళ (చివర == begin + 1) తిరిగి terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        స్థిరరాశి కొలత_t వెడల్పు = terms[చివర].స్థానభ్రంశం - terms[begin].స్థానభ్రంశం;
        స్థిరరాశి కొలత_t mid_స్థానభ్రంశం = వెడల్పు / 2 + terms[begin].స్థానభ్రంశం;
        స్థిరరాశి స్వయంచాలక beginIt = terms.begin() + begin;
        స్థిరరాశి స్వయంచాలక చివర_It = terms.begin() + చివర;
        స్థిరరాశి స్వయంచాలక midIt = std::క్రింది_bound(beginIt + 1, చివర_It - 1, mid_స్థానభ్రంశం,  //
                                            [&](స్థిరరాశి పదం_2& పదం, కొలత_t విలువ) {  //
                                                తిరిగి పదం.స్థానభ్రంశం < విలువ;
                                            });
        స్థిరరాశి కొలత_t mid = begin + std::దూరం(beginIt, midIt);
        UASSERT(begin < mid && mid < చివర, "Must make some progress");
        // Construct the subtrees
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = construct(terms, begin, mid);
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = construct(terms, mid, చివర);
        // Construct new node
        Ast_కణుపు_సమాసం* newp = కొత్త AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(నిజం);  // Must not attempt to balance again.
        తిరిగి newp;
    }

    // Returns replacement node, or nullptr if no change
    స్థిర AstConcat* balance(Ast_కణుపు_సమాసం* స్థిరరాశి rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        స్థిరరాశి std::సదిశ<Ast_కణుపు_సమాసం*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        ఒకవేళ (exprps.కొలత() <= 3) తిరిగి శూన్యసూచిక;
        // Don't do it if any of the terms are impure
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి exprp : exprps) {
            ఒకవేళ (!exprp->is_శుద్ధ()) తిరిగి శూన్యసూచిక;
        }

        // Construct the terms Vector that we are going to do processing on
        std::సదిశ<పదం_2> terms(exprps.కొలత() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].స్థానభ్రంశం = 0;
        terms[exprps.కొలత()].exprp = శూన్యసూచిక;
        కొరకు (కొలత_t i = 0; i < exprps.కొలత(); ++i) {
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = [&]() -> Ast_కణుపు_సమాసం* {
                ఒకవేళ (AstExtend* స్థిరరాశి extp = VN_CAST(exprps[i], Extend)) {
                    స్థిరరాశి పూర్ణాంకం వెడల్పు = extp->వెడల్పు() - extp->lhsp()->వెడల్పు();
                    తిరిగి కొత్త AstConst{extp->fileline(), AstConst::Widthed_విలువ{}, వెడల్పు, 0};
                }
                తిరిగి exprps[i]->clone_వృక్షం_శుద్ధ(అబద్ధం);
            }();
            terms[i].exprp = exprp;
            terms[i + 1].స్థానభ్రంశం = terms[i].స్థానభ్రంశం + exprp->వెడల్పు();
        }

        // Round 1: try to create terms ending on VL_EDATASIZE boundaries.
        // This ensures we pack bits within a VL_EDATASIZE first is possible,
        // and then hopefully we can just assemble VL_EDATASIZE words afterward.
        std::సదిశ<పదం_2> terms2;
        {
            terms2.reserve(terms.కొలత());

            కొలత_t begin = 0;  // Start of current range considered
            కొలత_t చివర = 0;  // End of current range considered
            కొలత_t స్థానభ్రంశం = 0;  // Offset of current range considered

            // Create a term from the current range
            స్థిరరాశి స్వయంచాలక make_పదం = [&]() {
                Ast_కణుపు_సమాసం* స్థిరరాశి exprp = construct(terms, begin, చివర);
                terms2.emplace_వెనుక(exprp, స్థానభ్రంశం);
                స్థానభ్రంశం += exprp->వెడల్పు();
                begin = చివర;
            };

            // Create all terms ending on a boundary.
            అయితే (++చివర < terms.కొలత() - 1) {
                ఒకవేళ (terms[చివర].స్థానభ్రంశం % VL_EDATASIZE == 0) make_పదం();
            }
            // Final term. Loop condition above ensures this always exists,
            // and might or might not be on a boundary.
            make_పదం();
            // Sentinel term
            terms2.emplace_వెనుక(శూన్యసూచిక, స్థానభ్రంశం);
            // should have ended up with the same number of bits at least...
            UASSERT(terms2.వెనుక().స్థానభ్రంశం == terms.వెనుక().స్థానభ్రంశం, "Inconsistent terms");
        }

        // Round 2: Combine the partial terms
        తిరిగి VN_AS(construct(terms2, 0, terms2.కొలత() - 1), Concat);
    }

బహిరంగ:
    స్థిర Ast_కణుపు_సమాసం* apply(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (!v3Global.opt.fFuncBalanceCat()) తిరిగి శూన్యసూచిక;
        ఒకవేళ (nodep->user1()) తిరిగి శూన్యసూచిక;  // Created by us, don't try to balance again
        ఒకవేళ (VN_IS(nodep->backp(), Concat)) తిరిగి శూన్యసూచిక;  // Not root of tree
        ఒకవేళ (VN_IS(nodep->backp(), Extend)) తిరిగి శూన్యసూచిక;  // Not root of tree
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = balance(nodep);
        ఒకవేళ (exprp) exprp->user1(నిజం);  // Must not attempt again.
        తిరిగి exprp;
    }
};

నిర్మాణం FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_సమతుల్య_Concats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = అప్రమేయ;
    ~FuncOptStats() {
        V3Stats::కూడు_Stat("Optimizations, FuncOpt concat trees balanced", m_సమతుల్య_Concats);
        V3Stats::కూడు_Stat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

తరగతి FuncOptVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    స్థిర తార్కిక readsLhs(Ast_కణుపు_కేటాయించు* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_సమితి<స్థిరరాశి AstVar*> lhsWrVarps;
        std::unordered_సమితి<స్థిరరాశి AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](స్థిరరాశి AstVarRef* refp) {
            ఒకవేళ (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            ఒకవేళ (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        ఒకవేళ (lhsWrVarps.కొలత() == 1) {
            స్థిరరాశి AstVar* స్థిరరాశి lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            స్థిరరాశి తార్కిక rhsReadsWritten = nodep->rhsp()->exists([=](స్థిరరాశి AstVarRef* refp) {  //
                తిరిగి refp->varp() == lhsWrVarp;
            });
            ఒకవేళ (rhsReadsWritten) తిరిగి నిజం;
            // Check Lhs doesn't read the written var
            తిరిగి lhsRdVarps.లెక్క(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        స్థిరరాశి తార్కిక rhsReadsWritten = nodep->rhsp()->exists([&](స్థిరరాశి AstVarRef* refp) {  //
            తిరిగి lhsWrVarps.లెక్క(refp->varp());
        });
        ఒకవేళ (rhsReadsWritten) తిరిగి నిజం;
        // Check Lhs doesn't read the written vars
        కొరకు (స్థిరరాశి AstVar* స్థిరరాశి lhsWrVarp : lhsWrVarps) {
            ఒకవేళ (lhsRdVarps.లెక్క(lhsWrVarp)) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    తార్కిక విభజించు_Concat(Ast_కణుపు_కేటాయించు* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        ఒకవేళ (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) తిరిగి అబద్ధం;
        // Will need the LHS
        Ast_కణుపు_సమాసం* lhsp = nodep->lhsp();
        ఒకవేళ (!VN_IS(lhsp->dtypep()->skipRefp(), వరుస_DType))
            UASSERT_OBJ(lhsp->వెడల్పు() == rhsp->వెడల్పు(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        ఒకవేళ (!nodep->user1() && (!lhsp->is_శుద్ధ() || !rhsp->is_శుద్ధ())) తిరిగి అబద్ధం;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        ఒకవేళ (lhsp->exists([](AstVarRef* refp) { తిరిగి refp->varp()->isSc(); })) తిరిగి అబద్ధం;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(lhsp, Sel)) {
            ఒకవేళ (AstConst* స్థిరరాశి lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->గమ్యం_UInt();
            } లేకపోతే {
                // Don't optimize if it's a variable select
                తిరిగి అబద్ధం;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        ఒకవేళ (lhsp->వెడల్పు() <= VL_QUADSIZE) తిరిగి అబద్ధం;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        స్థిరరాశి పూర్ణాంకం lsb_మాట = lsb / VL_EDATASIZE;
        స్థిరరాశి పూర్ణాంకం msb_మాట = (lsb + rhsp->వెడల్పు() - 1) / VL_EDATASIZE;
        ఒకవేళ (lsb_మాట == msb_మాట) తిరిగి అబద్ధం;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        ఒకవేళ (!nodep->user1() && readsLhs(nodep)) తిరిగి అబద్ధం;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_కణుపు_సమాసం* స్థిరరాశి rrp = [rhsp]() -> Ast_కణుపు_సమాసం* {
            ఒకవేళ (AstConcat* స్థిరరాశి catp = VN_CAST(rhsp, Concat)) {
                తిరిగి catp->rhsp()->unlink_Fr_వెనుక();
            }
            తిరిగి VN_AS(rhsp, Extend)->lhsp()->unlink_Fr_వెనుక();
        }();
        Ast_కణుపు_సమాసం* స్థిరరాశి rlp = [rhsp, rrp]() -> Ast_కణుపు_సమాసం* {
            ఒకవేళ (AstConcat* స్థిరరాశి catp = VN_CAST(rhsp, Concat)) {
                తిరిగి catp->lhsp()->unlink_Fr_వెనుక();
            }
            స్థిరరాశి పూర్ణాంకం l_వెడల్పు = rhsp->వెడల్పు() - rrp->వెడల్పు();
            తిరిగి కొత్త AstConst{rhsp->fileline(), AstConst::Widthed_విలువ{}, l_వెడల్పు, 0};
        }();
        స్థిరరాశి పూర్ణాంకం rLsb = lsb;
        స్థిరరాశి పూర్ణాంకం lLsb = lsb + rrp->వెడల్పు();
        // Insert the 2 assignment right after the original. They will be visited next.
        Ast_కేటాయించు* స్థిరరాశి arp = కొత్త Ast_కేటాయించు{
            nodep->fileline(),
            కొత్త AstSel{lhsp->fileline(), lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), rLsb, rrp->వెడల్పు()}, rrp};
        Ast_కేటాయించు* స్థిరరాశి alp = కొత్త Ast_కేటాయించు{
            nodep->fileline(),
            కొత్త AstSel{lhsp->fileline(), lhsp->unlink_Fr_వెనుక(), lLsb, rlp->వెడల్పు()}, rlp};
        nodep->కూడు_తదుపరి_ఇక్కడ(arp);
        arp->కూడు_తదుపరి_ఇక్కడ(alp);
        // Safe to split these.
        arp->user1(నిజం);
        alp->user1(నిజం);
        // Nuke what is left
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        తిరిగి నిజం;
    }

    //సందర్శించు
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        ఒకవేళ (v3Global.opt.f_Func_విభజించు_Cat()) {
            ఒకవేళ (విభజించు_Concat(nodep)) తిరిగి;  // Must return here, in case more code is added below
        }
    }

    శూన్య సందర్శించు(AstConcat* nodep) override {
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి newp = Balance_Concat_వృక్షం::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_సమతుల్య_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstExtend* nodep) override {
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి newp = Balance_Concat_వృక్షం::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_సమతుల్య_Concats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    స్పష్టమైన FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

బహిరంగ:
    స్థిర శూన్య apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

శూన్య V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
        FuncOptStats stats;
        కొరకు (Ast_కణుపు_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), కణుపు_Module)) {
            కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ఒకవేళ (AstCFunc* స్థిరరాశి cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_వృక్షం("funcopt", 0, dump_వృక్షం_Either_Level() >= 3);
}
