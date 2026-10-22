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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3FuncOpt.h"

#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം Balance_Concat_വൃക്ഷം final {
    // STATELESS

    // We keep the expressions, together with their offsets within a concatenation tree
    ഘടന Term final {
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nullptr;
        size_t offset = 0;

        Term() = സ്ഥിരസ്ഥിതി;
        Term(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, size_t offset)
            : exprp{exprp}
            , offset{offset} {}
    };

    // Recursive implementation of 'gatherTerms' below.
    സ്ഥിതം ശൂന്യം gatherTermsRecursive(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& terms) {
        എങ്കിൽ (AstConcat* സ്ഥിരം catp = VN_CAST(exprp, Concat)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
            തിരികെ;
        }
        എങ്കിൽ (AstExtend* സ്ഥിരം extp = VN_CAST(exprp, Extend)) {
            // Recursive case: gather sub terms, right to left
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
            തിരികെ;
        }

        // Base case: different operation
        terms.emplace_back(exprp);
    }

    // Gather terms in the tree rooted at the given node.
    // Results are right to left, that is, index 0 in the returned vector
    // is the rightmost term, index size()-1 is the leftmost term.
    // If a term is an AstExtend, it represents the extension part only.
    സ്ഥിതം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> gatherTerms(Ast_കണ്ണി_വ്യഞ്ജകം* rootp) {
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> terms;
        എങ്കിൽ (AstConcat* സ്ഥിരം catp = VN_CAST(rootp, Concat)) {
            gatherTermsRecursive(catp->rhsp(), terms);
            gatherTermsRecursive(catp->lhsp(), terms);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstExtend* സ്ഥിരം extp = VN_CAST(rootp, Extend)) {
            gatherTermsRecursive(extp->lhsp(), terms);
            terms.emplace_back(extp);
        } അല്ലെങ്കിൽ {
            rootp->v3fatalSrc("Unexpected node type");
        }
        തിരികെ terms;
    }

    // Construct a balanced concatenation from the given terms,
    // between indices begin (inclusive), and end (exclusive).
    // Note term[end].offset must be valid. term[end].vtxp is
    // never referenced.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* construct(സ്ഥിരം std::vector<Term>& terms, സ്ഥിരം size_t begin,
                                  സ്ഥിരം size_t end) {
        UASSERT(end < terms.size(), "Invalid end");
        UASSERT(begin < end, "Invalid range");
        // Base case: just return the term
        എങ്കിൽ (end == begin + 1) തിരികെ terms[begin].exprp;

        // Recursive case:
        // Compute the mid-point, trying to create roughly equal width intermediates
        സ്ഥിരം size_t width = terms[end].offset - terms[begin].offset;
        സ്ഥിരം size_t midOffset = width / 2 + terms[begin].offset;
        സ്ഥിരം auto beginIt = terms.begin() + begin;
        സ്ഥിരം auto endIt = terms.begin() + end;
        സ്ഥിരം auto midIt = std::lower_bound(beginIt + 1, endIt - 1, midOffset,  //
                                            [&](സ്ഥിരം Term& term, size_t മൂല്യം) {  //
                                                തിരികെ term.offset < മൂല്യം;
                                            });
        സ്ഥിരം size_t mid = begin + std::distance(beginIt, midIt);
        UASSERT(begin < mid && mid < end, "Must make some progress");
        // Construct the subtrees
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = construct(terms, begin, mid);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = construct(terms, mid, end);
        // Construct new node
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = പുതിയ AstConcat{lhsp->fileline(), lhsp, rhsp};
        newp->user1(സത്യം);  // Must not attempt to balance again.
        തിരികെ newp;
    }

    // Returns replacement node, or nullptr if no change
    സ്ഥിതം AstConcat* balance(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rootp) {
        UINFO(9, "balanceConcat " << rootp);
        // Gather all input vertices of the tree
        സ്ഥിരം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> exprps = gatherTerms(rootp);
        // Don't bother with trivial trees
        എങ്കിൽ (exprps.size() <= 3) തിരികെ nullptr;
        // Don't do it if any of the terms are impure
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp : exprps) {
            എങ്കിൽ (!exprp->isPure()) തിരികെ nullptr;
        }

        // Construct the terms Vector that we are going to do processing on
        std::vector<Term> terms(exprps.size() + 1);
        // These are redundant (constructor does the same), but here they are for clarity
        terms[0].offset = 0;
        terms[exprps.size()].exprp = nullptr;
        വേണ്ടി (size_t i = 0; i < exprps.size(); ++i) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                എങ്കിൽ (AstExtend* സ്ഥിരം extp = VN_CAST(exprps[i], Extend)) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ width = extp->width() - extp->lhsp()->width();
                    തിരികെ പുതിയ AstConst{extp->fileline(), AstConst::Widthed_മൂല്യം{}, width, 0};
                }
                തിരികെ exprps[i]->clone_വൃക്ഷം_Pure(അസത്യം);
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
            സ്ഥിരം auto makeTerm = [&]() {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = construct(terms, begin, end);
                terms2.emplace_back(exprp, offset);
                offset += exprp->width();
                begin = end;
            };

            // Create all terms ending on a boundary.
            ആകുമ്പോൾ (++end < terms.size() - 1) {
                എങ്കിൽ (terms[end].offset % VL_EDATASIZE == 0) makeTerm();
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
        തിരികെ VN_AS(construct(terms2, 0, terms2.size() - 1), Concat);
    }

പൊതു:
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* apply(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (!v3Global.opt.fFuncBalanceCat()) തിരികെ nullptr;
        എങ്കിൽ (nodep->user1()) തിരികെ nullptr;  // Created by us, don't try to balance again
        എങ്കിൽ (VN_IS(nodep->backp(), Concat)) തിരികെ nullptr;  // Not root of tree
        എങ്കിൽ (VN_IS(nodep->backp(), Extend)) തിരികെ nullptr;  // Not root of tree
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = balance(nodep);
        എങ്കിൽ (exprp) exprp->user1(സത്യം);  // Must not attempt again.
        തിരികെ exprp;
    }
};

ഘടന FuncOptStats final {
    // STATE - Statistic tracking
    VDouble0 m_balancedConcats;  // Number of concatenations balanced
    VDouble0 m_concatSplits;  // Number of splits in assignments with Concat on RHS

    FuncOptStats() = സ്ഥിരസ്ഥിതി;
    ~FuncOptStats() {
        V3Stats::addStat("Optimizations, FuncOpt concat trees balanced", m_balancedConcats);
        V3Stats::addStat("Optimizations, FuncOpt concat splits", m_concatSplits);
    }
};

വർഗ്ഗം FuncOptVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstNodeAssign::user()     -> bool.  Already checked, safe to split. Omit expensive check.
    //  AstConcat::user()         -> bool.  Already balanced.

    // STATE
    FuncOptStats& m_stats;  // Statistics

    // True for e.g.: foo = foo >> 1; or foo[foo[0]] = ...;
    സ്ഥിതം തർക്കം readsLhs(Ast_കണ്ണി_Assign* nodep) {
        // It is expected that the number of vars written on the LHS is very small (should be 1).
        std::unordered_set<സ്ഥിരം AstVar*> lhsWrVarps;
        std::unordered_set<സ്ഥിരം AstVar*> lhsRdVarps;
        nodep->lhsp()->foreach([&](സ്ഥിരം AstVarRef* refp) {
            എങ്കിൽ (refp->access().isWriteOrRW()) lhsWrVarps.emplace(refp->varp());
            എങ്കിൽ (refp->access().isReadOrRW()) lhsRdVarps.emplace(refp->varp());
        });

        // Common case of 1 variable on the LHS - special handling for speed
        എങ്കിൽ (lhsWrVarps.size() == 1) {
            സ്ഥിരം AstVar* സ്ഥിരം lhsWrVarp = *lhsWrVarps.begin();
            // Check Rhs doesn't read the written var
            സ്ഥിരം തർക്കം rhsReadsWritten = nodep->rhsp()->exists([=](സ്ഥിരം AstVarRef* refp) {  //
                തിരികെ refp->varp() == lhsWrVarp;
            });
            എങ്കിൽ (rhsReadsWritten) തിരികെ സത്യം;
            // Check Lhs doesn't read the written var
            തിരികെ lhsRdVarps.എണ്ണം(lhsWrVarp);
        }

        // Generic case of multiple vars written on LHS
        // TODO: this might be impossible due to earlier transforms, not sure
        // Check Rhs doesn't read the written vars
        സ്ഥിരം തർക്കം rhsReadsWritten = nodep->rhsp()->exists([&](സ്ഥിരം AstVarRef* refp) {  //
            തിരികെ lhsWrVarps.എണ്ണം(refp->varp());
        });
        എങ്കിൽ (rhsReadsWritten) തിരികെ സത്യം;
        // Check Lhs doesn't read the written vars
        വേണ്ടി (സ്ഥിരം AstVar* സ്ഥിരം lhsWrVarp : lhsWrVarps) {
            എങ്കിൽ (lhsRdVarps.എണ്ണം(lhsWrVarp)) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // METHODS
    // Split wide assignments with a wide concatenation on the RHS.
    // Returns true if 'nodep' was deleted
    തർക്കം splitConcat(Ast_കണ്ണി_Assign* nodep) {
        UINFO(9, "splitConcat " << nodep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp();
        // Only care about concatenations an zero extend on the RHS
        എങ്കിൽ (!VN_IS(rhsp, Concat) && !VN_IS(rhsp, Extend)) തിരികെ അസത്യം;
        // Will need the LHS
        Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nodep->lhsp();
        എങ്കിൽ (!VN_IS(lhsp->dtypep()->skipRefp(), QueueDType))
            UASSERT_OBJ(lhsp->width() == rhsp->width(), nodep, "Inconsistent assignment");
        // Only consider pure assignments. Nodes inserted below are safe.
        എങ്കിൽ (!nodep->user1() && (!lhsp->isPure() || !rhsp->isPure())) തിരികെ അസത്യം;
        // Do not split assignments to SC variables, they cannot be assigned in parts
        എങ്കിൽ (lhsp->exists([](AstVarRef* refp) { തിരികെ refp->varp()->isSc(); })) തിരികെ അസത്യം;
        // Check for a Sel on the LHS if present, and skip over it
        uint32_t lsb = 0;
        എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(lhsp, Sel)) {
            എങ്കിൽ (AstConst* സ്ഥിരം lsbp = VN_CAST(selp->lsbp(), Const)) {
                lhsp = selp->fromp();
                lsb = lsbp->toUInt();
            } അല്ലെങ്കിൽ {
                // Don't optimize if it's a variable select
                തിരികെ അസത്യം;
            }
        }
        // No need to split assignments targeting storage smaller than a machine register
        എങ്കിൽ (lhsp->width() <= VL_QUADSIZE) തിരികെ അസത്യം;

        // If it's a concat straddling a word boundary, try to split it.
        // The next visit on the new nodes will split it recursively.
        // Otherwise, keep the original assignment.
        സ്ഥിരം പൂർണ്ണസംഖ്യ lsbWord = lsb / VL_EDATASIZE;
        സ്ഥിരം പൂർണ്ണസംഖ്യ msbWord = (lsb + rhsp->width() - 1) / VL_EDATASIZE;
        എങ്കിൽ (lsbWord == msbWord) തിരികെ അസത്യം;

        // If the RHS reads the LHS, we can't actually do this. Nodes inserted below are safe.
        എങ്കിൽ (!nodep->user1() && readsLhs(nodep)) തിരികെ അസത്യം;

        // Ok, actually split it now
        UINFO(5, "splitConcat optimizing " << nodep);
        ++m_stats.m_concatSplits;
        // The 2 parts and their offsets
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rrp = [rhsp]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            എങ്കിൽ (AstConcat* സ്ഥിരം catp = VN_CAST(rhsp, Concat)) {
                തിരികെ catp->rhsp()->unlinkFrBack();
            }
            തിരികെ VN_AS(rhsp, Extend)->lhsp()->unlinkFrBack();
        }();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rlp = [rhsp, rrp]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            എങ്കിൽ (AstConcat* സ്ഥിരം catp = VN_CAST(rhsp, Concat)) {
                തിരികെ catp->lhsp()->unlinkFrBack();
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ lWidth = rhsp->width() - rrp->width();
            തിരികെ പുതിയ AstConst{rhsp->fileline(), AstConst::Widthed_മൂല്യം{}, lWidth, 0};
        }();
        സ്ഥിരം പൂർണ്ണസംഖ്യ rLsb = lsb;
        സ്ഥിരം പൂർണ്ണസംഖ്യ lLsb = lsb + rrp->width();
        // Insert the 2 assignment right after the original. They will be visited next.
        AstAssign* സ്ഥിരം arp = പുതിയ AstAssign{
            nodep->fileline(),
            പുതിയ AstSel{lhsp->fileline(), lhsp->clone_വൃക്ഷം_Pure(അസത്യം), rLsb, rrp->width()}, rrp};
        AstAssign* സ്ഥിരം alp = പുതിയ AstAssign{
            nodep->fileline(),
            പുതിയ AstSel{lhsp->fileline(), lhsp->unlinkFrBack(), lLsb, rlp->width()}, rlp};
        nodep->addNextHere(arp);
        arp->addNextHere(alp);
        // Safe to split these.
        arp->user1(സത്യം);
        alp->user1(സത്യം);
        // Nuke what is left
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        തിരികെ സത്യം;
    }

    // VISIT
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        // TODO: Only thing remaining inside functions should be AstAssign (that is, an actual
        //       assignment statement), but we stil use AstAssignW, AstAssignDly, and all, fix.
        iterateChildren(nodep);

        എങ്കിൽ (v3Global.opt.fFuncSplitCat()) {
            എങ്കിൽ (splitConcat(nodep)) തിരികെ;  // Must return here, in case more code is added below
        }
    }

    ശൂന്യം visit(AstConcat* nodep) override {
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = Balance_Concat_വൃക്ഷം::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstExtend* nodep) override {
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = Balance_Concat_വൃക്ഷം::apply(nodep)) {
            UINFO(5, "balanceConcat optimizing " << nodep);
            ++m_stats.m_balancedConcats;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;  // The new node will be iterated next
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit FuncOptVisitor(FuncOptStats& stats, AstCFunc* funcp)
        : m_stats{stats} {
        iterateChildren(funcp);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(FuncOptStats& stats, AstCFunc* funcp) { FuncOptVisitor{stats, funcp}; }
};

//######################################################################

ശൂന്യം V3FuncOpt::funcOptAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // NODE STATE
        //  AstNode::user1()     -> bool.  Processed
        സ്ഥിരം VNUser1InUse user1InUse;
        FuncOptStats stats;
        വേണ്ടി (Ast_കണ്ണി_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
            വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                എങ്കിൽ (AstCFunc* സ്ഥിരം cfuncp = VN_CAST(stmtp, CFunc)) {
                    FuncOptVisitor::apply(stats, cfuncp);
                }
            }
        }
    }
    V3Global::dump_Check_Global_വൃക്ഷം("funcopt", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
