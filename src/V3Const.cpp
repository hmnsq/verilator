// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Constant folding
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
// CONST TRANSFORMATIONS:
//   Call on one node for PARAM values, or netlist for overall constant folding:
//      Bottom up traversal:
//          Attempt to convert operands to constants
//          If operands are constant, replace this node with constant.
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Simulate.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#define ವೃಕ್ಷ_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

ಸ್ಥಿರ ತಾರ್ಕಿಕ isConst(ಧ್ರುವ Ast_ಗಂಟು* nodep, uint64_t v) {
    ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const);
    ಪ್ರತ್ಯಾಗಮ constp && constp->toUQuad() == v;
}

template <typename T>
ಸ್ಥಿರ typename std::enable_if<std::is_integral<T>::ಮೌಲ್ಯ, ತಾರ್ಕಿಕ>::type isPow2(T val) {
    ಪ್ರತ್ಯಾಗಮ (val & (val - 1)) == 0;
}

ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ ಎಣಿಕೆ_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#ಯದಿ defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    ಪ್ರತ್ಯಾಗಮ __builtin_ctzll(val);
#ಅನ್ಯಥಾ
    ಪೂರ್ಣಾಙ್ಕ bit = 0;
    val = ~val;
    ಯಾವತ್ (val & 1) {
        ++bit;
        val >>= 1;
    }
    ಪ್ರತ್ಯಾಗಮ bit;
#endif
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
ವರ್ಗ VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4InUse> m_inuser4p;
    size_t m_nextId = 0;  // Id allocated most recently

ಸಾರ್ವಜನಿಕ:
    // Return the unique id of the given node, allocating a new one if it has none yet
    size_t operator()(Ast_ಗಂಟು* nodep) {
        ಯದಿ (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        ಯದಿ (!nodep->user4p()) nodep->user4p(reinterpret_cast<ಶೂನ್ಯ*>(++m_nextId));
        ಪ್ರತ್ಯಾಗಮ reinterpret_cast<size_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
ವರ್ಗ Const_Bit_Op_ವೃಕ್ಷ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, unsigned, ತಾರ್ಕಿಕ>;

    ವರ್ಗ LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        ತಾರ್ಕಿಕ m_polarity = ಸತ್ಯ;  // Invert result due to NOT
        ಪೂರ್ಣಾಙ್ಕ m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        ಪೂರ್ಣಾಙ್ಕ m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        ಪೂರ್ಣಾಙ್ಕ m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = nullptr;  // Leaf's variable reference
        ಧ್ರುವ AstConst* m_constp = nullptr;  // Leaf's constant

    ಸಾರ್ವಜನಿಕ:
        // CONSTRUCTORS
        LeafInfo() = ಸ್ವಭಾವ;
        LeafInfo(ಧ್ರುವ LeafInfo& other) = ಸ್ವಭಾವ;
        LeafInfo& operator=(ಧ್ರುವ LeafInfo& other) = ಸ್ವಭಾವ;
        LeafInfo(LeafInfo&& other) = ಸ್ವಭಾವ;
        LeafInfo& operator=(LeafInfo&& other) = ಸ್ವಭಾವ;
        explicit LeafInfo(ಪೂರ್ಣಾಙ್ಕ lsb)
            : m_lsb{lsb} {}

        // METHODS
        ಶೂನ್ಯ setLeaf(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->widthMin() - 1;
        }
        ಶೂನ್ಯ setLeaf(ಧ್ರುವ AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->widthMin() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        ಶೂನ್ಯ updateBitRange(ಪೂರ್ಣಾಙ್ಕ newLsb, ಪೂರ್ಣಾಙ್ಕ newMsb) {
            ಯದಿ ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(ಸತ್ಯ);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        ಶೂನ್ಯ updateBitRange(ಧ್ರುವ AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->width() - 1));
        }
        ಶೂನ್ಯ updateBitRange(ಧ್ರುವ AstShiftR* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->toUInt(), m_msb);
        }
        ಶೂನ್ಯ limitBitRangeToLsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        ಪೂರ್ಣಾಙ್ಕ wordIdx() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_wordIdx; }
        ಶೂನ್ಯ wordIdx(ಪೂರ್ಣಾಙ್ಕ i) { m_wordIdx = i; }
        ತಾರ್ಕಿಕ polarity() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_polarity; }
        ಶೂನ್ಯ polarity(ತಾರ್ಕಿಕ p) { m_polarity = p; }

        AstVarRef* refp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_refp; }
        ಧ್ರುವ AstConst* constp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constp; }
        ತಾರ್ಕಿಕ missingWordSel() ಧ್ರುವ {
            // When V3Expand is skipped, WordSel is not inserted.
            ಪ್ರತ್ಯಾಗಮ m_refp->isWide() && m_wordIdx == -1;
        }
        ಪೂರ್ಣಾಙ್ಕ lsb() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lsb; }

        ಪೂರ್ಣಾಙ್ಕ msb() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ std::min(m_msb, varWidth() - 1); }
        ಪೂರ್ಣಾಙ್ಕ varWidth() ಧ್ರುವ {
            UASSERT(m_refp, "m_refp should be set");
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = m_refp->varp()->widthMin();
            ಯದಿ (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                ಪ್ರತ್ಯಾಗಮ width;
            } ಅನ್ಯಥಾ {
                ಯದಿ (missingWordSel()) ಪ್ರತ್ಯಾಗಮ width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                ಪ್ರತ್ಯಾಗಮ m_wordIdx == m_refp->width_ಪದಗಳು() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    ಸಂರಚನಾ BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        ತಾರ್ಕಿಕ m_polarity = ಅಸತ್ಯ;  // Polarity the bit must have to match
        ಪೂರ್ಣಾಙ್ಕ m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(ಧ್ರುವ LeafInfo& info, ತಾರ್ಕಿಕ pol, ಪೂರ್ಣಾಙ್ಕ bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = ಸ್ವಭಾವ;
    };

    ಸಂರಚನಾ Frozen_ಗಂಟು_Info final {  // Context when a frozen node is found
        ತಾರ್ಕಿಕ m_polarity;  // Polarity the frozen node must match
        ಪೂರ್ಣಾಙ್ಕ m_lsb;  // LSB position of the frozen node
        ತಾರ್ಕಿಕ operator<(ಧ್ರುವ Frozen_ಗಂಟು_Info& other) ಧ್ರುವ {
            ಯದಿ (m_lsb != other.m_lsb) ಪ್ರತ್ಯಾಗಮ m_lsb < other.m_lsb;
            ಪ್ರತ್ಯಾಗಮ m_polarity < other.m_polarity;
        }
    };

    ವರ್ಗ Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_ವೃಕ್ಷ_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ಧ್ರುವ size_t m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ಧ್ರುವ size_t m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ಧ್ರುವ unsigned m_ops;  // Saved m_visitor.m_ops to restore
        ಧ್ರುವ ತಾರ್ಕಿಕ m_polarity;  // Saved m_visitor.m_polarity to restore
        ತಾರ್ಕಿಕ m_restore = ಸತ್ಯ;  // Whether the destructor still needs to restore state

    ಸಾರ್ವಜನಿಕ:
        explicit Restorer(Const_Bit_Op_ವೃಕ್ಷ_Visitor& visitor)
            : m_visitor{visitor}
            , m_polaritiesSize{visitor.m_bitPolarities.size()}
            , m_frozenSize{visitor.m_frozenNodes.size()}
            , m_ops{visitor.m_ops}
            , m_polarity{visitor.m_polarity} {}
        ~Restorer() {
            UASSERT(m_visitor.m_bitPolarities.size() >= m_polaritiesSize,
                    "m_bitPolarities must grow monotonically");
            UASSERT(m_visitor.m_frozenNodes.size() >= m_frozenSize,
                    "m_frozenNodes must grow monotonically");
            ಯದಿ (m_restore) restoreNow();
        }
        ಶೂನ್ಯ disableRestore() { m_restore = ಅಸತ್ಯ; }
        ಶೂನ್ಯ restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = ಅಸತ್ಯ;
        }
    };
    // Collect information for each Variable to transform as below
    ವರ್ಗ VarInfo final {
        // MEMBERS
        ಪೂರ್ಣಾಙ್ಕ m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        ಧ್ರುವ Const_Bit_Op_ವೃಕ್ಷ_Visitor* ಧ್ರುವ
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ಧ್ರುವ m_refp;  // Points the variable that this VarInfo covers
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_width;  // Width of term this VarInfo refers to
        V3_ಸಂಖ್ಯೆ m_bitPolarity;  // Coefficient of each bit

    ಸಾರ್ವಜನಿಕ:
        // METHODS
        ತಾರ್ಕಿಕ hasConstResult() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        ತಾರ್ಕಿಕ getConstResult() ಧ್ರುವ {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            ಪ್ರತ್ಯಾಗಮ m_knownResult == 1;
        }
        ಧ್ರುವ AstVarRef* refp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_refp; }
        ತಾರ್ಕಿಕ sameVarAs(ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* otherp) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_refp->same_ಗಂಟು(otherp); }
        ಶೂನ್ಯ setPolarity(ತಾರ್ಕಿಕ compBit, ಪೂರ್ಣಾಙ್ಕ bit) {
            // Ignore if already determined a known reduction
            ಯದಿ (m_knownResult >= 0) ಪ್ರತ್ಯಾಗಮ;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            ಯದಿ (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } ಅನ್ಯಥಾ {  // The bit has already been marked with some polarity
                ಧ್ರುವ ತಾರ್ಕಿಕ sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                ಯದಿ (m_parentp->is_Xor_ವೃಕ್ಷ()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } ಅನ್ಯಥಾ {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    ಯದಿ (sameFlag) ಪ್ರತ್ಯಾಗಮ;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->is_And_ವೃಕ್ಷ() ? 0 : 1;
                    m_bitPolarity.setAllBitsX();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ResultTerm getResultTerm() ಧ್ರುವ {
            UASSERT_OBJ(!hasConstResult(), m_refp, "getTerm on reduction that yields constant");
            File_ಸಾಲು* ಧ್ರುವ fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = VN_CAST(m_refp->backp(), WordSel);
            ಯದಿ (!srcp) srcp = m_refp;
            srcp = srcp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);

            // Signed variables might have redundant sign bits that need masking.
            ಧ್ರುವ ತಾರ್ಕಿಕ hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3_ಸಂಖ್ಯೆ mask_ಅಂಕೆ{srcp, m_width, 0};
            mask_ಅಂಕೆ.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            ಧ್ರುವ uint64_t maskVal = mask_ಅಂಕೆ.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = srcp;  // The tree for this term
            unsigned ops = 0;  // Number of ops in this term
            ತಾರ್ಕಿಕ clean = ಅಸತ್ಯ;  // Whether the term is clean (has value 0 or 1)

            ಯದಿ (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bit = ಎಣಿಕೆ_Trailing_Zeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                ಯದಿ (bit != 0) {
                    resultp = ನೂತನ AstShiftR{fl, resultp,
                                            ನೂತನ AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                ಧ್ರುವ ತಾರ್ಕಿಕ negate = m_bitPolarity.bitIs0(bit);
                ಯದಿ (negate) {
                    resultp = ನೂತನ AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } ಅನ್ಯಥಾ {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ಧ್ರುವ ತಾರ್ಕಿಕ needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                ಯದಿ (needsMasking) {
                    resultp = ನೂತನ AstAnd{fl, ನೂತನ AstConst{fl, mask_ಅಂಕೆ}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                ಯದಿ (m_parentp->is_Xor_ವೃಕ್ಷ()) {
                    ಯದಿ (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthMin = mask_ಅಂಕೆ.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = ನೂತನ AstRedXor{fl, resultp};
                    ++ops;
                    clean = ಅಸತ್ಯ;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } ಅನ್ಯಥಾ ಯದಿ (m_parentp->is_And_ವೃಕ್ಷ()) {
                    V3_ಸಂಖ್ಯೆ comp_ಅಂಕೆ{srcp, m_width, 0};
                    comp_ಅಂಕೆ.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, comp_ಅಂಕೆ}, resultp};
                    ++ops;
                    clean = ಸತ್ಯ;
                } ಅನ್ಯಥಾ {  // Or
                    V3_ಸಂಖ್ಯೆ comp_ಅಂಕೆ{srcp, m_width, 0};
                    comp_ಅಂಕೆ.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_ಅಂಕೆ.opXor(V3_ಸಂಖ್ಯೆ{comp_ಅಂಕೆ}, mask_ಅಂಕೆ);
                    resultp = ನೂತನ AstNeq{fl, ನೂತನ AstConst{fl, comp_ಅಂಕೆ}, resultp};
                    ++ops;
                    clean = ಸತ್ಯ;
                }
            }

            ಪ್ರತ್ಯಾಗಮ ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_ವೃಕ್ಷ_Visitor* parent, AstVarRef* refp, ಪೂರ್ಣಾಙ್ಕ width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    ತಾರ್ಕಿಕ m_failed = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ m_polarity = ಸತ್ಯ;  // Flip when AstNot comes
    unsigned m_ops;  // Number of operations such as And, Or, Xor, Sel...
    ಪೂರ್ಣಾಙ್ಕ m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ m_rootp;  // Root of this AST subtree

    std::vector<std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Frozen_ಗಂಟು_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<size_t, ಪೂರ್ಣಾಙ್ಕ> m_baseIdxs;

    // METHODS

    ತಾರ್ಕಿಕ is_And_ವೃಕ್ಷ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_IS(m_rootp, And); }
    ತಾರ್ಕಿಕ is_Or_ವೃಕ್ಷ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_IS(m_rootp, Or); }
    ತಾರ್ಕಿಕ is_Xor_ವೃಕ್ಷ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    ಯದಿ (setFailed(cond, #cond, nodep, __LINE__)) ಪ್ರತ್ಯಾಗಮ

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(ಸತ್ಯ, reason, nodep, __LINE__)

    ತಾರ್ಕಿಕ setFailed(ತಾರ್ಕಿಕ fail, ಧ್ರುವ ವರ್ಣ* reason, ಧ್ರುವ Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ ಸಾಲು) {
        ಯದಿ (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << ಸಾಲು << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ m_failed;
    }
    ಶೂನ್ಯ incrOps(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ ಸಾಲು) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << ಸಾಲು);
    }
    VarInfo& getVarInfo(ಧ್ರುವ LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_ಗಂಟು* nodep = ref.refp()->varScopep();
        ಯದಿ (!nodep) nodep = ref.refp()->varp();  // Not scoped
        ಪೂರ್ಣಾಙ್ಕ& baseIdxr = m_baseIdxs[m_ids(nodep)];
        ಯದಿ (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_ಪದಗಳು
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->width_ಪದಗಳು() : 1;
            m_varInfos.resize(m_varInfos.size() + ಅಂಕೆ_ಪದಗಳು);
        }
        ಧ್ರುವ size_t idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        ಯದಿ (!varInfop) {
            varInfop = ನೂತನ VarInfo{ಏತತ್, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            ಯದಿ (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } ಅನ್ಯಥಾ {
            ಯದಿ (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        ಪ್ರತ್ಯಾಗಮ *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ expectConst) {
        ಯದಿ (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) ಪ್ರತ್ಯಾಗಮ LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        ತಾರ್ಕಿಕ ok = !m_failed;
        ಯದಿ (expectConst) {
            ok &= !info.refp() && info.constp();
        } ಅನ್ಯಥಾ {
            ok &= info.refp() && !info.constp();
        }
        ಪ್ರತ್ಯಾಗಮ ok ? info : LeafInfo{};
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    ಶೂನ್ಯ visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        ಯದಿ (m_leafp) m_leafp->updateBitRange(nodep);
    }
    ಶೂನ್ಯ visit(AstShiftR* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->toUInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->toUInt();
    }
    ಶೂನ್ಯ visit(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->widthMin() != 1, nodep);
        Ast_ಗಂಟು* lhsp = nodep->lhsp();
        AstCCast* ಧ್ರುವ castp = VN_CAST(lhsp, CCast);
        ಯದಿ (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_ವೃಕ್ಷ() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        ಯದಿ (!is_Xor_ವೃಕ್ಷ()) m_polarity = !m_polarity;
        ಯದಿ (m_leafp && castp) m_leafp->updateBitRange(castp);
        ಯದಿ (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    ಶೂನ್ಯ visit(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->toSInt());
        iterateConst(nodep->fromp());
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }

    ಶೂನ್ಯ visit(AstRedXor* nodep) override {
        Restorer restorer{*ಏತತ್};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_ಗಂಟು* lhsp = nodep->lhsp();
        ಧ್ರುವ AstCCast* ಧ್ರುವ castp = VN_CAST(lhsp, CCast);
        ಯದಿ (castp) lhsp = castp->lhsp();
        ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ಧ್ರುವ LeafInfo& mask = findLeaf(andp->lhsp(), ಸತ್ಯ);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), ಅಸತ್ಯ);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            ಯದಿ (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ಧ್ರುವ V3_ಸಂಖ್ಯೆ& mask_ಅಂಕೆ = mask.constp()->ಅಂಕೆ();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ maxBitIdx = std::min(ref.lsb() + mask_ಅಂಕೆ.width(), ref.msb() + 1);
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ maskIdx = bitIdx - ref.lsb();
                ಯದಿ (mask_ಅಂಕೆ.bitIs0(maskIdx)) ಅನುವರ್ತ;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, ಸತ್ಯ, bitIdx);
            }
        } ಅನ್ಯಥಾ {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, ಅಸತ್ಯ);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            ಯದಿ (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, ಸತ್ಯ, bitIdx);
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Biop* nodep) override {
        ಯದಿ (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*ಏತತ್};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            ಯದಿ (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } ಅನ್ಯಥಾ ಯದಿ (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_ವೃಕ್ಷ(), nodep);
            incrOps(nodep, __LINE__);
            ಪ್ರತಿ (ಧ್ರುವ ತಾರ್ಕಿಕ ಬಲ : {ಅಸತ್ಯ, ಸತ್ಯ}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*ಏತತ್};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* opp = ಬಲ ? nodep->rhsp() : nodep->lhsp();
                ಧ್ರುವ ತಾರ್ಕಿಕ origFailed = m_failed;
                iterateConst(opp);
                ಯದಿ (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    ಯದಿ (ಧ್ರುವ AstCCast* ಧ್ರುವ castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ಧ್ರುವ ತಾರ್ಕಿಕ pol = is_Xor_ವೃಕ್ಷ() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, Frozen_ಗಂಟು_Info{pol, m_lsb});
                    m_failed = origFailed;
                    ಅನುವರ್ತ;
                }
                restorer.disableRestore();  // Now all checks passed
                ಯದಿ (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    ಯದಿ (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, is_Xor_ವೃಕ್ಷ() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } ಅನ್ಯಥಾ ಯದಿ ((is_And_ವೃಕ್ಷ() && leafInfo.polarity())
                               || (is_Or_ವೃಕ್ಷ() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, ಸತ್ಯ, 0);
                        m_bitPolarities.emplace_back(leafInfo, ಅಸತ್ಯ, 0);
                    }
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ ((is_And_ವೃಕ್ಷ() && VN_IS(nodep, Eq)) || (is_Or_ವೃಕ್ಷ() && VN_IS(nodep, Neq))) {
            Restorer restorer{*ಏತತ್};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ಧ್ರುವ Ast_ಗಂಟು* lhsp = nodep->lhsp();
            ಯದಿ (ಧ್ರುವ AstCCast* ಧ್ರುವ castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ಧ್ರುವ V3_ಸಂಖ್ಯೆ& comp_ಅಂಕೆ = constp->ಅಂಕೆ();

            auto setPolarities = [ಏತತ್, &comp_ಅಂಕೆ](ಧ್ರುವ LeafInfo& ref, ಧ್ರುವ V3_ಸಂಖ್ಯೆ* maskp) {
                ಧ್ರುವ ತಾರ್ಕಿಕ maskFlip = is_And_ವೃಕ್ಷ() ^ ref.polarity();
                ಪೂರ್ಣಾಙ್ಕ constantWidth = comp_ಅಂಕೆ.width();
                ಯದಿ (maskp) constantWidth = std::max(constantWidth, maskp->width());
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ maskIdx = bitIdx - ref.lsb();
                    ಧ್ರುವ ತಾರ್ಕಿಕ mask0 = maskp && maskp->bitIs0(maskIdx);
                    ಧ್ರುವ ತಾರ್ಕಿಕ outOfRange = bitIdx > ref.msb();
                    ಯದಿ (mask0 || outOfRange) {  // RHS is 0
                        ಯದಿ (comp_ಅಂಕೆ.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, ಸತ್ಯ, 0);
                            m_bitPolarities.emplace_back(ref, ಅಸತ್ಯ, 0);
                            ಭಙ್ಗ;
                        } ಅನ್ಯಥಾ {  // This bitIdx is irrelevant
                            ಅನುವರ್ತ;
                        }
                    }
                    ಧ್ರುವ ತಾರ್ಕಿಕ polarity = comp_ಅಂಕೆ.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ಧ್ರುವ LeafInfo& mask = findLeaf(andp->lhsp(), ಸತ್ಯ);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                ಧ್ರುವ LeafInfo& ref = findLeaf(andp->rhsp(), ಅಸತ್ಯ);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& mask_ಅಂಕೆ = mask.constp()->ಅಂಕೆ();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &mask_ಅಂಕೆ);
            } ಅನ್ಯಥಾ {  // comp == v
                ಧ್ರುವ LeafInfo& ref = findLeaf(nodep->rhsp(), ಅಸತ್ಯ);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } ಅನ್ಯಥಾ {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_ವೃಕ್ಷ_Visitor(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, unsigned externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ತಳ್ಳು_back(nullptr);
        CONST_BITOP_RETURN_IF(!is_And_ವೃಕ್ಷ() && !is_Or_ವೃಕ್ಷ() && !is_Xor_ವೃಕ್ಷ(), nodep);
        ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ biopp = VN_CAST(nodep, ಗಂಟು_Biop)) {
            iterateConst(biopp);
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        ಪ್ರತಿ (auto&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(is_Xor_ವೃಕ್ಷ() || m_polarity, nodep, "must be the original polarity");
    }
    ಆಭಾಸ ~Const_Bit_Op_ವೃಕ್ಷ_Visitor() = ಸ್ವಭಾವ;
#undef CONST_BITOP_RETURN_IF
#undef CONST_BITOP_SET_FAILED

ಸಾರ್ವಜನಿಕ:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* simplify(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಙ್ಕ resultWidth, unsigned externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ಧ್ರುವ Const_Bit_Op_ವೃಕ್ಷ_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        ಯದಿ (visitor.m_failed || visitor.m_varInfos.size() == 1) ಪ್ರತ್ಯಾಗಮ nullptr;

        // FileLine used for constructing all new nodes in this function
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        unsigned resultOps = 0;
        ತಾರ್ಕಿಕ hasCleanTerm = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ hasDirtyTerm = ಅಸತ್ಯ;

        ಪ್ರತಿ (auto&& v : visitor.m_varInfos) {
            ಯದಿ (!v) ಅನುವರ್ತ;  // Skip nullptr at m_varInfos[0]
            ಯದಿ (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
                ಯದಿ (v->getConstResult()) {
                    UASSERT_OBJ(visitor.is_Or_ವೃಕ್ಷ(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = ನೂತನ AstConst{fl, AstConst::BitTrue{}};
                } ಅನ್ಯಥಾ ಯದಿ (visitor.is_And_ವೃಕ್ಷ()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = ನೂತನ AstConst{fl, AstConst::BitFalse{}};
                } ಅನ್ಯಥಾ {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    ಅನುವರ್ತ;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ termp : termps) VL_DO_DANGLING(termp->delete_ವೃಕ್ಷ(), termp);
                ಪ್ರತ್ಯಾಗಮ resultp;
            }
            ಧ್ರುವ ResultTerm result = v->getResultTerm();
            termps.ತಳ್ಳು_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            ಯದಿ (std::get<2>(result)) {
                hasCleanTerm = ಸತ್ಯ;
                UINFO(9, "Clean term: " << termps.back());
            } ಅನ್ಯಥಾ {
                hasDirtyTerm = ಸತ್ಯ;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_ಗಂಟು_Info, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> frozenNodes;
        // Check if frozen terms are clean or not
        ಪ್ರತಿ (ಧ್ರುವ auto& frozenInfo : visitor.m_frozenNodes) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp = frozenInfo.first;
            // Comparison operators are clean
            ಯದಿ ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = ಸತ್ಯ;
            } ಅನ್ಯಥಾ {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = ಸತ್ಯ;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].ತಳ್ಳು_back(termp);
        }

        // Figure out if a final negation is required
        ಧ್ರುವ ತಾರ್ಕಿಕ needsFlip = visitor.is_Xor_ವೃಕ್ಷ() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ಧ್ರುವ ತಾರ್ಕಿಕ needsCleaning = visitor.is_And_ವೃಕ್ಷ() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        ಪ್ರತಿ (ಧ್ರುವ auto& lsbAndNodes : frozenNodes) {
            ಯದಿ (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            ಯದಿ (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        ಯದಿ (needsFlip) ++resultOps;
        // Add final cleaning AND
        ಯದಿ (needsCleaning) ++resultOps;

        ಯದಿ (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp : termps) termp->dump_ವೃಕ್ಷ("-  Reduced term: ");
            ಪ್ರತಿ (ಧ್ರುವ std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Frozen_ಗಂಟು_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_ವೃಕ್ಷ("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        ಯದಿ (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultp = needsFlip ? ನೂತನ AstConst{fl, AstConst::BitTrue{}}
                                                   : ನೂತನ AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            ಪ್ರತ್ಯಾಗಮ resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        ಯದಿ (visitor.m_ops <= resultOps) {
            ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ termp : termps) VL_DO_DANGLING(termp->delete_ವೃಕ್ಷ(), termp);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        ಧ್ರುವ auto reduce = [&visitor, fl](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಯದಿ (!lhsp) ಪ್ರತ್ಯಾಗಮ rhsp;
            ಯದಿ (visitor.is_And_ವೃಕ್ಷ()) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAnd{fl, lhsp, rhsp};
            } ಅನ್ಯಥಾ ಯದಿ (visitor.is_Or_ವೃಕ್ಷ()) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstOr{fl, lhsp, rhsp};
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        ಪ್ರತಿ (auto&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* termp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            ಯದಿ (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = termp->dtypep();
                termp = ನೂತನ AstShiftR{termp->fileline(), termp,
                                      ನೂತನ AstConst(termp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            ಯದಿ (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = termp->dtypep();
                termp = ನೂತನ AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ maskWidth
            = std::max(resultp->width(), resultWidth == 1 ? VL_IDATASIZE : resultWidth);

        // Apply final polarity flip
        ಯದಿ (needsFlip) {
            ಯದಿ (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = ನೂತನ AstNot{fl, resultp};
            } ಅನ್ಯಥಾ {
                // Keep result clean by using XOR(1, _)
                AstConst* ಧ್ರುವ maskp = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, maskWidth, 1};
                resultp = ನೂತನ AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        ಯದಿ (needsCleaning) {
            AstConst* ಧ್ರುವ maskp = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, maskWidth, 1};
            resultp = ನೂತನ AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        ಯದಿ (resultp->width() != resultWidth) {
            resultp = ನೂತನ AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        ಪ್ರತ್ಯಾಗಮ resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

ವರ್ಗ ConstVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // CONSTANTS
    ಸ್ಥಿರ constexpr unsigned CONCAT_MERGABLE_MAX_DEPTH = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ CONCAT_ASSIGN_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    ತಾರ್ಕಿಕ m_params = ಅಸತ್ಯ;  // If true, propagate parameterized and true numbers only
    ತಾರ್ಕಿಕ m_required = ಅಸತ್ಯ;  // If true, must become a constant
    ತಾರ್ಕಿಕ m_wremove = ಸತ್ಯ;  // Inside scope, no assignw removal
    ತಾರ್ಕಿಕ m_warn = ಅಸತ್ಯ;  // Output warnings
    ತಾರ್ಕಿಕ m_doExpensive = ಅಸತ್ಯ;  // Enable computationally expensive optimizations
    ತಾರ್ಕಿಕ m_doCpp = ಅಸತ್ಯ;  // Enable late-stage C++ optimizations
    ತಾರ್ಕಿಕ m_doNConst = ಅಸತ್ಯ;  // Enable non-constant-child simplifications
    ತಾರ್ಕಿಕ m_doV = ಅಸತ್ಯ;  // Verilog, not C++ conversion
    ತಾರ್ಕಿಕ m_doGenerate = ಅಸತ್ಯ;  // Postpone width checking inside generate
    ತಾರ್ಕಿಕ m_convertLogicToBit = ಅಸತ್ಯ;  // Convert logical operators to bitwise
    ತಾರ್ಕಿಕ m_hasJumpDelay = ಅಸತ್ಯ;  // JumpGo or Delay under this loop
    ತಾರ್ಕಿಕ m_hasLoopTest = ಅಸತ್ಯ;  // Contains AstLoopTest
    ತಾರ್ಕಿಕ m_underRecFunc = ಅಸತ್ಯ;  // Under a recursive function
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    ಧ್ರುವ AstArraySel* m_selp = nullptr;  // Current select
    ಧ್ರುವ AstScope* m_scopep = nullptr;  // Current scope
    ಧ್ರುವ AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ಧ್ರುವ ತಾರ್ಕಿಕ m_globalPass;  // ConstVisitor invoked as a global pass
    ಸ್ಥಿರ uint32_t s_global_Pass_ಅಂಕೆ;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<size_t, ತಾರ್ಕಿಕ> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<ಧ್ರುವ AstEnumItem*> m_recursing_Enum_ವಸ್ತುಗಳು;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ canDeleteStatements() { ಪ್ರತ್ಯಾಗಮ v3Global.staticsLifted(); }

    ಶೂನ್ಯ deleteVarScopesUnder(Ast_ಗಂಟು* subtreep) {
        ಯದಿ (!subtreep) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!m_scopep) ಪ್ರತ್ಯಾಗಮ;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.ಸೇರಿಸು(varp); });
        ಯದಿ (varps.empty()) ಪ್ರತ್ಯಾಗಮ;
        ಪ್ರತಿ (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ಯದಿ (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3_ಸಂಖ್ಯೆ const_ಅಂಕೆ_V(Ast_ಗಂಟು* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ& numc = VN_AS(nodep, Const)->ಅಂಕೆ();
        ಪ್ರತ್ಯಾಗಮ !numc.is_ಸಂಖ್ಯೆ() ? numc : V3_ಸಂಖ್ಯೆ{nodep, nodep->widthMinV(), numc};
    }
    V3_ಸಂಖ್ಯೆ to_ಅಂಕೆ_C(Ast_ಗಂಟು* nodep, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& numv) {
        // Extend V width back to C width for given node
        ಪ್ರತ್ಯಾಗಮ !numv.is_ಸಂಖ್ಯೆ() ? numv : V3_ಸಂಖ್ಯೆ{nodep, nodep->width(), numv};
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ lowerAsFixedAggregate(ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        ಪ್ರತ್ಯಾಗಮ dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp, ಧ್ರುವ AstMemberDType* ಧ್ರುವ itemp) {
        AstStructSel* ಧ್ರುವ selp = ನೂತನ AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        ಪ್ರತ್ಯಾಗಮ selp;
    }

    ಶೂನ್ಯ collectFixedAggregateTerms(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& termps,
                                    ಧ್ರುವ ತಾರ್ಕಿಕ packReal) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = fromp->dtypep()->skipRefp();
        ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ idx : unpackDtypep->declRange().seq_ಎಡ_To_ಬಲ()) {
                AstArraySel* ಧ್ರುವ selp
                    = ನೂತನ AstArraySel{fromp->fileline(), fromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep
                   = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
            ಯದಿ (sdtypep->packed()) {
                termps.ತಳ್ಳು_back(fromp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
        } ಅನ್ಯಥಾ ಯದಿ (packReal && dtypep->isDouble()) {
            termps.ತಳ್ಳು_back(ನೂತನ AstRealToBits{fromp->fileline(), fromp});
        } ಅನ್ಯಥಾ {
            termps.ತಳ್ಳು_back(fromp);
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* packFixedAggregate(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp) {
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps;
        collectFixedAggregateTerms(fromp, termps, ಸತ್ಯ);
        UASSERT(!termps.empty(), "No stream terms");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = termps[0];
        ಪ್ರತಿ (size_t i = 1; i < termps.size(); ++i) {
            resultp = ನೂತನ AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    ಶೂನ್ಯ replaceAssignToFixedAggregate(Ast_ಗಂಟು_Assign* ಧ್ರುವ nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dstp,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ dstWidth = dstp->dtypep()->widthStream();
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> termps;
        collectFixedAggregateTerms(dstp, termps, ಅಸತ್ಯ);
        ಪೂರ್ಣಾಙ್ಕ srcWidth = srcp->width();
        ಯದಿ (srcWidth < dstWidth) {
            AstExtend* ಧ್ರುವ extendp = ನೂತನ AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = ನೂತನ AstShiftL{
                extendp->fileline(), extendp,
                ನೂತನ AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        Ast_ಗಂಟು_Assign* newp = nullptr;
        ಪೂರ್ಣಾಙ್ಕ offset = 0;
        ಪ್ರತಿ (size_t i = 0; i < termps.size(); ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp = termps[i];
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = termp->dtypep()->widthStream();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = srcWidth - offset - width;
            offset += width;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp
                = ನೂತನ AstSel{srcp->fileline(), srcp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), lsb, width};
            ಯದಿ (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = ನೂತನ AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_ಗಂಟು::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(srcp), srcp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ತಾರ್ಕಿಕ operandConst(ಧ್ರುವ Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Const); }
    ತಾರ್ಕಿಕ operandAsvConst(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ಧ್ರುವ Ast_ಗಂಟು_Bi_Com_Asv* ಧ್ರುವ bnodep = VN_CAST(nodep, ಗಂಟು_Bi_Com_Asv);
        ಯದಿ (!bnodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!VN_IS(bnodep->lhsp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_Bi_Com_Asv* ಧ್ರುವ rnodep = VN_CAST(bnodep->rhsp(), ಗಂಟು_Bi_Com_Asv);
        ಯದಿ (!rnodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (rnodep->type() != bnodep->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (rnodep->width() != bnodep->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (rnodep->lhsp()->width() != bnodep->lhsp()->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!VN_IS(rnodep->lhsp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ operandAsvSame(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ಧ್ರುವ Ast_ಗಂಟು_Bi_Com_Asv* ಧ್ರುವ bnodep = VN_CAST(nodep, ಗಂಟು_Bi_Com_Asv);
        ಯದಿ (!bnodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_Bi_Com_Asv* ಧ್ರುವ rnodep = VN_CAST(bnodep->rhsp(), ಗಂಟು_Bi_Com_Asv);
        ಯದಿ (!rnodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (rnodep->type() != bnodep->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (rnodep->width() != bnodep->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    ತಾರ್ಕಿಕ operandAsvLUp(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r)) ?
        //
        // Example of how this is useful:
        // BIASV(BIASV(CONSTa,b...),BIASV(CONSTc,d...))  // hits operandAsvUp
        // BIASV(CONSTa,BIASV(b...,BIASV(CONSTc,d...)))  // hits operandAsvUp
        // BIASV(CONSTa,BIASV(CONSTc,BIASV(c...,d...)))  // hits operandAsvConst
        // BIASV(BIASV(CONSTa,CONSTc),BIASV(c...,d...))) // hits normal constant propagation
        // BIASV(CONST_a_c,BIASV(c...,d...)))
        //
        // Idea for the future: All BiComAsvs could be lists, sorted by if they're constant
        ಧ್ರುವ Ast_ಗಂಟು_Bi_Com_Asv* ಧ್ರುವ bnodep = VN_CAST(nodep, ಗಂಟು_Bi_Com_Asv);
        ಯದಿ (!bnodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_Bi_Com_Asv* ಧ್ರುವ lnodep = VN_CAST(bnodep->lhsp(), ಗಂಟು_Bi_Com_Asv);
        ಯದಿ (!lnodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (lnodep->type() != bnodep->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (lnodep->width() != bnodep->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ VN_IS(lnodep->lhsp(), Const);
    }
    ತಾರ್ಕಿಕ operandAsvRUp(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ಧ್ರುವ Ast_ಗಂಟು_Bi_Com_Asv* ಧ್ರುವ bnodep = VN_CAST(nodep, ಗಂಟು_Bi_Com_Asv);
        ಯದಿ (!bnodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_Bi_Com_Asv* ಧ್ರುವ rnodep = VN_CAST(bnodep->rhsp(), ಗಂಟು_Bi_Com_Asv);
        ಯದಿ (!rnodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (rnodep->type() != bnodep->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (rnodep->width() != bnodep->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ VN_IS(rnodep->lhsp(), Const);
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ operandSubAdd(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ np = VN_CAST(nodep, ಗಂಟು_Biop);
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ lp = VN_CAST(np->lhsp(), ಗಂಟು_Biop);
        ಪ್ರತ್ಯಾಗಮ (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    ತಾರ್ಕಿಕ matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ಧ್ರುವ constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        ಧ್ರುವ auto isEqOrNeq = [](ಧ್ರುವ Ast_ಗಂಟು* nodep) -> ತಾರ್ಕಿಕ {  //
            ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        ಯದಿ (isEqOrNeq(rhsp)) {
            ccastp = ನೂತನ AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } ಅನ್ಯಥಾ ಯದಿ (AstCCast* ಧ್ರುವ tmpp = VN_CAST(rhsp, CCast)) {
            ಯದಿ (isEqOrNeq(tmpp->lhsp())) {
                ಯದಿ (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } ಅನ್ಯಥಾ {
                    ccastp = ನೂತನ AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        ಯದಿ (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(andp), andp);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ operandAndOrSame(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ np = VN_CAST(nodep, ಗಂಟು_Biop);
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ lp = VN_CAST(np->lhsp(), ಗಂಟು_Biop);
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ rp = VN_CAST(np->rhsp(), ಗಂಟು_Biop);
        ಪ್ರತ್ಯಾಗಮ (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    ತಾರ್ಕಿಕ matchOrAndNot(Ast_ಗಂಟು_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap;
        Ast_ಗಂಟು_Biop* andp;
        ಯದಿ (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಧ್ರುವ Ast_ಗಂಟು_Uniop* notp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cp;
        ಯದಿ (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bp = notp->lhsp();
        ಯದಿ (!operandsSame(ap, bp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        ಯದಿ (nodep->rhsp()) {
            nodep->lhsp(cp);
        } ಅನ್ಯಥಾ {
            nodep->rhsp(cp);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* ಧ್ರುವ condp = VN_CAST(nodep->rhsp(), Cond);
        ಯದಿ (!condp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstConst* ಧ್ರುವ maskp = VN_CAST(nodep->lhsp(), Const);
        ಯದಿ (!maskp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* ಧ್ರುವ newp = ನೂತನ AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            ನೂತನ AstAnd{nodep->fileline(), maskp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), condp->thenp()->unlinkFrBack()},
            ನೂತನ AstAnd{nodep->fileline(), maskp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ matchCondCond(ಧ್ರುವ AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ಯದಿ (VN_IS(nodep->backp(), Cond)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Was checked when visited parent
        ಯದಿ (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Short circuit
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> truesp;
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Can optimize further
    }
    ಶೂನ್ಯ matchCondCondRecurse(ಧ್ರುವ AstCond* nodep, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& truesp,
                              std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ಯದಿ (truesp.size() > 4 || falsesp.size() > 4) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nodep->condp()->isPure()) ಪ್ರತ್ಯಾಗಮ;
        ತಾರ್ಕಿಕ replaced = ಅಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp : truesp) {
            ಯದಿ (replaced) ಭಙ್ಗ;
            ಯದಿ (!operandsSame(nodep->condp(), condp)) ಅನುವರ್ತ;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_ಅಂಕೆ(nodep->condp(), 1);
            replaced = ಸತ್ಯ;
            ++m_stat_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;
        }
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp : falsesp) {
            ಯದಿ (replaced) ಭಙ್ಗ;
            ಯದಿ (!operandsSame(nodep->condp(), condp)) ಅನುವರ್ತ;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ಸತ್ಯ;
            ++m_stat_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;
        }
        ಯದಿ (ಧ್ರುವ AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            ಯದಿ (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ಯದಿ (!replaced) truesp.ತೆಗೆ_back();
        }
        ಯದಿ (ಧ್ರುವ AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            ಯದಿ (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ಯದಿ (!replaced) falsesp.ತೆಗೆ_back();
        }
    }
    ಶೂನ್ಯ matchIfCondCond(ಧ್ರುವ Ast_ಗಂಟು_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ಯದಿ (VN_IS(nodep->backp(), If)) ಪ್ರತ್ಯಾಗಮ;  // Was checked when visited parent
        ಯದಿ (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) ಪ್ರತ್ಯಾಗಮ;  // Short circuit
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> truesp;
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    ಶೂನ್ಯ matchIfCondCondRecurse(ಧ್ರುವ Ast_ಗಂಟು_If* nodep, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& truesp,
                                std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ಯದಿ (truesp.size() > 4 || falsesp.size() > 4) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nodep->condp()->isPure()) ಪ್ರತ್ಯಾಗಮ;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        ಯದಿ (VN_IS(nodep->condp(), Const)) ಪ್ರತ್ಯಾಗಮ;
        ತಾರ್ಕಿಕ replaced = ಅಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp : truesp) {
            ಯದಿ (replaced) ಭಙ್ಗ;
            ಯದಿ (!operandsSame(nodep->condp(), condp)) ಅನುವರ್ತ;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_ಅಂಕೆ(nodep->condp(), 1);
            replaced = ಸತ್ಯ;
            ++m_stat_If_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;
        }
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp : falsesp) {
            ಯದಿ (replaced) ಭಙ್ಗ;
            ಯದಿ (!operandsSame(nodep->condp(), condp)) ಅನುವರ್ತ;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ಸತ್ಯ;
            ++m_stat_If_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_If* subIfp = VN_CAST(nodep->thensp(), ಗಂಟು_If)) {
            ಯದಿ (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ಯದಿ (!replaced) truesp.ತೆಗೆ_back();
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_If* subIfp = VN_CAST(nodep->elsesp(), ಗಂಟು_If)) {
            ಯದಿ (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ಯದಿ (!replaced) falsesp.ತೆಗೆ_back();
        }
    }

    ಶೂನ್ಯ matchIfSameLast(Ast_ಗಂಟು_If* nodep) {
        ಯದಿ (!nodep->thensp()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nodep->elsesp()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!canDeleteStatements()) ಪ್ರತ್ಯಾಗಮ;
        // Iterate bodies in reverse
        Ast_ಗಂಟು* tp = nodep->thensp()->lastp();
        Ast_ಗಂಟು* ep = nodep->elsesp()->lastp();
        ಯಾವತ್ (tp && ep && tp->same_ವೃಕ್ಷ(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_ಗಂಟು* ಧ್ರುವ tPrevp = tp->prevp();
            Ast_ಗಂಟು* ಧ್ರುವ ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ep->unlinkFrBack()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    ತಾರ್ಕಿಕ matchMaskedOr(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(nodep->lhsp(), Const);
        ಧ್ರುವ uint32_t significantBits = constp->ಅಂಕೆ().widthToFit();

        AstOr* ಧ್ರುವ orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ಧ್ರುವ auto checkBottomClear = [=](ಧ್ರುವ Ast_ಗಂಟು* nodep) -> ತಾರ್ಕಿಕ {
            ಯದಿ (ಧ್ರುವ AstShiftL* ಧ್ರುವ shiftp = VN_CAST(nodep, ShiftL)) {
                ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ scp = VN_CAST(shiftp->rhsp(), Const)) {
                    ಪ್ರತ್ಯಾಗಮ scp->ಅಂಕೆ().toUInt() >= significantBits;
                }
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        };

        ಧ್ರುವ ತಾರ್ಕಿಕ orLIsRedundant = checkBottomClear(orp->lhsp());
        ಧ್ರುವ ತಾರ್ಕಿಕ orRIsRedundant = checkBottomClear(orp->rhsp());

        ಯದಿ (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                ನೂತನ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(orp), orp);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // input node is still valid, keep going
        } ಅನ್ಯಥಾ ಯದಿ (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(orp), orp);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // input node is still valid, keep going
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
    }
    ತಾರ್ಕಿಕ matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ಧ್ರುವ auto checkMask = [nodep, ಏತತ್](ಧ್ರುವ V3_ಸಂಖ್ಯೆ& mask) -> ತಾರ್ಕಿಕ {
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(nodep->lhsp(), Const);
            ಯದಿ (constp->ಅಂಕೆ().isCaseEq(mask)) {
                Ast_ಗಂಟು* ಧ್ರುವ rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        };

        // Check if masking is redundant
        ಯದಿ (ಧ್ರುವ AstShiftR* ಧ್ರುವ shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_ಸಂಖ್ಯೆ maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->ಅಂಕೆ().toUInt());
                ಪ್ರತ್ಯಾಗಮ checkMask(maskLo);
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstShiftL* ಧ್ರುವ shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_ಸಂಖ್ಯೆ mask{nodep, nodep->width()};
                ಧ್ರುವ uint32_t shiftAmount = scp->ಅಂಕೆ().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                ಪ್ರತ್ಯಾಗಮ checkMask(mask);
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ matchMaskedZero(ಧ್ರುವ AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ಧ್ರುವ AstConst* ಧ್ರುವ maskp = VN_AS(nodep->lhsp(), Const);
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp();
        ಧ್ರುವ uint32_t msbP1 = maskp->ಅಂಕೆ().mostSetBitP1();
        ಯದಿ (!msbP1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Don't rewrite, separate rule matches for this
        ಧ್ರುವ uint32_t msb = msbP1 - 1;
        ಧ್ರುವ uint32_t lsb = maskp->ಅಂಕೆ().leastSetBitP1() - 1;

        ಯದಿ (ಧ್ರುವ AstShiftL* ಧ್ರುವ shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            ಯದಿ (AstConst* ಧ್ರುವ scp = VN_CAST(shiftp->rhsp(), Const)) {
                ಪ್ರತ್ಯಾಗಮ scp->ಅಂಕೆ().fitsInUInt()  //
                       && (scp->ಅಂಕೆ().toUInt() > msb);
            }
        }
        ಯದಿ (ಧ್ರುವ AstShiftR* ಧ್ರುವ shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            ಯದಿ (AstConst* ಧ್ರುವ scp = VN_CAST(shiftp->rhsp(), Const)) {
                ಪ್ರತ್ಯಾಗಮ scp->ಅಂಕೆ().fitsInUInt()
                       && (lsb + scp->ಅಂಕೆ().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        ಯದಿ (ಧ್ರುವ AstMul* ಧ್ರುವ mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            ಯದಿ (AstConst* ಧ್ರುವ cp = VN_CAST(mulp->lhsp(), Const)) {
                ಪ್ರತ್ಯಾಗಮ cp->ಅಂಕೆ().leastSetBitP1() > msb + 1;
            }
        }
        ಯದಿ (ಧ್ರುವ AstExtend* ಧ್ರುವ extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            ಪ್ರತ್ಯಾಗಮ lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ match_Bit_Op_ವೃಕ್ಷ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (nodep->widthMin() != 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!v3Global.opt.f_Const_Bit_Op_ವೃಕ್ಷ()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->width();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rootp = nodep;
        unsigned externalOps = 0;
        // Reach past a plain making AND
        ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ andp = VN_CAST(nodep, And)) {
            ಯದಿ (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        ಯದಿ (rootp->type() == nodep->backp()->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        std::string debugPrefix;
        ಯದಿ (debug() >= 9) {  // LCOV_EXCL_START
            ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dump_ವೃಕ್ಷ(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = Const_Bit_Op_ವೃಕ್ಷ_Visitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        ಯದಿ (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }

        ಯದಿ (debug() >= 9) {  // LCOV_EXCL_START
            ಯದಿ (newp) {
                newp->dump_ವೃಕ್ಷ(debugPrefix + "RESULT: ");
            } ಅನ್ಯಥಾ {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        ಪ್ರತ್ಯಾಗಮ newp;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ operandShiftSame(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ np = VN_AS(nodep, ಗಂಟು_Biop);
        {
            ಧ್ರುವ AstShiftL* ಧ್ರುವ lp = VN_CAST(np->lhsp(), ShiftL);
            ಧ್ರುವ AstShiftL* ಧ್ರುವ rp = VN_CAST(np->rhsp(), ShiftL);
            ಯದಿ (lp && rp) {
                ಪ್ರತ್ಯಾಗಮ (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ಧ್ರುವ AstShiftR* ಧ್ರುವ lp = VN_CAST(np->lhsp(), ShiftR);
            ಧ್ರುವ AstShiftR* ಧ್ರುವ rp = VN_CAST(np->rhsp(), ShiftR);
            ಯದಿ (lp && rp) {
                ಪ್ರತ್ಯಾಗಮ (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ operandHugeShiftL(ಧ್ರುವ Ast_ಗಂಟು_Biop* nodep) {
        ಪ್ರತ್ಯಾಗಮ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    ತಾರ್ಕಿಕ operandHugeShiftR(ಧ್ರುವ Ast_ಗಂಟು_Biop* nodep) {
        ಪ್ರತ್ಯಾಗಮ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    ತಾರ್ಕಿಕ operandIsTwo(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // not constant
        ಯದಿ (constp->ಅಂಕೆ().isFourState()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // four-state
        ಯದಿ (nodep->width() > VL_QUADSIZE) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // too wide
        ಯದಿ (nodep->isSigned() && constp->ಅಂಕೆ().isNegative()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // signed and negative
        ಪ್ರತ್ಯಾಗಮ constp->toUQuad() == 2;
    }
    ತಾರ್ಕಿಕ operandIsTwostate(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->ಅಂಕೆ().isFourState());
    }
    ತಾರ್ಕಿಕ operandIsPowTwo(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಯದಿ (!operandIsTwostate(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ (1 == VN_AS(nodep, Const)->ಅಂಕೆ().ಎಣಿಕೆ_Ones());
    }
    ತಾರ್ಕಿಕ operandShiftOp(ಧ್ರುವ Ast_ಗಂಟು_Biop* nodep) {
        ಯದಿ (!VN_IS(nodep->rhsp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp(), ಗಂಟು_Biop);
        ಯದಿ (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->width() != lhsp->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->width() != lhsp->lhsp()->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->width() != lhsp->rhsp()->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        ಯದಿ (VN_IS(lhsp->lhsp(), Const)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (VN_IS(lhsp->rhsp(), Const)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // - Const shift on either side
        ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ llp = VN_CAST(lhsp->lhsp(), ಗಂಟು_Biop)) {
            ಪ್ರತ್ಯಾಗಮ (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ lrp = VN_CAST(lhsp->rhsp(), ಗಂಟು_Biop)) {
            ಪ್ರತ್ಯಾಗಮ (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ operandShiftShift(ಧ್ರುವ Ast_ಗಂಟು_Biop* nodep) {
        // We could add a AND though.
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp(), ಗಂಟು_Biop);
        ಯದಿ (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        ಯದಿ (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->ಅಂಕೆ().isFourState())
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->width() != lhsp->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->width() != lhsp->lhsp()->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ operandWordOOB(ಧ್ರುವ AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        ಪ್ರತ್ಯಾಗಮ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ಗಂಟು_Var_Ref)
                && VN_AS(nodep->fromp(), ಗಂಟು_Var_Ref)->access().isReadOnly()
                && (static_cast<ಪೂರ್ಣಾಙ್ಕ>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), ಗಂಟು_Var_Ref)->varp()->width_ಪದಗಳು()));
    }
    ತಾರ್ಕಿಕ operandSelFull(ಧ್ರುವ AstSel* nodep) {
        ಪ್ರತ್ಯಾಗಮ (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<ಪೂರ್ಣಾಙ್ಕ>(nodep->widthConst()) == nodep->fromp()->width());
    }
    ತಾರ್ಕಿಕ operandSelExtend(AstSel* nodep) {
        ಯದಿ (!m_doV) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstExtend* ಧ್ರುವ extendp = VN_CAST(nodep->fromp(), Extend);
        ಯದಿ (!extendp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstConst* ಧ್ರುವ lsbp = VN_CAST(nodep->lsbp(), Const);
        ಯದಿ (!lsbp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->widthConst();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = lsbp->toSInt();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = lsb + width - 1;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = extendp->lhsp();
        ಯದಿ (!lhsp->isPure()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Selecting the entire extended expression, replace with that
        ಯದಿ (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // Select entirely in the extended part - replace with zero
        ಯದಿ (lsb >= lhsp->width()) {
            replaceZero(nodep);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // Select entirely in the extended expression - replace with select from that
        ಯದಿ (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(ನೂತನ AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // Select straddles both sides, but is just a shorter extend
        ಯದಿ (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(ನೂತನ AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_ಗಂಟು_Biop* ಧ್ರುವ bip = VN_CAST(nodep->fromp(), ಗಂಟು_Biop);
        ಯದಿ (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bilhsp = bip->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(ನೂತನ AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(ನೂತನ AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ಧ್ರುವ AstShiftR* ಧ್ರುವ shiftp = VN_CAST(nodep->fromp(), ShiftR);
        ಯದಿ (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ap = shiftp->lhsp();
        ಧ್ರುವ AstConst* ಧ್ರುವ bp = VN_AS(shiftp->rhsp(), Const);
        ಧ್ರುವ AstConst* ಧ್ರುವ lp = VN_AS(nodep->lsbp(), Const);
        ಯದಿ (bp->isWide() || bp->ಅಂಕೆ().isFourState() || bp->ಅಂಕೆ().isNegative() || lp->isWide()
            || lp->ಅಂಕೆ().isFourState() || lp->ಅಂಕೆ().isNegative()) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ newLsb = lp->toSInt() + bp->toSInt();
        ಯದಿ (newLsb + nodep->widthConst() > ap->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ಧ್ರುವ newp
            = ನೂತನ AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ operandBiExtendConstShrink(Ast_ಗಂಟು_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ಧ್ರುವ extendp = VN_CAST(nodep->rhsp(), Extend);
        ಯದಿ (!extendp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ smallerp = extendp->lhsp();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ subsize = smallerp->width();
        AstConst* ಧ್ರುವ constp = VN_CAST(nodep->lhsp(), Const);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!constp->ಅಂಕೆ().isBitsZero(constp->width() - 1, subsize)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ{constp, subsize, constp->ಅಂಕೆ()};
        nodep->lhsp(ನೂತನ AstConst{constp->fileline(), ಅಂಕೆ});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(constp), constp);
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // input node is still valid, keep going
    }
    ತಾರ್ಕಿಕ operandBiExtendConstOver(ಧ್ರುವ Ast_ಗಂಟು_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ಧ್ರುವ AstExtend* ಧ್ರುವ extendp = VN_CAST(nodep->rhsp(), Extend);
        ಯದಿ (!extendp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ smallerp = extendp->lhsp();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ subsize = smallerp->width();
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->lhsp(), Const);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (constp->ಅಂಕೆ().isBitsZero(constp->width() - 1, subsize)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Extraction checks
    ತಾರ್ಕಿಕ warnSelect(AstSel* nodep) {
        ಯದಿ (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ಧ್ರುವ ತಾರ್ಕಿಕ doit = ಸತ್ಯ;
        ಯದಿ (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            ಯದಿ (VN_AS(nodep->lsbp(), Const)->ಅಂಕೆ().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } ಅನ್ಯಥಾ ಯದಿ (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                ಯದಿ (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } ಅನ್ಯಥಾ {
                    msbLsbProtected = std::to_string(nodep->msbConst() / nodep->declElWidth())
                                      + ":"
                                      + std::to_string(nodep->lsbConst() / nodep->declElWidth());
                }
                nodep->v3warn(SELRANGE,
                              "Selection index out of range: "
                                  << msbLsbProtected << " outside "
                                  << nodep->declRange().hiMaxSelect() << ":0"
                                  << (nodep->declRange().lo() >= 0
                                          ? ""
                                          : (" (adjusted +" + cvtToStr(-nodep->declRange().lo())
                                             + " to account for negative lsb)")));
                UINFO(1, "    Related Raw index is " << nodep->msbConst() << ":"
                                                     << nodep->lsbConst());
                // Don't replace with zero, we'll do it later
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Not a transform, so NOP
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ operandsSame(ಧ್ರುವ Ast_ಗಂಟು* node1p, ಧ್ರುವ Ast_ಗಂಟು* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ const1p = VN_CAST(node1p, Const)) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ const2p = VN_CAST(node2p, Const)) {
                ಪ್ರತ್ಯಾಗಮ V3_ಸಂಖ್ಯೆ{node1p->fileline(), 1, 0}
                    .opEq(const1p->ಅಂಕೆ(), const2p->ಅಂಕೆ())
                    .isEqOne();
            }
        }
        ಯದಿ (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            ಪ್ರತ್ಯಾಗಮ node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ and1p = VN_CAST(node1p, And)) {
            ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ and2p = VN_CAST(node2p, And)) {
                ಯದಿ (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), ಗಂಟು_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), ಗಂಟು_Var_Ref))
                    ಪ್ರತ್ಯಾಗಮ node1p->same_Gate_ವೃಕ್ಷ(node2p);
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ ifSameAssign(ಧ್ರುವ Ast_ಗಂಟು_If* nodep) {
        ಧ್ರುವ Ast_ಗಂಟು_Assign* ಧ್ರುವ thensp = VN_CAST(nodep->thensp(), ಗಂಟು_Assign);
        ಧ್ರುವ Ast_ಗಂಟು_Assign* ಧ್ರುವ elsesp = VN_CAST(nodep->elsesp(), ಗಂಟು_Assign);
        ಯದಿ (!thensp || thensp->nextp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Must be SINGLE statement
        ಯದಿ (!elsesp || elsesp->nextp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (thensp->type() != elsesp->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Can't mix an assigndly with assign
        ಯದಿ (!thensp->lhsp()->same_Gate_ವೃಕ್ಷ(elsesp->lhsp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!thensp->rhsp()->gate_ವೃಕ್ಷ()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!elsesp->rhsp()->gate_ವೃಕ್ಷ()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Must not create an expression with unpacked array type
        ಯದಿ (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (m_underRecFunc) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // This optimization may lead to infinite recursion
        ಯದಿ (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            ಯದಿ (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        ಪ್ರತ್ಯಾಗಮ !VN_IS(thensp->rhsp(), ಗಂಟು_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), ಗಂಟು_FTask_Ref)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    ತಾರ್ಕಿಕ operandIfIf(ಧ್ರುವ Ast_ಗಂಟು_If* nodep) {
        ಯದಿ (nodep->elsesp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_If* ಧ್ರುವ lowerIfp = VN_CAST(nodep->thensp(), ಗಂಟು_If);
        ಯದಿ (!lowerIfp || lowerIfp->nextp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->type() != lowerIfp->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (Ast_ಗಂಟು::afterCommentp(lowerIfp->elsesp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ ifConcatMergeableBiop(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    ತಾರ್ಕಿಕ ifAdjacentSel(ಧ್ರುವ AstSel* lhsp, ಧ್ರುವ AstSel* rhsp) {
        ಯದಿ (!v3Global.opt.fAssemble()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // opt disabled
        ಯದಿ (!lhsp || !rhsp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ lfromp = lhsp->fromp();
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ rfromp = rhsp->fromp();
        ಯದಿ (!lfromp || !rfromp || !lfromp->same_Gate_ವೃಕ್ಷ(rfromp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstConst* ಧ್ರುವ lstart = VN_CAST(lhsp->lsbp(), Const);
        ಧ್ರುವ AstConst* ಧ್ರುವ rstart = VN_CAST(rhsp->lsbp(), Const);
        ಯದಿ (!lstart || !rstart) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // too complicated
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rend = (rstart->toSInt() + rhsp->widthConst());
        ಪ್ರತ್ಯಾಗಮ (rend == lstart->toSInt());
    }
    ತಾರ್ಕಿಕ ifMergeAdjacent(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        ಯದಿ (!v3Global.opt.fAssemble()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // opt disabled
        // Two same varref
        ಯದಿ (operandsSame(lhsp, rhsp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಧ್ರುವ AstSel* ಧ್ರುವ lselp = VN_CAST(lhsp, Sel);
        ಧ್ರುವ AstSel* ಧ್ರುವ rselp = VN_CAST(rhsp, Sel);
        ಯದಿ (!lselp && !rselp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (lselp && !VN_IS(lselp->lsbp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (rselp && !VN_IS(rselp->lsbp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // a[i:j] {a[j-1:k], b}
        ಯದಿ (lselp && !rselp && VN_IS(rhsp, Concat))
            ಪ್ರತ್ಯಾಗಮ ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        ಯದಿ (rselp && !lselp && VN_IS(lhsp, Concat))
            ಪ್ರತ್ಯಾಗಮ ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_ವೃಕ್ಷ(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_ವೃಕ್ಷ(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        ಯದಿ (!lfromp || !rfromp || !lfromp->same_Gate_ವೃಕ್ಷ(rfromp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಧ್ರುವ int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ಧ್ರುವ int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ಧ್ರುವ int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        ಯದಿ (rend == lstart) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // a[i:0] a[msb:j]
        ಯದಿ (rend == rfromp->width() && lstart == 0) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ concatMergeable(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, unsigned depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        ಯದಿ (!v3Global.opt.fAssemble()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // opt disabled
        ಯದಿ (lhsp->type() != rhsp->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!ifConcatMergeableBiop(lhsp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (depth > CONCAT_MERGABLE_MAX_DEPTH) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // As worse case O(n^2) algorithm

        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ lp = VN_CAST(lhsp, ಗಂಟು_Biop);
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ rp = VN_CAST(rhsp, ಗಂಟು_Biop);
        ಯದಿ (!lp || !rp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // {a[]&b[], a[]&b[]}
        ಧ್ರುವ ತಾರ್ಕಿಕ lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        ಧ್ರುವ ತಾರ್ಕಿಕ rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        ಯದಿ (lad && rad) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        ಯದಿ (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        ಯದಿ (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        ಯದಿ (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ operandsSameWidth(ಧ್ರುವ Ast_ಗಂಟು* lhsp, ಧ್ರುವ Ast_ಗಂಟು* rhsp) {
        ಪ್ರತ್ಯಾಗಮ lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    ಶೂನ್ಯ replace_ಅಂಕೆ(Ast_ಗಂಟು* oldp, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ, ಧ್ರುವ string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->ಅಂಕೆ().isFourState()), oldp,
                    "Already constant??");
        AstConst* ಧ್ರುವ newp = ನೂತನ AstConst{oldp->fileline(), ಅಂಕೆ};
        ಯದಿ (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp), oldp);
    }
    ಶೂನ್ಯ replace_ಅಂಕೆ(Ast_ಗಂಟು* nodep, uint32_t val) {
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, nodep->width(), val};
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }
    ಶೂನ್ಯ replace_ಅಂಕೆ_Signed(Ast_ಗಂಟು_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        ಯದಿ (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, val), nodep);
    }
    ಶೂನ್ಯ replace_ಅಂಕೆ_Limited(Ast_ಗಂಟು_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        ಯದಿ (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, val), nodep);
    }
    ಶೂನ್ಯ replaceZero(Ast_ಗಂಟು* nodep) { VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, 0), nodep); }
    ಶೂನ್ಯ replaceZeroChkPure(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        ಯದಿ (checkp->isPure()) {
            VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, 0), nodep);
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstAnd{nodep->fileline(), ನೂತನ AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ replaceAllOnes(Ast_ಗಂಟು* nodep) {
        V3_ಸಂಖ್ಯೆ ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ones), nodep);
    }
    ಶೂನ್ಯ replaceConst(Ast_ಗಂಟು_Uniop* nodep) {
        V3_ಸಂಖ್ಯೆ numv{nodep, nodep->widthMinV()};
        nodep->ಸಂಖ್ಯೆ_Operate(numv, const_ಅಂಕೆ_V(nodep->lhsp()));
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = to_ಅಂಕೆ_C(nodep, numv);
        UINFO(4, "UNICONST -> " << ಅಂಕೆ);
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }
    ಶೂನ್ಯ replaceConst(Ast_ಗಂಟು_Biop* nodep) {
        V3_ಸಂಖ್ಯೆ numv{nodep, nodep->widthMinV()};
        nodep->ಸಂಖ್ಯೆ_Operate(numv, const_ಅಂಕೆ_V(nodep->lhsp()), const_ಅಂಕೆ_V(nodep->rhsp()));
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = to_ಅಂಕೆ_C(nodep, numv);
        UINFO(4, "BICONST -> " << ಅಂಕೆ);
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }
    ಶೂನ್ಯ replaceConst(Ast_ಗಂಟು_Triop* nodep) {
        V3_ಸಂಖ್ಯೆ numv{nodep, nodep->widthMinV()};
        nodep->ಸಂಖ್ಯೆ_Operate(numv, const_ಅಂಕೆ_V(nodep->lhsp()), const_ಅಂಕೆ_V(nodep->rhsp()),
                             const_ಅಂಕೆ_V(nodep->thsp()));
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = to_ಅಂಕೆ_C(nodep, numv);
        UINFO(4, "TRICONST -> " << ಅಂಕೆ);
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }
    ಶೂನ್ಯ replaceConst(Ast_ಗಂಟು_Quadop* nodep) {
        V3_ಸಂಖ್ಯೆ numv{nodep, nodep->widthMinV()};
        nodep->ಸಂಖ್ಯೆ_Operate(numv, const_ಅಂಕೆ_V(nodep->lhsp()), const_ಅಂಕೆ_V(nodep->rhsp()),
                             const_ಅಂಕೆ_V(nodep->thsp()), const_ಅಂಕೆ_V(nodep->fhsp()));
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = to_ಅಂಕೆ_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << ಅಂಕೆ);
        VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
    }

    ಶೂನ್ಯ replaceConstString(Ast_ಗಂಟು* oldp, ಧ್ರುವ string& ಅಂಕೆ) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstConst{oldp->fileline(), AstConst::String{}, ಅಂಕೆ};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    ಶೂನ್ಯ replaceWChild(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ replaceWChildBool(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        ಯದಿ (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } ಅನ್ಯಥಾ {
            nodep->replaceWithKeepDType(ನೂತನ AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    ಶೂನ್ಯ replaceWIteratedRhs(Ast_ಗಂಟು_Triop* nodep) {
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    ಶೂನ್ಯ replaceWIteratedThs(Ast_ಗಂಟು_Triop* nodep) {
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    ಶೂನ್ಯ replaceWLhs(Ast_ಗಂಟು_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ಶೂನ್ಯ replaceWLhs(Ast_ಗಂಟು_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ಶೂನ್ಯ replaceWRhs(Ast_ಗಂಟು_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    ಶೂನ್ಯ replaceWLhsBool(Ast_ಗಂಟು_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    ಶೂನ್ಯ replaceWRhsBool(Ast_ಗಂಟು_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    ಶೂನ್ಯ replaceAsv(Ast_ಗಂಟು_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ap = nodep->lhsp();
        Ast_ಗಂಟು_Biop* ಧ್ರುವ rp = VN_AS(nodep->rhsp(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bp = rp->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        ಯದಿ (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    ಶೂನ್ಯ replaceAsvLUp(Ast_ಗಂಟು_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_ಗಂಟು_Biop* ಧ್ರುವ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ llp = lp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ಶೂನ್ಯ replaceAsvRUp(Ast_ಗಂಟು_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lp = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Biop* ಧ್ರುವ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ಶೂನ್ಯ replaceAndOr(Ast_ಗಂಟು_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_ಗಂಟು_Biop* ಧ್ರುವ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ llp = lp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Biop* ಧ್ರುವ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        ಯದಿ (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(rp), rp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(rlp), rlp);
        } ಅನ್ಯಥಾ ಯದಿ (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(rp), rp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lrp), lrp);
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    ಶೂನ್ಯ replaceShiftSame(Ast_ಗಂಟು_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_ಗಂಟು_Biop* ಧ್ರುವ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ llp = lp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Biop* ಧ್ರುವ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rp), rp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    ಶೂನ್ಯ replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ಧ್ರುವ lselp = VN_AS(nodep->lhsp()->unlinkFrBack(), Sel);
        AstSel* ಧ್ರುವ rselp = VN_AS(nodep->rhsp()->unlinkFrBack(), Sel);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lstart = lselp->lsbConst();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lwidth = lselp->widthConst();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rstart = rselp->lsbConst();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rwidth = rselp->widthConst();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ಧ್ರುವ newselp = ನೂತನ AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlinkFrBack(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(lselp), lselp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rselp), rselp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_ಗಂಟು_Biop* ಧ್ರುವ lp = VN_AS(nodep->lhsp(), ಗಂಟು_Biop);
        Ast_ಗಂಟು_Biop* ಧ್ರುವ rp = VN_AS(nodep->rhsp(), ಗಂಟು_Biop);
        ಯದಿ (concatMergeable(lp, rp, 0)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ llp = lp->lhsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lrp = lp->rhsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rlp = rp->lhsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rrp = rp->rhsp();
            AstConcat* ಧ್ರುವ newlp = ನೂತನ AstConcat{rlp->fileline(), llp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                   rlp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            AstConcat* ಧ್ರುವ newrp = ನೂತನ AstConcat{rrp->fileline(), lrp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                   rrp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    ಶೂನ್ಯ replaceExtend(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replacePowShift(Ast_ಗಂಟು_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ಧ್ರುವ newp
            = ನೂತನ AstShiftL{nodep->fileline(), ನೂತನ AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ amount = VN_AS(nodep->lhsp(), Const)->ಅಂಕೆ().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ಧ್ರುವ newp
            = ನೂತನ AstShiftL{nodep->fileline(), opp, ನೂತನ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ amount = VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* ಧ್ರುವ newp
            = ನೂತನ AstShiftR{nodep->fileline(), opp, ನೂತನ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ amount = VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().mostSetBitP1() - 1;  // 2^n->n+1
        V3_ಸಂಖ್ಯೆ mask{nodep, nodep->width()};
        mask.setMask(amount);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* ಧ್ರುವ newp
            = ನೂತನ AstAnd{nodep->fileline(), opp, ನೂತನ AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replaceShiftOp(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->width();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        Ast_ಗಂಟು_Biop* ಧ್ರುವ lhsp = VN_AS(nodep->lhsp(), ಗಂಟು_Biop);
        lhsp->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ shiftp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ap = lhsp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bp = lhsp->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Biop* ಧ್ರುವ shift1p = nodep;
        Ast_ಗಂಟು_Biop* ಧ್ರುವ shift2p = nodep->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_ಗಂಟು_Biop* ಧ್ರುವ newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    ಶೂನ್ಯ replaceShiftShift(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_ಗಂಟು_Biop* ಧ್ರುವ lhsp = VN_AS(nodep->lhsp(), ಗಂಟು_Biop);
        lhsp->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ap = lhsp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ shift1p = lhsp->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        ಯದಿ (nodep->type() == lhsp->type()) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ shift1 = VN_AS(shift1p, Const)->toUInt();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ shift2 = VN_AS(shift2p, Const)->toUInt();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ newshift = shift1 + shift2;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(ನೂತನ AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } ಅನ್ಯಥಾ {
            // We know shift amounts are constant, but might be a mixed left/right shift
            ಪೂರ್ಣಾಙ್ಕ shift1 = VN_AS(shift1p, Const)->toUInt();
            ಯದಿ (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            ಪೂರ್ಣಾಙ್ಕ shift2 = VN_AS(shift2p, Const)->toUInt();
            ಯದಿ (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ newshift = shift1 + shift2;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(shift2p), shift2p);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            V3_ಸಂಖ್ಯೆ mask1{nodep, nodep->width()};
            V3_ಸಂಖ್ಯೆ ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            ಯದಿ (shift1 < 0) {
                mask1.opShiftR(ones, V3_ಸಂಖ್ಯೆ(nodep, VL_IDATASIZE, -shift1));
            } ಅನ್ಯಥಾ {
                mask1.opShiftL(ones, V3_ಸಂಖ್ಯೆ(nodep, VL_IDATASIZE, shift1));
            }
            V3_ಸಂಖ್ಯೆ mask{nodep, nodep->width()};
            ಯದಿ (shift2 < 0) {
                mask.opShiftR(mask1, V3_ಸಂಖ್ಯೆ(nodep, VL_IDATASIZE, -shift2));
            } ಅನ್ಯಥಾ {
                mask.opShiftL(mask1, V3_ಸಂಖ್ಯೆ(nodep, VL_IDATASIZE, shift2));
            }
            ಯದಿ (newshift < 0) {
                newp = ನೂತನ AstShiftR{nodep->fileline(), ap,
                                     ನೂತನ AstConst(nodep->fileline(), -newshift)};
            } ಅನ್ಯಥಾ {
                newp = ನೂತನ AstShiftL{nodep->fileline(), ap,
                                     ನೂತನ AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = ನೂತನ AstAnd{nodep->fileline(), newp, ನೂತನ AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
    }

    ತಾರ್ಕಿಕ replaceAssignMultiSel(Ast_ಗಂಟು_Assign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        ಯದಿ (!m_modp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ಧ್ರುವ sel1p = VN_CAST(nodep->lhsp(), Sel);
        ಯದಿ (!sel1p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        Ast_ಗಂಟು_Assign* ಧ್ರುವ nextp = VN_CAST(nodep->nextp(), ಗಂಟು_Assign);
        ಯದಿ (!nextp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->type() != nextp->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstSel* ಧ್ರುವ sel2p = VN_CAST(nextp->lhsp(), Sel);
        ಯದಿ (!sel2p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVarRef* ಧ್ರುವ varref1p = VN_CAST(sel1p->fromp(), VarRef);
        ಯದಿ (!varref1p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVarRef* ಧ್ರುವ varref2p = VN_CAST(sel2p->fromp(), VarRef);
        ಯದಿ (!varref2p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!varref1p->same_Gate_ವೃಕ್ಷ(varref2p)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstConst* ಧ್ರುವ con1p = VN_CAST(sel1p->lsbp(), Const);
        ಯದಿ (!con1p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstConst* ಧ್ರುವ con2p = VN_CAST(sel2p->lsbp(), Const);
        ಯದಿ (!con2p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        ಯದಿ (!varNotReferenced(nodep->rhsp(), varref1p->varp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!varNotReferenced(nextp->rhsp(), varref2p->varp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // If a variable is marked split_var, access to the variable should not be merged.
        ಯದಿ (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Swap?
        ಯದಿ ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhs2p = nextp->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Assign* newp;
        ಯದಿ (lsbFirstAssign) {
            newp = nodep->cloneType(ನೂತನ AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    ನೂತನ AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } ಅನ್ಯಥಾ {
            newp = nodep->cloneType(ನೂತನ AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    ನೂತನ AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nextp->unlinkFrBack()), nextp);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ replaceAssignSFormatF(Ast_ಗಂಟು_Assign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        ಯದಿ (!VN_IS(nodep, Assign)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVarRef* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        ಯದಿ (!lhsp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstSFormatF* ಧ್ರುವ fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        ಯದಿ (!fmtp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(ನೂತನ AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ varNotReferenced(Ast_ಗಂಟು* nodep, AstVar* varp, ಪೂರ್ಣಾಙ್ಕ level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (level > 2) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!nodep->isPure()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // For example a $fgetc can't be reordered
        ಯದಿ (VN_IS(nodep, ಗಂಟು_Var_Ref) && VN_AS(nodep, ಗಂಟು_Var_Ref)->varp() == varp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    ತಾರ್ಕಿಕ replace_ಗಂಟು_Assign(Ast_ಗಂಟು_Assign* nodep) {
        ಯದಿ (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            ಯದಿ (VN_IS(nodep, AssignW)) {
                ಯದಿ (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Don't delete the assign, or V3Gate will freak out
            } ಅನ್ಯಥಾ {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        } ಅನ್ಯಥಾ ಯದಿ (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            ತಾರ್ಕಿಕ need_temp = ಅಸತ್ಯ;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ಧ್ರುವ ತಾರ್ಕಿಕ waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ಧ್ರುವ ತಾರ್ಕಿಕ needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            ಯದಿ (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<size_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ಧ್ರುವ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ಧ್ರುವ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    ಯದಿ (lhsVarIds.ಎಣಿಕೆ(m_ids(refp->varp()))) need_temp = ಸತ್ಯ;
                });
            }
            ಯದಿ (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* ಧ್ರುವ tempPurep = ನೂತನ AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* ಧ್ರುವ tempPureRefp
                    = ನೂತನ AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                Ast_ಗಂಟು_Assign* ಧ್ರುವ asnp
                    = VN_IS(nodep, AssignDly)
                          ? ನೂತನ AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(ನೂತನ AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                ಯದಿ (waits) ತಳ್ಳು_Deletep(nodep->timingControlp()->unlinkFrBack());
            } ಅನ್ಯಥಾ ಯದಿ (need_temp) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } ಅನ್ಯಥಾ {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* ಧ್ರುವ delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            ಯದಿ (delayp && !delayp->lhsp()->isPure()) {
                File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = delayp->lhsp()->unlinkFrBack();
                AstVar* ಧ್ರುವ tempp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswapNames.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempp);
                nodep->addHereThisAsNext(
                    ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(ನೂತನ AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhs2p = rhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            // Calc widths
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb2 = 0;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb2 = lsb2 + lc2p->width() - 1;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb1 = msb2 + 1;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb1 = lsb1 + lc1p->width() - 1;
            UASSERT_OBJ(msb1 == (conp->width() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ಧ್ರುವ sel1p = ನೂತನ AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ಧ್ರುವ sel2p = ನೂತನ AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_ಗಂಟು_Assign* newp = nullptr;
            ಯದಿ (!need_temp) {
                Ast_ಗಂಟು_Assign* ಧ್ರುವ asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_ಗಂಟು_Assign* ಧ್ರುವ asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_ಗಂಟು::addNext(newp, asn1ap);
                newp = Ast_ಗಂಟು::addNext(newp, asn2ap);
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ಧ್ರುವ temp1p
                    = ನೂತನ AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ಧ್ರುವ temp2p
                    = ನೂತನ AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->addStmtsp(temp1p);
                m_modp->addStmtsp(temp2p);
                Ast_ಗಂಟು_Assign* ಧ್ರುವ asn1ap = nodep->cloneType(
                    ನೂತನ AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_ಗಂಟು_Assign* ಧ್ರುವ asn2ap = nodep->cloneType(
                    ನೂತನ AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_ಗಂಟು_Assign* ಧ್ರುವ asn1bp = nodep->cloneType(
                    lc1p, ನೂತನ AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_ಗಂಟು_Assign* ಧ್ರುವ asn2bp = nodep->cloneType(
                    lc2p, ನೂತನ AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_ಗಂಟು::addNext(newp, asn1ap);
                newp = Ast_ಗಂಟು::addNext(newp, asn2ap);
                newp = Ast_ಗಂಟು::addNext(newp, asn1bp);
                newp = Ast_ಗಂಟು::addNext(newp, asn2bp);
            }
            ಯದಿ (debug() >= 9 && newp) newp->dump_ವೃಕ್ಷ_And_Next(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* ಧ್ರುವ cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = cvtp->fromp()->unlinkFrBack();
            ಯದಿ (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_Stream* ಧ್ರುವ streamp = VN_CAST(srcp, ಗಂಟು_Stream)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ streamSrcp = streamp->lhsp();
                ಯದಿ (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* ಧ್ರುವ streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = streamp->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_DType* ಧ್ರುವ srcDTypep = srcp->dtypep()->skipRefp();
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ಯದಿ (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                ಯದಿ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    ಪೂರ್ಣಾಙ್ಕ srcElementBits = 0;
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ಪೂರ್ಣಾಙ್ಕ dstElementBits = 0;
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = ನೂತನ AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), ಅಸತ್ಯ, 1,
                        dstElementBits,   srcElementBits};
                } ಅನ್ಯಥಾ {
                    srcp = ನೂತನ AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = ನೂತನ AstCvtArrayToPacked{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ packedBits = nodep->lhsp()->widthMin();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ unpackBits
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->widthMin();
                ಧ್ರುವ uint32_t offset = packedBits > unpackBits ? packedBits - unpackBits : 0;
                srcp = ನೂತನ AstShiftL{srcp->fileline(), srcp,
                                     ನೂತನ AstConst{srcp->fileline(), offset}, packedBits};
            }
            ಯದಿ (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sWidth = srcp->width();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ dWidth = nodep->lhsp()->width();
                ಯದಿ (sWidth < dWidth) {
                    AstExtend* ಧ್ರುವ extendp = ನೂತನ AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = ನೂತನ AstShiftL{
                        srcp->fileline(), extendp,
                        ನೂತನ AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* streamp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_DType* ಧ್ರುವ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = nodep->rhsp()->unlinkFrBack();
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            ಯದಿ ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                ಪೂರ್ಣಾಙ್ಕ blockSize = 1;
                ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    ಯದಿ (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                ಪೂರ್ಣಾಙ್ಕ srcElementBits = 0;
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                ಪೂರ್ಣಾಙ್ಕ dstElementBits = 0;
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(ನೂತನ AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, ಸತ್ಯ,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಯದಿ (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sWidth = srcp->width();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            ಯದಿ (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } ಅನ್ಯಥಾ {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            ಯದಿ (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = ನೂತನ AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } ಅನ್ಯಥಾ {
                ಯದಿ (dWidth == 0) {
                    streamp = ನೂತನ AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } ಅನ್ಯಥಾ ಯದಿ (sWidth >= dWidth) {
                    streamp = ನೂತನ AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ streamp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_DType* ಧ್ರುವ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            ಯದಿ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* origSrcp = srcp;
                ಯದಿ (AstCvtArrayToPacked* ಧ್ರುವ cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ origSrcDTypep = origSrcp->dtypep()->skipRefp();
                ಯದಿ (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    ಪೂರ್ಣಾಙ್ಕ srcElementBits = 0;
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ಪೂರ್ಣಾಙ್ಕ dstElementBits = 0;
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    ಯದಿ (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    ತಾರ್ಕಿಕ reverse = ಅಸತ್ಯ;
                    ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(ನೂತನ AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sWidth = srcp->width();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ dWidth = dstp->width();
            ಯದಿ (VN_IS(dstDTypep, UnpackArrayDType)) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                ಯದಿ (sWidth > dstBitWidth) {
                    srcp
                        = ನೂತನ AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = ನೂತನ AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } ಅನ್ಯಥಾ {
                ಯದಿ (dWidth == 0) {
                    srcp = ನೂತನ AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } ಅನ್ಯಥಾ ಯದಿ (sWidth >= dWidth) {
                    srcp = ನೂತನ AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } ಅನ್ಯಥಾ {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    ಯದಿ (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* ಧ್ರುವ extendp = ನೂತನ AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = ನೂತನ AstShiftL{
                            srcp->fileline(), extendp,
                            ನೂತನ AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } ಅನ್ಯಥಾ {  // if it is a queue we dont need do do a shiftL
                        srcp = ನೂತನ AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = streamp->lhsp();
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ಗಂಟು_DType* ಧ್ರುವ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ಯದಿ (lowerAsFixedAggregate(srcDTypep)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } ಅನ್ಯಥಾ ಯದಿ ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                ಯದಿ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    ಪೂರ್ಣಾಙ್ಕ blockSize = 1;
                    ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        ಯದಿ (VL_UNLIKELY(blockSize <= 0)) {
                            // Not reachable due to higher level checks when parsing stream
                            // operators commented out to not fail v3error-coverage-checks.
                            // nodep->v3error("Stream block size must be positive, got " <<
                            // blockSize); nodep->v3error("Stream block size must be positive, got
                            // " << blockSize);
                            blockSize = 1;
                        }
                    }
                    // Not reachable due to higher level checks when parsing stream operators
                    // commented out to not fail v3error-coverage-checks.
                    // else {
                    // nodep->v3error("Stream block size must be constant (got " <<
                    // streamp->rhsp()->prettyTypeName() << ")");
                    // }
                    ಪೂರ್ಣಾಙ್ಕ srcElementBits = 0;
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ಪೂರ್ಣಾಙ್ಕ dstElementBits = 0;
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = ನೂತನ AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     ಸತ್ಯ,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
                } ಅನ್ಯಥಾ {
                    streamp->lhsp(ನೂತನ AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (m_doV && replaceAssignMultiSel(nodep)) {
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (replaceAssignSFormatF(nodep)) {
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Boolean replacements
    ತಾರ್ಕಿಕ operandBoolShift(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        ಯದಿ (!VN_IS(nodep, And)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstShiftR* ಧ್ರುವ shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        ಯದಿ (!VN_IS(shiftp->rhsp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಶೂನ್ಯ replaceBoolShift(Ast_ಗಂಟು* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ಧ್ರುವ andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* ಧ್ರುವ shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3_ಸಂಖ್ಯೆ val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->ಅಂಕೆ(), shiftConstp->ಅಂಕೆ());
        AstAnd* ಧ್ರುವ newp
            = ನೂತನ AstAnd{nodep->fileline(), ನೂತನ AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    ಶೂನ್ಯ replaceWithSimulation(Ast_ಗಂಟು* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        ಯದಿ (!simvis.optimizable()) {
            ಧ್ರುವ Ast_ಗಂಟು* errorp = simvis.whyNotNodep();
            ಯದಿ (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } ಅನ್ಯಥಾ {
            // Fetch the result
            Ast_ಗಂಟು* ಧ್ರುವ valuep = simvis.fetch_ಮೌಲ್ಯ_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_ಗಂಟು* ಧ್ರುವ newp = valuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = ಅಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = ಅಸತ್ಯ;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ swapSides(Ast_ಗಂಟು_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    ತಾರ್ಕಿಕ containsMemberAccessRecurse(Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ size_t id = m_ids(nodep);
        ಧ್ರುವ auto it = m_containsMemberAccess.find(id);
        ಯದಿ (it != m_containsMemberAccess.end()) ಪ್ರತ್ಯಾಗಮ it->second;
        ++m_statMemberAccessVisits;
        ತಾರ್ಕಿಕ result = ಅಸತ್ಯ;
        ಯದಿ (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ funcRefp = VN_CAST(nodep, ಗಂಟು_FTask_Ref)) {
            ಯದಿ (containsMemberAccessRecurse(funcRefp->taskp())) result = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_CCall* ಧ್ರುವ funcRefp = VN_CAST(nodep, ಗಂಟು_CCall)) {
            ಯದಿ (containsMemberAccessRecurse(funcRefp->funcp())) result = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ funcp = VN_CAST(nodep, ಗಂಟು_FTask)) {
            // Assume that it has a member access
            ಯದಿ (funcp->recursive()) result = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = VN_CAST(nodep, CFunc)) {
            ಯದಿ (funcp->recursive()) result = ಸತ್ಯ;
        }
        ಯದಿ (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        ಯದಿ (!result && !VN_IS(nodep, ಗಂಟು_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = ಸತ್ಯ;
        }
        m_containsMemberAccess.emplace(id, result);
        ಪ್ರತ್ಯಾಗಮ result;
    }

    ತಾರ್ಕಿಕ matchBiopToBitwise(Ast_ಗಂಟು_Biop* ಧ್ರುವ nodep) {
        ಯದಿ (!m_convertLogicToBit) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!nodep->lhsp()->width1()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!nodep->rhsp()->width1()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!nodep->isPure()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (containsMemberAccessRecurse(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ಧ್ರುವ aRandp = VN_CAST(nodep->lhsp(), Rand);
        ಧ್ರುವ AstRand* ಧ್ರುವ bRandp = VN_CAST(nodep->rhsp(), Rand);
        ಯದಿ (!aRandp || !bRandp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!aRandp->combinable(bRandp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ಧ್ರುವ aRandp = VN_CAST(nodep->fromp(), Rand);
        ಯದಿ (!aRandp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (aRandp->seedp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        ಯದಿ (ಧ್ರುವ AstConsArray* ಧ್ರುವ consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            ಯದಿ (!(m_doExpensive || m_params)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (!consp->is_Constant_ವೃಕ್ಷ()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(nodep->lhsp(), Const)) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        replaceWithSimulation(nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪೂರ್ಣಾಙ್ಕ operandConcatMove(ಧ್ರುವ AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ಧ್ರುವ AstConcat* ಧ್ರುವ abConcp = VN_CAST(nodep->lhsp(), Concat);
        ಧ್ರುವ AstConcat* ಧ್ರುವ bcConcp = VN_CAST(nodep->rhsp(), Concat);
        ಯದಿ (!abConcp && !bcConcp) ಪ್ರತ್ಯಾಗಮ 0;
        ಯದಿ (bcConcp) {
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ap = nodep->lhsp();
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            ಯದಿ (VL_BITBIT_I(ap->width() + bp->width()) == 0) ಪ್ರತ್ಯಾಗಮ 2;  // Transform 2: to abConc
        } ಅನ್ಯಥಾ {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            ಯದಿ (VL_BITBIT_I(nodep->lhsp()->width()) != 0) ಪ್ರತ್ಯಾಗಮ 1;  // Transform 1: to bcConc
        }
        ಪ್ರತ್ಯಾಗಮ 0;  // ok
    }
    ಶೂನ್ಯ moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        ಯದಿ (operandConcatMove(nodep) > 1) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* ಧ್ರುವ bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bp = bcConcp->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* ಧ್ರುವ abConcp = ನೂತನ AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            ಯದಿ (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(bcConcp), bcConcp);
        } ಅನ್ಯಥಾ {
            AstConcat* ಧ್ರುವ abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ap = abConcp->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bp = abConcp->rhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* ಧ್ರುವ bcConcp = ನೂತನ AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            ಯದಿ (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    ಶೂನ್ಯ visit(AstConst*) override {}  // Already constant

    ಶೂನ್ಯ visit(AstCell* nodep) override {
        ಯದಿ (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_ಅಂಕೆ(nodep, AstMatchMasked::fold(constp->ಅಂಕೆ(), nodep->matchp()->varp()));
        }
    }

    ಶೂನ್ಯ visit(AstPin* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ಧ್ರುವ newp = ನೂತನ AstLogAnd{
            nodep->fileline(),
            ನೂತನ AstLogOr{nodep->fileline(), ನೂತನ AstLogNot{nodep->fileline(), lhsp}, rhsp},
            ನೂತನ AstLogOr{nodep->fileline(),
                         ನೂತನ AstLogNot{nodep->fileline(), rhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)},
                         lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ಧ್ರುವ belowp = VN_AS(nodep->fromp(), Sel);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = belowp->fromp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsb1p = nodep->lsbp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newlsbp;
        ಯದಿ (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = ನೂತನ AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(lsb2p), lsb2p);
        } ಅನ್ಯಥಾ {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = widep->width() > lsb2p->width()
                                          ? ನೂತನ AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = widep->width() > lsb1p->width()
                                          ? ನೂತನ AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = ನೂತನ AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ಧ್ರುವ newp = ನೂತನ AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ಧ್ರುವ conp = VN_AS(nodep->fromp(), Concat);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ conLhsp = conp->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ conRhsp = conp->rhsp();
        ಯದಿ (static_cast<ಪೂರ್ಣಾಙ್ಕ>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* ಧ್ರುವ newp
                = ನೂತನ AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } ಅನ್ಯಥಾ ಯದಿ (static_cast<ಪೂರ್ಣಾಙ್ಕ>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* ಧ್ರುವ newp
                = ನೂತನ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } ಅನ್ಯಥಾ {
            // Yuk, split between the two
            conRhsp->unlinkFrBack();
            conLhsp->unlinkFrBack();
            AstConcat* ಧ್ರುವ newp
                = ನೂತನ AstConcat{nodep->fileline(),
                                ನೂತನ AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->width() + 1},
                                ನೂತನ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->width() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ತಾರ್ಕಿಕ operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ಧ್ರುವ repp = VN_AS(nodep->fromp(), Replicate);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = repp->srcp();
        ಧ್ರುವ AstConst* ಧ್ರುವ lsbp = VN_CAST(nodep->lsbp(), Const);
        ಯದಿ (!lsbp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        ಯದಿ ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        //
        fromp->unlinkFrBack();
        AstSel* ಧ್ರುವ newp = ನೂತನ AstSel{
            nodep->fileline(), fromp,
            ನೂತನ AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ಧ್ರುವ rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ from2p = rep2p->srcp();
        ಧ್ರುವ AstConst* ಧ್ರುವ cnt1p = VN_CAST(nodep->countp(), Const);
        ಯದಿ (!cnt1p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstConst* ಧ್ರುವ cnt2p = VN_CAST(rep2p->countp(), Const);
        ಯದಿ (!cnt2p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        //
        from2p->unlinkFrBack();
        AstReplicate* ಧ್ರುವ newp
            = ನೂತನ AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        ಯದಿ (VN_IS(from1p, Replicate)) {
            ಧ್ರುವ AstConst* ಧ್ರುವ cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            ಯದಿ (!cnt1p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        ಯದಿ (VN_IS(from2p, Replicate)) {
            ಧ್ರುವ AstConst* ಧ್ರುವ cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            ಯದಿ (!cnt2p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        ಯದಿ (!operandsSame(from1p, from2p)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        //
        from1p->unlinkFrBack();
        AstReplicate* ಧ್ರುವ newp = ನೂತನ AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಶೂನ್ಯ replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_ಗಂಟು_Biop* ಧ್ರುವ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ಗಂಟು_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bilhsp = fromp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            ನೂತನ AstSel{nodep->fileline(), bilhsp, lsbp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ), nodep->widthConst()});
        fromp->rhsp(ನೂತನ AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_ಗಂಟು_Uniop* ಧ್ರುವ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ಗಂಟು_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(ನೂತನ AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        ಯದಿ (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        ಯದಿ (VN_IS(nodep->fromp(), Const)) {  // It did.
            ಯದಿ (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } ಅನ್ಯಥಾ {
                Ast_ಗಂಟು* ಧ್ರುವ fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ಧ್ರುವ AstConsArray* ಧ್ರುವ consp = VN_AS(nodep->fromp(), ConsArray);
            ಧ್ರುವ uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ itemp = consp->valuep(bit);
            ಯದಿ (VN_IS(itemp, Const)) {
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = VN_AS(itemp, Const)->ಅಂಕೆ();
                VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
        m_selp = nullptr;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    ಶೂನ್ಯ visit(AstSliceSel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        ಯದಿ (!m_required) ಪ್ರತ್ಯಾಗಮ;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        ಯದಿ (ಧ್ರುವ AstConsStruct* ಧ್ರುವ consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = consp->memberValuep(nodep->name());
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, constp->ಅಂಕೆ()), nodep);
            } ಅನ್ಯಥಾ {
                nodep->replaceWith(valuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }

    ಶೂನ್ಯ visit(AstCAwait* nodep) override {
        m_hasJumpDelay = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        ತಾರ್ಕಿಕ did = ಅಸತ್ಯ;
        ಯದಿ (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_ಗಂಟು* ಧ್ರುವ valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ಧ್ರುವ AstConsArray* ಧ್ರುವ consp = VN_CAST(valuep, ConsArray);
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            ಯದಿ (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                ಯದಿ (operandConst(valuep)) {
                    ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = VN_AS(valuep, Const)->ಅಂಕೆ();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ಧ್ರುವ string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ, origParamName), nodep);
                    did = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, itemp->ಅಂಕೆ()), nodep);
                    did = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_ಗಂಟು* ಧ್ರುವ newp = valuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    did = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_ಗಂಟು* ಧ್ರುವ newp = valuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    did = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_ಗಂಟು* ಧ್ರುವ newp = valuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    did = ಸತ್ಯ;
                }
            }
        }
        ಯದಿ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!Ast_ಗಂಟು::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    ಶೂನ್ಯ visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ತಾರ್ಕಿಕ did = ಅಸತ್ಯ;
        ಯದಿ (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ಧ್ರುವ AstEnumItem* ಧ್ರುವ itemp = nodep->itemp();
            ಯದಿ (m_recursing_Enum_ವಸ್ತುಗಳು.ಎಣಿಕೆ(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } ಅನ್ಯಥಾ {
                m_recursing_Enum_ವಸ್ತುಗಳು.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursing_Enum_ವಸ್ತುಗಳು.erase(itemp);
            }
            ಯದಿ (AstConst* ಧ್ರುವ valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = valuep->ಅಂಕೆ();
                VL_DO_DANGLING(replace_ಅಂಕೆ(nodep, ಅಂಕೆ), nodep);
                did = ಸತ್ಯ;
            }
        }
        ಯದಿ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    ತಾರ್ಕಿಕ only_Sen_Item_In_Sen_ವೃಕ್ಷ(ಧ್ರುವ AstSenItem* nodep) {
        // Only one if it's not in a list
        ಪ್ರತ್ಯಾಗಮ (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            ಯದಿ (nodep->isClocked()) {  // A constant can never get a pos/negedge
                ಯದಿ (only_Sen_Item_In_Sen_ವೃಕ್ಷ(nodep)) {
                    ಯದಿ (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            ನೂತನ AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } ಅನ್ಯಥಾ {
                        nodep->replaceWith(ನೂತನ AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಅನ್ಯಥಾ {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } ಅನ್ಯಥಾ {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(ನೂತನ AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        } ಅನ್ಯಥಾ ಯದಿ (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_ಗಂಟು* ಧ್ರುವ sensp = nodep->sensp();
            Ast_ಗಂಟು* lastSensp = sensp;
            ತಾರ್ಕಿಕ invert = ಅಸತ್ಯ;
            ಯಾವತ್ (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            ಯದಿ (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(sensp), sensp);
        }
    }

    ವರ್ಗ SenItemCmp final {
        ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ cmp(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) {
            ಧ್ರುವ VNType aType = ap->type();
            ಧ್ರುವ VNType bType = bp->type();
            ಯದಿ (aType != bType) ಪ್ರತ್ಯಾಗಮ static_cast<ಪೂರ್ಣಾಙ್ಕ>(bType) - static_cast<ಪೂರ್ಣಾಙ್ಕ>(aType);

            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ aRefp = VN_CAST(ap, VarRef)) {
                ಧ್ರುವ AstVarRef* ಧ್ರುವ bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                ಯದಿ (aRefp->name() < bRefp->name()) ಪ್ರತ್ಯಾಗಮ -1;
                ಯದಿ (aRefp->name() > bRefp->name()) ಪ್ರತ್ಯಾಗಮ 1;
                // But might be same name with different scopes
                ಯದಿ (aRefp->varScopep() < bRefp->varScopep()) ಪ್ರತ್ಯಾಗಮ -1;
                ಯದಿ (aRefp->varScopep() > bRefp->varScopep()) ಪ್ರತ್ಯಾಗಮ 1;
                // Or rarely, different data types
                ಯದಿ (aRefp->dtypep() < bRefp->dtypep()) ಪ್ರತ್ಯಾಗಮ -1;
                ಯದಿ (aRefp->dtypep() > bRefp->dtypep()) ಪ್ರತ್ಯಾಗಮ 1;
                ಪ್ರತ್ಯಾಗಮ 0;
            }

            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ aConstp = VN_CAST(ap, Const)) {
                ಧ್ರುವ AstConst* ಧ್ರುವ bConstp = VN_AS(bp, Const);
                ಯದಿ (aConstp->ಅಂಕೆ().isLtXZ(bConstp->ಅಂಕೆ())) ಪ್ರತ್ಯಾಗಮ -1;
                ಯದಿ (bConstp->ಅಂಕೆ().isLtXZ(aConstp->ಅಂಕೆ())) ಪ್ರತ್ಯಾಗಮ 1;
                ಪ್ರತ್ಯಾಗಮ 0;
            }

            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ aBiOpp = VN_CAST(ap, ಗಂಟು_Biop)) {
                ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ bBiOpp = VN_AS(bp, ಗಂಟು_Biop);
                // Compare RHSs first, as LHS might be const
                ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) ಪ್ರತ್ಯಾಗಮ c;
                ಪ್ರತ್ಯಾಗಮ cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            ಯದಿ (ಧ್ರುವ AstCMethodHard* ಧ್ರುವ aCallp = VN_CAST(ap, CMethodHard)) {
                ಧ್ರುವ AstCMethodHard* ಧ್ರುವ bCallp = VN_AS(bp, CMethodHard);
                ಯದಿ (aCallp->method() < bCallp->method()) ಪ್ರತ್ಯಾಗಮ -1;
                ಯದಿ (aCallp->method() > bCallp->method()) ಪ್ರತ್ಯಾಗಮ 1;
                ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ c = cmp(aCallp->fromp(), bCallp->fromp())) ಪ್ರತ್ಯಾಗಮ c;
                ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* aPinsp = aCallp->pinsp();
                ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bPinsp = bCallp->pinsp();
                ಯಾವತ್ (aPinsp && bPinsp) {
                    ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ c = cmp(aPinsp, bPinsp)) ಪ್ರತ್ಯಾಗಮ c;
                    aPinsp = VN_AS(aPinsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    bPinsp = VN_AS(bPinsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                }
                ಪ್ರತ್ಯಾಗಮ aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            ಪ್ರತ್ಯಾಗಮ 0;
        }

    ಸಾರ್ವಜನಿಕ:
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ AstSenItem* lhsp, ಧ್ರುವ AstSenItem* rhsp) ಧ್ರುವ {
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lSensp = lhsp->sensp();
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rSensp = rhsp->sensp();
            ಯದಿ (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ c = cmp(lSensp, rSensp)) ಪ್ರತ್ಯಾಗಮ c < 0;
            } ಅನ್ಯಥಾ ಯದಿ (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                ಪ್ರತ್ಯಾಗಮ rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            ಪ್ರತ್ಯಾಗಮ lhsp->edgeType() < rhsp->edgeType();
        }
    };

    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_doExpensive) {
            // UINFOTREE(1, nodep, "", "ssin");
            // Optimize ideas for the future:
            //   SENTREE(... SENGATE(x,a), SENGATE(SENITEM(x),b) ...)  => SENGATE(x,OR(a,b))

            //   SENTREE(... SENITEM(x),   SENGATE(SENITEM(x),*) ...)  => SENITEM(x)
            // Do we need the SENITEM's to be identical?  No because we're
            // ORing between them; we just need to ensure that the result is at
            // least as frequently activating.  So we
            // SENGATE(SENITEM(x)) -> SENITEM(x), then let it collapse with the
            // other SENITEM(x).

            // Pass 1: Sort the sensitivity items so "posedge a or b" and "posedge b or a" and
            // similar, optimizable expressions end up next to each other.
            ಪ್ರತಿ (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ಧ್ರುವ SenItemCmp cmp;
                ಯದಿ (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    ಪ್ರತಿ (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.ತಳ್ಳು_back(senp);
                    }
                    stable_sort(vec.begin(), vec.end(), SenItemCmp());
                    ಪ್ರತಿ (ಧ್ರುವ auto& ip : vec) ip->unlinkFrBack();
                    ಪ್ರತಿ (ಧ್ರುವ auto& ip : vec) nodep->addSensesp(ip);
                    ಭಙ್ಗ;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            ಪ್ರತಿ (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                ಯದಿ (!nextp) ಭಙ್ಗ;
                AstSenItem* ಧ್ರುವ lItemp = senp;
                AstSenItem* ಧ್ರುವ rItemp = nextp;
                ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lSenp = lItemp->sensp();
                ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rSenp = rItemp->sensp();
                ಯದಿ (!lSenp || !rSenp) ಅನುವರ್ತ;

                ಯದಿ (lSenp->same_Gate_ವೃಕ್ಷ(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    ಯದಿ (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    ಯದಿ (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->timingControlp()) m_hasJumpDelay = ಸತ್ಯ;
        ಯದಿ (m_doNConst && replace_ಗಂಟು_Assign(nodep)) ಪ್ರತ್ಯಾಗಮ;
    }
    ಶೂನ್ಯ visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ಶೂನ್ಯ visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_doNConst && replace_ಗಂಟು_Assign(nodep)) ಪ್ರತ್ಯಾಗಮ;
        // Process containing this AssignW as single body statement
        AstAlways* ಧ್ರುವ procp = VN_CAST(nodep->backp(), Always);
        ಯದಿ (!procp || procp->stmtsp() != nodep || nodep->nextp()) ಪ್ರತ್ಯಾಗಮ;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(nodep->lhsp(), VarRef);
        ಯದಿ (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            AstInitial* ಧ್ರುವ newinitp = ನೂತನ AstInitial{flp, ನೂತನ AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_ಗಂಟು* ಧ್ರುವ initvaluep = exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            varrefp->varp()->isConst(ಸತ್ಯ);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    ಶೂನ್ಯ visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!m_doNConst) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        ಯದಿ (!bdtypep) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!bdtypep->isZeroInit()) ಪ್ರತ್ಯಾಗಮ;
        AstConst* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), V3_ಸಂಖ್ಯೆ{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ಧ್ರುವ fromp = VN_CAST(nodep->fromp(), ConsArray);
        ಯದಿ (!fromp) ಪ್ರತ್ಯಾಗಮ;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ಧ್ರುವ ತಾರ್ಕಿಕ reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ಧ್ರುವ uint64_t lastIndex = toArrayp->elementsConst() - 1;
        Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = toArrayp->subDTypep();
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ಧ್ರುವ auto convert = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            valuep->unlinkFrBack();
            ಯದಿ (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) ಪ್ರತ್ಯಾಗಮ valuep;
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ಧ್ರುವ newp
            = ನೂತನ AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : nullptr};
        ಪ್ರತಿ (ಧ್ರುವ AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? lastIndex - itemp->index() : itemp->index(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    ಶೂನ್ಯ visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_ಗಂಟು_Stream* streamp = nullptr;
        ತಾರ್ಕಿಕ isReverse = ಅಸತ್ಯ;
        ಯದಿ (AstStreamL* ಧ್ರುವ streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = ಸತ್ಯ;  // StreamL reverses the operation
        } ಅನ್ಯಥಾ ಯದಿ (AstStreamR* ಧ್ರುವ streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = ಅಸತ್ಯ;  // StreamR doesn't reverse the operation
        }
        ಯದಿ (streamp) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = streamp->lhsp();
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ಗಂಟು_DType* ಧ್ರುವ dstDTypep = nodep->dtypep()->skipRefp();
            ಯದಿ (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                ಪೂರ್ಣಾಙ್ಕ blockSize = 1;
                ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    ಯದಿ (VL_UNLIKELY(blockSize <= 0)) {
                        // Not reachable due to higher level checks when parsing stream
                        // operators commented out to not fail v3error-coverage-checks.
                        // nodep->v3error("Stream block size must be positive, got " <<
                        // blockSize);
                        blockSize = 1;
                    }
                }
                // Not reachable due to higher level checks when parsing stream operators
                // commented out to not fail v3error-coverage-checks.
                // else {
                //    nodep->v3error("Stream block size must be constant (got " <<
                //    streamp->rhsp()->prettyTypeName() << ")");
                // }
                ಪೂರ್ಣಾಙ್ಕ srcElementBits = 0;
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                ಪೂರ್ಣಾಙ್ಕ dstElementBits = 0;
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ನೂತನ AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamp), streamp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }
    ಶೂನ್ಯ visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        ಯದಿ (AstConcat* ಧ್ರುವ concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            AstRelease* ಧ್ರುವ newLp = ನೂತನ AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* ಧ್ರುವ newRp = ನೂತನ AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_doNConst) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->condp(), Const)) {
                Ast_ಗಂಟು* keepp = nullptr;
                Ast_ಗಂಟು* delp = nullptr;
                ಯದಿ (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } ಅನ್ಯಥಾ ಯದಿ (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } ಅನ್ಯಥಾ {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                ಯದಿ (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                ಯದಿ (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } ಅನ್ಯಥಾ {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_ವೃಕ್ಷ();
            } ಅನ್ಯಥಾ ಯದಿ (!Ast_ಗಂಟು::afterCommentp(nodep->thensp())
                       && !Ast_ಗಂಟು::afterCommentp(nodep->elsesp())) {
                ಯದಿ (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, ಸತ್ಯ);
                    Ast_ಗಂಟು_Stmt* ಧ್ರುವ newp
                        = ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಅನ್ಯಥಾ {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } ಅನ್ಯಥಾ ಯದಿ (!Ast_ಗಂಟು::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = nodep->condp();
                Ast_ಗಂಟು* ಧ್ರುವ elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                ಯದಿ (nodep->thensp()) {  // Must have been comment
                    ತಳ್ಳು_Deletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(ನೂತನ AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } ಅನ್ಯಥಾ ಯದಿ (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp
                    = VN_AS(nodep->condp(), ಗಂಟು_Uniop)->lhsp()->unlinkFrBackWithNext();
                Ast_ಗಂಟು* ಧ್ರುವ thensp = nodep->thensp()->unlinkFrBackWithNext();
                Ast_ಗಂಟು* ಧ್ರುವ elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_ಗಂಟು_Assign* ಧ್ರುವ thensp = VN_AS(nodep->thensp(), ಗಂಟು_Assign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_ಗಂಟು_Assign* ಧ್ರುವ elsesp = VN_AS(nodep->elsesp(), ಗಂಟು_Assign);
                thensp->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = nodep->condp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ truep = thensp->rhsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(ನೂತನ AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (ಅಸತ್ಯ  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_ಗಂಟು_If* ಧ್ರುವ lowerIfp = VN_AS(nodep->thensp(), ಗಂಟು_If);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = nodep->condp()->unlinkFrBack();
                Ast_ಗಂಟು* ಧ್ರುವ lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(ನೂತನ AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(lowerIfp), lowerIfp);
            } ಅನ್ಯಥಾ {
                // Optimizations that don't reform the IF itself
                ಯದಿ (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    ಶೂನ್ಯ visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        ಯದಿ (stmtDisplayDisplay(nodep)) ಪ್ರತ್ಯಾಗಮ;
    }
    ತಾರ್ಕಿಕ stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        ಯದಿ (!m_modp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Don't optimize under single statement
        AstDisplay* ಧ್ರುವ prevp = VN_CAST(nodep->backp(), Display);
        ಯದಿ (!prevp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_ವೃಕ್ಷ(nodep->filep())))
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstSFormatF* ಧ್ರುವ pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* ಧ್ರುವ nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        ಯದಿ (nformatp->scopeNamep() && pformatp->scopeNamep()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        ಯದಿ (!m_doCpp && pformatp->exprsp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!m_doCpp && nformatp->exprsp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Avoid huge merges
        ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ DISPLAY_MAX_MERGE_LENGTH = 500;
        ಯದಿ (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        ಯದಿ (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        ಯದಿ (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        ಯದಿ (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        ಯದಿ (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        ಯದಿ (AstScopeName* ಧ್ರುವ scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterateAndNextNull(nodep->exprp());
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        ಯದಿ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            ಯದಿ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->ಅಂಕೆ().isFromString()) {
                AstConst* ಧ್ರುವ fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->ಅಂಕೆ().toString());
                nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2(ಅಸತ್ಯ);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(fmtp), fmtp);
            }
        }
        ಯದಿ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) ಪ್ರತ್ಯಾಗಮ;  // Runtime, unfortunately  (unless constify format later)
        ತಾರ್ಕಿಕ anyconst = ಅಸತ್ಯ;
        ಪ್ರತಿ (Ast_ಗಂಟು* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ಧ್ರುವ fargp = VN_CAST(argp, SFormatArg);
            Ast_ಗಂಟು* ಧ್ರುವ subargp = fargp ? fargp->exprp() : argp;
            ಯದಿ (VN_IS(subargp, Const)) {
                anyconst = ಸತ್ಯ;
                ಭಙ್ಗ;
            }
        }
        ಯದಿ (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            ತಾರ್ಕಿಕ inPct = ಅಸತ್ಯ;
            Ast_ಗಂಟು* argp = nodep->exprsp();
            ಧ್ರುವ string text = nodep->text();
            ಪ್ರತಿ (ವರ್ಣ ch : text) {
                ಯದಿ (!inPct && ch == '%') {
                    inPct = ಸತ್ಯ;
                    fmt = ch;
                } ಅನ್ಯಥಾ ಯದಿ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ಅನ್ಯಥಾ ಯದಿ (inPct) {
                    inPct = ಅಸತ್ಯ;
                    fmt += ch;
                    ch = std::tolower(ch);
                    ವಿಕಲ್ಪ (ch) {
                    ಪಕ್ಷ '%': ಭಙ್ಗ;  // %% - still %%
                    ಪಕ್ಷ 'm': ಭಙ್ಗ;  // %m - still %m - auto insert "name"
                    ಪಕ್ಷ 'l': ಭಙ್ಗ;  // %l - still %l - auto insert "library"
                    ಪಕ್ಷ 't':  // %t - don't know $timeformat so can't constify
                        ಯದಿ (argp) argp = argp->nextp();
                        ಭಙ್ಗ;
                    ಸ್ವಭಾವ:  // Most operators, just move to next argument
                        ಯದಿ (argp) {
                            Ast_ಗಂಟು* ಧ್ರುವ nextp = argp->nextp();
                            AstSFormatArg* ಧ್ರುವ fargp = VN_CAST(argp, SFormatArg);
                            Ast_ಗಂಟು* ಧ್ರುವ subargp = fargp ? fargp->exprp() : argp;
                            ಧ್ರುವ VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            ಯದಿ (VN_IS(subargp, Const)) {  // Convert it
                                ಧ್ರುವ string out
                                    = formatAttr.isEnum()
                                          ? const_ಅಂಕೆ_V(subargp).displayedEnum(fargp, fmt)
                                          : const_ಅಂಕೆ_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(ತಳ್ಳು_Deletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        ಭಙ್ಗ;
                    }  // switch
                    newFormat += fmt;
                } ಅನ್ಯಥಾ {
                    newFormat += ch;
                }
            }
            ಯದಿ (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        ಯದಿ (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (VN_IS(nodep->backp(), ಗಂಟು_Assign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
    }
    ಶೂನ್ಯ visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!m_doNConst) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ಧ್ರುವ fmtp = nodep->fmtp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp();
            AstBasicDType* ಧ್ರುವ basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            ಯದಿ (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* ಧ್ರುವ strp
                    = ನೂತನ AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(ನೂತನ AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        ಯದಿ (nodep->recursive()) m_underRecFunc = ಸತ್ಯ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = ಸತ್ಯ;  // As don't analyze inside tasks for timing controls
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = ಸತ್ಯ;  // As don't analyze inside tasks for timing controls
    }
    ಶೂನ್ಯ visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_ಗಂಟು_FTask_Ref*>(nodep));
        ಯದಿ (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        ಧ್ರುವ ತಾರ್ಕಿಕ oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = ಅಸತ್ಯ;
        m_hasLoopTest = ಅಸತ್ಯ;
        iterateChildren(nodep);
        ಧ್ರುವ ತಾರ್ಕಿಕ thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        ಯದಿ (ಧ್ರುವ AstLoopTest* ಧ್ರುವ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            ಯದಿ (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        // If last statement always breaks, repalce loop with body
        ಯದಿ (Ast_ಗಂಟು* lastp = nodep->stmtsp()) {
            ಯಾವತ್ (Ast_ಗಂಟು* ಧ್ರುವ nextp = lastp->nextp()) lastp = nextp;
            ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(lastp, LoopTest)) {
                ಯದಿ (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ತಳ್ಳು_Deletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
        // Warn on infinite loop
        ಯದಿ (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, ಸತ್ಯ);  // Complain once
        }
    }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        ಯದಿ (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        m_hasLoopTest = ಸತ್ಯ;

        // If always breaks, subsequent statements are dead code, delete them
        ಯದಿ (nodep->condp()->isZero()) {
            Ast_ಗಂಟು* ಧ್ರುವ nextp = nodep->nextp();
            ಯದಿ (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    ಶೂನ್ಯ visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Pre_Sel*) override {}

    // Ignored, can eliminate early
    ಶೂನ್ಯ visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_doNConst) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        ಯದಿ (!nodep->exprp()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Remove if expression is trivially pure
        ಯದಿ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Remove calls to empty functions
        ಯದಿ (ಧ್ರುವ AstCCall* ಧ್ರುವ callp = VN_CAST(nodep->exprp(), CCall)) {
            ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = callp->funcp();
            ಯದಿ (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // TODO if there's an ExprStmt underneath just keep lower statements
        // (No current test case needs this)
        // TODO if non-pure, can remove. First need to clean up that many expressions used
        // under this node do not yet properly indicate non-pure.
        // Also, under here, a NodeUniOp/NodeBiOp that is pure (but child of such is not
        // pure) can be peeled off until get to the non-pure child-node expression,
        // because all that is needed is to execute what causes the side effect.
    }

    // Simplify
    ಶೂನ್ಯ visit(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    ಶೂನ್ಯ visit(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        ಯದಿ (nodep->nextp() && canDeleteStatements()) {
            ತಳ್ಳು_Deletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        ಯದಿ (!nodep->nextp()) {
            ಪ್ರತಿ (Ast_ಗಂಟು* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                ಯದಿ (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                ಯದಿ (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) ಭಙ್ಗ;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        ಯದಿ (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // If no JumpGo points to this node, replace it with its body
        ಯದಿ (!m_usedJumpBlocks.ಎಣಿಕೆ(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    ವೃಕ್ಷ_SKIP_VISIT("ArraySel");
    ವೃಕ್ಷ_SKIP_VISIT("CAwait");

    //-----
    //  "AstNODETYPE {             # bracket not paren
    //                $accessor_name, ...
    //                             # .castFoo is the test VN_IS(object,Foo)
    //                             # ,, gets replaced with a , rather than &&
    //                DISABLE_BASE # Turnes off checking Ast's base class treeops
    //               }"            # bracket not paren
    //    ,"what to call"
    //
    // Where "what_to_call" is:
    //          "function to call"
    //          "AstREPLACEMENT_TYPE{ $accessor }"
    //          "!              # Print line number when matches, so can see operations
    //          "NEVER"         # Print error message
    //          "DONE"          # Process of matching did the transform already

    // In the future maybe support more complicated match & replace:
    //   ("AstOr  {%a, AstAnd{AstNot{%b}, %c}} if %a.width1 if %a==%b", "AstOr{%a,%c}; %b.delete");
    // Lhs/rhs would be implied; for non-math operations you'd need $lhsp etc.

    //    v--- * * This op done on Verilog or C+++ mode, in all non-m_doConst stages
    //    v--- *1* These ops are always first, as we warn before replacing
    //    v--- *C* This op is a (C)++ op, only in m_doCpp mode
    //    v--- *V* This op is a (V)erilog op, only in m_doV mode
    //    v--- *A* This op works on (A)ll constant children, allowed in m_doConst mode
    //    v--- *S* This op specifies a type should use (S)hort-circuiting of its lhs op

    TREEOP1("AstSel{warnSelect(nodep)}",        "NEVER");
    // Generic constants on both side.  Do this first to avoid other replacements
    TREEOPA("AstNodeBiop {$lhsp.castConst, $rhsp.castConst, nodep->isPredictOptimizable()}",  "replaceConst(nodep)");
    TREEOPA("AstNodeUniop{$lhsp.castConst, nodep->isPredictOptimizable()}",  "replaceConst(nodep)");
    TREEOPA("AstNodeQuadop{$lhsp.castConst, $rhsp.castConst, $thsp.castConst, $fhsp.castConst}",  "replaceConst(nodep)");
    // Zero on one side or the other
    TREEOP ("AstAdd   {$lhsp.isZero, $rhsp}",   "replaceWRhs(nodep)");
    TREEOP ("AstAnd   {$lhsp.isZero, $rhsp, $rhsp.isPure}",   "replaceZero(nodep)");  // Can't use replaceZeroChkPure as we make this pattern in ChkPure
    // Masking that always yields zero
    TREEOP ("AstAnd   {$lhsp.castConst, matchMaskedZero(nodep)}", "replaceZeroChkPure(nodep, $rhsp)");
    // This visit function here must allow for short-circuiting.
    TREEOPS("AstLogAnd   {$lhsp.isZero}",       "replaceZero(nodep)");
    TREEOP ("AstLogAnd{$lhsp.isZero, $rhsp}",   "replaceZero(nodep)");
    // This visit function here must allow for short-circuiting.
    TREEOPS("AstLogOr   {$lhsp.isOne}",         "replaceNum(nodep, 1)");
    TREEOP ("AstLogOr {$lhsp.isZero, $rhsp}",   "replaceWRhsBool(nodep)");
    TREEOP ("AstDiv   {$lhsp.isZero, $rhsp}",   "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstDivS  {$lhsp.isZero, $rhsp}",   "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstMul   {$lhsp.isZero, $rhsp}",   "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstMulS  {$lhsp.isZero, $rhsp}",   "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstPow   {$rhsp.isZero}",          "replaceNum(nodep, 1)");  // Overrides lhs zero rule
    TREEOP ("AstPowSS {$rhsp.isZero}",          "replaceNum(nodep, 1)");  // Overrides lhs zero rule
    TREEOP ("AstPowSU {$rhsp.isZero}",          "replaceNum(nodep, 1)");  // Overrides lhs zero rule
    TREEOP ("AstPowUS {$rhsp.isZero}",          "replaceNum(nodep, 1)");  // Overrides lhs zero rule
    TREEOP ("AstOr    {$lhsp.isZero, $rhsp}",   "replaceWRhs(nodep)");
    TREEOP ("AstShiftL    {$lhsp.isZero, $rhsp}",  "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstShiftLOvr {$lhsp.isZero, $rhsp}",  "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstShiftR    {$lhsp.isZero, $rhsp}",  "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstShiftROvr {$lhsp.isZero, $rhsp}",  "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstShiftRS   {$lhsp.isZero, $rhsp}",  "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstShiftRSOvr{$lhsp.isZero, $rhsp}",  "replaceZeroChkPure(nodep,$rhsp)");
    TREEOP ("AstXor   {$lhsp.isZero, $rhsp}",   "replaceWRhs(nodep)");
    TREEOP ("AstSub   {$lhsp.isZero, $rhsp}",   "AstNegate{$rhsp}");
    TREEOP ("AstAdd   {$lhsp, $rhsp.isZero}",   "replaceWLhs(nodep)");
    TREEOP ("AstAnd   {$lhsp, $rhsp.isZero}",   "replaceZeroChkPure(nodep,$lhsp)");
    TREEOP ("AstLogAnd{$lhsp, $rhsp.isZero}",   "replaceZeroChkPure(nodep,$lhsp)");
    TREEOP ("AstLogOr {$lhsp, $rhsp.isZero}",   "replaceWLhsBool(nodep)");
    TREEOP ("AstMul   {$lhsp, $rhsp.isZero}",   "replaceZeroChkPure(nodep,$lhsp)");
    TREEOP ("AstMulS  {$lhsp, $rhsp.isZero}",   "replaceZeroChkPure(nodep,$lhsp)");
    TREEOP ("AstOr    {$lhsp, $rhsp.isZero}",   "replaceWLhs(nodep)");
    TREEOP ("AstShiftL    {$lhsp, $rhsp.isZero}",  "replaceWLhs(nodep)");
    TREEOP ("AstShiftLOvr {$lhsp, $rhsp.isZero}",  "replaceWLhs(nodep)");
    TREEOP ("AstShiftR    {$lhsp, $rhsp.isZero}",  "replaceWLhs(nodep)");
    TREEOP ("AstShiftROvr {$lhsp, $rhsp.isZero}",  "replaceWLhs(nodep)");
    TREEOP ("AstShiftRS   {$lhsp, $rhsp.isZero}",  "replaceWLhs(nodep)");
    TREEOP ("AstShiftRSOvr{$lhsp, $rhsp.isZero}",  "replaceWLhs(nodep)");
    TREEOP ("AstSub   {$lhsp, $rhsp.isZero}",   "replaceWLhs(nodep)");
    TREEOP ("AstXor   {$lhsp, $rhsp.isZero}",   "replaceWLhs(nodep)");
    // Non-zero on one side or the other
    TREEOP ("AstAnd   {$lhsp.isAllOnes, $rhsp}",        "replaceWRhs(nodep)");
    TREEOP ("AstLogAnd{$lhsp.isNeqZero, $rhsp}",        "replaceWRhsBool(nodep)");
    TREEOP ("AstOr    {$lhsp.isAllOnes, $rhsp, $rhsp.isPure}",        "replaceWLhs(nodep)");  // ->allOnes
    TREEOP ("AstLogOr {$lhsp.isNeqZero, $rhsp}",        "replaceNum(nodep,1)");
    TREEOP ("AstAnd   {$lhsp, $rhsp.isAllOnes}",        "replaceWLhs(nodep)");
    TREEOP ("AstLogAnd{$lhsp, $rhsp.isNeqZero}",        "replaceWLhsBool(nodep)");
    TREEOP ("AstOr    {$lhsp, $rhsp.isAllOnes, $lhsp.isPure}",        "replaceWRhs(nodep)");  // ->allOnes
    TREEOP ("AstLogOr {$lhsp, $rhsp.isNeqZero, $lhsp.isPure, nodep->isPure()}",        "replaceNum(nodep,1)");
    TREEOP ("AstXor   {$lhsp.isAllOnes, $rhsp}",        "AstNot{$rhsp}");
    TREEOP ("AstMul   {$lhsp.isOne, $rhsp}",    "replaceWRhs(nodep)");
    TREEOP ("AstMulS  {$lhsp.isOne, $rhsp}",    "replaceWRhs(nodep)");
    TREEOP ("AstDiv   {$lhsp, $rhsp.isOne}",    "replaceWLhs(nodep)");
    TREEOP ("AstDivS  {$lhsp, $rhsp.isOne}",    "replaceWLhs(nodep)");
    TREEOP ("AstMul   {operandIsPowTwo($lhsp), operandsSameWidth($lhsp,,$rhsp)}", "replaceMulShift(nodep)");  // a*2^n -> a<<n
    TREEOP ("AstDiv   {$lhsp, operandIsPowTwo($rhsp)}", "replaceDivShift(nodep)");  // a/2^n -> a>>n
    TREEOP ("AstModDiv{$lhsp, operandIsPowTwo($rhsp)}", "replaceModAnd(nodep)");  // a % 2^n -> a&(2^n-1)
    TREEOP ("AstPow   {operandIsTwo($lhsp), !$rhsp.isZero}",    "replacePowShift(nodep)");  // 2**a == 1<<a
    TREEOP ("AstPowSU {operandIsTwo($lhsp), !$rhsp.isZero}",    "replacePowShift(nodep)");  // 2**a == 1<<a
    TREEOP ("AstSub   {$lhsp.castAdd, operandSubAdd(nodep)}", "AstAdd{AstSub{$lhsp->castAdd()->lhsp(),$rhsp}, $lhsp->castAdd()->rhsp()}");  // ((a+x)-y) -> (a+(x-y))
    TREEOPC("AstAnd   {$lhsp.isOne, matchRedundantClean(nodep)}", "DONE")  // 1 & (a == b) -> (IData)(a == b)
    // Trinary ops
    // Note V3Case::Sel requires Cond to always be conditionally executed in C to prevent core dump!
    TREEOP ("AstCond{$condp.isZero,       $thenp, $elsep}", "replaceWChild(nodep,$elsep)");
    TREEOP ("AstCond{$condp.isNeqZero,    $thenp, $elsep}", "replaceWChild(nodep,$thenp)");
    TREEOPA("AstCond{$condp.isZero,       $thenp.castConst, $elsep.castConst}", "replaceWChild(nodep,$elsep)");
    TREEOPA("AstCond{$condp.isNeqZero,    $thenp.castConst, $elsep.castConst}", "replaceWChild(nodep,$thenp)");
    TREEOP ("AstCond{$condp, operandsSame($thenp,,$elsep)}","replaceWChild(nodep,$thenp)");
    TREEOP ("AstCond{$condp, matchCondCond(nodep)}", "DONE")  // Same condition then skip
    // This visit function here must allow for short-circuiting.
    TREEOPS("AstCond{$condp.isZero}",           "replaceWIteratedThs(nodep)");
    TREEOPS("AstCond{$condp.isNeqZero}",        "replaceWIteratedRhs(nodep)");
    TREEOP ("AstCond{$condp.castNot, $thenp, $elsep}", "AstCond{$condp->castNot()->lhsp(), $elsep, $thenp}");
    TREEOP ("AstCond{$condp.width1, $thenp.width1, $thenp.isAllOnes, $elsep}", "AstLogOr {$condp, $elsep}");  // a?1:b == a||b
    TREEOP ("AstCond{$condp.width1, $thenp.width1, $thenp, $elsep.isZero, !$elsep.isClassHandleValue}", "AstLogAnd{$condp, $thenp}");  // a?b:0 == a&&b
    TREEOP ("AstCond{$condp.width1, $thenp.width1, $thenp, $elsep.isAllOnes}", "AstLogOr {AstNot{$condp}, $thenp}");  // a?b:1 == ~a||b
    TREEOP ("AstCond{$condp.width1, $thenp.width1, $thenp.isZero, !$thenp.isClassHandleValue, $elsep}", "AstLogAnd{AstNot{$condp}, $elsep}");  // a?0:b == ~a&&b
    TREEOP ("AstCond{!$condp.width1, operandBoolShift(nodep->condp())}", "replaceBoolShift(nodep->condp())");
    // Prefer constants on left, since that often needs a shift, it lets
    // constant red remove the shift
    TREEOP ("AstNodeBiCom{!$lhsp.castConst, $rhsp.castConst}",  "swapSides(nodep)");
    TREEOP ("AstNodeBiComAsv{operandAsvConst(nodep)}",  "replaceAsv(nodep)");
    TREEOP ("AstNodeBiComAsv{operandAsvSame(nodep)}",   "replaceAsv(nodep)");
    TREEOP ("AstNodeBiComAsv{operandAsvLUp(nodep)}",    "replaceAsvLUp(nodep)");
    TREEOP ("AstNodeBiComAsv{operandAsvRUp(nodep)}",    "replaceAsvRUp(nodep)");
    TREEOP ("AstLt   {!$lhsp.castConst,$rhsp.castConst}",       "AstGt  {$rhsp,$lhsp}");
    TREEOP ("AstLtS  {!$lhsp.castConst,$rhsp.castConst}",       "AstGtS {$rhsp,$lhsp}");
    TREEOP ("AstLte  {!$lhsp.castConst,$rhsp.castConst}",       "AstGte {$rhsp,$lhsp}");
    TREEOP ("AstLteS {!$lhsp.castConst,$rhsp.castConst}",       "AstGteS{$rhsp,$lhsp}");
    TREEOP ("AstGt   {!$lhsp.castConst,$rhsp.castConst}",       "AstLt  {$rhsp,$lhsp}");
    TREEOP ("AstGtS  {!$lhsp.castConst,$rhsp.castConst}",       "AstLtS {$rhsp,$lhsp}");
    TREEOP ("AstGte  {!$lhsp.castConst,$rhsp.castConst}",       "AstLte {$rhsp,$lhsp}");
    TREEOP ("AstGteS {!$lhsp.castConst,$rhsp.castConst}",       "AstLteS{$rhsp,$lhsp}");
    //    v--- *1* as These ops are always first, as we warn before replacing
    TREEOP1("AstLt   {$lhsp, $rhsp.isZero}",            "replaceNumSigned(nodep,0)");
    TREEOP1("AstGte  {$lhsp, $rhsp.isZero}",            "replaceNumSigned(nodep,1)");
    TREEOP1("AstGt   {$lhsp.isZero, $rhsp}",            "replaceNumSigned(nodep,0)");
    TREEOP1("AstLte  {$lhsp.isZero, $rhsp}",            "replaceNumSigned(nodep,1)");
    TREEOP1("AstGt   {$lhsp, $rhsp.isAllOnes, $lhsp->width()==$rhsp->width()}",  "replaceNumLimited(nodep,0)");
    TREEOP1("AstLte  {$lhsp, $rhsp.isAllOnes, $lhsp->width()==$rhsp->width()}",  "replaceNumLimited(nodep,1)");
    TREEOP1("AstLt   {$lhsp.isAllOnes, $rhsp, $lhsp->width()==$rhsp->width()}",  "replaceNumLimited(nodep,0)");
    TREEOP1("AstGte  {$lhsp.isAllOnes, $rhsp, $lhsp->width()==$rhsp->width()}",  "replaceNumLimited(nodep,1)");
    // Two level bubble pushing
    TREEOP ("AstNot   {$lhsp.castNot,  $lhsp->width()==VN_AS($lhsp,,Not)->lhsp()->width()}", "replaceWChild(nodep, $lhsp->castNot()->lhsp())");  // NOT(NOT(x))->x
    TREEOP ("AstLogNot{$lhsp.castLogNot}",              "replaceWChild(nodep, $lhsp->castLogNot()->lhsp())");  // LOGNOT(LOGNOT(x))->x
    TREEOPV("AstNot   {$lhsp.castEqCase, $lhsp.width1}","AstNeqCase{$lhsp->castEqCase()->lhsp(),$lhsp->castEqCase()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castEqCase}",              "AstNeqCase{$lhsp->castEqCase()->lhsp(),$lhsp->castEqCase()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castNeqCase, $lhsp.width1}","AstEqCase{$lhsp->castNeqCase()->lhsp(),$lhsp->castNeqCase()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castNeqCase}",             "AstEqCase {$lhsp->castNeqCase()->lhsp(),$lhsp->castNeqCase()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castEqWild, $lhsp.width1}","AstNeqWild{$lhsp->castEqWild()->lhsp(),$lhsp->castEqWild()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castEqWild}",              "AstNeqWild{$lhsp->castEqWild()->lhsp(),$lhsp->castEqWild()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castNeqWild, $lhsp.width1}","AstEqWild{$lhsp->castNeqWild()->lhsp(),$lhsp->castNeqWild()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castNeqWild}",             "AstEqWild {$lhsp->castNeqWild()->lhsp(),$lhsp->castNeqWild()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castEq, $lhsp.width1}",    "AstNeq {$lhsp->castEq()->lhsp(),$lhsp->castEq()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castEq}",                  "AstNeq {$lhsp->castEq()->lhsp(),$lhsp->castEq()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castNeq, $lhsp.width1}",   "AstEq  {$lhsp->castNeq()->lhsp(),$lhsp->castNeq()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castNeq}",                 "AstEq  {$lhsp->castNeq()->lhsp(),$lhsp->castNeq()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castLt, $lhsp.width1}",    "AstGte {$lhsp->castLt()->lhsp(),$lhsp->castLt()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castLt}",                  "AstGte {$lhsp->castLt()->lhsp(),$lhsp->castLt()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castLtS, $lhsp.width1}",   "AstGteS{$lhsp->castLtS()->lhsp(),$lhsp->castLtS()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castLtS}",                 "AstGteS{$lhsp->castLtS()->lhsp(),$lhsp->castLtS()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castLte, $lhsp.width1}",   "AstGt  {$lhsp->castLte()->lhsp(),$lhsp->castLte()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castLte}",                 "AstGt  {$lhsp->castLte()->lhsp(),$lhsp->castLte()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castLteS, $lhsp.width1}",  "AstGtS {$lhsp->castLteS()->lhsp(),$lhsp->castLteS()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castLteS}",                "AstGtS {$lhsp->castLteS()->lhsp(),$lhsp->castLteS()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castGt, $lhsp.width1}",    "AstLte {$lhsp->castGt()->lhsp(),$lhsp->castGt()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castGt}",                  "AstLte {$lhsp->castGt()->lhsp(),$lhsp->castGt()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castGtS, $lhsp.width1}",   "AstLteS{$lhsp->castGtS()->lhsp(),$lhsp->castGtS()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castGtS}",                 "AstLteS{$lhsp->castGtS()->lhsp(),$lhsp->castGtS()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castGte, $lhsp.width1}",   "AstLt  {$lhsp->castGte()->lhsp(),$lhsp->castGte()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castGte}",                 "AstLt  {$lhsp->castGte()->lhsp(),$lhsp->castGte()->rhsp()}");
    TREEOPV("AstNot   {$lhsp.castGteS, $lhsp.width1}",  "AstLtS {$lhsp->castGteS()->lhsp(),$lhsp->castGteS()->rhsp()}");
    TREEOP ("AstLogNot{$lhsp.castGteS}",                "AstLtS {$lhsp->castGteS()->lhsp(),$lhsp->castGteS()->rhsp()}");
    // Not common, but avoids compiler warnings about over shifting
    TREEOP ("AstShiftL   {operandHugeShiftL(nodep)}",   "replaceZero(nodep)");
    TREEOP ("AstShiftLOvr{operandHugeShiftL(nodep)}",   "replaceZero(nodep)");
    TREEOP ("AstShiftR   {operandHugeShiftR(nodep)}",   "replaceZero(nodep)");
    TREEOP ("AstShiftROvr{operandHugeShiftR(nodep)}",   "replaceZero(nodep)");
    TREEOP ("AstShiftL{operandShiftOp(nodep)}",         "replaceShiftOp(nodep)");
    TREEOP ("AstShiftR{operandShiftOp(nodep)}",         "replaceShiftOp(nodep)");
    TREEOP ("AstShiftL{operandShiftShift(nodep)}",      "replaceShiftShift(nodep)");
    TREEOP ("AstShiftR{operandShiftShift(nodep)}",      "replaceShiftShift(nodep)");
    TREEOP ("AstWordSel{operandWordOOB(nodep)}",        "replaceZero(nodep)");
    // Compress out EXTENDs to appease loop unroller
    TREEOPV("AstEq    {$rhsp.castExtend,operandBiExtendConstShrink(nodep)}",    "DONE");
    TREEOPV("AstNeq   {$rhsp.castExtend,operandBiExtendConstShrink(nodep)}",    "DONE");
    TREEOPV("AstGt    {$rhsp.castExtend,operandBiExtendConstShrink(nodep)}",    "DONE");
    TREEOPV("AstGte   {$rhsp.castExtend,operandBiExtendConstShrink(nodep)}",    "DONE");
    TREEOPV("AstLt    {$rhsp.castExtend,operandBiExtendConstShrink(nodep)}",    "DONE");
    TREEOPV("AstLte   {$rhsp.castExtend,operandBiExtendConstShrink(nodep)}",    "DONE");
    TREEOPV("AstEq    {$rhsp.castExtend,operandBiExtendConstOver(nodep)}",      "replaceZero(nodep)");
    TREEOPV("AstNeq   {$rhsp.castExtend,operandBiExtendConstOver(nodep)}",      "replaceNum(nodep,1)");
    TREEOPV("AstGt    {$rhsp.castExtend,operandBiExtendConstOver(nodep)}",      "replaceNum(nodep,1)");
    TREEOPV("AstGte   {$rhsp.castExtend,operandBiExtendConstOver(nodep)}",      "replaceNum(nodep,1)");
    TREEOPV("AstLt    {$rhsp.castExtend,operandBiExtendConstOver(nodep)}",      "replaceZero(nodep)");
    TREEOPV("AstLte   {$rhsp.castExtend,operandBiExtendConstOver(nodep)}",      "replaceZero(nodep)");
    // Identical operands on both sides
    // AstLogAnd/AstLogOr already converted to AstAnd/AstOr for these rules
    // AstAdd->ShiftL(#,1) but uncommon
    TREEOP ("AstAnd    {operandsSame($lhsp,,$rhsp)}",   "replaceWLhs(nodep)");
    TREEOP ("AstDiv    {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstDivS   {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstOr     {operandsSame($lhsp,,$rhsp)}",   "replaceWLhs(nodep)");
    TREEOP ("AstSub    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstXor    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstEq     {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");  // We let X==X -> 1, although in a true 4-state sim it's X.
    TREEOP ("AstEqN    {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");  // We let X==X -> 1, although in a true 4-state sim it's X.
    TREEOP ("AstEqCase {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstEqWild {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstGt     {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstGtD    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstGtN    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstGtS    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstGte    {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstGteN   {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstGteS   {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstLt     {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstLtD    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstLtN    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstLtS    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstLte    {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstLteN   {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstLteS   {operandsSame($lhsp,,$rhsp)}",   "replaceNum(nodep,1)");
    TREEOP ("AstNeq    {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstNeqN   {operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstNeqCase{operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstNeqWild{operandsSame($lhsp,,$rhsp)}",   "replaceZero(nodep)");
    TREEOP ("AstLogAnd {operandsSame($lhsp,,$rhsp)}",   "replaceWLhsBool(nodep)");
    TREEOP ("AstLogOr  {operandsSame($lhsp,,$rhsp)}",   "replaceWLhsBool(nodep)");
    ///=== Verilog operators
    // Comparison against 1'b0/1'b1; must be careful about widths.
    // These use Not, so must be Verilog only
    TREEOPV("AstEq    {$rhsp.width1, $lhsp.isZero,    $rhsp}",  "AstNot{$rhsp}");
    TREEOPV("AstEq    {$lhsp.width1, $lhsp, $rhsp.isZero}",     "AstNot{$lhsp}");
    TREEOPV("AstEq    {$rhsp.width1, $lhsp.isAllOnes, $rhsp}",  "replaceWRhs(nodep)");
    TREEOPV("AstEq    {$lhsp.width1, $lhsp, $rhsp.isAllOnes}",  "replaceWLhs(nodep)");
    TREEOPV("AstNeq   {$rhsp.width1, $lhsp.isZero,    $rhsp}",  "replaceWRhs(nodep)");
    TREEOPV("AstNeq   {$lhsp.width1, $lhsp, $rhsp.isZero}",     "replaceWLhs(nodep)");
    TREEOPV("AstNeq   {$rhsp.width1, $lhsp.isAllOnes, $rhsp}",  "AstNot{$rhsp}");
    TREEOPV("AstNeq   {$lhsp.width1, $lhsp, $rhsp.isAllOnes}",  "AstNot{$lhsp}");
    TREEOPV("AstLt    {$rhsp.width1, $lhsp.isZero,    $rhsp}",  "replaceWRhs(nodep)");  // Because not signed #s
    TREEOPV("AstGt    {$lhsp.width1, $lhsp, $rhsp.isZero}",     "replaceWLhs(nodep)");  // Because not signed #s
    // Useful for CONDs added around ARRAYSEL's in V3Case step
    TREEOPV("AstLte   {$lhsp->width()==$rhsp->width(), $rhsp.isAllOnes}", "replaceNum(nodep,1)");
    // Simplify reduction operators
    // This also gets &{...,0,....} => const 0  (Common for unused_ok signals)
    TREEOPV("AstRedAnd{$lhsp, $lhsp.width1}",   "replaceWLhs(nodep)");
    TREEOPV("AstRedOr {$lhsp, $lhsp.width1}",   "replaceWLhs(nodep)");
    TREEOPV("AstRedXor{$lhsp, $lhsp.width1}",   "replaceWLhs(nodep)");
    TREEOPV("AstRedAnd{$lhsp.castConcat}",      "AstAnd{AstRedAnd{$lhsp->castConcat()->lhsp()}, AstRedAnd{$lhsp->castConcat()->rhsp()}}");  // &{a,b} => {&a}&{&b}
    TREEOPV("AstRedOr {$lhsp.castConcat}",      "AstOr {AstRedOr {$lhsp->castConcat()->lhsp()}, AstRedOr {$lhsp->castConcat()->rhsp()}}");  // |{a,b} => {|a}|{|b}
    TREEOPV("AstRedXor{$lhsp.castConcat}",      "AstXor{AstRedXor{$lhsp->castConcat()->lhsp()}, AstRedXor{$lhsp->castConcat()->rhsp()}}");  // ^{a,b} => {^a}^{^b}
    TREEOPV("AstRedAnd{$lhsp.castExtend, $lhsp->width() > VN_AS($lhsp,,Extend)->lhsp()->width()}", "replaceZero(nodep)");  // &{0,...} => 0  Prevents compiler limited range error
    TREEOPV("AstRedOr {$lhsp.castExtend}",      "AstRedOr {$lhsp->castExtend()->lhsp()}");
    TREEOPV("AstRedXor{$lhsp.castExtend}",      "AstRedXor{$lhsp->castExtend()->lhsp()}");
    TREEOP ("AstRedXor{$lhsp.castXor, VN_IS(VN_AS($lhsp,,Xor)->lhsp(),,Const)}", "AstXor{AstRedXor{$lhsp->castXor()->lhsp()}, AstRedXor{$lhsp->castXor()->rhsp()}}");  // ^(const ^ a) => (^const)^(^a)
    TREEOPC("AstAnd {$lhsp.castConst, $rhsp.castRedXor, matchBitOpTree(nodep)}", "DONE");
    TREEOPV("AstOneHot{$lhsp.width1}",          "replaceWLhs(nodep)");
    TREEOPV("AstOneHot0{$lhsp.width1}",         "replaceNum(nodep,1)");
    // Binary AND/OR is faster than logical and/or (usually)
    TREEOPV("AstLogAnd{matchBiopToBitwise(nodep)}", "AstAnd{$lhsp,$rhsp}");
    TREEOPV("AstLogOr {matchBiopToBitwise(nodep)}", "AstOr{$lhsp,$rhsp}");
    TREEOPV("AstLogNot{$lhsp.width1}",  "AstNot{$lhsp}");
    // CONCAT(CONCAT({a},{b}),{c}) -> CONCAT({a},CONCAT({b},{c}))
    // CONCAT({const},CONCAT({const},{c})) -> CONCAT((constifiedCONC{const|const},{c}))
    TREEOPV("AstConcat{matchConcatRand(nodep)}",      "DONE");
    TREEOPV("AstConcat{operandConcatMove(nodep)}",      "moveConcat(nodep)");
    TREEOPV("AstConcat{$lhsp.isZero, $rhsp}",           "replaceExtend(nodep, nodep->rhsp())");
    // CONCAT(a[1],a[0]) -> a[1:0]
    TREEOPV("AstConcat{$lhsp.castSel, $rhsp.castSel, ifAdjacentSel(VN_AS($lhsp,,Sel),,VN_AS($rhsp,,Sel))}",  "replaceConcatSel(nodep)");
    TREEOPV("AstConcat{ifConcatMergeableBiop($lhsp), concatMergeable($lhsp,,$rhsp,,0)}", "replaceConcatMerge(nodep)");
    // Common two-level operations that can be simplified
    TREEOP ("AstAnd {$lhsp.castConst,matchAndCond(nodep)}", "DONE");
    TREEOP ("AstAnd {$lhsp.castConst, $rhsp.castOr, matchMaskedOr(nodep)}", "DONE");
    TREEOPC("AstAnd {$lhsp.castConst, matchMaskedShift(nodep)}", "DONE");
    TREEOP ("AstAnd {$lhsp.castOr, $rhsp.castOr, operandAndOrSame(nodep)}", "replaceAndOr(nodep)");
    TREEOP ("AstOr  {$lhsp.castAnd,$rhsp.castAnd,operandAndOrSame(nodep)}", "replaceAndOr(nodep)");
    TREEOP ("AstOr  {matchOrAndNot(nodep)}",            "DONE");
    TREEOP ("AstAnd {operandShiftSame(nodep)}",         "replaceShiftSame(nodep)");
    TREEOP ("AstOr  {operandShiftSame(nodep)}",         "replaceShiftSame(nodep)");
    TREEOP ("AstXor {operandShiftSame(nodep)}",         "replaceShiftSame(nodep)");
    TREEOPC("AstAnd {matchBitOpTree(nodep)}", "DONE");
    TREEOPC("AstOr  {matchBitOpTree(nodep)}", "DONE");
    TREEOPC("AstXor {matchBitOpTree(nodep)}", "DONE");
    // Note can't simplify a extend{extends}, extends{extend}, as the sign
    // bits end up in the wrong places
    TREEOPV("AstExtend{operandsSameWidth(nodep,,$lhsp)}",  "replaceWLhs(nodep)");
    TREEOPV("AstExtend{$lhsp.castExtend}",  "replaceExtend(nodep, VN_AS(nodep->lhsp(), Extend)->lhsp())");
    TREEOPV("AstExtendS{$lhsp.castExtendS}", "replaceExtend(nodep, VN_AS(nodep->lhsp(), ExtendS)->lhsp())");
    TREEOPV("AstReplicate{$srcp, $countp.isOne, $srcp->width()==nodep->width()}", "replaceWLhs(nodep)");  // {1{lhs}}->lhs
    TREEOPV("AstReplicateN{$lhsp, $rhsp.isOne, $lhsp->width()==nodep->width()}", "replaceWLhs(nodep)");  // {1{lhs}}->lhs
    TREEOPV("AstReplicate{$srcp.castReplicate, operandRepRep(nodep)}", "DONE");  // {2{3{lhs}}}->{6{lhs}}
    TREEOPV("AstConcat{operandConcatSame(nodep)}", "DONE");  // {a,a}->{2{a}}, {a,2{a}}->{3{a}, etc
    // Next rule because AUTOINST puts the width of bits in
    // to pins, even when the widths are exactly the same across the hierarchy.
    TREEOPV("AstSel{matchSelRand(nodep)}",      "DONE");
    TREEOPV("AstSel{operandSelExtend(nodep)}",  "DONE");
    TREEOPV("AstSel{operandSelFull(nodep)}",    "replaceWChild(nodep, nodep->fromp())");
    TREEOPV("AstSel{$fromp.castSel}",           "replaceSelSel(nodep)");
    TREEOPV("AstSel{$fromp.castAdd, operandSelBiLower(nodep)}", "DONE");
    TREEOPV("AstSel{$fromp.castAnd, operandSelBiLower(nodep)}", "DONE");
    TREEOPV("AstSel{$fromp.castOr,  operandSelBiLower(nodep)}", "DONE");
    TREEOPV("AstSel{$fromp.castSub, operandSelBiLower(nodep)}", "DONE");
    TREEOPV("AstSel{$fromp.castXor, operandSelBiLower(nodep)}", "DONE");
    TREEOPV("AstSel{$fromp.castShiftR, operandSelShiftLower(nodep)}",   "DONE");
    TREEOPA("AstSel{$fromp.castConst, $lsbp.castConst, }",   "replaceConst(nodep)");
    TREEOPV("AstSel{$fromp.castConcat, $lsbp.castConst, }",  "replaceSelConcat(nodep)");
    TREEOPV("AstSel{$fromp.castReplicate, $lsbp.castConst, operandSelReplicate(nodep) }",    "DONE");
    // V3Tristate requires selects below BufIf1.
    // Also do additional operators that are bit-independent, but only definite
    // win if bit select is a constant (otherwise we may need to compute bit index several times)
    TREEOPV("AstSel{$fromp.castBufIf1}",                "replaceSelIntoBiop(nodep)");
    TREEOPV("AstSel{$fromp.castNot}",                   "replaceSelIntoUniop(nodep)");
    TREEOPV("AstSel{$fromp.castAnd,$lsbp.castConst}",   "replaceSelIntoBiop(nodep)");
    TREEOPV("AstSel{$fromp.castOr,$lsbp.castConst}",    "replaceSelIntoBiop(nodep)");
    TREEOPV("AstSel{$fromp.castXor,$lsbp.castConst}",   "replaceSelIntoBiop(nodep)");
    // This visit function here must allow for short-circuiting.
    TREEOPS("AstLogIf{$lhsp.isZero}",  "replaceNum(nodep, 1)");
    TREEOPV("AstLogIf{$lhsp, $rhsp}",  "AstLogOr{AstLogNot{$lhsp},$rhsp}");
    TREEOPV("AstLogEq{$lhsp, $rhsp}",  "replaceLogEq(nodep)");
    // Strings
    TREEOPA("AstPutcN{$lhsp.castConst, $rhsp.castConst, $thsp.castConst}",  "replaceConst(nodep)");
    TREEOPA("AstSubstrN{$lhsp.castConst, $rhsp.castConst, $thsp.castConst}",  "replaceConst(nodep)");
    TREEOP ("AstToStringN{DISABLE_BASE}", "DONE");  // Avoid uniop(const); use matchToStringNConst
    TREEOP ("AstToStringN{matchToStringNConst(nodep)}", "DONE");
    // Custom
    // Implied by AstIsUnbounded::numberOperate: V("AstIsUnbounded{$lhsp.castConst}", "replaceNum(nodep, 0)");
    TREEOPV("AstIsUnbounded{$lhsp.castUnbounded}", "replaceNum(nodep, 1)");
    // Sampled value functions of a constant.
    // $rose/$fell/$stable/$changed are lowered to $past by V3AssertPre, so they fold via AstPast
    TREEOPV("AstSampled{$exprp.castConst}", "replaceWChild(nodep, VN_AS(nodep->exprp(), NodeExpr))");
    TREEOPV("AstPast{$exprp.castConst, !$ticksp}", "replaceWChild(nodep, nodep->exprp())");
    // clang-format on

    // Possible futures:
    // (a?(b?y:x):y) -> (a&&!b)?x:y
    // (a?(b?x:y):y) -> (a&&b)?x:y
    // (a?x:(b?x:y)) -> (a||b)?x:y
    // (a?x:(b?y:x)) -> (a||!b)?x:y

    // Note we can't convert EqCase/NeqCase to Eq/Neq here because that would break 3'b1x1==3'b101

    //-----
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        // Default: Just iterate
        ಯದಿ (m_required) {
            ಯದಿ (VN_IS(nodep, ಗಂಟು_DType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } ಅನ್ಯಥಾ ಯದಿ (AstCellRef* ಧ್ರುವ crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(crp), crp);
                }
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->isTimingControl()) m_hasJumpDelay = ಸತ್ಯ;
            // Calculate the width of this operation
            ಯದಿ (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // Processing Mode Enum
    ಗಣನಾ ProcMode : uint8_t {
        PROC_PARAMS_NOWARN,
        PROC_PARAMS,
        PROC_GENERATE,
        PROC_LIVE,
        PROC_V_WARN,
        PROC_V_NOWARN,
        PROC_V_EXPENSIVE,
        PROC_CPP
    };

    // CONSTRUCTORS
    ConstVisitor(ProcMode pmode, ತಾರ್ಕಿಕ globalPass)
        : m_globalPass{globalPass}
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_global_Pass_ಅಂಕೆ++)) : ""} {
        // clang-format off
        ವಿಕಲ್ಪ (pmode) {
        ಪಕ್ಷ PROC_PARAMS_NOWARN:  m_doV = ಸತ್ಯ;  m_doNConst = ಸತ್ಯ; m_params = ಸತ್ಯ;
                                  m_required = ಅಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ PROC_PARAMS:         m_doV = ಸತ್ಯ;  m_doNConst = ಸತ್ಯ; m_params = ಸತ್ಯ;
                                  m_required = ಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ PROC_GENERATE:       m_doV = ಸತ್ಯ;  m_doNConst = ಸತ್ಯ; m_params = ಸತ್ಯ;
                                  m_required = ಸತ್ಯ; m_doGenerate = ಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ PROC_LIVE:           ಭಙ್ಗ;
        ಪಕ್ಷ PROC_V_WARN:         m_doV = ಸತ್ಯ;  m_doNConst = ಸತ್ಯ; m_warn = ಸತ್ಯ; m_convertLogicToBit = ಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ PROC_V_NOWARN:       m_doV = ಸತ್ಯ;  m_doNConst = ಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ PROC_V_EXPENSIVE:    m_doV = ಸತ್ಯ;  m_doNConst = ಸತ್ಯ; m_doExpensive = ಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ PROC_CPP:            m_doV = ಅಸತ್ಯ; m_doNConst = ಸತ್ಯ; m_doCpp = ಸತ್ಯ; ಭಙ್ಗ;
        ಸ್ವಭಾವ:                  v3fatalSrc("Bad case"); ಭಙ್ಗ;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        ಯದಿ (m_doCpp) {
            ಯದಿ (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } ಅನ್ಯಥಾ {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_stat_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_ಅಭಿವ್ಯಕ್ತಿ_Redundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_ಗಂಟು* mainAcceptEdit(Ast_ಗಂಟು* nodep) {
        VIsCached::clear_Cache_ವೃಕ್ಷ();  // Avoid using any stale isPure
        // Operate starting at a random place
        ಪ್ರತ್ಯಾಗಮ iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_ಅಂಕೆ = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
ಶೂನ್ಯ V3Const::constifyParamsEdit(Ast_ಗಂಟು* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ ಅಸತ್ಯ};
    // cppcheck-suppress constVariablePointer // edited below
    ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ಯದಿ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ಅನ್ಯಥಾ {
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
ಶೂನ್ಯ V3Const::constifyParamsNoWarnEdit(Ast_ಗಂಟು* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ ಅಸತ್ಯ};
    // cppcheck-suppress constVariablePointer // edited below
    ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ಯದಿ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ಅನ್ಯಥಾ {
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Force this cell node's parameter list to become a constant inside generate.
//! If we are inside a generated "if", "case" or "for", we don't want to
//! trigger warnings when we deal with the width. It is possible that these
//! are spurious, existing within sub-expressions that will not actually be
//! generated. Since such occurrences, must be constant, in order to be
//! something a generate block can depend on, we can wait until later to do the
//! width check.
ಶೂನ್ಯ V3Const::constifyGenerateParamsEdit(Ast_ಗಂಟು* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ ಅಸತ್ಯ};
    // cppcheck-suppress constVariablePointer // edited below
    ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ಯದಿ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ಅನ್ಯಥಾ {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

ಶೂನ್ಯ V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ ಸತ್ಯ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("const", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ಸತ್ಯ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("const_cpp", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

Ast_ಗಂಟು* V3Const::constifyEdit(Ast_ಗಂಟು* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ ಅಸತ್ಯ};
    nodep = visitor.mainAcceptEdit(nodep);
    ಪ್ರತ್ಯಾಗಮ nodep;
}

Ast_ಗಂಟು* V3Const::constifyEditCpp(Ast_ಗಂಟು* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ಅಸತ್ಯ};
    nodep = visitor.mainAcceptEdit(nodep);
    ಪ್ರತ್ಯಾಗಮ nodep;
}

ಶೂನ್ಯ V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ ಸತ್ಯ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("const", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ಸತ್ಯ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("const", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

Ast_ಗಂಟು* V3Const::constifyExpensiveEdit(Ast_ಗಂಟು* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ಅಸತ್ಯ};
    nodep = visitor.mainAcceptEdit(nodep);
    ಪ್ರತ್ಯಾಗಮ nodep;
}
