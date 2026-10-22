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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Simulate.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"
#ଅନ୍ତର୍ଭାଵ "V3Width.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <type_traits>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#define ବୃକ୍ଷ_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

ସ୍ଥିର ତାର୍କିକ isConst(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, uint64_t v) {
    ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const);
    ଫେରନ୍ତୁ constp && constp->toUQuad() == v;
}

template <typename T>
ସ୍ଥିର typename std::enable_if<std::is_integral<T>::ମୂଲ୍ୟ, ତାର୍କିକ>::type isPow2(T val) {
    ଫେରନ୍ତୁ (val & (val - 1)) == 0;
}

ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣନା_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#ଯଦି defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    ଫେରନ୍ତୁ __builtin_ctzll(val);
#ଅନ୍ୟଥା
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0;
    val = ~val;
    ଯେତେବେଳେ (val & 1) {
        ++bit;
        val >>= 1;
    }
    ଫେରନ୍ତୁ bit;
#endif
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
ଶ୍ରେଣୀ VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4InUse> m_inuser4p;
    size_t m_nextId = 0;  // Id allocated most recently

ସର୍ବସାଧାରଣ:
    // Return the unique id of the given node, allocating a new one if it has none yet
    size_t operator()(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        ଯଦି (!nodep->user4p()) nodep->user4p(reinterpret_cast<ଶୂନ୍ୟ*>(++m_nextId));
        ଫେରନ୍ତୁ reinterpret_cast<size_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
ଶ୍ରେଣୀ Const_Bit_Op_ବୃକ୍ଷ_Visitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, unsigned, ତାର୍କିକ>;

    ଶ୍ରେଣୀ LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        ତାର୍କିକ m_polarity = ସତ୍ୟ;  // Invert result due to NOT
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = nullptr;  // Leaf's variable reference
        ଅଚଳ AstConst* m_constp = nullptr;  // Leaf's constant

    ସର୍ବସାଧାରଣ:
        // CONSTRUCTORS
        LeafInfo() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        LeafInfo(ଅଚଳ LeafInfo& other) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        LeafInfo& operator=(ଅଚଳ LeafInfo& other) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        LeafInfo(LeafInfo&& other) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        LeafInfo& operator=(LeafInfo&& other) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        explicit LeafInfo(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb)
            : m_lsb{lsb} {}

        // METHODS
        ଶୂନ୍ୟ setLeaf(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->widthMin() - 1;
        }
        ଶୂନ୍ୟ setLeaf(ଅଚଳ AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->widthMin() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        ଶୂନ୍ୟ updateBitRange(ପୂର୍ଣ୍ଣସଂଖ୍ୟା newLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା newMsb) {
            ଯଦି ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(ସତ୍ୟ);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        ଶୂନ୍ୟ updateBitRange(ଅଚଳ AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->width() - 1));
        }
        ଶୂନ୍ୟ updateBitRange(ଅଚଳ AstShiftR* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->toUInt(), m_msb);
        }
        ଶୂନ୍ୟ limitBitRangeToLsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା wordIdx() ଅଚଳ { ଫେରନ୍ତୁ m_wordIdx; }
        ଶୂନ୍ୟ wordIdx(ପୂର୍ଣ୍ଣସଂଖ୍ୟା i) { m_wordIdx = i; }
        ତାର୍କିକ polarity() ଅଚଳ { ଫେରନ୍ତୁ m_polarity; }
        ଶୂନ୍ୟ polarity(ତାର୍କିକ p) { m_polarity = p; }

        AstVarRef* refp() ଅଚଳ { ଫେରନ୍ତୁ m_refp; }
        ଅଚଳ AstConst* constp() ଅଚଳ { ଫେରନ୍ତୁ m_constp; }
        ତାର୍କିକ missingWordSel() ଅଚଳ {
            // When V3Expand is skipped, WordSel is not inserted.
            ଫେରନ୍ତୁ m_refp->isWide() && m_wordIdx == -1;
        }
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb() ଅଚଳ { ଫେରନ୍ତୁ m_lsb; }

        ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb() ଅଚଳ { ଫେରନ୍ତୁ std::min(m_msb, varWidth() - 1); }
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା varWidth() ଅଚଳ {
            UASSERT(m_refp, "m_refp should be set");
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = m_refp->varp()->widthMin();
            ଯଦି (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                ଫେରନ୍ତୁ width;
            } ଅନ୍ୟଥା {
                ଯଦି (missingWordSel()) ଫେରନ୍ତୁ width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                ଫେରନ୍ତୁ m_wordIdx == m_refp->width_ଶବ୍ଦଗୁଡ଼ିକ() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    ସଂରଚନା BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        ତାର୍କିକ m_polarity = ମିଥ୍ୟା;  // Polarity the bit must have to match
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(ଅଚଳ LeafInfo& info, ତାର୍କିକ pol, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    };

    ସଂରଚନା Frozen_ଗଣ୍ଠି_Info final {  // Context when a frozen node is found
        ତାର୍କିକ m_polarity;  // Polarity the frozen node must match
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lsb;  // LSB position of the frozen node
        ତାର୍କିକ operator<(ଅଚଳ Frozen_ଗଣ୍ଠି_Info& other) ଅଚଳ {
            ଯଦି (m_lsb != other.m_lsb) ଫେରନ୍ତୁ m_lsb < other.m_lsb;
            ଫେରନ୍ତୁ m_polarity < other.m_polarity;
        }
    };

    ଶ୍ରେଣୀ Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_ବୃକ୍ଷ_Visitor& m_visitor;  // Visitor whose state is saved and restored
        ଅଚଳ size_t m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        ଅଚଳ size_t m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        ଅଚଳ unsigned m_ops;  // Saved m_visitor.m_ops to restore
        ଅଚଳ ତାର୍କିକ m_polarity;  // Saved m_visitor.m_polarity to restore
        ତାର୍କିକ m_restore = ସତ୍ୟ;  // Whether the destructor still needs to restore state

    ସର୍ବସାଧାରଣ:
        explicit Restorer(Const_Bit_Op_ବୃକ୍ଷ_Visitor& visitor)
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
            ଯଦି (m_restore) restoreNow();
        }
        ଶୂନ୍ୟ disableRestore() { m_restore = ମିଥ୍ୟା; }
        ଶୂନ୍ୟ restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = ମିଥ୍ୟା;
        }
    };
    // Collect information for each Variable to transform as below
    ଶ୍ରେଣୀ VarInfo final {
        // MEMBERS
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        ଅଚଳ Const_Bit_Op_ବୃକ୍ଷ_Visitor* ଅଚଳ
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* ଅଚଳ m_refp;  // Points the variable that this VarInfo covers
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_width;  // Width of term this VarInfo refers to
        V3_ସଂଖ୍ୟା m_bitPolarity;  // Coefficient of each bit

    ସର୍ବସାଧାରଣ:
        // METHODS
        ତାର୍କିକ hasConstResult() ଅଚଳ { ଫେରନ୍ତୁ m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        ତାର୍କିକ getConstResult() ଅଚଳ {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            ଫେରନ୍ତୁ m_knownResult == 1;
        }
        ଅଚଳ AstVarRef* refp() ଅଚଳ { ଫେରନ୍ତୁ m_refp; }
        ତାର୍କିକ sameVarAs(ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* otherp) ଅଚଳ { ଫେରନ୍ତୁ m_refp->same_ଗଣ୍ଠି(otherp); }
        ଶୂନ୍ୟ setPolarity(ତାର୍କିକ compBit, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) {
            // Ignore if already determined a known reduction
            ଯଦି (m_knownResult >= 0) ଫେରନ୍ତୁ;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            ଯଦି (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } ଅନ୍ୟଥା {  // The bit has already been marked with some polarity
                ଅଚଳ ତାର୍କିକ sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                ଯଦି (m_parentp->is_Xor_ବୃକ୍ଷ()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } ଅନ୍ୟଥା {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    ଯଦି (sameFlag) ଫେରନ୍ତୁ;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->is_And_ବୃକ୍ଷ() ? 0 : 1;
                    m_bitPolarity.setAllBitsX();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ResultTerm getResultTerm() ଅଚଳ {
            UASSERT_OBJ(!hasConstResult(), m_refp, "getTerm on reduction that yields constant");
            File_ଧାଡ଼ି* ଅଚଳ fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp = VN_CAST(m_refp->backp(), WordSel);
            ଯଦି (!srcp) srcp = m_refp;
            srcp = srcp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);

            // Signed variables might have redundant sign bits that need masking.
            ଅଚଳ ତାର୍କିକ hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3_ସଂଖ୍ୟା mask_ଅଙ୍କ{srcp, m_width, 0};
            mask_ଅଙ୍କ.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            ଅଚଳ uint64_t maskVal = mask_ଅଙ୍କ.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = srcp;  // The tree for this term
            unsigned ops = 0;  // Number of ops in this term
            ତାର୍କିକ clean = ମିଥ୍ୟା;  // Whether the term is clean (has value 0 or 1)

            ଯଦି (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = ଗଣନା_Trailing_Zeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                ଯଦି (bit != 0) {
                    resultp = ନୂତନ AstShiftR{fl, resultp,
                                            ନୂତନ AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                ଅଚଳ ତାର୍କିକ negate = m_bitPolarity.bitIs0(bit);
                ଯଦି (negate) {
                    resultp = ନୂତନ AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } ଅନ୍ୟଥା {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                ଅଚଳ ତାର୍କିକ needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                ଯଦି (needsMasking) {
                    resultp = ନୂତନ AstAnd{fl, ନୂତନ AstConst{fl, mask_ଅଙ୍କ}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                ଯଦି (m_parentp->is_Xor_ବୃକ୍ଷ()) {
                    ଯଦି (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin = mask_ଅଙ୍କ.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = ନୂତନ AstRedXor{fl, resultp};
                    ++ops;
                    clean = ମିଥ୍ୟା;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } ଅନ୍ୟଥା ଯଦି (m_parentp->is_And_ବୃକ୍ଷ()) {
                    V3_ସଂଖ୍ୟା comp_ଅଙ୍କ{srcp, m_width, 0};
                    comp_ଅଙ୍କ.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, comp_ଅଙ୍କ}, resultp};
                    ++ops;
                    clean = ସତ୍ୟ;
                } ଅନ୍ୟଥା {  // Or
                    V3_ସଂଖ୍ୟା comp_ଅଙ୍କ{srcp, m_width, 0};
                    comp_ଅଙ୍କ.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_ଅଙ୍କ.opXor(V3_ସଂଖ୍ୟା{comp_ଅଙ୍କ}, mask_ଅଙ୍କ);
                    resultp = ନୂତନ AstNeq{fl, ନୂତନ AstConst{fl, comp_ଅଙ୍କ}, resultp};
                    ++ops;
                    clean = ସତ୍ୟ;
                }
            }

            ଫେରନ୍ତୁ ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_ବୃକ୍ଷ_Visitor* parent, AstVarRef* refp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    ତାର୍କିକ m_failed = ମିଥ୍ୟା;
    ତାର୍କିକ m_polarity = ସତ୍ୟ;  // Flip when AstNot comes
    unsigned m_ops;  // Number of operations such as And, Or, Xor, Sel...
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ m_rootp;  // Root of this AST subtree

    std::vector<std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Frozen_ଗଣ୍ଠି_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<size_t, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> m_baseIdxs;

    // METHODS

    ତାର୍କିକ is_And_ବୃକ୍ଷ() ଅଚଳ { ଫେରନ୍ତୁ VN_IS(m_rootp, And); }
    ତାର୍କିକ is_Or_ବୃକ୍ଷ() ଅଚଳ { ଫେରନ୍ତୁ VN_IS(m_rootp, Or); }
    ତାର୍କିକ is_Xor_ବୃକ୍ଷ() ଅଚଳ { ଫେରନ୍ତୁ VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    ଯଦି (setFailed(cond, #cond, nodep, __LINE__)) ଫେରନ୍ତୁ

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(ସତ୍ୟ, reason, nodep, __LINE__)

    ତାର୍କିକ setFailed(ତାର୍କିକ fail, ଅଚଳ ଅକ୍ଷର* reason, ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଧାଡ଼ି) {
        ଯଦି (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << ଧାଡ଼ି << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ m_failed;
    }
    ଶୂନ୍ୟ incrOps(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଧାଡ଼ି) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << ଧାଡ଼ି);
    }
    VarInfo& getVarInfo(ଅଚଳ LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_ଗଣ୍ଠି* nodep = ref.refp()->varScopep();
        ଯଦି (!nodep) nodep = ref.refp()->varp();  // Not scoped
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା& baseIdxr = m_baseIdxs[m_ids(nodep)];
        ଯଦି (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_ଶବ୍ଦଗୁଡ଼ିକ
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->width_ଶବ୍ଦଗୁଡ଼ିକ() : 1;
            m_varInfos.resize(m_varInfos.size() + ଅଙ୍କ_ଶବ୍ଦଗୁଡ଼ିକ);
        }
        ଅଚଳ size_t idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        ଯଦି (!varInfop) {
            varInfop = ନୂତନ VarInfo{ଏହି, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            ଯଦି (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } ଅନ୍ୟଥା {
            ଯଦି (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        ଫେରନ୍ତୁ *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ expectConst) {
        ଯଦି (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) ଫେରନ୍ତୁ LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        ତାର୍କିକ ok = !m_failed;
        ଯଦି (expectConst) {
            ok &= !info.refp() && info.constp();
        } ଅନ୍ୟଥା {
            ok &= info.refp() && !info.constp();
        }
        ଫେରନ୍ତୁ ok ? info : LeafInfo{};
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    ଶୂନ୍ୟ visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        ଯଦି (m_leafp) m_leafp->updateBitRange(nodep);
    }
    ଶୂନ୍ୟ visit(AstShiftR* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->toUInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->toUInt();
    }
    ଶୂନ୍ୟ visit(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->widthMin() != 1, nodep);
        Ast_ଗଣ୍ଠି* lhsp = nodep->lhsp();
        AstCCast* ଅଚଳ castp = VN_CAST(lhsp, CCast);
        ଯଦି (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_ବୃକ୍ଷ() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        ଯଦି (!is_Xor_ବୃକ୍ଷ()) m_polarity = !m_polarity;
        ଯଦି (m_leafp && castp) m_leafp->updateBitRange(castp);
        ଯଦି (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    ଶୂନ୍ୟ visit(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->toSInt());
        iterateConst(nodep->fromp());
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }

    ଶୂନ୍ୟ visit(AstRedXor* nodep) override {
        Restorer restorer{*ଏହି};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_ଗଣ୍ଠି* lhsp = nodep->lhsp();
        ଅଚଳ AstCCast* ଅଚଳ castp = VN_CAST(lhsp, CCast);
        ଯଦି (castp) lhsp = castp->lhsp();
        ଯଦି (ଅଚଳ AstAnd* ଅଚଳ andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            ଅଚଳ LeafInfo& mask = findLeaf(andp->lhsp(), ସତ୍ୟ);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), ମିଥ୍ୟା);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            ଯଦି (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            ଅଚଳ V3_ସଂଖ୍ୟା& mask_ଅଙ୍କ = mask.constp()->ଅଙ୍କ();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxBitIdx = std::min(ref.lsb() + mask_ଅଙ୍କ.width(), ref.msb() + 1);
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା maskIdx = bitIdx - ref.lsb();
                ଯଦି (mask_ଅଙ୍କ.bitIs0(maskIdx)) ଜାରି;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, ସତ୍ୟ, bitIdx);
            }
        } ଅନ୍ୟଥା {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, ମିଥ୍ୟା);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            ଯଦି (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, ସତ୍ୟ, bitIdx);
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Biop* nodep) override {
        ଯଦି (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*ଏହି};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            ଯଦି (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } ଅନ୍ୟଥା ଯଦି (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_ବୃକ୍ଷ(), nodep);
            incrOps(nodep, __LINE__);
            ପାଇଁ (ଅଚଳ ତାର୍କିକ ଡାହାଣ : {ମିଥ୍ୟା, ସତ୍ୟ}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*ଏହି};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* opp = ଡାହାଣ ? nodep->rhsp() : nodep->lhsp();
                ଅଚଳ ତାର୍କିକ origFailed = m_failed;
                iterateConst(opp);
                ଯଦି (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    ଯଦି (ଅଚଳ AstCCast* ଅଚଳ castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    ଅଚଳ ତାର୍କିକ pol = is_Xor_ବୃକ୍ଷ() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, Frozen_ଗଣ୍ଠି_Info{pol, m_lsb});
                    m_failed = origFailed;
                    ଜାରି;
                }
                restorer.disableRestore();  // Now all checks passed
                ଯଦି (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    ଯଦି (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, is_Xor_ବୃକ୍ଷ() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } ଅନ୍ୟଥା ଯଦି ((is_And_ବୃକ୍ଷ() && leafInfo.polarity())
                               || (is_Or_ବୃକ୍ଷ() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, ସତ୍ୟ, 0);
                        m_bitPolarities.emplace_back(leafInfo, ମିଥ୍ୟା, 0);
                    }
                }
            }
        } ଅନ୍ୟଥା ଯଦି ((is_And_ବୃକ୍ଷ() && VN_IS(nodep, Eq)) || (is_Or_ବୃକ୍ଷ() && VN_IS(nodep, Neq))) {
            Restorer restorer{*ଏହି};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            ଅଚଳ Ast_ଗଣ୍ଠି* lhsp = nodep->lhsp();
            ଯଦି (ଅଚଳ AstCCast* ଅଚଳ castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            ଅଚଳ V3_ସଂଖ୍ୟା& comp_ଅଙ୍କ = constp->ଅଙ୍କ();

            auto setPolarities = [ଏହି, &comp_ଅଙ୍କ](ଅଚଳ LeafInfo& ref, ଅଚଳ V3_ସଂଖ୍ୟା* maskp) {
                ଅଚଳ ତାର୍କିକ maskFlip = is_And_ବୃକ୍ଷ() ^ ref.polarity();
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା constantWidth = comp_ଅଙ୍କ.width();
                ଯଦି (maskp) constantWidth = std::max(constantWidth, maskp->width());
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା maskIdx = bitIdx - ref.lsb();
                    ଅଚଳ ତାର୍କିକ mask0 = maskp && maskp->bitIs0(maskIdx);
                    ଅଚଳ ତାର୍କିକ outOfRange = bitIdx > ref.msb();
                    ଯଦି (mask0 || outOfRange) {  // RHS is 0
                        ଯଦି (comp_ଅଙ୍କ.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, ସତ୍ୟ, 0);
                            m_bitPolarities.emplace_back(ref, ମିଥ୍ୟା, 0);
                            ଭାଙ୍ଗ;
                        } ଅନ୍ୟଥା {  // This bitIdx is irrelevant
                            ଜାରି;
                        }
                    }
                    ଅଚଳ ତାର୍କିକ polarity = comp_ଅଙ୍କ.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            ଯଦି (ଅଚଳ AstAnd* ଅଚଳ andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                ଅଚଳ LeafInfo& mask = findLeaf(andp->lhsp(), ସତ୍ୟ);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                ଅଚଳ LeafInfo& ref = findLeaf(andp->rhsp(), ମିଥ୍ୟା);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                ଅଚଳ V3_ସଂଖ୍ୟା& mask_ଅଙ୍କ = mask.constp()->ଅଙ୍କ();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &mask_ଅଙ୍କ);
            } ଅନ୍ୟଥା {  // comp == v
                ଅଚଳ LeafInfo& ref = findLeaf(nodep->rhsp(), ମିଥ୍ୟା);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } ଅନ୍ୟଥା {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_ବୃକ୍ଷ_Visitor(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, unsigned externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ଠେଲ_back(nullptr);
        CONST_BITOP_RETURN_IF(!is_And_ବୃକ୍ଷ() && !is_Or_ବୃକ୍ଷ() && !is_Xor_ବୃକ୍ଷ(), nodep);
        ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ biopp = VN_CAST(nodep, ଗଣ୍ଠି_Biop)) {
            iterateConst(biopp);
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        ପାଇଁ (auto&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(is_Xor_ବୃକ୍ଷ() || m_polarity, nodep, "must be the original polarity");
    }
    ଆଭାସୀ ~Const_Bit_Op_ବୃକ୍ଷ_Visitor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
#undef CONST_BITOP_RETURN_IF
#undef CONST_BITOP_SET_FAILED

ସର୍ବସାଧାରଣ:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* simplify(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା resultWidth, unsigned externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        ଅଚଳ Const_Bit_Op_ବୃକ୍ଷ_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        ଯଦି (visitor.m_failed || visitor.m_varInfos.size() == 1) ଫେରନ୍ତୁ nullptr;

        // FileLine used for constructing all new nodes in this function
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        unsigned resultOps = 0;
        ତାର୍କିକ hasCleanTerm = ମିଥ୍ୟା;
        ତାର୍କିକ hasDirtyTerm = ମିଥ୍ୟା;

        ପାଇଁ (auto&& v : visitor.m_varInfos) {
            ଯଦି (!v) ଜାରି;  // Skip nullptr at m_varInfos[0]
            ଯଦି (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = nullptr;
                ଯଦି (v->getConstResult()) {
                    UASSERT_OBJ(visitor.is_Or_ବୃକ୍ଷ(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = ନୂତନ AstConst{fl, AstConst::BitTrue{}};
                } ଅନ୍ୟଥା ଯଦି (visitor.is_And_ବୃକ୍ଷ()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = ନୂତନ AstConst{fl, AstConst::BitFalse{}};
                } ଅନ୍ୟଥା {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    ଜାରି;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ termp : termps) VL_DO_DANGLING(termp->delete_ବୃକ୍ଷ(), termp);
                ଫେରନ୍ତୁ resultp;
            }
            ଅଚଳ ResultTerm result = v->getResultTerm();
            termps.ଠେଲ_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            ଯଦି (std::get<2>(result)) {
                hasCleanTerm = ସତ୍ୟ;
                UINFO(9, "Clean term: " << termps.back());
            } ଅନ୍ୟଥା {
                hasDirtyTerm = ସତ୍ୟ;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_ଗଣ୍ଠି_Info, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> frozenNodes;
        // Check if frozen terms are clean or not
        ପାଇଁ (ଅଚଳ auto& frozenInfo : visitor.m_frozenNodes) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp = frozenInfo.first;
            // Comparison operators are clean
            ଯଦି ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = ସତ୍ୟ;
            } ଅନ୍ୟଥା {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = ସତ୍ୟ;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].ଠେଲ_back(termp);
        }

        // Figure out if a final negation is required
        ଅଚଳ ତାର୍କିକ needsFlip = visitor.is_Xor_ବୃକ୍ଷ() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        ଅଚଳ ତାର୍କିକ needsCleaning = visitor.is_And_ବୃକ୍ଷ() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        ପାଇଁ (ଅଚଳ auto& lsbAndNodes : frozenNodes) {
            ଯଦି (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            ଯଦି (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        ଯଦି (needsFlip) ++resultOps;
        // Add final cleaning AND
        ଯଦି (needsCleaning) ++resultOps;

        ଯଦି (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp : termps) termp->dump_ବୃକ୍ଷ("-  Reduced term: ");
            ପାଇଁ (ଅଚଳ std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, Frozen_ଗଣ୍ଠି_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_ବୃକ୍ଷ("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        ଯଦି (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultp = needsFlip ? ନୂତନ AstConst{fl, AstConst::BitTrue{}}
                                                   : ନୂତନ AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            ଫେରନ୍ତୁ resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        ଯଦି (visitor.m_ops <= resultOps) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ termp : termps) VL_DO_DANGLING(termp->delete_ବୃକ୍ଷ(), termp);
            ଫେରନ୍ତୁ nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        ଅଚଳ auto reduce = [&visitor, fl](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            ଯଦି (!lhsp) ଫେରନ୍ତୁ rhsp;
            ଯଦି (visitor.is_And_ବୃକ୍ଷ()) {
                ଫେରନ୍ତୁ ନୂତନ AstAnd{fl, lhsp, rhsp};
            } ଅନ୍ୟଥା ଯଦି (visitor.is_Or_ବୃକ୍ଷ()) {
                ଫେରନ୍ତୁ ନୂତନ AstOr{fl, lhsp, rhsp};
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ ନୂତନ AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        ପାଇଁ (auto&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* termp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            ଯଦି (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = termp->dtypep();
                termp = ନୂତନ AstShiftR{termp->fileline(), termp,
                                      ନୂତନ AstConst(termp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            ଯଦି (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = termp->dtypep();
                termp = ନୂତନ AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା maskWidth
            = std::max(resultp->width(), resultWidth == 1 ? VL_IDATASIZE : resultWidth);

        // Apply final polarity flip
        ଯଦି (needsFlip) {
            ଯଦି (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = ନୂତନ AstNot{fl, resultp};
            } ଅନ୍ୟଥା {
                // Keep result clean by using XOR(1, _)
                AstConst* ଅଚଳ maskp = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, maskWidth, 1};
                resultp = ନୂତନ AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        ଯଦି (needsCleaning) {
            AstConst* ଅଚଳ maskp = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, maskWidth, 1};
            resultp = ନୂତନ AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        ଯଦି (resultp->width() != resultWidth) {
            resultp = ନୂତନ AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        ଫେରନ୍ତୁ resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

ଶ୍ରେଣୀ ConstVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // CONSTANTS
    ସ୍ଥିର constexpr unsigned CONCAT_MERGABLE_MAX_DEPTH = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା CONCAT_ASSIGN_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    ତାର୍କିକ m_params = ମିଥ୍ୟା;  // If true, propagate parameterized and true numbers only
    ତାର୍କିକ m_required = ମିଥ୍ୟା;  // If true, must become a constant
    ତାର୍କିକ m_wremove = ସତ୍ୟ;  // Inside scope, no assignw removal
    ତାର୍କିକ m_warn = ମିଥ୍ୟା;  // Output warnings
    ତାର୍କିକ m_doExpensive = ମିଥ୍ୟା;  // Enable computationally expensive optimizations
    ତାର୍କିକ m_doCpp = ମିଥ୍ୟା;  // Enable late-stage C++ optimizations
    ତାର୍କିକ m_doNConst = ମିଥ୍ୟା;  // Enable non-constant-child simplifications
    ତାର୍କିକ m_doV = ମିଥ୍ୟା;  // Verilog, not C++ conversion
    ତାର୍କିକ m_doGenerate = ମିଥ୍ୟା;  // Postpone width checking inside generate
    ତାର୍କିକ m_convertLogicToBit = ମିଥ୍ୟା;  // Convert logical operators to bitwise
    ତାର୍କିକ m_hasJumpDelay = ମିଥ୍ୟା;  // JumpGo or Delay under this loop
    ତାର୍କିକ m_hasLoopTest = ମିଥ୍ୟା;  // Contains AstLoopTest
    ତାର୍କିକ m_underRecFunc = ମିଥ୍ୟା;  // Under a recursive function
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    ଅଚଳ AstArraySel* m_selp = nullptr;  // Current select
    ଅଚଳ AstScope* m_scopep = nullptr;  // Current scope
    ଅଚଳ AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_ପଦସମଷ୍ଟି_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_ପଦସମଷ୍ଟି_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    ଅଚଳ ତାର୍କିକ m_globalPass;  // ConstVisitor invoked as a global pass
    ସ୍ଥିର uint32_t s_global_Pass_ଅଙ୍କ;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<size_t, ତାର୍କିକ> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<ଅଚଳ AstEnumItem*> m_recursing_Enum_ବସ୍ତୁଗୁଡ଼ିକ;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    ସ୍ଥିର ତାର୍କିକ canDeleteStatements() { ଫେରନ୍ତୁ v3Global.staticsLifted(); }

    ଶୂନ୍ୟ deleteVarScopesUnder(Ast_ଗଣ୍ଠି* subtreep) {
        ଯଦି (!subtreep) ଫେରନ୍ତୁ;
        ଯଦି (!m_scopep) ଫେରନ୍ତୁ;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.ଭର୍ତ୍ତି(varp); });
        ଯଦି (varps.empty()) ଫେରନ୍ତୁ;
        ପାଇଁ (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            ଯଦି (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(ଠେଲ_Deletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3_ସଂଖ୍ୟା const_ଅଙ୍କ_V(Ast_ଗଣ୍ଠି* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        ଅଚଳ V3_ସଂଖ୍ୟା& numc = VN_AS(nodep, Const)->ଅଙ୍କ();
        ଫେରନ୍ତୁ !numc.is_ସଂଖ୍ୟା() ? numc : V3_ସଂଖ୍ୟା{nodep, nodep->widthMinV(), numc};
    }
    V3_ସଂଖ୍ୟା to_ଅଙ୍କ_C(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ V3_ସଂଖ୍ୟା& numv) {
        // Extend V width back to C width for given node
        ଫେରନ୍ତୁ !numv.is_ସଂଖ୍ୟା() ? numv : V3_ସଂଖ୍ୟା{nodep, nodep->width(), numv};
    }

    ସ୍ଥିର ତାର୍କିକ lowerAsFixedAggregate(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        ଫେରନ୍ତୁ dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp, ଅଚଳ AstMemberDType* ଅଚଳ itemp) {
        AstStructSel* ଅଚଳ selp = ନୂତନ AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        ଫେରନ୍ତୁ selp;
    }

    ଶୂନ୍ୟ collectFixedAggregateTerms(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& termps,
                                    ଅଚଳ ତାର୍କିକ packReal) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = fromp->dtypep()->skipRefp();
        ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx : unpackDtypep->declRange().seq_ବାମ_To_ଡାହାଣ()) {
                AstArraySel* ଅଚଳ selp
                    = ନୂତନ AstArraySel{fromp->fileline(), fromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(fromp), fromp);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep
                   = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଯଦି (sdtypep->packed()) {
                termps.ଠେଲ_back(fromp);
                ଫେରନ୍ତୁ;
            }
            ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(fromp), fromp);
        } ଅନ୍ୟଥା ଯଦି (packReal && dtypep->isDouble()) {
            termps.ଠେଲ_back(ନୂତନ AstRealToBits{fromp->fileline(), fromp});
        } ଅନ୍ୟଥା {
            termps.ଠେଲ_back(fromp);
        }
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* packFixedAggregate(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp) {
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> termps;
        collectFixedAggregateTerms(fromp, termps, ସତ୍ୟ);
        UASSERT(!termps.empty(), "No stream terms");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = termps[0];
        ପାଇଁ (size_t i = 1; i < termps.size(); ++i) {
            resultp = ନୂତନ AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        ଫେରନ୍ତୁ resultp;
    }

    ଶୂନ୍ୟ replaceAssignToFixedAggregate(Ast_ଗଣ୍ଠି_Assign* ଅଚଳ nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dstp,
                                       Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstWidth = dstp->dtypep()->widthStream();
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> termps;
        collectFixedAggregateTerms(dstp, termps, ମିଥ୍ୟା);
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcWidth = srcp->width();
        ଯଦି (srcWidth < dstWidth) {
            AstExtend* ଅଚଳ extendp = ନୂତନ AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = ନୂତନ AstShiftL{
                extendp->fileline(), extendp,
                ନୂତନ AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        Ast_ଗଣ୍ଠି_Assign* newp = nullptr;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା offset = 0;
        ପାଇଁ (size_t i = 0; i < termps.size(); ++i) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp = termps[i];
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = termp->dtypep()->widthStream();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = srcWidth - offset - width;
            offset += width;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp
                = ନୂତନ AstSel{srcp->fileline(), srcp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), lsb, width};
            ଯଦି (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = ନୂତନ AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_ଗଣ୍ଠି::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(srcp), srcp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ତାର୍କିକ operandConst(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ VN_IS(nodep, Const); }
    ତାର୍କିକ operandAsvConst(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        ଅଚଳ Ast_ଗଣ୍ଠି_Bi_Com_Asv* ଅଚଳ bnodep = VN_CAST(nodep, ଗଣ୍ଠି_Bi_Com_Asv);
        ଯଦି (!bnodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!VN_IS(bnodep->lhsp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_Bi_Com_Asv* ଅଚଳ rnodep = VN_CAST(bnodep->rhsp(), ଗଣ୍ଠି_Bi_Com_Asv);
        ଯଦି (!rnodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (rnodep->type() != bnodep->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (rnodep->width() != bnodep->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (rnodep->lhsp()->width() != bnodep->lhsp()->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!VN_IS(rnodep->lhsp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ operandAsvSame(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        ଅଚଳ Ast_ଗଣ୍ଠି_Bi_Com_Asv* ଅଚଳ bnodep = VN_CAST(nodep, ଗଣ୍ଠି_Bi_Com_Asv);
        ଯଦି (!bnodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_Bi_Com_Asv* ଅଚଳ rnodep = VN_CAST(bnodep->rhsp(), ଗଣ୍ଠି_Bi_Com_Asv);
        ଯଦି (!rnodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (rnodep->type() != bnodep->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (rnodep->width() != bnodep->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    ତାର୍କିକ operandAsvLUp(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
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
        ଅଚଳ Ast_ଗଣ୍ଠି_Bi_Com_Asv* ଅଚଳ bnodep = VN_CAST(nodep, ଗଣ୍ଠି_Bi_Com_Asv);
        ଯଦି (!bnodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_Bi_Com_Asv* ଅଚଳ lnodep = VN_CAST(bnodep->lhsp(), ଗଣ୍ଠି_Bi_Com_Asv);
        ଯଦି (!lnodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (lnodep->type() != bnodep->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (lnodep->width() != bnodep->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ VN_IS(lnodep->lhsp(), Const);
    }
    ତାର୍କିକ operandAsvRUp(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        ଅଚଳ Ast_ଗଣ୍ଠି_Bi_Com_Asv* ଅଚଳ bnodep = VN_CAST(nodep, ଗଣ୍ଠି_Bi_Com_Asv);
        ଯଦି (!bnodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_Bi_Com_Asv* ଅଚଳ rnodep = VN_CAST(bnodep->rhsp(), ଗଣ୍ଠି_Bi_Com_Asv);
        ଯଦି (!rnodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (rnodep->type() != bnodep->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (rnodep->width() != bnodep->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ VN_IS(rnodep->lhsp(), Const);
    }
    ସ୍ଥିର ତାର୍କିକ operandSubAdd(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ np = VN_CAST(nodep, ଗଣ୍ଠି_Biop);
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lp = VN_CAST(np->lhsp(), ଗଣ୍ଠି_Biop);
        ଫେରନ୍ତୁ (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    ତାର୍କିକ matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* ଅଚଳ constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        ଅଚଳ auto isEqOrNeq = [](ଅଚଳ Ast_ଗଣ୍ଠି* nodep) -> ତାର୍କିକ {  //
            ଫେରନ୍ତୁ VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        ଯଦି (isEqOrNeq(rhsp)) {
            ccastp = ନୂତନ AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } ଅନ୍ୟଥା ଯଦି (AstCCast* ଅଚଳ tmpp = VN_CAST(rhsp, CCast)) {
            ଯଦି (isEqOrNeq(tmpp->lhsp())) {
                ଯଦି (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } ଅନ୍ୟଥା {
                    ccastp = ନୂତନ AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        ଯଦି (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ଠେଲ_Deletep(andp), andp);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ operandAndOrSame(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ np = VN_CAST(nodep, ଗଣ୍ଠି_Biop);
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lp = VN_CAST(np->lhsp(), ଗଣ୍ଠି_Biop);
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ rp = VN_CAST(np->rhsp(), ଗଣ୍ଠି_Biop);
        ଫେରନ୍ତୁ (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    ତାର୍କିକ matchOrAndNot(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ap;
        Ast_ଗଣ୍ଠି_Biop* andp;
        ଯଦି (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଅଚଳ Ast_ଗଣ୍ଠି_Uniop* notp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* cp;
        ଯଦି (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } ଅନ୍ୟଥା ଯଦି (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bp = notp->lhsp();
        ଯଦି (!operandsSame(ap, bp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        ଯଦି (nodep->rhsp()) {
            nodep->lhsp(cp);
        } ଅନ୍ୟଥା {
            nodep->rhsp(cp);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* ଅଚଳ condp = VN_CAST(nodep->rhsp(), Cond);
        ଯଦି (!condp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstConst* ଅଚଳ maskp = VN_CAST(nodep->lhsp(), Const);
        ଯଦି (!maskp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* ଅଚଳ newp = ନୂତନ AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            ନୂତନ AstAnd{nodep->fileline(), maskp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), condp->thenp()->unlinkFrBack()},
            ନୂତନ AstAnd{nodep->fileline(), maskp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ matchCondCond(ଅଚଳ AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ଯଦି (VN_IS(nodep->backp(), Cond)) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Was checked when visited parent
        ଯଦି (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Short circuit
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> truesp;
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Can optimize further
    }
    ଶୂନ୍ୟ matchCondCondRecurse(ଅଚଳ AstCond* nodep, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& truesp,
                              std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ଯଦି (truesp.size() > 4 || falsesp.size() > 4) ଫେରନ୍ତୁ;
        ଯଦି (!nodep->condp()->isPure()) ଫେରନ୍ତୁ;
        ତାର୍କିକ replaced = ମିଥ୍ୟା;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp : truesp) {
            ଯଦି (replaced) ଭାଙ୍ଗ;
            ଯଦି (!operandsSame(nodep->condp(), condp)) ଜାରି;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_ଅଙ୍କ(nodep->condp(), 1);
            replaced = ସତ୍ୟ;
            ++m_stat_Cond_ପଦସମଷ୍ଟି_Redundant;
        }
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp : falsesp) {
            ଯଦି (replaced) ଭାଙ୍ଗ;
            ଯଦି (!operandsSame(nodep->condp(), condp)) ଜାରି;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ସତ୍ୟ;
            ++m_stat_Cond_ପଦସମଷ୍ଟି_Redundant;
        }
        ଯଦି (ଅଚଳ AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            ଯଦି (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ଯଦି (!replaced) truesp.କାଢ଼_back();
        }
        ଯଦି (ଅଚଳ AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            ଯଦି (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            ଯଦି (!replaced) falsesp.କାଢ଼_back();
        }
    }
    ଶୂନ୍ୟ matchIfCondCond(ଅଚଳ Ast_ଗଣ୍ଠି_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        ଯଦି (VN_IS(nodep->backp(), If)) ଫେରନ୍ତୁ;  // Was checked when visited parent
        ଯଦି (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) ଫେରନ୍ତୁ;  // Short circuit
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> truesp;
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    ଶୂନ୍ୟ matchIfCondCondRecurse(ଅଚଳ Ast_ଗଣ୍ଠି_If* nodep, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& truesp,
                                std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        ଯଦି (truesp.size() > 4 || falsesp.size() > 4) ଫେରନ୍ତୁ;
        ଯଦି (!nodep->condp()->isPure()) ଫେରନ୍ତୁ;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        ଯଦି (VN_IS(nodep->condp(), Const)) ଫେରନ୍ତୁ;
        ତାର୍କିକ replaced = ମିଥ୍ୟା;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp : truesp) {
            ଯଦି (replaced) ଭାଙ୍ଗ;
            ଯଦି (!operandsSame(nodep->condp(), condp)) ଜାରି;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_ଅଙ୍କ(nodep->condp(), 1);
            replaced = ସତ୍ୟ;
            ++m_stat_If_Cond_ପଦସମଷ୍ଟି_Redundant;
        }
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp : falsesp) {
            ଯଦି (replaced) ଭାଙ୍ଗ;
            ଯଦି (!operandsSame(nodep->condp(), condp)) ଜାରି;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = ସତ୍ୟ;
            ++m_stat_If_Cond_ପଦସମଷ୍ଟି_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_If* subIfp = VN_CAST(nodep->thensp(), ଗଣ୍ଠି_If)) {
            ଯଦି (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ଯଦି (!replaced) truesp.କାଢ଼_back();
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_If* subIfp = VN_CAST(nodep->elsesp(), ଗଣ୍ଠି_If)) {
            ଯଦି (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            ଯଦି (!replaced) falsesp.କାଢ଼_back();
        }
    }

    ଶୂନ୍ୟ matchIfSameLast(Ast_ଗଣ୍ଠି_If* nodep) {
        ଯଦି (!nodep->thensp()) ଫେରନ୍ତୁ;
        ଯଦି (!nodep->elsesp()) ଫେରନ୍ତୁ;
        ଯଦି (!canDeleteStatements()) ଫେରନ୍ତୁ;
        // Iterate bodies in reverse
        Ast_ଗଣ୍ଠି* tp = nodep->thensp()->lastp();
        Ast_ଗଣ୍ଠି* ep = nodep->elsesp()->lastp();
        ଯେତେବେଳେ (tp && ep && tp->same_ବୃକ୍ଷ(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_ଗଣ୍ଠି* ଅଚଳ tPrevp = tp->prevp();
            Ast_ଗଣ୍ଠି* ଅଚଳ ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(ep->unlinkFrBack()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    ତାର୍କିକ matchMaskedOr(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(nodep->lhsp(), Const);
        ଅଚଳ uint32_t significantBits = constp->ଅଙ୍କ().widthToFit();

        AstOr* ଅଚଳ orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        ଅଚଳ auto checkBottomClear = [=](ଅଚଳ Ast_ଗଣ୍ଠି* nodep) -> ତାର୍କିକ {
            ଯଦି (ଅଚଳ AstShiftL* ଅଚଳ shiftp = VN_CAST(nodep, ShiftL)) {
                ଯଦି (ଅଚଳ AstConst* ଅଚଳ scp = VN_CAST(shiftp->rhsp(), Const)) {
                    ଫେରନ୍ତୁ scp->ଅଙ୍କ().toUInt() >= significantBits;
                }
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        };

        ଅଚଳ ତାର୍କିକ orLIsRedundant = checkBottomClear(orp->lhsp());
        ଅଚଳ ତାର୍କିକ orRIsRedundant = checkBottomClear(orp->rhsp());

        ଯଦି (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                ନୂତନ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(orp), orp);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;  // input node is still valid, keep going
        } ଅନ୍ୟଥା ଯଦି (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(orp), orp);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;  // input node is still valid, keep going
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
    }
    ତାର୍କିକ matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        ଅଚଳ auto checkMask = [nodep, ଏହି](ଅଚଳ V3_ସଂଖ୍ୟା& mask) -> ତାର୍କିକ {
            ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(nodep->lhsp(), Const);
            ଯଦି (constp->ଅଙ୍କ().isCaseEq(mask)) {
                Ast_ଗଣ୍ଠି* ଅଚଳ rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        };

        // Check if masking is redundant
        ଯଦି (ଅଚଳ AstShiftR* ଅଚଳ shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_ସଂଖ୍ୟା maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->ଅଙ୍କ().toUInt());
                ଫେରନ୍ତୁ checkMask(maskLo);
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstShiftL* ଅଚଳ shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_ସଂଖ୍ୟା mask{nodep, nodep->width()};
                ଅଚଳ uint32_t shiftAmount = scp->ଅଙ୍କ().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                ଫେରନ୍ତୁ checkMask(mask);
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ତାର୍କିକ matchMaskedZero(ଅଚଳ AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        ଅଚଳ AstConst* ଅଚଳ maskp = VN_AS(nodep->lhsp(), Const);
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp();
        ଅଚଳ uint32_t msbP1 = maskp->ଅଙ୍କ().mostSetBitP1();
        ଯଦି (!msbP1) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Don't rewrite, separate rule matches for this
        ଅଚଳ uint32_t msb = msbP1 - 1;
        ଅଚଳ uint32_t lsb = maskp->ଅଙ୍କ().leastSetBitP1() - 1;

        ଯଦି (ଅଚଳ AstShiftL* ଅଚଳ shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            ଯଦି (AstConst* ଅଚଳ scp = VN_CAST(shiftp->rhsp(), Const)) {
                ଫେରନ୍ତୁ scp->ଅଙ୍କ().fitsInUInt()  //
                       && (scp->ଅଙ୍କ().toUInt() > msb);
            }
        }
        ଯଦି (ଅଚଳ AstShiftR* ଅଚଳ shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            ଯଦି (AstConst* ଅଚଳ scp = VN_CAST(shiftp->rhsp(), Const)) {
                ଫେରନ୍ତୁ scp->ଅଙ୍କ().fitsInUInt()
                       && (lsb + scp->ଅଙ୍କ().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        ଯଦି (ଅଚଳ AstMul* ଅଚଳ mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            ଯଦି (AstConst* ଅଚଳ cp = VN_CAST(mulp->lhsp(), Const)) {
                ଫେରନ୍ତୁ cp->ଅଙ୍କ().leastSetBitP1() > msb + 1;
            }
        }
        ଯଦି (ଅଚଳ AstExtend* ଅଚଳ extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            ଫେରନ୍ତୁ lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ତାର୍କିକ match_Bit_Op_ବୃକ୍ଷ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (nodep->widthMin() != 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!v3Global.opt.f_Const_Bit_Op_ବୃକ୍ଷ()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->width();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rootp = nodep;
        unsigned externalOps = 0;
        // Reach past a plain making AND
        ଯଦି (ଅଚଳ AstAnd* ଅଚଳ andp = VN_CAST(nodep, And)) {
            ଯଦି (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        ଯଦି (rootp->type() == nodep->backp()->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        std::string debugPrefix;
        ଯଦି (debug() >= 9) {  // LCOV_EXCL_START
            ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dump_ବୃକ୍ଷ(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = Const_Bit_Op_ବୃକ୍ଷ_Visitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        ଯଦି (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }

        ଯଦି (debug() >= 9) {  // LCOV_EXCL_START
            ଯଦି (newp) {
                newp->dump_ବୃକ୍ଷ(debugPrefix + "RESULT: ");
            } ଅନ୍ୟଥା {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        ଫେରନ୍ତୁ newp;
    }
    ସ୍ଥିର ତାର୍କିକ operandShiftSame(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ np = VN_AS(nodep, ଗଣ୍ଠି_Biop);
        {
            ଅଚଳ AstShiftL* ଅଚଳ lp = VN_CAST(np->lhsp(), ShiftL);
            ଅଚଳ AstShiftL* ଅଚଳ rp = VN_CAST(np->rhsp(), ShiftL);
            ଯଦି (lp && rp) {
                ଫେରନ୍ତୁ (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            ଅଚଳ AstShiftR* ଅଚଳ lp = VN_CAST(np->lhsp(), ShiftR);
            ଅଚଳ AstShiftR* ଅଚଳ rp = VN_CAST(np->rhsp(), ShiftR);
            ଯଦି (lp && rp) {
                ଫେରନ୍ତୁ (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ତାର୍କିକ operandHugeShiftL(ଅଚଳ Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଫେରନ୍ତୁ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    ତାର୍କିକ operandHugeShiftR(ଅଚଳ Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଫେରନ୍ତୁ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    ତାର୍କିକ operandIsTwo(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const);
        ଯଦି (!constp) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // not constant
        ଯଦି (constp->ଅଙ୍କ().isFourState()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // four-state
        ଯଦି (nodep->width() > VL_QUADSIZE) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // too wide
        ଯଦି (nodep->isSigned() && constp->ଅଙ୍କ().isNegative()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // signed and negative
        ଫେରନ୍ତୁ constp->toUQuad() == 2;
    }
    ତାର୍କିକ operandIsTwostate(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->ଅଙ୍କ().isFourState());
    }
    ତାର୍କିକ operandIsPowTwo(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!operandIsTwostate(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ (1 == VN_AS(nodep, Const)->ଅଙ୍କ().ଗଣନା_Ones());
    }
    ତାର୍କିକ operandShiftOp(ଅଚଳ Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଯଦି (!VN_IS(nodep->rhsp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lhsp = VN_CAST(nodep->lhsp(), ଗଣ୍ଠି_Biop);
        ଯଦି (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->width() != lhsp->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->width() != lhsp->lhsp()->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->width() != lhsp->rhsp()->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        ଯଦି (VN_IS(lhsp->lhsp(), Const)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (VN_IS(lhsp->rhsp(), Const)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // - Const shift on either side
        ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ llp = VN_CAST(lhsp->lhsp(), ଗଣ୍ଠି_Biop)) {
            ଫେରନ୍ତୁ (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        ଯଦି (Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lrp = VN_CAST(lhsp->rhsp(), ଗଣ୍ଠି_Biop)) {
            ଫେରନ୍ତୁ (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ତାର୍କିକ operandShiftShift(ଅଚଳ Ast_ଗଣ୍ଠି_Biop* nodep) {
        // We could add a AND though.
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lhsp = VN_CAST(nodep->lhsp(), ଗଣ୍ଠି_Biop);
        ଯଦି (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        ଯଦି (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->ଅଙ୍କ().isFourState())
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->width() != lhsp->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->width() != lhsp->lhsp()->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ operandWordOOB(ଅଚଳ AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        ଫେରନ୍ତୁ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ଗଣ୍ଠି_Var_Ref)
                && VN_AS(nodep->fromp(), ଗଣ୍ଠି_Var_Ref)->access().isReadOnly()
                && (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), ଗଣ୍ଠି_Var_Ref)->varp()->width_ଶବ୍ଦଗୁଡ଼ିକ()));
    }
    ତାର୍କିକ operandSelFull(ଅଚଳ AstSel* nodep) {
        ଫେରନ୍ତୁ (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(nodep->widthConst()) == nodep->fromp()->width());
    }
    ତାର୍କିକ operandSelExtend(AstSel* nodep) {
        ଯଦି (!m_doV) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstExtend* ଅଚଳ extendp = VN_CAST(nodep->fromp(), Extend);
        ଯଦି (!extendp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstConst* ଅଚଳ lsbp = VN_CAST(nodep->lsbp(), Const);
        ଯଦି (!lsbp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->widthConst();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = lsbp->toSInt();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lsb + width - 1;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = extendp->lhsp();
        ଯଦି (!lhsp->isPure()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Selecting the entire extended expression, replace with that
        ଯଦି (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // Select entirely in the extended part - replace with zero
        ଯଦି (lsb >= lhsp->width()) {
            replaceZero(nodep);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // Select entirely in the extended expression - replace with select from that
        ଯଦି (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(ନୂତନ AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // Select straddles both sides, but is just a shorter extend
        ଯଦି (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(ନୂତନ AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ତାର୍କିକ operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ bip = VN_CAST(nodep->fromp(), ଗଣ୍ଠି_Biop);
        ଯଦି (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bilhsp = bip->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(ନୂତନ AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(ନୂତନ AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        ଅଚଳ AstShiftR* ଅଚଳ shiftp = VN_CAST(nodep->fromp(), ShiftR);
        ଯଦି (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ap = shiftp->lhsp();
        ଅଚଳ AstConst* ଅଚଳ bp = VN_AS(shiftp->rhsp(), Const);
        ଅଚଳ AstConst* ଅଚଳ lp = VN_AS(nodep->lsbp(), Const);
        ଯଦି (bp->isWide() || bp->ଅଙ୍କ().isFourState() || bp->ଅଙ୍କ().isNegative() || lp->isWide()
            || lp->ଅଙ୍କ().isFourState() || lp->ଅଙ୍କ().isNegative()) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା newLsb = lp->toSInt() + bp->toSInt();
        ଯଦି (newLsb + nodep->widthConst() > ap->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* ଅଚଳ newp
            = ନୂତନ AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ operandBiExtendConstShrink(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* ଅଚଳ extendp = VN_CAST(nodep->rhsp(), Extend);
        ଯଦି (!extendp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ smallerp = extendp->lhsp();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା subsize = smallerp->width();
        AstConst* ଅଚଳ constp = VN_CAST(nodep->lhsp(), Const);
        ଯଦି (!constp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!constp->ଅଙ୍କ().isBitsZero(constp->width() - 1, subsize)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ{constp, subsize, constp->ଅଙ୍କ()};
        nodep->lhsp(ନୂତନ AstConst{constp->fileline(), ଅଙ୍କ});
        VL_DO_DANGLING(ଠେଲ_Deletep(constp), constp);
        ଫେରନ୍ତୁ ମିଥ୍ୟା;  // input node is still valid, keep going
    }
    ତାର୍କିକ operandBiExtendConstOver(ଅଚଳ Ast_ଗଣ୍ଠି_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        ଅଚଳ AstExtend* ଅଚଳ extendp = VN_CAST(nodep->rhsp(), Extend);
        ଯଦି (!extendp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ smallerp = extendp->lhsp();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା subsize = smallerp->width();
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->lhsp(), Const);
        ଯଦି (!constp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (constp->ଅଙ୍କ().isBitsZero(constp->width() - 1, subsize)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Extraction checks
    ତାର୍କିକ warnSelect(AstSel* nodep) {
        ଯଦି (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        ଅଚଳ ତାର୍କିକ doit = ସତ୍ୟ;
        ଯଦି (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            ଯଦି (VN_AS(nodep->lsbp(), Const)->ଅଙ୍କ().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } ଅନ୍ୟଥା ଯଦି (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                ଯଦି (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } ଅନ୍ୟଥା {
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
        ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Not a transform, so NOP
    }

    ସ୍ଥିର ତାର୍କିକ operandsSame(ଅଚଳ Ast_ଗଣ୍ଠି* node1p, ଅଚଳ Ast_ଗଣ୍ଠି* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ const1p = VN_CAST(node1p, Const)) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ const2p = VN_CAST(node2p, Const)) {
                ଫେରନ୍ତୁ V3_ସଂଖ୍ୟା{node1p->fileline(), 1, 0}
                    .opEq(const1p->ଅଙ୍କ(), const2p->ଅଙ୍କ())
                    .isEqOne();
            }
        }
        ଯଦି (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            ଫେରନ୍ତୁ node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        ଯଦି (ଅଚଳ AstAnd* ଅଚଳ and1p = VN_CAST(node1p, And)) {
            ଯଦି (ଅଚଳ AstAnd* ଅଚଳ and2p = VN_CAST(node2p, And)) {
                ଯଦି (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), ଗଣ୍ଠି_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), ଗଣ୍ଠି_Var_Ref))
                    ଫେରନ୍ତୁ node1p->same_Gate_ବୃକ୍ଷ(node2p);
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ତାର୍କିକ ifSameAssign(ଅଚଳ Ast_ଗଣ୍ଠି_If* nodep) {
        ଅଚଳ Ast_ଗଣ୍ଠି_Assign* ଅଚଳ thensp = VN_CAST(nodep->thensp(), ଗଣ୍ଠି_Assign);
        ଅଚଳ Ast_ଗଣ୍ଠି_Assign* ଅଚଳ elsesp = VN_CAST(nodep->elsesp(), ଗଣ୍ଠି_Assign);
        ଯଦି (!thensp || thensp->nextp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Must be SINGLE statement
        ଯଦି (!elsesp || elsesp->nextp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (thensp->type() != elsesp->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Can't mix an assigndly with assign
        ଯଦି (!thensp->lhsp()->same_Gate_ବୃକ୍ଷ(elsesp->lhsp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!thensp->rhsp()->gate_ବୃକ୍ଷ()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!elsesp->rhsp()->gate_ବୃକ୍ଷ()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Must not create an expression with unpacked array type
        ଯଦି (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (m_underRecFunc) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // This optimization may lead to infinite recursion
        ଯଦି (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            ଯଦି (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        ଫେରନ୍ତୁ !VN_IS(thensp->rhsp(), ଗଣ୍ଠି_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), ଗଣ୍ଠି_FTask_Ref)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    ତାର୍କିକ operandIfIf(ଅଚଳ Ast_ଗଣ୍ଠି_If* nodep) {
        ଯଦି (nodep->elsesp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_If* ଅଚଳ lowerIfp = VN_CAST(nodep->thensp(), ଗଣ୍ଠି_If);
        ଯଦି (!lowerIfp || lowerIfp->nextp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->type() != lowerIfp->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (Ast_ଗଣ୍ଠି::afterCommentp(lowerIfp->elsesp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ ifConcatMergeableBiop(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    ତାର୍କିକ ifAdjacentSel(ଅଚଳ AstSel* lhsp, ଅଚଳ AstSel* rhsp) {
        ଯଦି (!v3Global.opt.fAssemble()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // opt disabled
        ଯଦି (!lhsp || !rhsp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ lfromp = lhsp->fromp();
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ rfromp = rhsp->fromp();
        ଯଦି (!lfromp || !rfromp || !lfromp->same_Gate_ବୃକ୍ଷ(rfromp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstConst* ଅଚଳ lstart = VN_CAST(lhsp->lsbp(), Const);
        ଅଚଳ AstConst* ଅଚଳ rstart = VN_CAST(rhsp->lsbp(), Const);
        ଯଦି (!lstart || !rstart) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // too complicated
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rend = (rstart->toSInt() + rhsp->widthConst());
        ଫେରନ୍ତୁ (rend == lstart->toSInt());
    }
    ତାର୍କିକ ifMergeAdjacent(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        ଯଦି (!v3Global.opt.fAssemble()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // opt disabled
        // Two same varref
        ଯଦି (operandsSame(lhsp, rhsp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଅଚଳ AstSel* ଅଚଳ lselp = VN_CAST(lhsp, Sel);
        ଅଚଳ AstSel* ଅଚଳ rselp = VN_CAST(rhsp, Sel);
        ଯଦି (!lselp && !rselp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (lselp && !VN_IS(lselp->lsbp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (rselp && !VN_IS(rselp->lsbp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // a[i:j] {a[j-1:k], b}
        ଯଦି (lselp && !rselp && VN_IS(rhsp, Concat))
            ଫେରନ୍ତୁ ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        ଯଦି (rselp && !lselp && VN_IS(lhsp, Concat))
            ଫେରନ୍ତୁ ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_ବୃକ୍ଷ(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_ବୃକ୍ଷ(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        ଯଦି (!lfromp || !rfromp || !lfromp->same_Gate_ବୃକ୍ଷ(rfromp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ଅଚଳ int32_t lstart = lselp ? lselp->lsbConst() : 0;
        ଅଚଳ int32_t rstart = rselp ? rselp->lsbConst() : 0;
        ଅଚଳ int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        ଯଦି (rend == lstart) ଫେରନ୍ତୁ ସତ୍ୟ;
        // a[i:0] a[msb:j]
        ଯଦି (rend == rfromp->width() && lstart == 0) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ତାର୍କିକ concatMergeable(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, unsigned depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        ଯଦି (!v3Global.opt.fAssemble()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // opt disabled
        ଯଦି (lhsp->type() != rhsp->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!ifConcatMergeableBiop(lhsp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (depth > CONCAT_MERGABLE_MAX_DEPTH) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // As worse case O(n^2) algorithm

        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lp = VN_CAST(lhsp, ଗଣ୍ଠି_Biop);
        ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ rp = VN_CAST(rhsp, ଗଣ୍ଠି_Biop);
        ଯଦି (!lp || !rp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // {a[]&b[], a[]&b[]}
        ଅଚଳ ତାର୍କିକ lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        ଅଚଳ ତାର୍କିକ rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        ଯଦି (lad && rad) ଫେରନ୍ତୁ ସତ୍ୟ;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        ଯଦି (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        ଯଦି (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        ଯଦି (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ସ୍ଥିର ତାର୍କିକ operandsSameWidth(ଅଚଳ Ast_ଗଣ୍ଠି* lhsp, ଅଚଳ Ast_ଗଣ୍ଠି* rhsp) {
        ଫେରନ୍ତୁ lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    ଶୂନ୍ୟ replace_ଅଙ୍କ(Ast_ଗଣ୍ଠି* oldp, ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ, ଅଚଳ string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->ଅଙ୍କ().isFourState()), oldp,
                    "Already constant??");
        AstConst* ଅଚଳ newp = ନୂତନ AstConst{oldp->fileline(), ଅଙ୍କ};
        ଯଦି (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ଠେଲ_Deletep(oldp), oldp);
    }
    ଶୂନ୍ୟ replace_ଅଙ୍କ(Ast_ଗଣ୍ଠି* nodep, uint32_t val) {
        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ{nodep, nodep->width(), val};
        VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ଅଙ୍କ), nodep);
    }
    ଶୂନ୍ୟ replace_ଅଙ୍କ_Signed(Ast_ଗଣ୍ଠି_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        ଯଦି (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, val), nodep);
    }
    ଶୂନ୍ୟ replace_ଅଙ୍କ_Limited(Ast_ଗଣ୍ଠି_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        ଯଦି (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, val), nodep);
    }
    ଶୂନ୍ୟ replaceZero(Ast_ଗଣ୍ଠି* nodep) { VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, 0), nodep); }
    ଶୂନ୍ୟ replaceZeroChkPure(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        ଯଦି (checkp->isPure()) {
            VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, 0), nodep);
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstAnd{nodep->fileline(), ନୂତନ AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ replaceAllOnes(Ast_ଗଣ୍ଠି* nodep) {
        V3_ସଂଖ୍ୟା ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ones), nodep);
    }
    ଶୂନ୍ୟ replaceConst(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        V3_ସଂଖ୍ୟା numv{nodep, nodep->widthMinV()};
        nodep->ସଂଖ୍ୟା_Operate(numv, const_ଅଙ୍କ_V(nodep->lhsp()));
        ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = to_ଅଙ୍କ_C(nodep, numv);
        UINFO(4, "UNICONST -> " << ଅଙ୍କ);
        VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ଅଙ୍କ), nodep);
    }
    ଶୂନ୍ୟ replaceConst(Ast_ଗଣ୍ଠି_Biop* nodep) {
        V3_ସଂଖ୍ୟା numv{nodep, nodep->widthMinV()};
        nodep->ସଂଖ୍ୟା_Operate(numv, const_ଅଙ୍କ_V(nodep->lhsp()), const_ଅଙ୍କ_V(nodep->rhsp()));
        ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = to_ଅଙ୍କ_C(nodep, numv);
        UINFO(4, "BICONST -> " << ଅଙ୍କ);
        VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ଅଙ୍କ), nodep);
    }
    ଶୂନ୍ୟ replaceConst(Ast_ଗଣ୍ଠି_Triop* nodep) {
        V3_ସଂଖ୍ୟା numv{nodep, nodep->widthMinV()};
        nodep->ସଂଖ୍ୟା_Operate(numv, const_ଅଙ୍କ_V(nodep->lhsp()), const_ଅଙ୍କ_V(nodep->rhsp()),
                             const_ଅଙ୍କ_V(nodep->thsp()));
        ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = to_ଅଙ୍କ_C(nodep, numv);
        UINFO(4, "TRICONST -> " << ଅଙ୍କ);
        VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ଅଙ୍କ), nodep);
    }
    ଶୂନ୍ୟ replaceConst(Ast_ଗଣ୍ଠି_Quadop* nodep) {
        V3_ସଂଖ୍ୟା numv{nodep, nodep->widthMinV()};
        nodep->ସଂଖ୍ୟା_Operate(numv, const_ଅଙ୍କ_V(nodep->lhsp()), const_ଅଙ୍କ_V(nodep->rhsp()),
                             const_ଅଙ୍କ_V(nodep->thsp()), const_ଅଙ୍କ_V(nodep->fhsp()));
        ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = to_ଅଙ୍କ_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << ଅଙ୍କ);
        VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ଅଙ୍କ), nodep);
    }

    ଶୂନ୍ୟ replaceConstString(Ast_ଗଣ୍ଠି* oldp, ଅଚଳ string& ଅଙ୍କ) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstConst{oldp->fileline(), AstConst::String{}, ଅଙ୍କ};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    ଶୂନ୍ୟ replaceWChild(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ replaceWChildBool(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        ଯଦି (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } ଅନ୍ୟଥା {
            nodep->replaceWithKeepDType(ନୂତନ AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    ଶୂନ୍ୟ replaceWIteratedRhs(Ast_ଗଣ୍ଠି_Triop* nodep) {
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    ଶୂନ୍ୟ replaceWIteratedThs(Ast_ଗଣ୍ଠି_Triop* nodep) {
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    ଶୂନ୍ୟ replaceWLhs(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ଶୂନ୍ୟ replaceWLhs(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ଶୂନ୍ୟ replaceWRhs(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    ଶୂନ୍ୟ replaceWLhsBool(Ast_ଗଣ୍ଠି_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    ଶୂନ୍ୟ replaceWRhsBool(Ast_ଗଣ୍ଠି_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    ଶୂନ୍ୟ replaceAsv(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ap = nodep->lhsp();
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ rp = VN_AS(nodep->rhsp(), ଗଣ୍ଠି_Biop);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bp = rp->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        ଯଦି (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    ଶୂନ୍ୟ replaceAsvLUp(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ଗଣ୍ଠି_Biop);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ llp = lp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ଶୂନ୍ୟ replaceAsvRUp(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lp = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ଗଣ୍ଠି_Biop);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ଶୂନ୍ୟ replaceAndOr(Ast_ଗଣ୍ଠି_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ଗଣ୍ଠି_Biop);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ llp = lp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ଗଣ୍ଠି_Biop);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        ଯଦି (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ଠେଲ_Deletep(rp), rp);
            VL_DO_DANGLING(ଠେଲ_Deletep(rlp), rlp);
        } ଅନ୍ୟଥା ଯଦି (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ଠେଲ_Deletep(rp), rp);
            VL_DO_DANGLING(ଠେଲ_Deletep(lrp), lrp);
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    ଶୂନ୍ୟ replaceShiftSame(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ଗଣ୍ଠି_Biop);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ llp = lp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ଗଣ୍ଠି_Biop);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ଠେଲ_Deletep(rp), rp);
        VL_DO_DANGLING(ଠେଲ_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    ଶୂନ୍ୟ replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* ଅଚଳ lselp = VN_AS(nodep->lhsp()->unlinkFrBack(), Sel);
        AstSel* ଅଚଳ rselp = VN_AS(nodep->rhsp()->unlinkFrBack(), Sel);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lstart = lselp->lsbConst();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lwidth = lselp->widthConst();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rstart = rselp->lsbConst();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rwidth = rselp->widthConst();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* ଅଚଳ newselp = ନୂତନ AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlinkFrBack(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(ଠେଲ_Deletep(lselp), lselp);
        VL_DO_DANGLING(ଠେଲ_Deletep(rselp), rselp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lp = VN_AS(nodep->lhsp(), ଗଣ୍ଠି_Biop);
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ rp = VN_AS(nodep->rhsp(), ଗଣ୍ଠି_Biop);
        ଯଦି (concatMergeable(lp, rp, 0)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ llp = lp->lhsp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lrp = lp->rhsp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rlp = rp->lhsp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rrp = rp->rhsp();
            AstConcat* ଅଚଳ newlp = ନୂତନ AstConcat{rlp->fileline(), llp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                   rlp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            AstConcat* ଅଚଳ newrp = ନୂତନ AstConcat{rrp->fileline(), lrp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                   rrp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ଠେଲ_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ଠେଲ_Deletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(ଠେଲ_Deletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    ଶୂନ୍ୟ replaceExtend(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ replacePowShift(Ast_ଗଣ୍ଠି_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ଅଚଳ newp
            = ନୂତନ AstShiftL{nodep->fileline(), ନୂତନ AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା amount = VN_AS(nodep->lhsp(), Const)->ଅଙ୍କ().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* ଅଚଳ newp
            = ନୂତନ AstShiftL{nodep->fileline(), opp, ନୂତନ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା amount = VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* ଅଚଳ newp
            = ନୂତନ AstShiftR{nodep->fileline(), opp, ନୂତନ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା amount = VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().mostSetBitP1() - 1;  // 2^n->n+1
        V3_ସଂଖ୍ୟା mask{nodep, nodep->width()};
        mask.setMask(amount);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* ଅଚଳ newp
            = ନୂତନ AstAnd{nodep->fileline(), opp, ନୂତନ AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ replaceShiftOp(Ast_ଗଣ୍ଠି_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->width();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lhsp = VN_AS(nodep->lhsp(), ଗଣ୍ଠି_Biop);
        lhsp->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ shiftp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ap = lhsp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bp = lhsp->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ shift1p = nodep;
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ shift2p = nodep->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    ଶୂନ୍ୟ replaceShiftShift(Ast_ଗଣ୍ଠି_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ lhsp = VN_AS(nodep->lhsp(), ଗଣ୍ଠି_Biop);
        lhsp->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ap = lhsp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ shift1p = lhsp->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        ଯଦି (nodep->type() == lhsp->type()) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା shift1 = VN_AS(shift1p, Const)->toUInt();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା shift2 = VN_AS(shift2p, Const)->toUInt();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା newshift = shift1 + shift2;
            VL_DO_DANGLING(ଠେଲ_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ଠେଲ_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(ନୂତନ AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } ଅନ୍ୟଥା {
            // We know shift amounts are constant, but might be a mixed left/right shift
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା shift1 = VN_AS(shift1p, Const)->toUInt();
            ଯଦି (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା shift2 = VN_AS(shift2p, Const)->toUInt();
            ଯଦି (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା newshift = shift1 + shift2;
            VL_DO_DANGLING(ଠେଲ_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ଠେଲ_Deletep(shift2p), shift2p);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
            V3_ସଂଖ୍ୟା mask1{nodep, nodep->width()};
            V3_ସଂଖ୍ୟା ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            ଯଦି (shift1 < 0) {
                mask1.opShiftR(ones, V3_ସଂଖ୍ୟା(nodep, VL_IDATASIZE, -shift1));
            } ଅନ୍ୟଥା {
                mask1.opShiftL(ones, V3_ସଂଖ୍ୟା(nodep, VL_IDATASIZE, shift1));
            }
            V3_ସଂଖ୍ୟା mask{nodep, nodep->width()};
            ଯଦି (shift2 < 0) {
                mask.opShiftR(mask1, V3_ସଂଖ୍ୟା(nodep, VL_IDATASIZE, -shift2));
            } ଅନ୍ୟଥା {
                mask.opShiftL(mask1, V3_ସଂଖ୍ୟା(nodep, VL_IDATASIZE, shift2));
            }
            ଯଦି (newshift < 0) {
                newp = ନୂତନ AstShiftR{nodep->fileline(), ap,
                                     ନୂତନ AstConst(nodep->fileline(), -newshift)};
            } ଅନ୍ୟଥା {
                newp = ନୂତନ AstShiftL{nodep->fileline(), ap,
                                     ନୂତନ AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = ନୂତନ AstAnd{nodep->fileline(), newp, ନୂତନ AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(lhsp), lhsp);
    }

    ତାର୍କିକ replaceAssignMultiSel(Ast_ଗଣ୍ଠି_Assign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        ଯଦି (!m_modp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ଅଚଳ sel1p = VN_CAST(nodep->lhsp(), Sel);
        ଯଦି (!sel1p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ nextp = VN_CAST(nodep->nextp(), ଗଣ୍ଠି_Assign);
        ଯଦି (!nextp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nodep->type() != nextp->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstSel* ଅଚଳ sel2p = VN_CAST(nextp->lhsp(), Sel);
        ଯଦି (!sel2p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstVarRef* ଅଚଳ varref1p = VN_CAST(sel1p->fromp(), VarRef);
        ଯଦି (!varref1p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVarRef* ଅଚଳ varref2p = VN_CAST(sel2p->fromp(), VarRef);
        ଯଦି (!varref2p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!varref1p->same_Gate_ବୃକ୍ଷ(varref2p)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstConst* ଅଚଳ con1p = VN_CAST(sel1p->lsbp(), Const);
        ଯଦି (!con1p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstConst* ଅଚଳ con2p = VN_CAST(sel2p->lsbp(), Const);
        ଯଦି (!con2p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        ଯଦି (!varNotReferenced(nodep->rhsp(), varref1p->varp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!varNotReferenced(nextp->rhsp(), varref2p->varp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // If a variable is marked split_var, access to the variable should not be merged.
        ଯଦି (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Swap?
        ଯଦି ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଅଚଳ ତାର୍କିକ lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhs2p = nextp->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_Assign* newp;
        ଯଦି (lsbFirstAssign) {
            newp = nodep->cloneType(ନୂତନ AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    ନୂତନ AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } ଅନ୍ୟଥା {
            newp = nodep->cloneType(ନୂତନ AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    ନୂତନ AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        VL_DO_DANGLING(ଠେଲ_Deletep(nextp->unlinkFrBack()), nextp);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ replaceAssignSFormatF(Ast_ଗଣ୍ଠି_Assign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        ଯଦି (!VN_IS(nodep, Assign)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstVarRef* ଅଚଳ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        ଯଦି (!lhsp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstSFormatF* ଅଚଳ fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        ଯଦି (!fmtp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(ନୂତନ AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ varNotReferenced(Ast_ଗଣ୍ଠି* nodep, AstVar* varp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        ଯଦି (!nodep) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (level > 2) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!nodep->isPure()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // For example a $fgetc can't be reordered
        ଯଦି (VN_IS(nodep, ଗଣ୍ଠି_Var_Ref) && VN_AS(nodep, ଗଣ୍ଠି_Var_Ref)->varp() == varp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    ତାର୍କିକ replace_ଗଣ୍ଠି_Assign(Ast_ଗଣ୍ଠି_Assign* nodep) {
        ଯଦି (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            ଯଦି (VN_IS(nodep, AssignW)) {
                ଯଦି (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Don't delete the assign, or V3Gate will freak out
            } ଅନ୍ୟଥା {
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        } ଅନ୍ୟଥା ଯଦି (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            ତାର୍କିକ need_temp = ମିଥ୍ୟା;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            ଅଚଳ ତାର୍କିକ waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            ଅଚଳ ତାର୍କିକ needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            ଯଦି (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<size_t> lhsVarIds;
                nodep->lhsp()->foreach([&](ଅଚଳ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](ଅଚଳ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    ଯଦି (lhsVarIds.ଗଣନା_2(m_ids(refp->varp()))) need_temp = ସତ୍ୟ;
                });
            }
            ଯଦି (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* ଅଚଳ tempPurep = ନୂତନ AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* ଅଚଳ tempPureRefp
                    = ନୂତନ AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asnp
                    = VN_IS(nodep, AssignDly)
                          ? ନୂତନ AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(ନୂତନ AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                ଯଦି (waits) ଠେଲ_Deletep(nodep->timingControlp()->unlinkFrBack());
            } ଅନ୍ୟଥା ଯଦି (need_temp) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } ଅନ୍ୟଥା {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* ଅଚଳ delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            ଯଦି (delayp && !delayp->lhsp()->isPure()) {
                File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = delayp->lhsp()->unlinkFrBack();
                AstVar* ଅଚଳ tempp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswapNames.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempp);
                nodep->addHereThisAsNext(
                    ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(ନୂତନ AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhs2p = rhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            // Calc widths
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb2 = 0;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb2 = lsb2 + lc2p->width() - 1;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb1 = msb2 + 1;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb1 = lsb1 + lc1p->width() - 1;
            UASSERT_OBJ(msb1 == (conp->width() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* ଅଚଳ sel1p = ନୂତନ AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* ଅଚଳ sel2p = ନୂତନ AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_ଗଣ୍ଠି_Assign* newp = nullptr;
            ଯଦି (!need_temp) {
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_ଗଣ୍ଠି::addNext(newp, asn1ap);
                newp = Ast_ଗଣ୍ଠି::addNext(newp, asn2ap);
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* ଅଚଳ temp1p
                    = ନୂତନ AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* ଅଚଳ temp2p
                    = ନୂତନ AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->addStmtsp(temp1p);
                m_modp->addStmtsp(temp2p);
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asn1ap = nodep->cloneType(
                    ନୂତନ AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asn2ap = nodep->cloneType(
                    ନୂତନ AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asn1bp = nodep->cloneType(
                    lc1p, ନୂତନ AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ asn2bp = nodep->cloneType(
                    lc2p, ନୂତନ AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_ଗଣ୍ଠି::addNext(newp, asn1ap);
                newp = Ast_ଗଣ୍ଠି::addNext(newp, asn2ap);
                newp = Ast_ଗଣ୍ଠି::addNext(newp, asn1bp);
                newp = Ast_ଗଣ୍ଠି::addNext(newp, asn2bp);
            }
            ଯଦି (debug() >= 9 && newp) newp->dump_ବୃକ୍ଷ_And_Next(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(ଠେଲ_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* ଅଚଳ cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp = cvtp->fromp()->unlinkFrBack();
            ଯଦି (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_Stream* ଅଚଳ streamp = VN_CAST(srcp, ଗଣ୍ଠି_Stream)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ streamSrcp = streamp->lhsp();
                ଯଦି (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* ଅଚଳ streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp = streamp->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ srcDTypep = srcp->dtypep()->skipRefp();
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ଯଦି (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                ଯଦି (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcElementBits = 0;
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstElementBits = 0;
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = ନୂତନ AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), ମିଥ୍ୟା, 1,
                        dstElementBits,   srcElementBits};
                } ଅନ୍ୟଥା {
                    srcp = ନୂତନ AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } ଅନ୍ୟଥା ଯଦି (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = ନୂତନ AstCvtArrayToPacked{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା packedBits = nodep->lhsp()->widthMin();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା unpackBits
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->widthMin();
                ଅଚଳ uint32_t offset = packedBits > unpackBits ? packedBits - unpackBits : 0;
                srcp = ନୂତନ AstShiftL{srcp->fileline(), srcp,
                                     ନୂତନ AstConst{srcp->fileline(), offset}, packedBits};
            }
            ଯଦି (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sWidth = srcp->width();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା dWidth = nodep->lhsp()->width();
                ଯଦି (sWidth < dWidth) {
                    AstExtend* ଅଚଳ extendp = ନୂତନ AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = ନୂତନ AstShiftL{
                        srcp->fileline(), extendp,
                        ନୂତନ AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ଠେଲ_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* streamp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp = nodep->rhsp()->unlinkFrBack();
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            ଯଦି ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା blockSize = 1;
                ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    ଯଦି (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcElementBits = 0;
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstElementBits = 0;
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(ନୂତନ AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, ସତ୍ୟ,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ଠେଲ_Deletep(streamp), streamp);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଯଦି (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sWidth = srcp->width();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            ଯଦି (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } ଅନ୍ୟଥା {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            ଯଦି (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = ନୂତନ AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } ଅନ୍ୟଥା {
                ଯଦି (dWidth == 0) {
                    streamp = ନୂତନ AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } ଅନ୍ୟଥା ଯଦି (sWidth >= dWidth) {
                    streamp = ନୂତନ AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ streamp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            ଯଦି (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* origSrcp = srcp;
                ଯଦି (AstCvtArrayToPacked* ଅଚଳ cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ origSrcDTypep = origSrcp->dtypep()->skipRefp();
                ଯଦି (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcElementBits = 0;
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstElementBits = 0;
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    ଯଦି (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(ଠେଲ_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    ତାର୍କିକ reverse = ମିଥ୍ୟା;
                    ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(ନୂତନ AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ଠେଲ_Deletep(streamp), streamp);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sWidth = srcp->width();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା dWidth = dstp->width();
            ଯଦି (VN_IS(dstDTypep, UnpackArrayDType)) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                ଯଦି (sWidth > dstBitWidth) {
                    srcp
                        = ନୂତନ AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = ନୂତନ AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } ଅନ୍ୟଥା {
                ଯଦି (dWidth == 0) {
                    srcp = ନୂତନ AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } ଅନ୍ୟଥା ଯଦି (sWidth >= dWidth) {
                    srcp = ନୂତନ AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } ଅନ୍ୟଥା {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    ଯଦି (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* ଅଚଳ extendp = ନୂତନ AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = ନୂତନ AstShiftL{
                            srcp->fileline(), extendp,
                            ନୂତନ AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } ଅନ୍ୟଥା {  // if it is a queue we dont need do do a shiftL
                        srcp = ନୂତନ AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ଠେଲ_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp = streamp->lhsp();
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            ଯଦି (lowerAsFixedAggregate(srcDTypep)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } ଅନ୍ୟଥା ଯଦି ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                ଯଦି (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା blockSize = 1;
                    ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        ଯଦି (VL_UNLIKELY(blockSize <= 0)) {
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
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcElementBits = 0;
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstElementBits = 0;
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = ନୂତନ AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     ସତ୍ୟ,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(streamp), streamp);
                } ଅନ୍ୟଥା {
                    streamp->lhsp(ନୂତନ AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } ଅନ୍ୟଥା ଯଦି (m_doV && replaceAssignMultiSel(nodep)) {
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (replaceAssignSFormatF(nodep)) {
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Boolean replacements
    ତାର୍କିକ operandBoolShift(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        ଯଦି (!VN_IS(nodep, And)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstShiftR* ଅଚଳ shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        ଯଦି (!VN_IS(shiftp->rhsp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଶୂନ୍ୟ replaceBoolShift(Ast_ଗଣ୍ଠି* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* ଅଚଳ andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* ଅଚଳ shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3_ସଂଖ୍ୟା val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->ଅଙ୍କ(), shiftConstp->ଅଙ୍କ());
        AstAnd* ଅଚଳ newp
            = ନୂତନ AstAnd{nodep->fileline(), ନୂତନ AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    ଶୂନ୍ୟ replaceWithSimulation(Ast_ଗଣ୍ଠି* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        ଯଦି (!simvis.optimizable()) {
            ଅଚଳ Ast_ଗଣ୍ଠି* errorp = simvis.whyNotNodep();
            ଯଦି (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } ଅନ୍ୟଥା {
            // Fetch the result
            Ast_ଗଣ୍ଠି* ଅଚଳ valuep = simvis.fetch_ମୂଲ୍ୟ_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = valuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = ମିଥ୍ୟା;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ swapSides(Ast_ଗଣ୍ଠି_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    ତାର୍କିକ containsMemberAccessRecurse(Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ size_t id = m_ids(nodep);
        ଅଚଳ auto it = m_containsMemberAccess.find(id);
        ଯଦି (it != m_containsMemberAccess.end()) ଫେରନ୍ତୁ it->second;
        ++m_statMemberAccessVisits;
        ତାର୍କିକ result = ମିଥ୍ୟା;
        ଯଦି (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ funcRefp = VN_CAST(nodep, ଗଣ୍ଠି_FTask_Ref)) {
            ଯଦି (containsMemberAccessRecurse(funcRefp->taskp())) result = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_CCall* ଅଚଳ funcRefp = VN_CAST(nodep, ଗଣ୍ଠି_CCall)) {
            ଯଦି (containsMemberAccessRecurse(funcRefp->funcp())) result = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ funcp = VN_CAST(nodep, ଗଣ୍ଠି_FTask)) {
            // Assume that it has a member access
            ଯଦି (funcp->recursive()) result = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCFunc* ଅଚଳ funcp = VN_CAST(nodep, CFunc)) {
            ଯଦି (funcp->recursive()) result = ସତ୍ୟ;
        }
        ଯଦି (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        ଯଦି (!result && !VN_IS(nodep, ଗଣ୍ଠି_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = ସତ୍ୟ;
        }
        m_containsMemberAccess.emplace(id, result);
        ଫେରନ୍ତୁ result;
    }

    ତାର୍କିକ matchBiopToBitwise(Ast_ଗଣ୍ଠି_Biop* ଅଚଳ nodep) {
        ଯଦି (!m_convertLogicToBit) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!nodep->lhsp()->width1()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!nodep->rhsp()->width1()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!nodep->isPure()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (containsMemberAccessRecurse(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* ଅଚଳ aRandp = VN_CAST(nodep->lhsp(), Rand);
        ଅଚଳ AstRand* ଅଚଳ bRandp = VN_CAST(nodep->rhsp(), Rand);
        ଯଦି (!aRandp || !bRandp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!aRandp->combinable(bRandp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* ଅଚଳ aRandp = VN_CAST(nodep->fromp(), Rand);
        ଯଦି (!aRandp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (aRandp->seedp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        ଯଦି (ଅଚଳ AstConsArray* ଅଚଳ consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            ଯଦି (!(m_doExpensive || m_params)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (!consp->is_Constant_ବୃକ୍ଷ()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        } ଅନ୍ୟଥା ଯଦି (!VN_IS(nodep->lhsp(), Const)) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        replaceWithSimulation(nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା operandConcatMove(ଅଚଳ AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        ଅଚଳ AstConcat* ଅଚଳ abConcp = VN_CAST(nodep->lhsp(), Concat);
        ଅଚଳ AstConcat* ଅଚଳ bcConcp = VN_CAST(nodep->rhsp(), Concat);
        ଯଦି (!abConcp && !bcConcp) ଫେରନ୍ତୁ 0;
        ଯଦି (bcConcp) {
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ap = nodep->lhsp();
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            ଯଦି (VL_BITBIT_I(ap->width() + bp->width()) == 0) ଫେରନ୍ତୁ 2;  // Transform 2: to abConc
        } ଅନ୍ୟଥା {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            ଯଦି (VL_BITBIT_I(nodep->lhsp()->width()) != 0) ଫେରନ୍ତୁ 1;  // Transform 1: to bcConc
        }
        ଫେରନ୍ତୁ 0;  // ok
    }
    ଶୂନ୍ୟ moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        ଯଦି (operandConcatMove(nodep) > 1) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* ଅଚଳ bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bp = bcConcp->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* ଅଚଳ abConcp = ନୂତନ AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            ଯଦି (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ଠେଲ_Deletep(bcConcp), bcConcp);
        } ଅନ୍ୟଥା {
            AstConcat* ଅଚଳ abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ap = abConcp->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bp = abConcp->rhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* ଅଚଳ bcConcp = ନୂତନ AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            ଯଦି (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ଠେଲ_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    ଶୂନ୍ୟ visit(AstConst*) override {}  // Already constant

    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        ଯଦି (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    ଶୂନ୍ୟ visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_ଅଙ୍କ(nodep, AstMatchMasked::fold(constp->ଅଙ୍କ(), nodep->matchp()->varp()));
        }
    }

    ଶୂନ୍ୟ visit(AstPin* nodep) override { iterateChildren(nodep); }

    ଶୂନ୍ୟ replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* ଅଚଳ newp = ନୂତନ AstLogAnd{
            nodep->fileline(),
            ନୂତନ AstLogOr{nodep->fileline(), ନୂତନ AstLogNot{nodep->fileline(), lhsp}, rhsp},
            ନୂତନ AstLogOr{nodep->fileline(),
                         ନୂତନ AstLogNot{nodep->fileline(), rhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)},
                         lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* ଅଚଳ belowp = VN_AS(nodep->fromp(), Sel);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = belowp->fromp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsb1p = nodep->lsbp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newlsbp;
        ଯଦି (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = ନୂତନ AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(ଠେଲ_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ଠେଲ_Deletep(lsb2p), lsb2p);
        } ଅନ୍ୟଥା {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = widep->width() > lsb2p->width()
                                          ? ନୂତନ AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = widep->width() > lsb1p->width()
                                          ? ନୂତନ AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = ନୂତନ AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* ଅଚଳ newp = ନୂତନ AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* ଅଚଳ conp = VN_AS(nodep->fromp(), Concat);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ conLhsp = conp->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ conRhsp = conp->rhsp();
        ଯଦି (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* ଅଚଳ newp
                = ନୂତନ AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } ଅନ୍ୟଥା ଯଦି (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* ଅଚଳ newp
                = ନୂତନ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } ଅନ୍ୟଥା {
            // Yuk, split between the two
            conRhsp->unlinkFrBack();
            conLhsp->unlinkFrBack();
            AstConcat* ଅଚଳ newp
                = ନୂତନ AstConcat{nodep->fileline(),
                                ନୂତନ AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->width() + 1},
                                ନୂତନ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->width() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ତାର୍କିକ operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ଅଚଳ repp = VN_AS(nodep->fromp(), Replicate);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = repp->srcp();
        ଅଚଳ AstConst* ଅଚଳ lsbp = VN_CAST(nodep->lsbp(), Const);
        ଯଦି (!lsbp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        ଯଦି ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        //
        fromp->unlinkFrBack();
        AstSel* ଅଚଳ newp = ନୂତନ AstSel{
            nodep->fileline(), fromp,
            ନୂତନ AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* ଅଚଳ rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ from2p = rep2p->srcp();
        ଅଚଳ AstConst* ଅଚଳ cnt1p = VN_CAST(nodep->countp(), Const);
        ଯଦି (!cnt1p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstConst* ଅଚଳ cnt2p = VN_CAST(rep2p->countp(), Const);
        ଯଦି (!cnt2p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        //
        from2p->unlinkFrBack();
        AstReplicate* ଅଚଳ newp
            = ନୂତନ AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        ଯଦି (VN_IS(from1p, Replicate)) {
            ଅଚଳ AstConst* ଅଚଳ cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            ଯଦି (!cnt1p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        ଯଦି (VN_IS(from2p, Replicate)) {
            ଅଚଳ AstConst* ଅଚଳ cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            ଯଦି (!cnt2p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        ଯଦି (!operandsSame(from1p, from2p)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        //
        from1p->unlinkFrBack();
        AstReplicate* ଅଚଳ newp = ନୂତନ AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଶୂନ୍ୟ replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ଗଣ୍ଠି_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bilhsp = fromp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            ନୂତନ AstSel{nodep->fileline(), bilhsp, lsbp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ), nodep->widthConst()});
        fromp->rhsp(ନୂତନ AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_ଗଣ୍ଠି_Uniop* ଅଚଳ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ଗଣ୍ଠି_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(ନୂତନ AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        ଯଦି (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        ଯଦି (VN_IS(nodep->fromp(), Const)) {  // It did.
            ଯଦି (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } ଅନ୍ୟଥା {
                Ast_ଗଣ୍ଠି* ଅଚଳ fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            ଅଚଳ AstConsArray* ଅଚଳ consp = VN_AS(nodep->fromp(), ConsArray);
            ଅଚଳ uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ itemp = consp->valuep(bit);
            ଯଦି (VN_IS(itemp, Const)) {
                ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = VN_AS(itemp, Const)->ଅଙ୍କ();
                VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ଅଙ୍କ), nodep);
            } ଅନ୍ୟଥା ଯଦି (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        }
        m_selp = nullptr;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    ଶୂନ୍ୟ visit(AstSliceSel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        ଯଦି (!m_required) ଫେରନ୍ତୁ;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        ଯଦି (ଅଚଳ AstConsStruct* ଅଚଳ consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = consp->memberValuep(nodep->name());
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, constp->ଅଙ୍କ()), nodep);
            } ଅନ୍ୟଥା {
                nodep->replaceWith(valuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        }
    }

    ଶୂନ୍ୟ visit(AstCAwait* nodep) override {
        m_hasJumpDelay = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        ତାର୍କିକ did = ମିଥ୍ୟା;
        ଯଦି (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_ଗଣ୍ଠି* ଅଚଳ valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            ଅଚଳ AstConsArray* ଅଚଳ consp = VN_CAST(valuep, ConsArray);
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            ଯଦି (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                ଯଦି (operandConst(valuep)) {
                    ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = VN_AS(valuep, Const)->ଅଙ୍କ();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    ଅଚଳ string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ଅଙ୍କ, origParamName), nodep);
                    did = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstConst* ଅଚଳ itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, itemp->ଅଙ୍କ()), nodep);
                    did = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp = valuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    did = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp = valuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    did = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp = valuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    did = ସତ୍ୟ;
                }
            }
        }
        ଯଦି (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!Ast_ଗଣ୍ଠି::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    ଶୂନ୍ୟ visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ତାର୍କିକ did = ମିଥ୍ୟା;
        ଯଦି (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            ଅଚଳ AstEnumItem* ଅଚଳ itemp = nodep->itemp();
            ଯଦି (m_recursing_Enum_ବସ୍ତୁଗୁଡ଼ିକ.ଗଣନା_2(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } ଅନ୍ୟଥା {
                m_recursing_Enum_ବସ୍ତୁଗୁଡ଼ିକ.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursing_Enum_ବସ୍ତୁଗୁଡ଼ିକ.erase(itemp);
            }
            ଯଦି (AstConst* ଅଚଳ valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = valuep->ଅଙ୍କ();
                VL_DO_DANGLING(replace_ଅଙ୍କ(nodep, ଅଙ୍କ), nodep);
                did = ସତ୍ୟ;
            }
        }
        ଯଦି (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    ତାର୍କିକ only_Sen_Item_In_Sen_ବୃକ୍ଷ(ଅଚଳ AstSenItem* nodep) {
        // Only one if it's not in a list
        ଫେରନ୍ତୁ (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            ଯଦି (nodep->isClocked()) {  // A constant can never get a pos/negedge
                ଯଦି (only_Sen_Item_In_Sen_ବୃକ୍ଷ(nodep)) {
                    ଯଦି (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            ନୂତନ AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } ଅନ୍ୟଥା {
                        nodep->replaceWith(ନୂତନ AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                } ଅନ୍ୟଥା {
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } ଅନ୍ୟଥା {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(ନୂତନ AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        } ଅନ୍ୟଥା ଯଦି (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_ଗଣ୍ଠି* ଅଚଳ sensp = nodep->sensp();
            Ast_ଗଣ୍ଠି* lastSensp = sensp;
            ତାର୍କିକ invert = ମିଥ୍ୟା;
            ଯେତେବେଳେ (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            ଯଦି (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(sensp), sensp);
        }
    }

    ଶ୍ରେଣୀ SenItemCmp final {
        ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା cmp(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ap, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bp) {
            ଅଚଳ VNType aType = ap->type();
            ଅଚଳ VNType bType = bp->type();
            ଯଦି (aType != bType) ଫେରନ୍ତୁ static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(bType) - static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(aType);

            ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ aRefp = VN_CAST(ap, VarRef)) {
                ଅଚଳ AstVarRef* ଅଚଳ bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                ଯଦି (aRefp->name() < bRefp->name()) ଫେରନ୍ତୁ -1;
                ଯଦି (aRefp->name() > bRefp->name()) ଫେରନ୍ତୁ 1;
                // But might be same name with different scopes
                ଯଦି (aRefp->varScopep() < bRefp->varScopep()) ଫେରନ୍ତୁ -1;
                ଯଦି (aRefp->varScopep() > bRefp->varScopep()) ଫେରନ୍ତୁ 1;
                // Or rarely, different data types
                ଯଦି (aRefp->dtypep() < bRefp->dtypep()) ଫେରନ୍ତୁ -1;
                ଯଦି (aRefp->dtypep() > bRefp->dtypep()) ଫେରନ୍ତୁ 1;
                ଫେରନ୍ତୁ 0;
            }

            ଯଦି (ଅଚଳ AstConst* ଅଚଳ aConstp = VN_CAST(ap, Const)) {
                ଅଚଳ AstConst* ଅଚଳ bConstp = VN_AS(bp, Const);
                ଯଦି (aConstp->ଅଙ୍କ().isLtXZ(bConstp->ଅଙ୍କ())) ଫେରନ୍ତୁ -1;
                ଯଦି (bConstp->ଅଙ୍କ().isLtXZ(aConstp->ଅଙ୍କ())) ଫେରନ୍ତୁ 1;
                ଫେରନ୍ତୁ 0;
            }

            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ aBiOpp = VN_CAST(ap, ଗଣ୍ଠି_Biop)) {
                ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ bBiOpp = VN_AS(bp, ଗଣ୍ଠି_Biop);
                // Compare RHSs first, as LHS might be const
                ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) ଫେରନ୍ତୁ c;
                ଫେରନ୍ତୁ cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            ଯଦି (ଅଚଳ AstCMethodHard* ଅଚଳ aCallp = VN_CAST(ap, CMethodHard)) {
                ଅଚଳ AstCMethodHard* ଅଚଳ bCallp = VN_AS(bp, CMethodHard);
                ଯଦି (aCallp->method() < bCallp->method()) ଫେରନ୍ତୁ -1;
                ଯଦି (aCallp->method() > bCallp->method()) ଫେରନ୍ତୁ 1;
                ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା c = cmp(aCallp->fromp(), bCallp->fromp())) ଫେରନ୍ତୁ c;
                ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* aPinsp = aCallp->pinsp();
                ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bPinsp = bCallp->pinsp();
                ଯେତେବେଳେ (aPinsp && bPinsp) {
                    ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା c = cmp(aPinsp, bPinsp)) ଫେରନ୍ତୁ c;
                    aPinsp = VN_AS(aPinsp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                    bPinsp = VN_AS(bPinsp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                }
                ଫେରନ୍ତୁ aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            ଫେରନ୍ତୁ 0;
        }

    ସର୍ବସାଧାରଣ:
        ତାର୍କିକ operator()(ଅଚଳ AstSenItem* lhsp, ଅଚଳ AstSenItem* rhsp) ଅଚଳ {
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lSensp = lhsp->sensp();
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rSensp = rhsp->sensp();
            ଯଦି (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା c = cmp(lSensp, rSensp)) ଫେରନ୍ତୁ c < 0;
            } ଅନ୍ୟଥା ଯଦି (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                ଫେରନ୍ତୁ rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            ଫେରନ୍ତୁ lhsp->edgeType() < rhsp->edgeType();
        }
    };

    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_doExpensive) {
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
            ପାଇଁ (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                ଅଚଳ SenItemCmp cmp;
                ଯଦି (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    ପାଇଁ (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.ଠେଲ_back(senp);
                    }
                    stable_sort(vec.begin(), vec.end(), SenItemCmp());
                    ପାଇଁ (ଅଚଳ auto& ip : vec) ip->unlinkFrBack();
                    ପାଇଁ (ଅଚଳ auto& ip : vec) nodep->addSensesp(ip);
                    ଭାଙ୍ଗ;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            ପାଇଁ (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                ଯଦି (!nextp) ଭାଙ୍ଗ;
                AstSenItem* ଅଚଳ lItemp = senp;
                AstSenItem* ଅଚଳ rItemp = nextp;
                ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lSenp = lItemp->sensp();
                ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rSenp = rItemp->sensp();
                ଯଦି (!lSenp || !rSenp) ଜାରି;

                ଯଦି (lSenp->same_Gate_ବୃକ୍ଷ(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    ଯଦି (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    ଯଦି (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(ଠେଲ_Deletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->timingControlp()) m_hasJumpDelay = ସତ୍ୟ;
        ଯଦି (m_doNConst && replace_ଗଣ୍ଠି_Assign(nodep)) ଫେରନ୍ତୁ;
    }
    ଶୂନ୍ୟ visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ଶୂନ୍ୟ visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_doNConst && replace_ଗଣ୍ଠି_Assign(nodep)) ଫେରନ୍ତୁ;
        // Process containing this AssignW as single body statement
        AstAlways* ଅଚଳ procp = VN_CAST(nodep->backp(), Always);
        ଯଦି (!procp || procp->stmtsp() != nodep || nodep->nextp()) ଫେରନ୍ତୁ;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(nodep->lhsp(), VarRef);
        ଯଦି (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            AstInitial* ଅଚଳ newinitp = ନୂତନ AstInitial{flp, ନୂତନ AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ଠେଲ_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_ଗଣ୍ଠି* ଅଚଳ initvaluep = exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            varrefp->varp()->isConst(ସତ୍ୟ);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    ଶୂନ୍ୟ visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!m_doNConst) ଫେରନ୍ତୁ;
        ଅଚଳ AstBasicDType* ଅଚଳ bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        ଯଦି (!bdtypep) ଫେରନ୍ତୁ;
        ଯଦି (!bdtypep->isZeroInit()) ଫେରନ୍ତୁ;
        AstConst* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), V3_ସଂଖ୍ୟା{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* ଅଚଳ fromp = VN_CAST(nodep->fromp(), ConsArray);
        ଯଦି (!fromp) ଫେରନ୍ତୁ;
        // Fold the outermost dimension, converting the elements for any dimension under it
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        ଅଚଳ ତାର୍କିକ reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        ଅଚଳ uint64_t lastIndex = toArrayp->elementsConst() - 1;
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = toArrayp->subDTypep();
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        ଅଚଳ auto convert = [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            valuep->unlinkFrBack();
            ଯଦି (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) ଫେରନ୍ତୁ valuep;
            ଫେରନ୍ତୁ ନୂତନ AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* ଅଚଳ newp
            = ନୂତନ AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : nullptr};
        ପାଇଁ (ଅଚଳ AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? lastIndex - itemp->index() : itemp->index(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    ଶୂନ୍ୟ visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_ଗଣ୍ଠି_Stream* streamp = nullptr;
        ତାର୍କିକ isReverse = ମିଥ୍ୟା;
        ଯଦି (AstStreamL* ଅଚଳ streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = ସତ୍ୟ;  // StreamL reverses the operation
        } ଅନ୍ୟଥା ଯଦି (AstStreamR* ଅଚଳ streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = ମିଥ୍ୟା;  // StreamR doesn't reverse the operation
        }
        ଯଦି (streamp) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcp = streamp->lhsp();
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dstDTypep = nodep->dtypep()->skipRefp();
            ଯଦି (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା blockSize = 1;
                ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    ଯଦି (VL_UNLIKELY(blockSize <= 0)) {
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
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcElementBits = 0;
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstElementBits = 0;
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = ନୂତନ AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(streamp), streamp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
        }
    }
    ଶୂନ୍ୟ visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        ଯଦି (AstConcat* ଅଚଳ concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            AstRelease* ଅଚଳ newLp = ନୂତନ AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* ଅଚଳ newRp = ନୂତନ AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_doNConst) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->condp(), Const)) {
                Ast_ଗଣ୍ଠି* keepp = nullptr;
                Ast_ଗଣ୍ଠି* delp = nullptr;
                ଯଦି (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } ଅନ୍ୟଥା ଯଦି (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } ଅନ୍ୟଥା {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    ଫେରନ୍ତୁ;
                }
                ଯଦି (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    ଫେରନ୍ତୁ;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                ଯଦି (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } ଅନ୍ୟଥା {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_ବୃକ୍ଷ();
            } ଅନ୍ୟଥା ଯଦି (!Ast_ଗଣ୍ଠି::afterCommentp(nodep->thensp())
                       && !Ast_ଗଣ୍ଠି::afterCommentp(nodep->elsesp())) {
                ଯଦି (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, ସତ୍ୟ);
                    Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ newp
                        = ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                } ଅନ୍ୟଥା {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } ଅନ୍ୟଥା ଯଦି (!Ast_ଗଣ୍ଠି::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = nodep->condp();
                Ast_ଗଣ୍ଠି* ଅଚଳ elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                ଯଦି (nodep->thensp()) {  // Must have been comment
                    ଠେଲ_Deletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(ନୂତନ AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } ଅନ୍ୟଥା ଯଦି (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp
                    = VN_AS(nodep->condp(), ଗଣ୍ଠି_Uniop)->lhsp()->unlinkFrBackWithNext();
                Ast_ଗଣ୍ଠି* ଅଚଳ thensp = nodep->thensp()->unlinkFrBackWithNext();
                Ast_ଗଣ୍ଠି* ଅଚଳ elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* ଅଚଳ ifp = ନୂତନ AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା ଯଦି (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ thensp = VN_AS(nodep->thensp(), ଗଣ୍ଠି_Assign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ elsesp = VN_AS(nodep->elsesp(), ଗଣ୍ଠି_Assign);
                thensp->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = nodep->condp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ truep = thensp->rhsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(ନୂତନ AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା ଯଦି (ମିଥ୍ୟା  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_ଗଣ୍ଠି_If* ଅଚଳ lowerIfp = VN_AS(nodep->thensp(), ଗଣ୍ଠି_If);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = nodep->condp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି* ଅଚଳ lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(ନୂତନ AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(ଠେଲ_Deletep(lowerIfp), lowerIfp);
            } ଅନ୍ୟଥା {
                // Optimizations that don't reform the IF itself
                ଯଦି (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    ଶୂନ୍ୟ visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        ଯଦି (stmtDisplayDisplay(nodep)) ଫେରନ୍ତୁ;
    }
    ତାର୍କିକ stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        ଯଦି (!m_modp) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Don't optimize under single statement
        AstDisplay* ଅଚଳ prevp = VN_CAST(nodep->backp(), Display);
        ଯଦି (!prevp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_ବୃକ୍ଷ(nodep->filep())))
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstSFormatF* ଅଚଳ pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* ଅଚଳ nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        ଯଦି (nformatp->scopeNamep() && pformatp->scopeNamep()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        ଯଦି (!m_doCpp && pformatp->exprsp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!m_doCpp && nformatp->exprsp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Avoid huge merges
        ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା DISPLAY_MAX_MERGE_LENGTH = 500;
        ଯଦି (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        ଯଦି (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        ଯଦି (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        ଯଦି (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        ଯଦି (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        ଯଦି (AstScopeName* ଅଚଳ scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଶୂନ୍ୟ visit(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterateAndNextNull(nodep->exprp());
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        ଯଦି (nodep->ପଦସମଷ୍ଟି_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            ଯଦି (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->ଅଙ୍କ().isFromString()) {
                AstConst* ଅଚଳ fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->ଅଙ୍କ().toString());
                nodep->ପଦସମଷ୍ଟି_Format_2(ମିଥ୍ୟା);
                VL_DO_DANGLING(ଠେଲ_Deletep(fmtp), fmtp);
            }
        }
        ଯଦି (nodep->ପଦସମଷ୍ଟି_Format_2()) ଫେରନ୍ତୁ;  // Runtime, unfortunately  (unless constify format later)
        ତାର୍କିକ anyconst = ମିଥ୍ୟା;
        ପାଇଁ (Ast_ଗଣ୍ଠି* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* ଅଚଳ fargp = VN_CAST(argp, SFormatArg);
            Ast_ଗଣ୍ଠି* ଅଚଳ subargp = fargp ? fargp->exprp() : argp;
            ଯଦି (VN_IS(subargp, Const)) {
                anyconst = ସତ୍ୟ;
                ଭାଙ୍ଗ;
            }
        }
        ଯଦି (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            ତାର୍କିକ inPct = ମିଥ୍ୟା;
            Ast_ଗଣ୍ଠି* argp = nodep->exprsp();
            ଅଚଳ string text = nodep->text();
            ପାଇଁ (ଅକ୍ଷର ch : text) {
                ଯଦି (!inPct && ch == '%') {
                    inPct = ସତ୍ୟ;
                    fmt = ch;
                } ଅନ୍ୟଥା ଯଦି (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ଅନ୍ୟଥା ଯଦି (inPct) {
                    inPct = ମିଥ୍ୟା;
                    fmt += ch;
                    ch = std::tolower(ch);
                    ବାଛ (ch) {
                    ସ୍ଥିତି '%': ଭାଙ୍ଗ;  // %% - still %%
                    ସ୍ଥିତି 'm': ଭାଙ୍ଗ;  // %m - still %m - auto insert "name"
                    ସ୍ଥିତି 'l': ଭାଙ୍ଗ;  // %l - still %l - auto insert "library"
                    ସ୍ଥିତି 't':  // %t - don't know $timeformat so can't constify
                        ଯଦି (argp) argp = argp->nextp();
                        ଭାଙ୍ଗ;
                    ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // Most operators, just move to next argument
                        ଯଦି (argp) {
                            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = argp->nextp();
                            AstSFormatArg* ଅଚଳ fargp = VN_CAST(argp, SFormatArg);
                            Ast_ଗଣ୍ଠି* ଅଚଳ subargp = fargp ? fargp->exprp() : argp;
                            ଅଚଳ VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            ଯଦି (VN_IS(subargp, Const)) {  // Convert it
                                ଅଚଳ string out
                                    = formatAttr.isEnum()
                                          ? const_ଅଙ୍କ_V(subargp).displayedEnum(fargp, fmt)
                                          : const_ଅଙ୍କ_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(ଠେଲ_Deletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        ଭାଙ୍ଗ;
                    }  // switch
                    newFormat += fmt;
                } ଅନ୍ୟଥା {
                    newFormat += ch;
                }
            }
            ଯଦି (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        ଯଦି (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (VN_IS(nodep->backp(), ଗଣ୍ଠି_Assign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
    }
    ଶୂନ୍ୟ visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!m_doNConst) ଫେରନ୍ତୁ;
        ଯଦି (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* ଅଚଳ fmtp = nodep->fmtp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp();
            AstBasicDType* ଅଚଳ basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            ଯଦି (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* ଅଚଳ strp
                    = ନୂତନ AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(ନୂତନ AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        ଯଦି (nodep->recursive()) m_underRecFunc = ସତ୍ୟ;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = ସତ୍ୟ;  // As don't analyze inside tasks for timing controls
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = ସତ୍ୟ;  // As don't analyze inside tasks for timing controls
    }
    ଶୂନ୍ୟ visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_ଗଣ୍ଠି_FTask_Ref*>(nodep));
        ଯଦି (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        ଅଚଳ ତାର୍କିକ oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = ମିଥ୍ୟା;
        m_hasLoopTest = ମିଥ୍ୟା;
        iterateChildren(nodep);
        ଅଚଳ ତାର୍କିକ thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        ଯଦି (ଅଚଳ AstLoopTest* ଅଚଳ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            ଯଦି (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
        }
        // If last statement always breaks, repalce loop with body
        ଯଦି (Ast_ଗଣ୍ଠି* lastp = nodep->stmtsp()) {
            ଯେତେବେଳେ (Ast_ଗଣ୍ଠି* ଅଚଳ nextp = lastp->nextp()) lastp = nextp;
            ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(lastp, LoopTest)) {
                ଯଦି (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ଠେଲ_Deletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
        }
        // Warn on infinite loop
        ଯଦି (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, ସତ୍ୟ);  // Complain once
        }
    }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        ଯଦି (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }

        m_hasLoopTest = ସତ୍ୟ;

        // If always breaks, subsequent statements are dead code, delete them
        ଯଦି (nodep->condp()->isZero()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = nodep->nextp();
            ଯଦି (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ଠେଲ_Deletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            ଫେରନ୍ତୁ;
        }

        ଯଦି (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    ଶୂନ୍ୟ visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Pre_Sel*) override {}

    // Ignored, can eliminate early
    ଶୂନ୍ୟ visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_doNConst) VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        ଯଦି (!nodep->exprp()) {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        // Remove if expression is trivially pure
        ଯଦି (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        // Remove calls to empty functions
        ଯଦି (ଅଚଳ AstCCall* ଅଚଳ callp = VN_CAST(nodep->exprp(), CCall)) {
            ଅଚଳ AstCFunc* ଅଚଳ funcp = callp->funcp();
            ଯଦି (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
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
    ଶୂନ୍ୟ visit(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    ଶୂନ୍ୟ visit(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        ଯଦି (nodep->nextp() && canDeleteStatements()) {
            ଠେଲ_Deletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        ଯଦି (!nodep->nextp()) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                ଯଦି (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                    ଫେରନ୍ତୁ;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                ଯଦି (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) ଭାଙ୍ଗ;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        ଯଦି (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }

        // If no JumpGo points to this node, replace it with its body
        ଯଦି (!m_usedJumpBlocks.ଗଣନା_2(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    ବୃକ୍ଷ_SKIP_VISIT("ArraySel");
    ବୃକ୍ଷ_SKIP_VISIT("CAwait");

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
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        // Default: Just iterate
        ଯଦି (m_required) {
            ଯଦି (VN_IS(nodep, ଗଣ୍ଠି_DType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } ଅନ୍ୟଥା ଯଦି (AstCellRef* ଅଚଳ crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(ଠେଲ_Deletep(crp), crp);
                }
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->isTimingControl()) m_hasJumpDelay = ସତ୍ୟ;
            // Calculate the width of this operation
            ଯଦି (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);
        }
    }

ସର୍ବସାଧାରଣ:
    // Processing Mode Enum
    ଗଣନା ProcMode : uint8_t {
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
    ConstVisitor(ProcMode pmode, ତାର୍କିକ globalPass)
        : m_globalPass{globalPass}
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_global_Pass_ଅଙ୍କ++)) : ""} {
        // clang-format off
        ବାଛ (pmode) {
        ସ୍ଥିତି PROC_PARAMS_NOWARN:  m_doV = ସତ୍ୟ;  m_doNConst = ସତ୍ୟ; m_params = ସତ୍ୟ;
                                  m_required = ମିଥ୍ୟା; ଭାଙ୍ଗ;
        ସ୍ଥିତି PROC_PARAMS:         m_doV = ସତ୍ୟ;  m_doNConst = ସତ୍ୟ; m_params = ସତ୍ୟ;
                                  m_required = ସତ୍ୟ; ଭାଙ୍ଗ;
        ସ୍ଥିତି PROC_GENERATE:       m_doV = ସତ୍ୟ;  m_doNConst = ସତ୍ୟ; m_params = ସତ୍ୟ;
                                  m_required = ସତ୍ୟ; m_doGenerate = ସତ୍ୟ; ଭାଙ୍ଗ;
        ସ୍ଥିତି PROC_LIVE:           ଭାଙ୍ଗ;
        ସ୍ଥିତି PROC_V_WARN:         m_doV = ସତ୍ୟ;  m_doNConst = ସତ୍ୟ; m_warn = ସତ୍ୟ; m_convertLogicToBit = ସତ୍ୟ; ଭାଙ୍ଗ;
        ସ୍ଥିତି PROC_V_NOWARN:       m_doV = ସତ୍ୟ;  m_doNConst = ସତ୍ୟ; ଭାଙ୍ଗ;
        ସ୍ଥିତି PROC_V_EXPENSIVE:    m_doV = ସତ୍ୟ;  m_doNConst = ସତ୍ୟ; m_doExpensive = ସତ୍ୟ; ଭାଙ୍ଗ;
        ସ୍ଥିତି PROC_CPP:            m_doV = ମିଥ୍ୟା; m_doNConst = ସତ୍ୟ; m_doCpp = ସତ୍ୟ; ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:                  v3fatalSrc("Bad case"); ଭାଙ୍ଗ;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        ଯଦି (m_doCpp) {
            ଯଦି (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } ଅନ୍ୟଥା {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_stat_Cond_ପଦସମଷ୍ଟି_Redundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_ପଦସମଷ୍ଟି_Redundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_ଗଣ୍ଠି* mainAcceptEdit(Ast_ଗଣ୍ଠି* nodep) {
        VIsCached::clear_Cache_ବୃକ୍ଷ();  // Avoid using any stale isPure
        // Operate starting at a random place
        ଫେରନ୍ତୁ iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_ଅଙ୍କ = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
ଶୂନ୍ୟ V3Const::constifyParamsEdit(Ast_ଗଣ୍ଠି* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ ମିଥ୍ୟା};
    // cppcheck-suppress constVariablePointer // edited below
    ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ଯଦି (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ଅନ୍ୟଥା {
        (ଶୂନ୍ୟ)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
ଶୂନ୍ୟ V3Const::constifyParamsNoWarnEdit(Ast_ଗଣ୍ଠି* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ ମିଥ୍ୟା};
    // cppcheck-suppress constVariablePointer // edited below
    ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ଯଦି (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ଅନ୍ୟଥା {
        (ଶୂନ୍ୟ)visitor.mainAcceptEdit(nodep);
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
ଶୂନ୍ୟ V3Const::constifyGenerateParamsEdit(Ast_ଗଣ୍ଠି* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ ମିଥ୍ୟା};
    // cppcheck-suppress constVariablePointer // edited below
    ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        ଯଦି (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } ଅନ୍ୟଥା {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

ଶୂନ୍ୟ V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ ସତ୍ୟ};
        (ଶୂନ୍ୟ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("const", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ସତ୍ୟ};
        (ଶୂନ୍ୟ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("const_cpp", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

Ast_ଗଣ୍ଠି* V3Const::constifyEdit(Ast_ଗଣ୍ଠି* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ ମିଥ୍ୟା};
    nodep = visitor.mainAcceptEdit(nodep);
    ଫେରନ୍ତୁ nodep;
}

Ast_ଗଣ୍ଠି* V3Const::constifyEditCpp(Ast_ଗଣ୍ଠି* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ମିଥ୍ୟା};
    nodep = visitor.mainAcceptEdit(nodep);
    ଫେରନ୍ତୁ nodep;
}

ଶୂନ୍ୟ V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ ସତ୍ୟ};
        (ଶୂନ୍ୟ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("const", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ସତ୍ୟ};
        (ଶୂନ୍ୟ)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("const", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

Ast_ଗଣ୍ଠି* V3Const::constifyExpensiveEdit(Ast_ଗଣ୍ଠି* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ମିଥ୍ୟା};
    nodep = visitor.mainAcceptEdit(nodep);
    ଫେରନ୍ତୁ nodep;
}
