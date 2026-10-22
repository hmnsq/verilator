// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for expand nodes
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Expand's Transformations:
//
// Each module:
//      Expand verilated.h macros into internal micro optimizations (RTL)
//      this will enable later optimizations.
//      Wide operands become assignments to each word of the vector, (WORDSELs)
//          Note in this case that the widthMin is not correct for the MSW of
//          the vector.  This must be accounted for if doing later constant
//          propagation across signals.
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Expand.h"

#include "V3Const.h"
#include "V3Stats.h"

#include <algorithm>
#include <array>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// The distinct expansion patterns, for statistics. One entry for each
// unique transformation ExpandVisitor applies. Keep sorted.

// clang-format off
#define FOR_EACH_EXPAND_PATTERN(macro) \
    macro(ASSIGN_AND) \
    macro(ASSIGN_ARRAYSEL) \
    macro(ASSIGN_CONCAT) \
    macro(ASSIGN_COND) \
    macro(ASSIGN_CONST) \
    macro(ASSIGN_EXTEND) \
    macro(ASSIGN_NOT) \
    macro(ASSIGN_OR) \
    macro(ASSIGN_REPLICATE) \
    macro(ASSIGN_SEL_ALIGNED) \
    macro(ASSIGN_SEL_UNALIGNED) \
    macro(ASSIGN_SHIFTL) \
    macro(ASSIGN_SHIFTR) \
    macro(ASSIGN_TO_SEL_CONST_NARROW) \
    macro(ASSIGN_TO_SEL_CONST_WIDE) \
    macro(ASSIGN_TO_SEL_VAR_NARROW) \
    macro(ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT) \
    macro(ASSIGN_VARREF) \
    macro(ASSIGN_XOR) \
    macro(CONCAT) \
    macro(REDAND_TO_EQ) \
    macro(REDOR_TO_NEQ) \
    macro(REPLICATE) \
    macro(REPLICATE_ONE_BIT) \
    macro(SEL_FROM_NARROW) \
    macro(SEL_FROM_WIDE) \
    macro(WORDIZE_EQ) \
    macro(WORDIZE_NEQ) \
    macro(WORDIZE_REDAND) \
    macro(WORDIZE_REDOR) \
    macro(WORDIZE_REDXOR)
// clang-format on

class VExpandPattern final {
public:
    ga_nanaa en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static const var_na* const names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        ལོག names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() const { ལོག m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

class ExpandOkVisitor final : public VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    const VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    bool m_isImpure = true;  // Currently pure

    void visit(AstNode* nodep) override {
        bool selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = false;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

public:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = default;
};

//######################################################################
// Expand state, as a visitor of each AstNode

class ExpandVisitor final : public VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    const VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    AstNode* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_statWideWords;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    void didPattern(VExpandPattern pattern, AstNode* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    static bool isImpure(AstNode* nodep) {
        const bool impure = nodep->user2();
        གལ་སྲིད (impure) UINFO(9, "      impure " << nodep);
        ལོག impure;
    }

    bool doExpandWide(AstNode* nodep) {
        གལ་སྲིད (isImpure(nodep)) {
            ++m_statImpure;
            ལོག false;
        }
        གལ་སྲིད (nodep->widthWords() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_statWideWords += nodep->widthWords();
            ལོག true;
        } དེ་མིན {
            ++m_statWideLimited;
            ལོག false;
        }
    }

    static ཧྲིལ longOrQuadWidth(AstNode* nodep) {
        ལོག (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    static V3Number notWideMask(AstNode* nodep) {
        ལོག V3Number{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    static V3Number wordMask(AstNode* nodep) {
        གལ་སྲིད (nodep->isWide()) {
            ལོག V3Number{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } དེ་མིན {
            V3Number mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            ལོག mask;
        }
    }

    static void insertBefore(AstNode* placep, AstNode* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    static void replaceWithDelete(AstNode* nodep, AstNode* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        གལ་སྲིད (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
    static AstNode* newWordAssign(AstNodeAssign* placep, ཧྲིལ word, AstNodeExpr* lhsp,
                                  AstNodeExpr* rhsp) {
        FileLine* const fl = placep->fileline();
        ལོག new AstAssign{fl,
                             new AstWordSel{fl, lhsp->cloneTreePure(true),
                                            new AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    static void addWordAssign(AstNodeAssign* placep, ཧྲིལ word, AstNodeExpr* lhsp,
                              AstNodeExpr* rhsp) {
        insertBefore(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    static void addWordAssign(AstNodeAssign* placep, ཧྲིལ word, AstNodeExpr* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(AstNode* placep, const var_na* namep, AstNodeExpr* valuep) {
        const std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        FileLine* const flp = placep->fileline();
        AstVar* const tmpp = new AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(true);
        tmpp->isInternal(true);
        tmpp->noReset(true);
        tmpp->substConstOnly(true);
        m_funcp->addVarsp(tmpp);
        insertBefore(placep, new AstAssign{flp, new AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        ལོག tmpp;
    }

    static void fixCloneLvalue(AstNode* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        གལ་སྲིད (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        གལ་སྲིད (AstNode* const refp = nodep->op1p()) fixCloneLvalue(refp);
        གལ་སྲིད (AstNode* const refp = nodep->op2p()) fixCloneLvalue(refp);
        གལ་སྲིད (AstNode* const refp = nodep->op3p()) fixCloneLvalue(refp);
        གལ་སྲིད (AstNode* const refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    static AstNodeExpr* newAstWordSelClone(AstNodeExpr* nodep, ཧྲིལ word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        FileLine* const fl = nodep->fileline();
        གལ་སྲིད (nodep->isWide() && word >= 0 && word < nodep->widthWords()) {
            ལོག new AstWordSel{fl, nodep->cloneTreePure(true),
                                  new AstConst{fl, static_cast<uint32_t>(word)}};
        } དེ་མིན གལ་སྲིད (nodep->isQuad() && word == 0) {
            AstNodeExpr* const quadfromp = nodep->cloneTreePure(true);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ལོག new AstCCast{fl, quadfromp, VL_EDATASIZE};
        } དེ་མིན གལ་སྲིད (nodep->isQuad() && word == 1) {
            AstNodeExpr* const quadfromp = nodep->cloneTreePure(true);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ལོག new AstCCast{
                fl, new AstShiftR{fl, quadfromp, new AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } དེ་མིན གལ་སྲིད (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            ལོག nodep->cloneTreePure(true);
        } དེ་མིན {  // Out of bounds
            ལོག new AstConst{fl, 0};
        }
    }

    static AstNodeExpr* newWordGrabShift(FileLine* fl, ཧྲིལ word, AstNodeExpr* lhsp, ཧྲིལ shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        const ཧྲིལ othword = word - shift / VL_EDATASIZE;
        AstNodeExpr* const lop = newAstWordSelClone(lhsp, othword);
        const uint32_t loShift = VL_BITBIT_E(shift);
        གལ་སྲིད (!loShift) ལོག lop;
        AstNodeExpr* const hip = newAstWordSelClone(lhsp, othword - 1);
        const uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        AstNodeExpr* const newp
            = new AstOr{fl,  //
                        new AstShiftR{fl, hip, new AstConst{fl, hiShift}, VL_EDATASIZE},
                        new AstShiftL{fl, lop, new AstConst{fl, loShift}, VL_EDATASIZE}};
        ལོག V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    static AstNodeExpr* newWordIndex(AstNodeExpr* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        FileLine* const flp = lsbp->fileline();
        གལ་སྲིད (const AstConst* constp = VN_CAST(lsbp, Const)) {
            ལོག new AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        གལ་སྲིད (lsbp->backp()) lsbp = lsbp->cloneTreePure(false);
        AstNodeExpr* const wwl2p = new AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        AstNodeExpr* indexp = new AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        གལ་སྲིད (wordOffset) indexp = new AstAdd{flp, new AstConst{flp, wordOffset}, indexp};
        ལོག indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    static AstNodeExpr* newWordSelWord(FileLine* flp, AstNodeExpr* fromp, AstNodeExpr* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        གལ་སྲིད (const AstConst* const constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        གལ་སྲིད (wordOffset >= static_cast<uint32_t>(fromp->widthWords())) {
            ལོག new AstConst{flp, AstConst::SizedEData{}, 0};
        }

        གལ་སྲིད (fromp->backp()) fromp = fromp->cloneTreePure(false);

        // If indexp was constant, just use it
        གལ་སྲིད (!indexp) ལོག new AstWordSel{flp, fromp, new AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        གལ་སྲིད (indexp->backp()) indexp = indexp->cloneTreePure(false);
        གལ་སྲིད (wordOffset) indexp = new AstAdd{flp, new AstConst{flp, wordOffset}, indexp};
        ལོག new AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    static AstNodeExpr* newWordSelBit(FileLine* flp, AstNodeExpr* fromp, AstNodeExpr* lsbp,
                                      uint32_t wordOffset = 0) {
        AstNodeExpr* const indexp = newWordIndex(lsbp, wordOffset);
        AstNodeExpr* const wordSelp = newWordSelWord(flp, fromp, indexp);
        གལ་སྲིད (!indexp->backp()) VL_DO_DANGLING(indexp->deleteTree(), indexp);
        ལོག wordSelp;
    }

    static AstNodeExpr* newSelBitBit(AstNodeExpr* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        FileLine* const fl = lsbp->fileline();
        གལ་སྲིད (VN_IS(lsbp, Const)) {
            ལོག new AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } དེ་མིན {
            ལོག new AstAnd{fl, new AstConst{fl, VL_EDATASIZE - 1}, lsbp->cloneTreePure(true)};
        }
    }

    //====================

    bool expandWide(AstNodeAssign* nodep, AstConst* rhsp) {
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        གལ་སྲིད (rhsp->num().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        FileLine* const fl = nodep->fileline();
        དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w,
                          new AstConst{fl, AstConst::SizedEData{}, rhsp->num().edataWord(w)});
        }
        ལོག true;
    }
    //-------- Uniops
    bool expandWide(AstNodeAssign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        གལ་སྲིད (rhsp->varp()->constPoolEntry()) ལོག false;
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ལོག true;
    }
    bool expandWide(AstNodeAssign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ལོག true;
    }
    bool expandWide(AstNodeAssign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        FileLine* const fl = rhsp->fileline();
        དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w, new AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        ལོག true;
    }
    //-------- Biops
    bool expandWide(AstNodeAssign* nodep, AstAnd* rhsp) {
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        FileLine* const fl = nodep->fileline();
        དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w,
                          new AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ལོག true;
    }
    bool expandWide(AstNodeAssign* nodep, AstOr* rhsp) {
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        FileLine* const fl = nodep->fileline();
        དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w,
                          new AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ལོག true;
    }
    bool expandWide(AstNodeAssign* nodep, AstXor* rhsp) {
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        FileLine* const fl = nodep->fileline();
        དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w,
                          new AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ལོག true;
    }
    bool expandWideShift(AstNodeAssign* nodep, AstNodeBiop* rhsp, bool isLeftShift) {
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        གལ་སྲིད (const AstConst* const rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            const uint32_t shiftBits = rhsConstp->toUInt();
            གལ་སྲིད (VL_BITBIT_E(shiftBits) == 0) {
                const ཧྲིལ widthWords = nodep->widthWords();
                const ཧྲིལ shiftWords = std::min<ཧྲིལ>(VL_BITWORD_E(shiftBits), widthWords);
                FileLine* const flp = rhsp->fileline();
                གལ་སྲིད (isLeftShift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    དོན་ལུ (ཧྲིལ w = 0; w < shiftWords; ++w) {
                        addWordAssign(nodep, w, new AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    དོན་ལུ (ཧྲིལ w = shiftWords; w < widthWords; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shiftWords));
                    }
                } དེ་མིན {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    དོན་ལུ (ཧྲིལ w = 0; w < widthWords - shiftWords; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shiftWords));
                    }
                    // High words of the result are zero
                    དོན་ལུ (ཧྲིལ w = widthWords - shiftWords; w < widthWords; ++w) {
                        addWordAssign(nodep, w, new AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                ལོག true;
            }
        }

        ལོག false;
    }

    //-------- Triops
    bool expandWide(AstNodeAssign* nodep, AstCond* rhsp) {
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        FileLine* const fl = nodep->fileline();
        དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w,
                          new AstCond{fl, rhsp->condp()->cloneTreePure(true),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        ལོག true;
    }

    // VISITORS
    void visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    void visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        const VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        གལ་སྲིད (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            AstNode* const editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    void visit(AstExtend* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        iterateChildren(nodep);
        གལ་སྲིད (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } དེ་མིན {
            གལ་སྲིད (isImpure(nodep)) ལོག;
            AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* newp = lhsp;
            གལ་སྲིད (nodep->isQuad()) {
                གལ་སྲིད (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } དེ་མིན གལ་སྲིད (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } དེ་མིན {
                    newp = new AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } དེ་མིན {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    bool expandWide(AstNodeAssign* nodep, AstExtend* rhsp) {
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        AstNodeExpr* const rlhsp = rhsp->lhsp();
        དོན་ལུ (ཧྲིལ w = 0; w < rlhsp->widthWords(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        དོན་ལུ (ཧྲིལ w = rlhsp->widthWords(); w < nodep->widthWords(); ++w) {
            addWordAssign(nodep, w, new AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        ལོག true;
    }

    void visit(AstSel* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        iterateChildren(nodep);

        const uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<ཧྲིལ>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        གལ་སྲིད (AstNodeAssign* const assignp = VN_CAST(nodep->backp(), NodeAssign)) {
            གལ་སྲིད (nodep == assignp->lhsp()) ལོག;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        གལ་སྲིད (nodep->isWide()) ལོག;

        // Skip Sel from stream or queue
        གལ་སྲིད (VN_IS(nodep->fromp()->dtypep(), StreamDType)) ལོག;
        གལ་སྲིད (VN_IS(nodep->fromp()->dtypep(), QueueDType)) ལོག;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        གལ་སྲིད (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            གལ་སྲིད (isImpure(nodep)) ལོག;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            FileLine* const nfl = nodep->fileline();
            FileLine* const lfl = nodep->lsbp()->fileline();
            FileLine* const ffl = nodep->fromp()->fileline();
            AstNodeExpr* const fromp = nodep->fromp();

            // To extract an up-to-64-bit value from a wide source, up to 3 words might be needed
            // from the source operand. Each word might need to be shifted by a different amount.
            // All word indices and shift amounts are derived from the LSB expression, so it might
            // be duplicated several times. The problem arises when the LSB expression itself is
            // a similar select, which results in an exponential size expansion.
            //
            // To avoid this, if the required expressions derived from the LSB expression are
            // large, and the LSB expression is required multiple times for the expansion, then
            // the LSB expression is evaluated into a temporary variable. This bounds the size
            // of the expansion on each Sel instance, hence the total expansion of nested Sels
            // is bounded linearly with the number of nested Sels (instead of exponentially).

            // Return simplified 'exprp' if it simplifies to a small tree, otherwise nullptr
            const auto tryFold = [&](AstNodeExpr* exprp) -> AstNodeExpr* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                static constexpr ཧྲིལ EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                གལ་སྲིད (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) ལོག exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->deleteTree(), exprp);
                ལོག nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            const auto wordIdx = [&](uint32_t offset) -> AstNodeExpr* {
                AstNodeExpr* const msbp = new AstAdd{lfl, new AstConst{lfl, offset},
                                                     nodep->lsbp()->cloneTreePure(false)};
                AstNodeExpr* const idxp = newWordIndex(msbp);
                གལ་སྲིད (!msbp->backp()) VL_DO_DANGLING(msbp->deleteTree(), msbp);
                ལོག idxp;
            };

            // Bit index of the selected LSB within its word
            AstNodeExpr* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            const bool aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            const bool mNeeded = width > 1 && !aligned;
            const bool hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            const uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            const uint32_t hMsbOffset = width - 1;
            AstNodeExpr* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            AstNodeExpr* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            AstNodeExpr* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            const ཧྲིལ nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            གལ་སྲིད (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                AstNodeExpr* const lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* const tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(new AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            གལ་སྲིད (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            གལ་སྲིད (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            གལ་སྲིད (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            གལ་སྲིད (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            const auto wordSel = [&](AstNodeExpr* idxp) -> AstNodeExpr* {
                AstNodeExpr* const clonep = idxp->cloneTreePure(false);
                AstNodeExpr* const wordp = newWordSelWord(ffl, fromp, clonep);
                གལ་སྲིད (!clonep->backp()) VL_DO_DANGLING(clonep->deleteTree(), clonep);
                ལོག wordp;
            };

            // Construct term containing the bits of the low word - always needed
            AstNodeExpr* const lTermp = [&]() -> AstNodeExpr* {
                AstNodeExpr* lWordp = wordSel(lIdxp);
                གལ་སྲིད (nodep->isQuad()) lWordp = new AstCCast{nfl, lWordp, nodep};
                ལོག new AstShiftR{nfl, lWordp, lBitp->cloneTreePure(false), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            AstNodeExpr* const mTermp = [&]() -> AstNodeExpr* {
                གལ་སྲིད (!mNeeded) ལོག nullptr;

                AstNodeExpr* mWordp = wordSel(mIdxp);
                གལ་སྲིད (nodep->isQuad()) mWordp = new AstCCast{nfl, mWordp, nodep};
                AstNodeExpr* const mShiftp = new AstSub{lfl, new AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->cloneTreePure(false)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                const V3Number zero{nodep, longOrQuadWidth(nodep)};
                ལོག new AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    new AstEq{nfl, new AstConst{nfl, 0}, lBitp->cloneTreePure(false)},
                    // 0 :
                    new AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    new AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            AstNodeExpr* const hTermp = [&]() -> AstNodeExpr* {
                གལ་སྲིད (!hNeeded) ལོག nullptr;

                AstNodeExpr* hWordp = wordSel(hIdxp);
                hWordp = new AstCCast{nfl, hWordp, nodep};
                AstNodeExpr* const hShiftp = new AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    new AstEq{nfl, new AstConst{nfl, 0}, lBitp->cloneTreePure(false)},
                    // VL_EDATASIZE :
                    new AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    new AstSub{lfl, new AstConst{lfl, 64}, lBitp->cloneTreePure(false)}};
                ལོག new AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->deleteTree(), lBitp);
            VL_DO_DANGLING(lIdxp->deleteTree(), lIdxp);
            གལ་སྲིད (mIdxp) VL_DO_DANGLING(mIdxp->deleteTree(), mIdxp);
            གལ་སྲིད (hIdxp) VL_DO_DANGLING(hIdxp->deleteTree(), hIdxp);

            // Or reduce the terms
            AstNodeExpr* newp = lTermp;
            གལ་སྲིད (mTermp) newp = new AstOr{nfl, mTermp, newp};
            གལ་སྲིད (hTermp) newp = new AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } དེ་མིན {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            FileLine* const fl = nodep->fileline();
            AstNodeExpr* fromp = nodep->fromp()->unlinkFrBack();
            AstNodeExpr* const lsbp = nodep->lsbp()->unlinkFrBack();
            གལ་སྲིད (nodep->isQuad() && !fromp->isQuad()) fromp = new AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            AstNodeExpr* newp = new AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            གལ་སྲིད (!nodep->isQuad() && fromp->isQuad()) newp = new AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    bool expandWide(AstNodeAssign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        གལ་སྲིད (!doExpandWide(nodep)) ལོག false;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        གལ་སྲིད (const AstConst* const lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            const uint32_t lsb = lsbConstp->toUInt();
            གལ་སྲིད (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                const uint32_t word = VL_BITWORD_E(lsb);
                དོན་ལུ (ཧྲིལ w = 0; w < nodep->widthWords(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                ལོག true;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        FileLine* const flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        AstNodeExpr* const wordLsbp = rhsp->lsbp()->cloneTreePure(false);
        AstNodeExpr* wordIdxp = newWordIndex(wordLsbp);
        གལ་སྲིད (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->deleteTree(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        གལ་སྲིད (!VN_IS(wordIdxp, Const)) {
            AstVar* const tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = new AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        AstNodeExpr* loShftp = nullptr;
        AstNodeExpr* hiShftp = nullptr;
        AstNodeExpr* hiMaskp = nullptr;
        གལ་སྲིད (const AstConst* const lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            const uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = new AstConst{flp, bitOffset};
            hiShftp = new AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } དེ་མིན {
            // Compute Low word shift amount: bottom bits of lsb index
            AstNodeExpr* const lsbp = rhsp->lsbp()->cloneTreePure(false);
            loShftp = new AstAnd{flp, new AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* const loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = new AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            AstNodeExpr* const zerop = new AstConst{flp, 0};
            AstNodeExpr* alignedp = new AstEq{flp, zerop, loShftp->cloneTreePure(false)};
            AstVar* const alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = new AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            AstNodeExpr* const edsp = new AstConst{flp, VL_EDATASIZE};
            AstNodeExpr* const subp = new AstSub{flp, edsp, loShftp->cloneTreePure(false)};
            hiShftp = new AstCond{flp, alignedp, new AstConst{flp, 0}, subp};
            AstVar* const hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = new AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = new AstCond{flp, alignedp->cloneTreePure(false),
                                  new AstConst{flp, AstConst::SizedEData{}, 0},
                                  new AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* const maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = new AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        AstNodeExpr* const fromp = rhsp->fromp();
        const ཧྲིལ selWords = nodep->widthWords();
        དོན་ལུ (ཧྲིལ w = 0; w < selWords; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            AstNodeExpr* const loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            AstNodeExpr* const loShftClonep = loShftp->cloneTreePure(false);
            AstNodeExpr* const lop = new AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            AstNodeExpr* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            གལ་སྲིད (w == selWords - 1) {
                const uint32_t max = fromp->widthWords() - w - 1;
                AstNodeExpr* const maxp = new AstConst{flp, max};
                AstNodeExpr* const condp = new AstGte{flp, wordIdxp->cloneTreePure(false), maxp};
                AstNodeExpr* const zerop = new AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = new AstCond{flp, condp, zerop, hiWordp};
            }
            AstNodeExpr* const hiShftClonep = hiShftp->cloneTreePure(false);
            AstNodeExpr* const hiPartp = new AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            AstNodeExpr* const hip
                = hiMaskp ? new AstAnd{flp, hiPartp, hiMaskp->cloneTreePure(false)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, new AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        གལ་སྲིད (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->deleteTree(), wordIdxp);
        གལ་སྲིད (!loShftp->backp()) VL_DO_DANGLING(loShftp->deleteTree(), loShftp);
        གལ་སྲིད (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->deleteTree(), hiShftp);
        གལ་སྲིད (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->deleteTree(), hiMaskp);

        ལོག true;
    }

    bool expandLhs(AstNodeAssign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        གལ་སྲིད (isImpure(nodep)) ལོག false;
        FileLine* const nfl = nodep->fileline();
        FileLine* const lfl = lhsp->fileline();
        const bool destwide = lhsp->fromp()->isWide();
        const bool ones = nodep->rhsp()->isAllOnesV();
        གལ་སྲིད (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            AstNodeExpr* rhsp = nodep->rhsp()->unlinkFrBack();
            AstNodeExpr* const destp = lhsp->fromp()->unlinkFrBack();
            const ཧྲིལ lsb = lhsp->lsbConst();
            const ཧྲིལ msb = lhsp->msbConst();
            V3Number maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3Number maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            གལ་སྲིད (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                དོན་ལུ (ཧྲིལ w = 0; w < destp->widthWords(); ++w) {
                    གལ་སྲིད (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        AstNodeExpr* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        གལ་སྲིད (!ones) {
                            oldvalp = new AstAnd{
                                lfl,
                                new AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        AstNodeExpr* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        གལ་སྲིད (w == destp->widthWords() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3Number cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = new AstAnd{lfl, newp, new AstConst{lfl, cleanmask}};
                        }
                        AstNodeExpr* const orp
                            = V3Const::constifyEditCpp(new AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->deleteTree(), rhsp);
                VL_DO_DANGLING(destp->deleteTree(), destp);
            } དེ་མིན {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                གལ་སྲིད (destp->isQuad() && !rhsp->isQuad()) rhsp = new AstCCast{nfl, rhsp, nodep};
                AstNodeExpr* oldvalp = destp->cloneTreePure(true);
                fixCloneLvalue(oldvalp);
                གལ་སྲིད (!ones) oldvalp = new AstAnd{lfl, new AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3Number cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                AstNodeExpr* const shifted = new AstShiftL{
                    lfl, rhsp, new AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                AstNodeExpr* const cleaned
                    = new AstAnd{lfl, shifted, new AstConst{lfl, cleanmask}};
                AstNodeExpr* const orp
                    = V3Const::constifyEditCpp(new AstOr{lfl, oldvalp, cleaned});
                insertBefore(nodep, new AstAssign{nfl, destp, orp});
            }
            ལོག true;
        } དེ་མིན {  // non-const select offset
            གལ་སྲིད (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                AstNodeExpr* const rhsp = nodep->rhsp()->unlinkFrBack();
                AstNodeExpr* const destp = lhsp->fromp()->unlinkFrBack();
                AstNodeExpr* oldvalp
                    = newWordSelBit(lfl, destp->cloneTreePure(false), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                གལ་སྲིད (!ones) {
                    oldvalp = new AstAnd{
                        lfl,
                        new AstNot{
                            lfl, new AstShiftL{lfl, new AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                AstNodeExpr* const shiftp = new AstAnd{nfl, lhsp->lsbp()->cloneTreePure(true),
                                                       new AstConst{nfl, VL_EDATASIZE - 1}};
                AstNode* const newp = new AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    new AstOr{lfl, oldvalp, new AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                insertBefore(nodep, newp);
                ལོག true;
            } དེ་མིན གལ་སྲིད (destwide) {
                UINFO(8, "    ASSIGNSEL(varlsb,wide) -- NoOp -- " << nodep);
                //   For wide destp, we can either form a equation for every destination word,
                // with the appropriate long equation of if it's being written or not.
                //   Or, we can use a LHS variable arraysel with
                //   non-constant index to set the vector.
                // Doing the variable arraysel is better for globals and large arrays,
                // doing every word is better for temporaries and if we're setting most words
                // since it may result in better substitution optimizations later.
                //   This results in so much code, we're better off leaving a function call.
                // Reconsider if we get subexpression elimination.
                ལོག false;
            } དེ་མིན {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                AstNodeExpr* rhsp = nodep->rhsp()->unlinkFrBack();
                AstNodeExpr* const destp = lhsp->fromp()->unlinkFrBack();
                AstNodeExpr* oldvalp = destp->cloneTreePure(true);
                fixCloneLvalue(oldvalp);

                V3Number maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                གལ་སྲིད (destp->isQuad() && !rhsp->isQuad()) rhsp = new AstCCast{nfl, rhsp, nodep};
                གལ་སྲིད (!ones) {
                    oldvalp = new AstAnd{
                        lfl,
                        new AstNot{lfl, new AstShiftL{lfl, new AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->cloneTreePure(true),
                                                      destp->width()}},
                        oldvalp};
                }
                AstNodeExpr* newp
                    = new AstShiftL{lfl, rhsp, lhsp->lsbp()->cloneTreePure(true), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                གལ་སྲིད (destp->widthMin() != 64) {
                    V3Number cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = new AstAnd{lfl, newp, new AstConst{lfl, cleanmask}};
                }

                insertBefore(nodep, new AstAssign{nfl, destp, new AstOr{lfl, oldvalp, newp}});
                ལོག true;
            }
        }
    }

    void visit(AstConcat* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        iterateChildren(nodep);
        གལ་སྲིད (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } དེ་མིན {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            FileLine* const fl = nodep->fileline();
            AstNodeExpr* lhsp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* rhsp = nodep->rhsp()->unlinkFrBack();
            const uint32_t rhsshift = rhsp->widthMin();
            གལ་སྲིད (nodep->isQuad() && !lhsp->isQuad()) lhsp = new AstCCast{fl, lhsp, nodep};
            གལ་སྲིད (nodep->isQuad() && !rhsp->isQuad()) rhsp = new AstCCast{fl, rhsp, nodep};
            AstNodeExpr* const newp = new AstOr{
                fl, new AstShiftL{fl, lhsp, new AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    bool expandWide(AstNodeAssign* nodep, AstConcat* rhsp) {
        གལ་སྲིད (!doExpandWide(rhsp)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        FileLine* const fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        const ཧྲིལ rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        དོན་ལུ (ཧྲིལ w = 0; w < rhsp->widthWords(); ++w) {
            addWordAssign(nodep, w,
                          new AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ལོག true;
    }

    void visit(AstReplicate* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        iterateChildren(nodep);
        གལ་སྲིད (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } དེ་མིན {
            གལ་སྲིད (isImpure(nodep)) ལོག;
            FileLine* const fl = nodep->fileline();
            AstNodeExpr* lhsp = nodep->srcp()->unlinkFrBack();
            AstNodeExpr* newp;
            const ཧྲིལ lhswidth = lhsp->widthMin();
            གལ་སྲིད (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = new AstNegate{fl, lhsp};
            } དེ་མིན {
                didPattern(VExpandPattern::REPLICATE, nodep);
                const AstConst* const constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                const uint32_t times = constp->toUInt();
                གལ་སྲིད (nodep->isQuad() && !lhsp->isQuad()) lhsp = new AstCCast{fl, lhsp, nodep};
                newp = lhsp->cloneTreePure(true);
                དོན་ལུ (unsigned repnum = 1; repnum < times; repnum++) {
                    const ཧྲིལ rhsshift = repnum * lhswidth;
                    newp = new AstOr{
                        fl,
                        new AstShiftL{fl, lhsp->cloneTreePure(true),
                                      new AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->deleteTree(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    bool expandWide(AstNodeAssign* nodep, AstReplicate* rhsp) {
        གལ་སྲིད (!doExpandWide(rhsp)) ལོག false;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        FileLine* const fl = nodep->fileline();
        AstNodeExpr* const lhsp = rhsp->srcp();
        const ཧྲིལ lhswidth = lhsp->widthMin();
        const AstConst* const constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        const uint32_t times = constp->toUInt();
        དོན་ལུ (ཧྲིལ w = 0; w < rhsp->widthWords(); ++w) {
            AstNodeExpr* newp;
            གལ་སྲིད (lhswidth == 1) {
                newp = new AstNegate{fl, lhsp->cloneTreePure(true)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } དེ་མིན {
                newp = newAstWordSelClone(lhsp, w);
                FileLine* const rfl = rhsp->fileline();
                དོན་ལུ (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = new AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        ལོག true;
    }

    void visitEqNeq(AstNodeBiop* nodep) {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        iterateChildren(nodep);
        གལ་སྲིད (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            གལ་སྲིད (isImpure(nodep)) ལོག;
            གལ་སྲིད (!doExpandWide(nodep->lhsp())) ལོག;
            གལ་སྲིད (!doExpandWide(nodep->rhsp())) ལོག;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            FileLine* const fl = nodep->fileline();
            AstNodeExpr* newp = nullptr;
            དོན་ལུ (ཧྲིལ w = 0; w < nodep->lhsp()->widthWords(); ++w) {
                AstNodeExpr* const eqp = new AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? new AstOr{fl, newp, eqp} : eqp;
            }
            གལ་སྲིད (VN_IS(nodep, Neq)) {
                newp = new AstNeq{fl, new AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } དེ་མིན {
                newp = new AstEq{fl, new AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    void visit(AstEq* nodep) override { visitEqNeq(nodep); }
    void visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    void visit(AstRedOr* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        iterateChildren(nodep);
        FileLine* const fl = nodep->fileline();
        གལ་སྲིད (nodep->lhsp()->isWide()) {
            གལ་སྲིད (isImpure(nodep)) ལོག;
            གལ་སྲིད (!doExpandWide(nodep->lhsp())) ལོག;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            AstNodeExpr* newp = nullptr;
            དོན་ལུ (ཧྲིལ w = 0; w < nodep->lhsp()->widthWords(); ++w) {
                AstNodeExpr* const eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? new AstOr{fl, newp, eqp} : eqp;
            }
            newp = new AstNeq{fl, new AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } དེ་མིན {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* const newp = new AstNeq{
                fl, new AstConst{fl, AstConst::WidthedValue{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    void visit(AstRedAnd* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        iterateChildren(nodep);
        FileLine* const fl = nodep->fileline();
        གལ་སྲིད (nodep->lhsp()->isWide()) {
            གལ་སྲིད (isImpure(nodep)) ལོག;
            གལ་སྲིད (!doExpandWide(nodep->lhsp())) ལོག;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            AstNodeExpr* newp = nullptr;
            དོན་ལུ (ཧྲིལ w = 0; w < nodep->lhsp()->widthWords(); ++w) {
                AstNodeExpr* eqp = newAstWordSelClone(nodep->lhsp(), w);
                གལ་སྲིད (w == nodep->lhsp()->widthWords() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = new AstOr{fl, new AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? new AstAnd{fl, newp, eqp} : eqp;
            }
            newp = new AstEq{fl, new AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } དེ་མིན {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* const newp = new AstEq{fl, new AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    void visit(AstRedXor* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        iterateChildren(nodep);
        གལ་སྲིད (nodep->lhsp()->isWide()) {
            གལ་སྲིད (isImpure(nodep)) ལོག;
            གལ་སྲིད (!doExpandWide(nodep->lhsp())) ལོག;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            FileLine* const fl = nodep->fileline();
            AstNodeExpr* newp = nullptr;
            དོན་ལུ (ཧྲིལ w = 0; w < nodep->lhsp()->widthWords(); ++w) {
                AstNodeExpr* const eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? new AstXor{fl, newp, eqp} : eqp;
            }
            newp = new AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    void visit(AstNodeStmt* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstNodeAssign* nodep) override {
        གལ་སྲིད (nodep->user1SetOnce()) ལོག;  // Process once
        གལ་སྲིད (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            ལོག;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        bool did = false;
        གལ་སྲིད (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            གལ་སྲིད (AstConst* const rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstVarRef* const rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstSel* const rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstArraySel* const rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstConcat* const rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstExtend* const rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstReplicate* const rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstAnd* const rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstOr* const rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstNot* const rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstXor* const rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } དེ་མིན གལ་སྲིད (AstShiftL* const rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ true);
            } དེ་མིན གལ་སྲིད (AstShiftR* const rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ false);
            } དེ་མིན གལ་སྲིད (AstCond* const rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } དེ་མིན གལ་སྲིད (AstSel* const lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        གལ་སྲིད (did) VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }

    //--------------------
    void visit(AstVar*) override {}  // Don't hit varrefs under vars
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_statWideWords);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        const auto emitStat = [this](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned var_na c) {  //
                ལོག c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::addStat("Optimizations, Expand, pattern " + str, m_statPatterns[pattern]);
        };
#define EXPAND_PATTERN_EMIT_STAT(id) emitStat(VExpandPattern::id);
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_EMIT_STAT)
#undef EXPAND_PATTERN_EMIT_STAT
    }
};

//######################################################################
// Expand class functions

void V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        const ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("expand", 0, dumpTreeEitherLevel() >= 3);
}
