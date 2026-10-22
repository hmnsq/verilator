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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Const.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Simulate.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3String.h"
#அந்தர்பாவ "V3UniqueNames.h"
#அந்தர்பாவ "V3Width.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <type_traits>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#define மரம்_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

நிலையான தருக்க isConst(மாறிலி Ast_முனை* nodep, uint64_t v) {
    மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const);
    திரும்பு constp && constp->toUQuad() == v;
}

template <typename T>
நிலையான typename std::enable_if<std::is_integral<T>::மதிப்பு, தருக்க>::type isPow2(T val) {
    திரும்பு (val & (val - 1)) == 0;
}

நிலையான முழு மொத்தம்_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#எனில் defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    திரும்பு __builtin_ctzll(val);
#இல்லையெனில்
    முழு bit = 0;
    val = ~val;
    போது (val & 1) {
        ++bit;
        val >>= 1;
    }
    திரும்பு bit;
#endif
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
வகுப்பு VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4InUse> m_inuser4p;
    size_t m_nextId = 0;  // Id allocated most recently

பொது:
    // Return the unique id of the given node, allocating a new one if it has none yet
    size_t operator()(Ast_முனை* nodep) {
        எனில் (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        எனில் (!nodep->user4p()) nodep->user4p(reinterpret_cast<வெறுமை*>(++m_nextId));
        திரும்பு reinterpret_cast<size_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
வகுப்பு Const_Bit_Op_மரம்_Visitor final : பொது VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<Ast_முனை_கோவை*, unsigned, தருக்க>;

    வகுப்பு LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        தருக்க m_polarity = உண்மை;  // Invert result due to NOT
        முழு m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        முழு m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        முழு m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = nullptr;  // Leaf's variable reference
        மாறிலி AstConst* m_constp = nullptr;  // Leaf's constant

    பொது:
        // CONSTRUCTORS
        LeafInfo() = இயல்பு;
        LeafInfo(மாறிலி LeafInfo& other) = இயல்பு;
        LeafInfo& operator=(மாறிலி LeafInfo& other) = இயல்பு;
        LeafInfo(LeafInfo&& other) = இயல்பு;
        LeafInfo& operator=(LeafInfo&& other) = இயல்பு;
        explicit LeafInfo(முழு lsb)
            : m_lsb{lsb} {}

        // METHODS
        வெறுமை setLeaf(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->widthMin() - 1;
        }
        வெறுமை setLeaf(மாறிலி AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->widthMin() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        வெறுமை updateBitRange(முழு newLsb, முழு newMsb) {
            எனில் ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(உண்மை);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        வெறுமை updateBitRange(மாறிலி AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->width() - 1));
        }
        வெறுமை updateBitRange(மாறிலி AstShiftR* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->toUInt(), m_msb);
        }
        வெறுமை limitBitRangeToLsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        முழு wordIdx() மாறிலி { திரும்பு m_wordIdx; }
        வெறுமை wordIdx(முழு i) { m_wordIdx = i; }
        தருக்க polarity() மாறிலி { திரும்பு m_polarity; }
        வெறுமை polarity(தருக்க p) { m_polarity = p; }

        AstVarRef* refp() மாறிலி { திரும்பு m_refp; }
        மாறிலி AstConst* constp() மாறிலி { திரும்பு m_constp; }
        தருக்க missingWordSel() மாறிலி {
            // When V3Expand is skipped, WordSel is not inserted.
            திரும்பு m_refp->isWide() && m_wordIdx == -1;
        }
        முழு lsb() மாறிலி { திரும்பு m_lsb; }

        முழு msb() மாறிலி { திரும்பு std::min(m_msb, varWidth() - 1); }
        முழு varWidth() மாறிலி {
            UASSERT(m_refp, "m_refp should be set");
            மாறிலி முழு width = m_refp->varp()->widthMin();
            எனில் (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                திரும்பு width;
            } இல்லையெனில் {
                எனில் (missingWordSel()) திரும்பு width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                மாறிலி முழு bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                திரும்பு m_wordIdx == m_refp->width_சொற்கள்() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    கட்டமைப்பு BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        தருக்க m_polarity = பொய்;  // Polarity the bit must have to match
        முழு m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(மாறிலி LeafInfo& info, தருக்க pol, முழு bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = இயல்பு;
    };

    கட்டமைப்பு Frozen_முனை_Info final {  // Context when a frozen node is found
        தருக்க m_polarity;  // Polarity the frozen node must match
        முழு m_lsb;  // LSB position of the frozen node
        தருக்க operator<(மாறிலி Frozen_முனை_Info& other) மாறிலி {
            எனில் (m_lsb != other.m_lsb) திரும்பு m_lsb < other.m_lsb;
            திரும்பு m_polarity < other.m_polarity;
        }
    };

    வகுப்பு Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_மரம்_Visitor& m_visitor;  // Visitor whose state is saved and restored
        மாறிலி size_t m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        மாறிலி size_t m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        மாறிலி unsigned m_ops;  // Saved m_visitor.m_ops to restore
        மாறிலி தருக்க m_polarity;  // Saved m_visitor.m_polarity to restore
        தருக்க m_restore = உண்மை;  // Whether the destructor still needs to restore state

    பொது:
        explicit Restorer(Const_Bit_Op_மரம்_Visitor& visitor)
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
            எனில் (m_restore) restoreNow();
        }
        வெறுமை disableRestore() { m_restore = பொய்; }
        வெறுமை restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = பொய்;
        }
    };
    // Collect information for each Variable to transform as below
    வகுப்பு VarInfo final {
        // MEMBERS
        முழு m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        மாறிலி Const_Bit_Op_மரம்_Visitor* மாறிலி
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* மாறிலி m_refp;  // Points the variable that this VarInfo covers
        மாறிலி முழு m_width;  // Width of term this VarInfo refers to
        V3_எண் m_bitPolarity;  // Coefficient of each bit

    பொது:
        // METHODS
        தருக்க hasConstResult() மாறிலி { திரும்பு m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        தருக்க getConstResult() மாறிலி {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            திரும்பு m_knownResult == 1;
        }
        மாறிலி AstVarRef* refp() மாறிலி { திரும்பு m_refp; }
        தருக்க sameVarAs(மாறிலி Ast_முனை_Var_Ref* otherp) மாறிலி { திரும்பு m_refp->same_முனை(otherp); }
        வெறுமை setPolarity(தருக்க compBit, முழு bit) {
            // Ignore if already determined a known reduction
            எனில் (m_knownResult >= 0) திரும்பு;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            எனில் (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } இல்லையெனில் {  // The bit has already been marked with some polarity
                மாறிலி தருக்க sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                எனில் (m_parentp->is_Xor_மரம்()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } இல்லையெனில் {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    எனில் (sameFlag) திரும்பு;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->is_And_மரம்() ? 0 : 1;
                    m_bitPolarity.setAllBitsX();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ResultTerm getResultTerm() மாறிலி {
            UASSERT_OBJ(!hasConstResult(), m_refp, "getTerm on reduction that yields constant");
            File_வரி* மாறிலி fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_முனை_கோவை* srcp = VN_CAST(m_refp->backp(), WordSel);
            எனில் (!srcp) srcp = m_refp;
            srcp = srcp->clone_மரம்(பொய்);

            // Signed variables might have redundant sign bits that need masking.
            மாறிலி தருக்க hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3_எண் mask_இலக்கம்{srcp, m_width, 0};
            mask_இலக்கம்.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            மாறிலி uint64_t maskVal = mask_இலக்கம்.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_முனை_கோவை* resultp = srcp;  // The tree for this term
            unsigned ops = 0;  // Number of ops in this term
            தருக்க clean = பொய்;  // Whether the term is clean (has value 0 or 1)

            எனில் (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                மாறிலி முழு bit = மொத்தம்_Trailing_Zeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                எனில் (bit != 0) {
                    resultp = புதிய AstShiftR{fl, resultp,
                                            புதிய AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                மாறிலி தருக்க negate = m_bitPolarity.bitIs0(bit);
                எனில் (negate) {
                    resultp = புதிய AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } இல்லையெனில் {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                மாறிலி தருக்க needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                எனில் (needsMasking) {
                    resultp = புதிய AstAnd{fl, புதிய AstConst{fl, mask_இலக்கம்}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                எனில் (m_parentp->is_Xor_மரம்()) {
                    எனில் (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        மாறிலி முழு widthMin = mask_இலக்கம்.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = புதிய AstRedXor{fl, resultp};
                    ++ops;
                    clean = பொய்;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } இல்லையெனில் எனில் (m_parentp->is_And_மரம்()) {
                    V3_எண் comp_இலக்கம்{srcp, m_width, 0};
                    comp_இலக்கம்.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = புதிய AstEq{fl, புதிய AstConst{fl, comp_இலக்கம்}, resultp};
                    ++ops;
                    clean = உண்மை;
                } இல்லையெனில் {  // Or
                    V3_எண் comp_இலக்கம்{srcp, m_width, 0};
                    comp_இலக்கம்.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_இலக்கம்.opXor(V3_எண்{comp_இலக்கம்}, mask_இலக்கம்);
                    resultp = புதிய AstNeq{fl, புதிய AstConst{fl, comp_இலக்கம்}, resultp};
                    ++ops;
                    clean = உண்மை;
                }
            }

            திரும்பு ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_மரம்_Visitor* parent, AstVarRef* refp, முழு width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    தருக்க m_failed = பொய்;
    தருக்க m_polarity = உண்மை;  // Flip when AstNot comes
    unsigned m_ops;  // Number of operations such as And, Or, Xor, Sel...
    முழு m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    மாறிலி Ast_முனை_கோவை* மாறிலி m_rootp;  // Root of this AST subtree

    std::vector<std::pair<Ast_முனை_கோவை*, Frozen_முனை_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<size_t, முழு> m_baseIdxs;

    // METHODS

    தருக்க is_And_மரம்() மாறிலி { திரும்பு VN_IS(m_rootp, And); }
    தருக்க is_Or_மரம்() மாறிலி { திரும்பு VN_IS(m_rootp, Or); }
    தருக்க is_Xor_மரம்() மாறிலி { திரும்பு VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    எனில் (setFailed(cond, #cond, nodep, __LINE__)) திரும்பு

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(உண்மை, reason, nodep, __LINE__)

    தருக்க setFailed(தருக்க fail, மாறிலி எழுத்து* reason, மாறிலி Ast_முனை* nodep, முழு வரி) {
        எனில் (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << வரி << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = உண்மை;
        }
        திரும்பு m_failed;
    }
    வெறுமை incrOps(மாறிலி Ast_முனை* nodep, முழு வரி) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << வரி);
    }
    VarInfo& getVarInfo(மாறிலி LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_முனை* nodep = ref.refp()->varScopep();
        எனில் (!nodep) nodep = ref.refp()->varp();  // Not scoped
        முழு& baseIdxr = m_baseIdxs[m_ids(nodep)];
        எனில் (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            மாறிலி முழு இலக்கம்_சொற்கள்
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->width_சொற்கள்() : 1;
            m_varInfos.resize(m_varInfos.size() + இலக்கம்_சொற்கள்);
        }
        மாறிலி size_t idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        எனில் (!varInfop) {
            varInfop = புதிய VarInfo{இது, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            எனில் (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } இல்லையெனில் {
            எனில் (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        திரும்பு *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(Ast_முனை* nodep, தருக்க expectConst) {
        எனில் (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) திரும்பு LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        தருக்க ok = !m_failed;
        எனில் (expectConst) {
            ok &= !info.refp() && info.constp();
        } இல்லையெனில் {
            ok &= info.refp() && !info.constp();
        }
        திரும்பு ok ? info : LeafInfo{};
    }

    // VISITORS
    வெறுமை visit(Ast_முனை* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    வெறுமை visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        எனில் (m_leafp) m_leafp->updateBitRange(nodep);
    }
    வெறுமை visit(AstShiftR* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->toUInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->toUInt();
    }
    வெறுமை visit(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->widthMin() != 1, nodep);
        Ast_முனை* lhsp = nodep->lhsp();
        AstCCast* மாறிலி castp = VN_CAST(lhsp, CCast);
        எனில் (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_மரம்() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        எனில் (!is_Xor_மரம்()) m_polarity = !m_polarity;
        எனில் (m_leafp && castp) m_leafp->updateBitRange(castp);
        எனில் (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    வெறுமை visit(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->toSInt());
        iterateConst(nodep->fromp());
    }
    வெறுமை visit(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }
    வெறுமை visit(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }

    வெறுமை visit(AstRedXor* nodep) override {
        Restorer restorer{*இது};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_முனை* lhsp = nodep->lhsp();
        மாறிலி AstCCast* மாறிலி castp = VN_CAST(lhsp, CCast);
        எனில் (castp) lhsp = castp->lhsp();
        எனில் (மாறிலி AstAnd* மாறிலி andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            மாறிலி LeafInfo& mask = findLeaf(andp->lhsp(), உண்மை);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), பொய்);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            எனில் (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            மாறிலி V3_எண்& mask_இலக்கம் = mask.constp()->இலக்கம்();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            மாறிலி முழு maxBitIdx = std::min(ref.lsb() + mask_இலக்கம்.width(), ref.msb() + 1);
            க்கு (முழு bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                மாறிலி முழு maskIdx = bitIdx - ref.lsb();
                எனில் (mask_இலக்கம்.bitIs0(maskIdx)) தொடர்;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, உண்மை, bitIdx);
            }
        } இல்லையெனில் {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, பொய்);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            எனில் (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            க்கு (முழு bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, உண்மை, bitIdx);
            }
        }
    }

    வெறுமை visit(Ast_முனை_Biop* nodep) override {
        எனில் (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*இது};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            எனில் (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } இல்லையெனில் எனில் (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_மரம்(), nodep);
            incrOps(nodep, __LINE__);
            க்கு (மாறிலி தருக்க வலது : {பொய், உண்மை}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*இது};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                Ast_முனை_கோவை* opp = வலது ? nodep->rhsp() : nodep->lhsp();
                மாறிலி தருக்க origFailed = m_failed;
                iterateConst(opp);
                எனில் (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    எனில் (மாறிலி AstCCast* மாறிலி castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    மாறிலி தருக்க pol = is_Xor_மரம்() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, Frozen_முனை_Info{pol, m_lsb});
                    m_failed = origFailed;
                    தொடர்;
                }
                restorer.disableRestore();  // Now all checks passed
                எனில் (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    எனில் (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, is_Xor_மரம்() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } இல்லையெனில் எனில் ((is_And_மரம்() && leafInfo.polarity())
                               || (is_Or_மரம்() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, உண்மை, 0);
                        m_bitPolarities.emplace_back(leafInfo, பொய், 0);
                    }
                }
            }
        } இல்லையெனில் எனில் ((is_And_மரம்() && VN_IS(nodep, Eq)) || (is_Or_மரம்() && VN_IS(nodep, Neq))) {
            Restorer restorer{*இது};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            மாறிலி Ast_முனை* lhsp = nodep->lhsp();
            எனில் (மாறிலி AstCCast* மாறிலி castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            மாறிலி AstConst* மாறிலி constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            மாறிலி V3_எண்& comp_இலக்கம் = constp->இலக்கம்();

            auto setPolarities = [இது, &comp_இலக்கம்](மாறிலி LeafInfo& ref, மாறிலி V3_எண்* maskp) {
                மாறிலி தருக்க maskFlip = is_And_மரம்() ^ ref.polarity();
                முழு constantWidth = comp_இலக்கம்.width();
                எனில் (maskp) constantWidth = std::max(constantWidth, maskp->width());
                மாறிலி முழு maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                க்கு (முழு bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    மாறிலி முழு maskIdx = bitIdx - ref.lsb();
                    மாறிலி தருக்க mask0 = maskp && maskp->bitIs0(maskIdx);
                    மாறிலி தருக்க outOfRange = bitIdx > ref.msb();
                    எனில் (mask0 || outOfRange) {  // RHS is 0
                        எனில் (comp_இலக்கம்.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, உண்மை, 0);
                            m_bitPolarities.emplace_back(ref, பொய், 0);
                            நிறுத்து;
                        } இல்லையெனில் {  // This bitIdx is irrelevant
                            தொடர்;
                        }
                    }
                    மாறிலி தருக்க polarity = comp_இலக்கம்.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            எனில் (மாறிலி AstAnd* மாறிலி andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                மாறிலி LeafInfo& mask = findLeaf(andp->lhsp(), உண்மை);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                மாறிலி LeafInfo& ref = findLeaf(andp->rhsp(), பொய்);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                மாறிலி V3_எண்& mask_இலக்கம் = mask.constp()->இலக்கம்();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &mask_இலக்கம்);
            } இல்லையெனில் {  // comp == v
                மாறிலி LeafInfo& ref = findLeaf(nodep->rhsp(), பொய்);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } இல்லையெனில் {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_மரம்_Visitor(Ast_முனை_கோவை* nodep, unsigned externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.தள்ளு_back(nullptr);
        CONST_BITOP_RETURN_IF(!is_And_மரம்() && !is_Or_மரம்() && !is_Xor_மரம்(), nodep);
        எனில் (Ast_முனை_Biop* மாறிலி biopp = VN_CAST(nodep, முனை_Biop)) {
            iterateConst(biopp);
        } இல்லையெனில் {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        க்கு (auto&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(is_Xor_மரம்() || m_polarity, nodep, "must be the original polarity");
    }
    மெய்நிகர் ~Const_Bit_Op_மரம்_Visitor() = இயல்பு;
#undef CONST_BITOP_RETURN_IF
#undef CONST_BITOP_SET_FAILED

பொது:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    நிலையான Ast_முனை_கோவை* simplify(Ast_முனை_கோவை* nodep, முழு resultWidth, unsigned externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        மாறிலி Const_Bit_Op_மரம்_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        எனில் (visitor.m_failed || visitor.m_varInfos.size() == 1) திரும்பு nullptr;

        // FileLine used for constructing all new nodes in this function
        File_வரி* மாறிலி fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<Ast_முனை_கோவை*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        unsigned resultOps = 0;
        தருக்க hasCleanTerm = பொய்;
        தருக்க hasDirtyTerm = பொய்;

        க்கு (auto&& v : visitor.m_varInfos) {
            எனில் (!v) தொடர்;  // Skip nullptr at m_varInfos[0]
            எனில் (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_முனை_கோவை* resultp = nullptr;
                எனில் (v->getConstResult()) {
                    UASSERT_OBJ(visitor.is_Or_மரம்(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = புதிய AstConst{fl, AstConst::BitTrue{}};
                } இல்லையெனில் எனில் (visitor.is_And_மரம்()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = புதிய AstConst{fl, AstConst::BitFalse{}};
                } இல்லையெனில் {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    தொடர்;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                க்கு (Ast_முனை* மாறிலி termp : termps) VL_DO_DANGLING(termp->delete_மரம்(), termp);
                திரும்பு resultp;
            }
            மாறிலி ResultTerm result = v->getResultTerm();
            termps.தள்ளு_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            எனில் (std::get<2>(result)) {
                hasCleanTerm = உண்மை;
                UINFO(9, "Clean term: " << termps.back());
            } இல்லையெனில் {
                hasDirtyTerm = உண்மை;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_முனை_Info, std::vector<Ast_முனை_கோவை*>> frozenNodes;
        // Check if frozen terms are clean or not
        க்கு (மாறிலி auto& frozenInfo : visitor.m_frozenNodes) {
            Ast_முனை_கோவை* மாறிலி termp = frozenInfo.first;
            // Comparison operators are clean
            எனில் ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = உண்மை;
            } இல்லையெனில் {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = உண்மை;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].தள்ளு_back(termp);
        }

        // Figure out if a final negation is required
        மாறிலி தருக்க needsFlip = visitor.is_Xor_மரம்() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        மாறிலி தருக்க needsCleaning = visitor.is_And_மரம்() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        க்கு (மாறிலி auto& lsbAndNodes : frozenNodes) {
            எனில் (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            எனில் (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        எனில் (needsFlip) ++resultOps;
        // Add final cleaning AND
        எனில் (needsCleaning) ++resultOps;

        எனில் (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            க்கு (மாறிலி Ast_முனை_கோவை* மாறிலி termp : termps) termp->dump_மரம்("-  Reduced term: ");
            க்கு (மாறிலி std::pair<Ast_முனை_கோவை*, Frozen_முனை_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_மரம்("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        எனில் (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            Ast_முனை_கோவை* மாறிலி resultp = needsFlip ? புதிய AstConst{fl, AstConst::BitTrue{}}
                                                   : புதிய AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            திரும்பு resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        எனில் (visitor.m_ops <= resultOps) {
            க்கு (Ast_முனை* மாறிலி termp : termps) VL_DO_DANGLING(termp->delete_மரம்(), termp);
            திரும்பு nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        மாறிலி auto reduce = [&visitor, fl](Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) -> Ast_முனை_கோவை* {
            எனில் (!lhsp) திரும்பு rhsp;
            எனில் (visitor.is_And_மரம்()) {
                திரும்பு புதிய AstAnd{fl, lhsp, rhsp};
            } இல்லையெனில் எனில் (visitor.is_Or_மரம்()) {
                திரும்பு புதிய AstOr{fl, lhsp, rhsp};
            } இல்லையெனில் {
                திரும்பு புதிய AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_முனை_கோவை* resultp = nullptr;
        க்கு (Ast_முனை_கோவை* மாறிலி termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        க்கு (auto&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_முனை_கோவை* termp = nullptr;
            க்கு (Ast_முனை_கோவை* மாறிலி itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            எனில் (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_முனை_DType* மாறிலி dtypep = termp->dtypep();
                termp = புதிய AstShiftR{termp->fileline(), termp,
                                      புதிய AstConst(termp->fileline(), AstConst::Widthed_மதிப்பு{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            எனில் (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_முனை_DType* மாறிலி dtypep = termp->dtypep();
                termp = புதிய AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        மாறிலி முழு maskWidth
            = std::max(resultp->width(), resultWidth == 1 ? VL_IDATASIZE : resultWidth);

        // Apply final polarity flip
        எனில் (needsFlip) {
            எனில் (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = புதிய AstNot{fl, resultp};
            } இல்லையெனில் {
                // Keep result clean by using XOR(1, _)
                AstConst* மாறிலி maskp = புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, maskWidth, 1};
                resultp = புதிய AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        எனில் (needsCleaning) {
            AstConst* மாறிலி maskp = புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, maskWidth, 1};
            resultp = புதிய AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        எனில் (resultp->width() != resultWidth) {
            resultp = புதிய AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        திரும்பு resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

வகுப்பு ConstVisitor final : பொது VNVisitor {
    // CONSTANTS
    நிலையான constexpr unsigned CONCAT_MERGABLE_MAX_DEPTH = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    நிலையான constexpr முழு CONCAT_ASSIGN_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    தருக்க m_params = பொய்;  // If true, propagate parameterized and true numbers only
    தருக்க m_required = பொய்;  // If true, must become a constant
    தருக்க m_wremove = உண்மை;  // Inside scope, no assignw removal
    தருக்க m_warn = பொய்;  // Output warnings
    தருக்க m_doExpensive = பொய்;  // Enable computationally expensive optimizations
    தருக்க m_doCpp = பொய்;  // Enable late-stage C++ optimizations
    தருக்க m_doNConst = பொய்;  // Enable non-constant-child simplifications
    தருக்க m_doV = பொய்;  // Verilog, not C++ conversion
    தருக்க m_doGenerate = பொய்;  // Postpone width checking inside generate
    தருக்க m_convertLogicToBit = பொய்;  // Convert logical operators to bitwise
    தருக்க m_hasJumpDelay = பொய்;  // JumpGo or Delay under this loop
    தருக்க m_hasLoopTest = பொய்;  // Contains AstLoopTest
    தருக்க m_underRecFunc = பொய்;  // Under a recursive function
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    மாறிலி AstArraySel* m_selp = nullptr;  // Current select
    மாறிலி AstScope* m_scopep = nullptr;  // Current scope
    மாறிலி AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_கோவை_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_கோவை_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    மாறிலி தருக்க m_globalPass;  // ConstVisitor invoked as a global pass
    நிலையான uint32_t s_global_Pass_இலக்கம்;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<size_t, தருக்க> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<மாறிலி AstEnumItem*> m_recursing_Enum_பொருட்கள்;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    நிலையான தருக்க canDeleteStatements() { திரும்பு v3Global.staticsLifted(); }

    வெறுமை deleteVarScopesUnder(Ast_முனை* subtreep) {
        எனில் (!subtreep) திரும்பு;
        எனில் (!m_scopep) திரும்பு;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.செருகு(varp); });
        எனில் (varps.empty()) திரும்பு;
        க்கு (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            எனில் (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(தள்ளு_Deletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3_எண் const_இலக்கம்_V(Ast_முனை* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        மாறிலி V3_எண்& numc = VN_AS(nodep, Const)->இலக்கம்();
        திரும்பு !numc.is_எண்() ? numc : V3_எண்{nodep, nodep->widthMinV(), numc};
    }
    V3_எண் to_இலக்கம்_C(Ast_முனை* nodep, மாறிலி V3_எண்& numv) {
        // Extend V width back to C width for given node
        திரும்பு !numv.is_எண்() ? numv : V3_எண்{nodep, nodep->width(), numv};
    }

    நிலையான தருக்க lowerAsFixedAggregate(மாறிலி Ast_முனை_DType* மாறிலி dtypep) {
        திரும்பு dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(Ast_முனை_கோவை* மாறிலி fromp, மாறிலி AstMemberDType* மாறிலி itemp) {
        AstStructSel* மாறிலி selp = புதிய AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        திரும்பு selp;
    }

    வெறுமை collectFixedAggregateTerms(Ast_முனை_கோவை* மாறிலி fromp, std::vector<Ast_முனை_கோவை*>& termps,
                                    மாறிலி தருக்க packReal) {
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = fromp->dtypep()->skipRefp();
        எனில் (மாறிலி AstUnpackArrayDType* மாறிலி unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            க்கு (மாறிலி முழு idx : unpackDtypep->declRange().seq_இடது_To_வலது()) {
                AstArraySel* மாறிலி selp
                    = புதிய AstArraySel{fromp->fileline(), fromp->clone_மரம்_Pure(பொய்), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(தள்ளு_Deletep(fromp), fromp);
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep
                   = VN_CAST(dtypep, முனை_UOr_Struct_DType)) {
            எனில் (sdtypep->packed()) {
                termps.தள்ளு_back(fromp);
                திரும்பு;
            }
            க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_மரம்_Pure(பொய்), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(தள்ளு_Deletep(fromp), fromp);
        } இல்லையெனில் எனில் (packReal && dtypep->isDouble()) {
            termps.தள்ளு_back(புதிய AstRealToBits{fromp->fileline(), fromp});
        } இல்லையெனில் {
            termps.தள்ளு_back(fromp);
        }
    }

    Ast_முனை_கோவை* packFixedAggregate(Ast_முனை_கோவை* மாறிலி fromp) {
        std::vector<Ast_முனை_கோவை*> termps;
        collectFixedAggregateTerms(fromp, termps, உண்மை);
        UASSERT(!termps.empty(), "No stream terms");
        Ast_முனை_கோவை* resultp = termps[0];
        க்கு (size_t i = 1; i < termps.size(); ++i) {
            resultp = புதிய AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        திரும்பு resultp;
    }

    வெறுமை replaceAssignToFixedAggregate(Ast_முனை_Assign* மாறிலி nodep, Ast_முனை_கோவை* மாறிலி dstp,
                                       Ast_முனை_கோவை* srcp) {
        மாறிலி முழு dstWidth = dstp->dtypep()->widthStream();
        std::vector<Ast_முனை_கோவை*> termps;
        collectFixedAggregateTerms(dstp, termps, பொய்);
        முழு srcWidth = srcp->width();
        எனில் (srcWidth < dstWidth) {
            AstExtend* மாறிலி extendp = புதிய AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = புதிய AstShiftL{
                extendp->fileline(), extendp,
                புதிய AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        Ast_முனை_Assign* newp = nullptr;
        முழு offset = 0;
        க்கு (size_t i = 0; i < termps.size(); ++i) {
            Ast_முனை_கோவை* மாறிலி termp = termps[i];
            மாறிலி முழு width = termp->dtypep()->widthStream();
            மாறிலி முழு lsb = srcWidth - offset - width;
            offset += width;
            Ast_முனை_கோவை* rhsp
                = புதிய AstSel{srcp->fileline(), srcp->clone_மரம்_Pure(பொய்), lsb, width};
            எனில் (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = புதிய AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            Ast_முனை_Assign* மாறிலி assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_முனை::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(srcp), srcp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    தருக்க operandConst(மாறிலி Ast_முனை* nodep) { திரும்பு VN_IS(nodep, Const); }
    தருக்க operandAsvConst(மாறிலி Ast_முனை* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        மாறிலி Ast_முனை_Bi_Com_Asv* மாறிலி bnodep = VN_CAST(nodep, முனை_Bi_Com_Asv);
        எனில் (!bnodep) திரும்பு பொய்;
        எனில் (!VN_IS(bnodep->lhsp(), Const)) திரும்பு பொய்;
        மாறிலி Ast_முனை_Bi_Com_Asv* மாறிலி rnodep = VN_CAST(bnodep->rhsp(), முனை_Bi_Com_Asv);
        எனில் (!rnodep) திரும்பு பொய்;
        எனில் (rnodep->type() != bnodep->type()) திரும்பு பொய்;
        எனில் (rnodep->width() != bnodep->width()) திரும்பு பொய்;
        எனில் (rnodep->lhsp()->width() != bnodep->lhsp()->width()) திரும்பு பொய்;
        எனில் (!VN_IS(rnodep->lhsp(), Const)) திரும்பு பொய்;
        திரும்பு உண்மை;
    }
    தருக்க operandAsvSame(மாறிலி Ast_முனை* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        மாறிலி Ast_முனை_Bi_Com_Asv* மாறிலி bnodep = VN_CAST(nodep, முனை_Bi_Com_Asv);
        எனில் (!bnodep) திரும்பு பொய்;
        மாறிலி Ast_முனை_Bi_Com_Asv* மாறிலி rnodep = VN_CAST(bnodep->rhsp(), முனை_Bi_Com_Asv);
        எனில் (!rnodep) திரும்பு பொய்;
        எனில் (rnodep->type() != bnodep->type()) திரும்பு பொய்;
        எனில் (rnodep->width() != bnodep->width()) திரும்பு பொய்;
        திரும்பு operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    தருக்க operandAsvLUp(மாறிலி Ast_முனை* nodep) {
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
        மாறிலி Ast_முனை_Bi_Com_Asv* மாறிலி bnodep = VN_CAST(nodep, முனை_Bi_Com_Asv);
        எனில் (!bnodep) திரும்பு பொய்;
        மாறிலி Ast_முனை_Bi_Com_Asv* மாறிலி lnodep = VN_CAST(bnodep->lhsp(), முனை_Bi_Com_Asv);
        எனில் (!lnodep) திரும்பு பொய்;
        எனில் (lnodep->type() != bnodep->type()) திரும்பு பொய்;
        எனில் (lnodep->width() != bnodep->width()) திரும்பு பொய்;
        திரும்பு VN_IS(lnodep->lhsp(), Const);
    }
    தருக்க operandAsvRUp(மாறிலி Ast_முனை* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        மாறிலி Ast_முனை_Bi_Com_Asv* மாறிலி bnodep = VN_CAST(nodep, முனை_Bi_Com_Asv);
        எனில் (!bnodep) திரும்பு பொய்;
        மாறிலி Ast_முனை_Bi_Com_Asv* மாறிலி rnodep = VN_CAST(bnodep->rhsp(), முனை_Bi_Com_Asv);
        எனில் (!rnodep) திரும்பு பொய்;
        எனில் (rnodep->type() != bnodep->type()) திரும்பு பொய்;
        எனில் (rnodep->width() != bnodep->width()) திரும்பு பொய்;
        திரும்பு VN_IS(rnodep->lhsp(), Const);
    }
    நிலையான தருக்க operandSubAdd(மாறிலி Ast_முனை* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        மாறிலி Ast_முனை_Biop* மாறிலி np = VN_CAST(nodep, முனை_Biop);
        மாறிலி Ast_முனை_Biop* மாறிலி lp = VN_CAST(np->lhsp(), முனை_Biop);
        திரும்பு (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    தருக்க matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* மாறிலி constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_முனை_கோவை* மாறிலி rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        மாறிலி auto isEqOrNeq = [](மாறிலி Ast_முனை* nodep) -> தருக்க {  //
            திரும்பு VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        எனில் (isEqOrNeq(rhsp)) {
            ccastp = புதிய AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } இல்லையெனில் எனில் (AstCCast* மாறிலி tmpp = VN_CAST(rhsp, CCast)) {
            எனில் (isEqOrNeq(tmpp->lhsp())) {
                எனில் (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } இல்லையெனில் {
                    ccastp = புதிய AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        எனில் (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(தள்ளு_Deletep(andp), andp);
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    நிலையான தருக்க operandAndOrSame(மாறிலி Ast_முனை* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        மாறிலி Ast_முனை_Biop* மாறிலி np = VN_CAST(nodep, முனை_Biop);
        மாறிலி Ast_முனை_Biop* மாறிலி lp = VN_CAST(np->lhsp(), முனை_Biop);
        மாறிலி Ast_முனை_Biop* மாறிலி rp = VN_CAST(np->rhsp(), முனை_Biop);
        திரும்பு (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    தருக்க matchOrAndNot(Ast_முனை_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        மாறிலி Ast_முனை_கோவை* ap;
        Ast_முனை_Biop* andp;
        எனில் (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } இல்லையெனில் எனில் (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } இல்லையெனில் {
            திரும்பு பொய்;
        }
        மாறிலி Ast_முனை_Uniop* notp;
        Ast_முனை_கோவை* cp;
        எனில் (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } இல்லையெனில் எனில் (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } இல்லையெனில் {
            திரும்பு பொய்;
        }
        மாறிலி Ast_முனை_கோவை* மாறிலி bp = notp->lhsp();
        எனில் (!operandsSame(ap, bp)) திரும்பு பொய்;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        எனில் (nodep->rhsp()) {
            nodep->lhsp(cp);
        } இல்லையெனில் {
            nodep->rhsp(cp);
        }
        திரும்பு உண்மை;
    }
    தருக்க matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* மாறிலி condp = VN_CAST(nodep->rhsp(), Cond);
        எனில் (!condp) திரும்பு பொய்;
        எனில் (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) திரும்பு பொய்;
        AstConst* மாறிலி maskp = VN_CAST(nodep->lhsp(), Const);
        எனில் (!maskp) திரும்பு பொய்;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* மாறிலி newp = புதிய AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            புதிய AstAnd{nodep->fileline(), maskp->clone_மரம்(பொய்), condp->thenp()->unlinkFrBack()},
            புதிய AstAnd{nodep->fileline(), maskp->clone_மரம்(பொய்),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }

    தருக்க matchCondCond(மாறிலி AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        எனில் (VN_IS(nodep->backp(), Cond)) திரும்பு பொய்;  // Was checked when visited parent
        எனில் (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            திரும்பு பொய்;  // Short circuit
        std::vector<Ast_முனை_கோவை*> truesp;
        std::vector<Ast_முனை_கோவை*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        திரும்பு பொய்;  // Can optimize further
    }
    வெறுமை matchCondCondRecurse(மாறிலி AstCond* nodep, std::vector<Ast_முனை_கோவை*>& truesp,
                              std::vector<Ast_முனை_கோவை*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        எனில் (truesp.size() > 4 || falsesp.size() > 4) திரும்பு;
        எனில் (!nodep->condp()->isPure()) திரும்பு;
        தருக்க replaced = பொய்;
        க்கு (மாறிலி Ast_முனை_கோவை* condp : truesp) {
            எனில் (replaced) நிறுத்து;
            எனில் (!operandsSame(nodep->condp(), condp)) தொடர்;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_இலக்கம்(nodep->condp(), 1);
            replaced = உண்மை;
            ++m_stat_Cond_கோவை_Redundant;
        }
        க்கு (மாறிலி Ast_முனை_கோவை* condp : falsesp) {
            எனில் (replaced) நிறுத்து;
            எனில் (!operandsSame(nodep->condp(), condp)) தொடர்;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = உண்மை;
            ++m_stat_Cond_கோவை_Redundant;
        }
        எனில் (மாறிலி AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            எனில் (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            எனில் (!replaced) truesp.எடு_back();
        }
        எனில் (மாறிலி AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            எனில் (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            எனில் (!replaced) falsesp.எடு_back();
        }
    }
    வெறுமை matchIfCondCond(மாறிலி Ast_முனை_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        எனில் (VN_IS(nodep->backp(), If)) திரும்பு;  // Was checked when visited parent
        எனில் (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) திரும்பு;  // Short circuit
        std::vector<Ast_முனை_கோவை*> truesp;
        std::vector<Ast_முனை_கோவை*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    வெறுமை matchIfCondCondRecurse(மாறிலி Ast_முனை_If* nodep, std::vector<Ast_முனை_கோவை*>& truesp,
                                std::vector<Ast_முனை_கோவை*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        எனில் (truesp.size() > 4 || falsesp.size() > 4) திரும்பு;
        எனில் (!nodep->condp()->isPure()) திரும்பு;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        எனில் (VN_IS(nodep->condp(), Const)) திரும்பு;
        தருக்க replaced = பொய்;
        க்கு (மாறிலி Ast_முனை_கோவை* condp : truesp) {
            எனில் (replaced) நிறுத்து;
            எனில் (!operandsSame(nodep->condp(), condp)) தொடர்;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_இலக்கம்(nodep->condp(), 1);
            replaced = உண்மை;
            ++m_stat_If_Cond_கோவை_Redundant;
        }
        க்கு (மாறிலி Ast_முனை_கோவை* condp : falsesp) {
            எனில் (replaced) நிறுத்து;
            எனில் (!operandsSame(nodep->condp(), condp)) தொடர்;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = உண்மை;
            ++m_stat_If_Cond_கோவை_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        எனில் (மாறிலி Ast_முனை_If* subIfp = VN_CAST(nodep->thensp(), முனை_If)) {
            எனில் (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            எனில் (!replaced) truesp.எடு_back();
        }
        எனில் (மாறிலி Ast_முனை_If* subIfp = VN_CAST(nodep->elsesp(), முனை_If)) {
            எனில் (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            எனில் (!replaced) falsesp.எடு_back();
        }
    }

    வெறுமை matchIfSameLast(Ast_முனை_If* nodep) {
        எனில் (!nodep->thensp()) திரும்பு;
        எனில் (!nodep->elsesp()) திரும்பு;
        எனில் (!canDeleteStatements()) திரும்பு;
        // Iterate bodies in reverse
        Ast_முனை* tp = nodep->thensp()->lastp();
        Ast_முனை* ep = nodep->elsesp()->lastp();
        போது (tp && ep && tp->same_மரம்(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_முனை* மாறிலி tPrevp = tp->prevp();
            Ast_முனை* மாறிலி ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(ep->unlinkFrBack()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    தருக்க matchMaskedOr(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        மாறிலி AstConst* மாறிலி constp = VN_AS(nodep->lhsp(), Const);
        மாறிலி uint32_t significantBits = constp->இலக்கம்().widthToFit();

        AstOr* மாறிலி orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        மாறிலி auto checkBottomClear = [=](மாறிலி Ast_முனை* nodep) -> தருக்க {
            எனில் (மாறிலி AstShiftL* மாறிலி shiftp = VN_CAST(nodep, ShiftL)) {
                எனில் (மாறிலி AstConst* மாறிலி scp = VN_CAST(shiftp->rhsp(), Const)) {
                    திரும்பு scp->இலக்கம்().toUInt() >= significantBits;
                }
            }
            திரும்பு பொய்;
        };

        மாறிலி தருக்க orLIsRedundant = checkBottomClear(orp->lhsp());
        மாறிலி தருக்க orRIsRedundant = checkBottomClear(orp->rhsp());

        எனில் (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                புதிய AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு உண்மை;
        } இல்லையெனில் எனில் (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(orp), orp);
            திரும்பு பொய்;  // input node is still valid, keep going
        } இல்லையெனில் எனில் (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(orp), orp);
            திரும்பு பொய்;  // input node is still valid, keep going
        } இல்லையெனில் {
            திரும்பு பொய்;
        }
    }
    தருக்க matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        மாறிலி auto checkMask = [nodep, இது](மாறிலி V3_எண்& mask) -> தருக்க {
            மாறிலி AstConst* மாறிலி constp = VN_AS(nodep->lhsp(), Const);
            எனில் (constp->இலக்கம்().isCaseEq(mask)) {
                Ast_முனை* மாறிலி rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு உண்மை;
            }
            திரும்பு பொய்;
        };

        // Check if masking is redundant
        எனில் (மாறிலி AstShiftR* மாறிலி shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            எனில் (மாறிலி AstConst* மாறிலி scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_எண் maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->இலக்கம்().toUInt());
                திரும்பு checkMask(maskLo);
            }
        } இல்லையெனில் எனில் (மாறிலி AstShiftL* மாறிலி shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            எனில் (மாறிலி AstConst* மாறிலி scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_எண் mask{nodep, nodep->width()};
                மாறிலி uint32_t shiftAmount = scp->இலக்கம்().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                திரும்பு checkMask(mask);
            }
        }
        திரும்பு பொய்;
    }

    தருக்க matchMaskedZero(மாறிலி AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        மாறிலி AstConst* மாறிலி maskp = VN_AS(nodep->lhsp(), Const);
        மாறிலி Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp();
        மாறிலி uint32_t msbP1 = maskp->இலக்கம்().mostSetBitP1();
        எனில் (!msbP1) திரும்பு பொய்;  // Don't rewrite, separate rule matches for this
        மாறிலி uint32_t msb = msbP1 - 1;
        மாறிலி uint32_t lsb = maskp->இலக்கம்().leastSetBitP1() - 1;

        எனில் (மாறிலி AstShiftL* மாறிலி shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            எனில் (AstConst* மாறிலி scp = VN_CAST(shiftp->rhsp(), Const)) {
                திரும்பு scp->இலக்கம்().fitsInUInt()  //
                       && (scp->இலக்கம்().toUInt() > msb);
            }
        }
        எனில் (மாறிலி AstShiftR* மாறிலி shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            எனில் (AstConst* மாறிலி scp = VN_CAST(shiftp->rhsp(), Const)) {
                திரும்பு scp->இலக்கம்().fitsInUInt()
                       && (lsb + scp->இலக்கம்().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        எனில் (மாறிலி AstMul* மாறிலி mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            எனில் (AstConst* மாறிலி cp = VN_CAST(mulp->lhsp(), Const)) {
                திரும்பு cp->இலக்கம்().leastSetBitP1() > msb + 1;
            }
        }
        எனில் (மாறிலி AstExtend* மாறிலி extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            திரும்பு lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        திரும்பு பொய்;
    }

    தருக்க match_Bit_Op_மரம்(Ast_முனை_கோவை* nodep) {
        எனில் (nodep->widthMin() != 1) திரும்பு பொய்;
        எனில் (!v3Global.opt.f_Const_Bit_Op_மரம்()) திரும்பு பொய்;

        மாறிலி முழு width = nodep->width();
        Ast_முனை_கோவை* rootp = nodep;
        unsigned externalOps = 0;
        // Reach past a plain making AND
        எனில் (மாறிலி AstAnd* மாறிலி andp = VN_CAST(nodep, And)) {
            எனில் (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        எனில் (rootp->type() == nodep->backp()->type()) திரும்பு பொய்;

        std::string debugPrefix;
        எனில் (debug() >= 9) {  // LCOV_EXCL_START
            நிலையான முழு s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dump_மரம்(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_முனை_கோவை* மாறிலி newp = Const_Bit_Op_மரம்_Visitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        எனில் (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }

        எனில் (debug() >= 9) {  // LCOV_EXCL_START
            எனில் (newp) {
                newp->dump_மரம்(debugPrefix + "RESULT: ");
            } இல்லையெனில் {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        திரும்பு newp;
    }
    நிலையான தருக்க operandShiftSame(மாறிலி Ast_முனை* nodep) {
        மாறிலி Ast_முனை_Biop* மாறிலி np = VN_AS(nodep, முனை_Biop);
        {
            மாறிலி AstShiftL* மாறிலி lp = VN_CAST(np->lhsp(), ShiftL);
            மாறிலி AstShiftL* மாறிலி rp = VN_CAST(np->rhsp(), ShiftL);
            எனில் (lp && rp) {
                திரும்பு (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            மாறிலி AstShiftR* மாறிலி lp = VN_CAST(np->lhsp(), ShiftR);
            மாறிலி AstShiftR* மாறிலி rp = VN_CAST(np->rhsp(), ShiftR);
            எனில் (lp && rp) {
                திரும்பு (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        திரும்பு பொய்;
    }
    தருக்க operandHugeShiftL(மாறிலி Ast_முனை_Biop* nodep) {
        திரும்பு (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->இலக்கம்().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->இலக்கம்().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    தருக்க operandHugeShiftR(மாறிலி Ast_முனை_Biop* nodep) {
        திரும்பு (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->இலக்கம்().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->இலக்கம்().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    தருக்க operandIsTwo(மாறிலி Ast_முனை* nodep) {
        மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const);
        எனில் (!constp) திரும்பு பொய்;  // not constant
        எனில் (constp->இலக்கம்().isFourState()) திரும்பு பொய்;  // four-state
        எனில் (nodep->width() > VL_QUADSIZE) திரும்பு பொய்;  // too wide
        எனில் (nodep->isSigned() && constp->இலக்கம்().isNegative()) திரும்பு பொய்;  // signed and negative
        திரும்பு constp->toUQuad() == 2;
    }
    தருக்க operandIsTwostate(மாறிலி Ast_முனை* nodep) {
        திரும்பு (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->இலக்கம்().isFourState());
    }
    தருக்க operandIsPowTwo(மாறிலி Ast_முனை* nodep) {
        எனில் (!operandIsTwostate(nodep)) திரும்பு பொய்;
        திரும்பு (1 == VN_AS(nodep, Const)->இலக்கம்().மொத்தம்_Ones());
    }
    தருக்க operandShiftOp(மாறிலி Ast_முனை_Biop* nodep) {
        எனில் (!VN_IS(nodep->rhsp(), Const)) திரும்பு பொய்;
        மாறிலி Ast_முனை_Biop* மாறிலி lhsp = VN_CAST(nodep->lhsp(), முனை_Biop);
        எனில் (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) திரும்பு பொய்;
        எனில் (nodep->width() != lhsp->width()) திரும்பு பொய்;
        எனில் (nodep->width() != lhsp->lhsp()->width()) திரும்பு பொய்;
        எனில் (nodep->width() != lhsp->rhsp()->width()) திரும்பு பொய்;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        எனில் (VN_IS(lhsp->lhsp(), Const)) திரும்பு உண்மை;
        எனில் (VN_IS(lhsp->rhsp(), Const)) திரும்பு உண்மை;
        // - Const shift on either side
        எனில் (Ast_முனை_Biop* மாறிலி llp = VN_CAST(lhsp->lhsp(), முனை_Biop)) {
            திரும்பு (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        எனில் (Ast_முனை_Biop* மாறிலி lrp = VN_CAST(lhsp->rhsp(), முனை_Biop)) {
            திரும்பு (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        திரும்பு பொய்;
    }
    தருக்க operandShiftShift(மாறிலி Ast_முனை_Biop* nodep) {
        // We could add a AND though.
        மாறிலி Ast_முனை_Biop* மாறிலி lhsp = VN_CAST(nodep->lhsp(), முனை_Biop);
        எனில் (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) திரும்பு பொய்;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        எனில் (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) திரும்பு பொய்;
        எனில் (VN_AS(nodep->rhsp(), Const)->இலக்கம்().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->இலக்கம்().isFourState())
            திரும்பு பொய்;
        எனில் (nodep->width() != lhsp->width()) திரும்பு பொய்;
        எனில் (nodep->width() != lhsp->lhsp()->width()) திரும்பு பொய்;
        திரும்பு உண்மை;
    }
    தருக்க operandWordOOB(மாறிலி AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        திரும்பு (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), முனை_Var_Ref)
                && VN_AS(nodep->fromp(), முனை_Var_Ref)->access().isReadOnly()
                && (static_cast<முழு>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), முனை_Var_Ref)->varp()->width_சொற்கள்()));
    }
    தருக்க operandSelFull(மாறிலி AstSel* nodep) {
        திரும்பு (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<முழு>(nodep->widthConst()) == nodep->fromp()->width());
    }
    தருக்க operandSelExtend(AstSel* nodep) {
        எனில் (!m_doV) திரும்பு பொய்;
        AstExtend* மாறிலி extendp = VN_CAST(nodep->fromp(), Extend);
        எனில் (!extendp) திரும்பு பொய்;
        AstConst* மாறிலி lsbp = VN_CAST(nodep->lsbp(), Const);
        எனில் (!lsbp) திரும்பு பொய்;
        மாறிலி முழு width = nodep->widthConst();
        மாறிலி முழு lsb = lsbp->toSInt();
        மாறிலி முழு msb = lsb + width - 1;
        Ast_முனை_கோவை* மாறிலி lhsp = extendp->lhsp();
        எனில் (!lhsp->isPure()) திரும்பு பொய்;

        // Selecting the entire extended expression, replace with that
        எனில் (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு உண்மை;
        }
        // Select entirely in the extended part - replace with zero
        எனில் (lsb >= lhsp->width()) {
            replaceZero(nodep);
            திரும்பு உண்மை;
        }
        // Select entirely in the extended expression - replace with select from that
        எனில் (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(புதிய AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு உண்மை;
        }
        // Select straddles both sides, but is just a shorter extend
        எனில் (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(புதிய AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு உண்மை;
        }

        திரும்பு பொய்;
    }
    தருக்க operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_முனை_Biop* மாறிலி bip = VN_CAST(nodep->fromp(), முனை_Biop);
        எனில் (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) திரும்பு பொய்;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_முனை_கோவை* மாறிலி bilhsp = bip->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(புதிய AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(புதிய AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        திரும்பு உண்மை;
    }
    தருக்க operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        மாறிலி AstShiftR* மாறிலி shiftp = VN_CAST(nodep->fromp(), ShiftR);
        எனில் (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            திரும்பு பொய்;
        }
        Ast_முனை_கோவை* மாறிலி ap = shiftp->lhsp();
        மாறிலி AstConst* மாறிலி bp = VN_AS(shiftp->rhsp(), Const);
        மாறிலி AstConst* மாறிலி lp = VN_AS(nodep->lsbp(), Const);
        எனில் (bp->isWide() || bp->இலக்கம்().isFourState() || bp->இலக்கம்().isNegative() || lp->isWide()
            || lp->இலக்கம்().isFourState() || lp->இலக்கம்().isNegative()) {
            திரும்பு பொய்;
        }
        மாறிலி முழு newLsb = lp->toSInt() + bp->toSInt();
        எனில் (newLsb + nodep->widthConst() > ap->width()) திரும்பு பொய்;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* மாறிலி newp
            = புதிய AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }

    தருக்க operandBiExtendConstShrink(Ast_முனை_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* மாறிலி extendp = VN_CAST(nodep->rhsp(), Extend);
        எனில் (!extendp) திரும்பு பொய்;
        Ast_முனை_கோவை* மாறிலி smallerp = extendp->lhsp();
        மாறிலி முழு subsize = smallerp->width();
        AstConst* மாறிலி constp = VN_CAST(nodep->lhsp(), Const);
        எனில் (!constp) திரும்பு பொய்;
        எனில் (!constp->இலக்கம்().isBitsZero(constp->width() - 1, subsize)) திரும்பு பொய்;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        மாறிலி V3_எண் இலக்கம்{constp, subsize, constp->இலக்கம்()};
        nodep->lhsp(புதிய AstConst{constp->fileline(), இலக்கம்});
        VL_DO_DANGLING(தள்ளு_Deletep(constp), constp);
        திரும்பு பொய்;  // input node is still valid, keep going
    }
    தருக்க operandBiExtendConstOver(மாறிலி Ast_முனை_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        மாறிலி AstExtend* மாறிலி extendp = VN_CAST(nodep->rhsp(), Extend);
        எனில் (!extendp) திரும்பு பொய்;
        மாறிலி Ast_முனை* மாறிலி smallerp = extendp->lhsp();
        மாறிலி முழு subsize = smallerp->width();
        மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->lhsp(), Const);
        எனில் (!constp) திரும்பு பொய்;
        எனில் (constp->இலக்கம்().isBitsZero(constp->width() - 1, subsize)) திரும்பு பொய்;
        திரும்பு உண்மை;
    }

    // Extraction checks
    தருக்க warnSelect(AstSel* nodep) {
        எனில் (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        மாறிலி தருக்க doit = உண்மை;
        எனில் (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            மாறிலி முழு maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            எனில் (VN_AS(nodep->lsbp(), Const)->இலக்கம்().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } இல்லையெனில் எனில் (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                எனில் (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } இல்லையெனில் {
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
        திரும்பு பொய்;  // Not a transform, so NOP
    }

    நிலையான தருக்க operandsSame(மாறிலி Ast_முனை* node1p, மாறிலி Ast_முனை* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        எனில் (மாறிலி AstConst* மாறிலி const1p = VN_CAST(node1p, Const)) {
            எனில் (மாறிலி AstConst* மாறிலி const2p = VN_CAST(node2p, Const)) {
                திரும்பு V3_எண்{node1p->fileline(), 1, 0}
                    .opEq(const1p->இலக்கம்(), const2p->இலக்கம்())
                    .isEqOne();
            }
        }
        எனில் (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            திரும்பு node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        எனில் (மாறிலி AstAnd* மாறிலி and1p = VN_CAST(node1p, And)) {
            எனில் (மாறிலி AstAnd* மாறிலி and2p = VN_CAST(node2p, And)) {
                எனில் (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), முனை_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), முனை_Var_Ref))
                    திரும்பு node1p->same_Gate_மரம்(node2p);
            }
        }
        திரும்பு பொய்;
    }
    தருக்க ifSameAssign(மாறிலி Ast_முனை_If* nodep) {
        மாறிலி Ast_முனை_Assign* மாறிலி thensp = VN_CAST(nodep->thensp(), முனை_Assign);
        மாறிலி Ast_முனை_Assign* மாறிலி elsesp = VN_CAST(nodep->elsesp(), முனை_Assign);
        எனில் (!thensp || thensp->nextp()) திரும்பு பொய்;  // Must be SINGLE statement
        எனில் (!elsesp || elsesp->nextp()) திரும்பு பொய்;
        எனில் (thensp->type() != elsesp->type()) திரும்பு பொய்;  // Can't mix an assigndly with assign
        எனில் (!thensp->lhsp()->same_Gate_மரம்(elsesp->lhsp())) திரும்பு பொய்;
        எனில் (!thensp->rhsp()->gate_மரம்()) திரும்பு பொய்;
        எனில் (!elsesp->rhsp()->gate_மரம்()) திரும்பு பொய்;
        // Must not create an expression with unpacked array type
        எனில் (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) திரும்பு பொய்;
        எனில் (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) திரும்பு பொய்;
        எனில் (m_underRecFunc) திரும்பு பொய்;  // This optimization may lead to infinite recursion
        எனில் (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            எனில் (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) திரும்பு பொய்;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        திரும்பு !VN_IS(thensp->rhsp(), முனை_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), முனை_FTask_Ref)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    தருக்க operandIfIf(மாறிலி Ast_முனை_If* nodep) {
        எனில் (nodep->elsesp()) திரும்பு பொய்;
        மாறிலி Ast_முனை_If* மாறிலி lowerIfp = VN_CAST(nodep->thensp(), முனை_If);
        எனில் (!lowerIfp || lowerIfp->nextp()) திரும்பு பொய்;
        எனில் (nodep->type() != lowerIfp->type()) திரும்பு பொய்;
        எனில் (Ast_முனை::afterCommentp(lowerIfp->elsesp())) திரும்பு பொய்;
        திரும்பு உண்மை;
    }
    தருக்க ifConcatMergeableBiop(மாறிலி Ast_முனை* nodep) {
        திரும்பு (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    தருக்க ifAdjacentSel(மாறிலி AstSel* lhsp, மாறிலி AstSel* rhsp) {
        எனில் (!v3Global.opt.fAssemble()) திரும்பு பொய்;  // opt disabled
        எனில் (!lhsp || !rhsp) திரும்பு பொய்;
        மாறிலி Ast_முனை* மாறிலி lfromp = lhsp->fromp();
        மாறிலி Ast_முனை* மாறிலி rfromp = rhsp->fromp();
        எனில் (!lfromp || !rfromp || !lfromp->same_Gate_மரம்(rfromp)) திரும்பு பொய்;
        மாறிலி AstConst* மாறிலி lstart = VN_CAST(lhsp->lsbp(), Const);
        மாறிலி AstConst* மாறிலி rstart = VN_CAST(rhsp->lsbp(), Const);
        எனில் (!lstart || !rstart) திரும்பு பொய்;  // too complicated
        மாறிலி முழு rend = (rstart->toSInt() + rhsp->widthConst());
        திரும்பு (rend == lstart->toSInt());
    }
    தருக்க ifMergeAdjacent(மாறிலி Ast_முனை_கோவை* lhsp, மாறிலி Ast_முனை_கோவை* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        எனில் (!v3Global.opt.fAssemble()) திரும்பு பொய்;  // opt disabled
        // Two same varref
        எனில் (operandsSame(lhsp, rhsp)) திரும்பு உண்மை;
        மாறிலி AstSel* மாறிலி lselp = VN_CAST(lhsp, Sel);
        மாறிலி AstSel* மாறிலி rselp = VN_CAST(rhsp, Sel);
        எனில் (!lselp && !rselp) திரும்பு பொய்;
        எனில் (lselp && !VN_IS(lselp->lsbp(), Const)) திரும்பு பொய்;
        எனில் (rselp && !VN_IS(rselp->lsbp(), Const)) திரும்பு பொய்;
        // a[i:j] {a[j-1:k], b}
        எனில் (lselp && !rselp && VN_IS(rhsp, Concat))
            திரும்பு ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        எனில் (rselp && !lselp && VN_IS(lhsp, Concat))
            திரும்பு ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        மாறிலி Ast_முனை_கோவை* மாறிலி lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_மரம்(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        மாறிலி Ast_முனை_கோவை* மாறிலி rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_மரம்(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        எனில் (!lfromp || !rfromp || !lfromp->same_Gate_மரம்(rfromp)) திரும்பு பொய்;

        மாறிலி int32_t lstart = lselp ? lselp->lsbConst() : 0;
        மாறிலி int32_t rstart = rselp ? rselp->lsbConst() : 0;
        மாறிலி int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        எனில் (rend == lstart) திரும்பு உண்மை;
        // a[i:0] a[msb:j]
        எனில் (rend == rfromp->width() && lstart == 0) திரும்பு உண்மை;
        திரும்பு பொய்;
    }
    தருக்க concatMergeable(மாறிலி Ast_முனை_கோவை* lhsp, மாறிலி Ast_முனை_கோவை* rhsp, unsigned depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        எனில் (!v3Global.opt.fAssemble()) திரும்பு பொய்;  // opt disabled
        எனில் (lhsp->type() != rhsp->type()) திரும்பு பொய்;
        எனில் (!ifConcatMergeableBiop(lhsp)) திரும்பு பொய்;
        எனில் (depth > CONCAT_MERGABLE_MAX_DEPTH) திரும்பு பொய்;  // As worse case O(n^2) algorithm

        மாறிலி Ast_முனை_Biop* மாறிலி lp = VN_CAST(lhsp, முனை_Biop);
        மாறிலி Ast_முனை_Biop* மாறிலி rp = VN_CAST(rhsp, முனை_Biop);
        எனில் (!lp || !rp) திரும்பு பொய்;
        // {a[]&b[], a[]&b[]}
        மாறிலி தருக்க lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        மாறிலி தருக்க rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        எனில் (lad && rad) திரும்பு உண்மை;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        எனில் (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) திரும்பு உண்மை;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        எனில் (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) திரும்பு உண்மை;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        எனில் (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    நிலையான தருக்க operandsSameWidth(மாறிலி Ast_முனை* lhsp, மாறிலி Ast_முனை* rhsp) {
        திரும்பு lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    வெறுமை replace_இலக்கம்(Ast_முனை* oldp, மாறிலி V3_எண்& இலக்கம், மாறிலி string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->இலக்கம்().isFourState()), oldp,
                    "Already constant??");
        AstConst* மாறிலி newp = புதிய AstConst{oldp->fileline(), இலக்கம்};
        எனில் (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(தள்ளு_Deletep(oldp), oldp);
    }
    வெறுமை replace_இலக்கம்(Ast_முனை* nodep, uint32_t val) {
        மாறிலி V3_எண் இலக்கம்{nodep, nodep->width(), val};
        VL_DO_DANGLING(replace_இலக்கம்(nodep, இலக்கம்), nodep);
    }
    வெறுமை replace_இலக்கம்_Signed(Ast_முனை_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        எனில் (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_இலக்கம்(nodep, val), nodep);
    }
    வெறுமை replace_இலக்கம்_Limited(Ast_முனை_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        எனில் (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_இலக்கம்(nodep, val), nodep);
    }
    வெறுமை replaceZero(Ast_முனை* nodep) { VL_DO_DANGLING(replace_இலக்கம்(nodep, 0), nodep); }
    வெறுமை replaceZeroChkPure(Ast_முனை* nodep, Ast_முனை_கோவை* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        எனில் (checkp->isPure()) {
            VL_DO_DANGLING(replace_இலக்கம்(nodep, 0), nodep);
        } இல்லையெனில் {
            Ast_முனை* மாறிலி newp = புதிய AstAnd{nodep->fileline(), புதிய AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை replaceAllOnes(Ast_முனை* nodep) {
        V3_எண் ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replace_இலக்கம்(nodep, ones), nodep);
    }
    வெறுமை replaceConst(Ast_முனை_Uniop* nodep) {
        V3_எண் numv{nodep, nodep->widthMinV()};
        nodep->எண்_Operate(numv, const_இலக்கம்_V(nodep->lhsp()));
        மாறிலி V3_எண்& இலக்கம் = to_இலக்கம்_C(nodep, numv);
        UINFO(4, "UNICONST -> " << இலக்கம்);
        VL_DO_DANGLING(replace_இலக்கம்(nodep, இலக்கம்), nodep);
    }
    வெறுமை replaceConst(Ast_முனை_Biop* nodep) {
        V3_எண் numv{nodep, nodep->widthMinV()};
        nodep->எண்_Operate(numv, const_இலக்கம்_V(nodep->lhsp()), const_இலக்கம்_V(nodep->rhsp()));
        மாறிலி V3_எண்& இலக்கம் = to_இலக்கம்_C(nodep, numv);
        UINFO(4, "BICONST -> " << இலக்கம்);
        VL_DO_DANGLING(replace_இலக்கம்(nodep, இலக்கம்), nodep);
    }
    வெறுமை replaceConst(Ast_முனை_Triop* nodep) {
        V3_எண் numv{nodep, nodep->widthMinV()};
        nodep->எண்_Operate(numv, const_இலக்கம்_V(nodep->lhsp()), const_இலக்கம்_V(nodep->rhsp()),
                             const_இலக்கம்_V(nodep->thsp()));
        மாறிலி V3_எண்& இலக்கம் = to_இலக்கம்_C(nodep, numv);
        UINFO(4, "TRICONST -> " << இலக்கம்);
        VL_DO_DANGLING(replace_இலக்கம்(nodep, இலக்கம்), nodep);
    }
    வெறுமை replaceConst(Ast_முனை_Quadop* nodep) {
        V3_எண் numv{nodep, nodep->widthMinV()};
        nodep->எண்_Operate(numv, const_இலக்கம்_V(nodep->lhsp()), const_இலக்கம்_V(nodep->rhsp()),
                             const_இலக்கம்_V(nodep->thsp()), const_இலக்கம்_V(nodep->fhsp()));
        மாறிலி V3_எண்& இலக்கம் = to_இலக்கம்_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << இலக்கம்);
        VL_DO_DANGLING(replace_இலக்கம்(nodep, இலக்கம்), nodep);
    }

    வெறுமை replaceConstString(Ast_முனை* oldp, மாறிலி string& இலக்கம்) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_முனை* மாறிலி newp = புதிய AstConst{oldp->fileline(), AstConst::String{}, இலக்கம்};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    வெறுமை replaceWChild(Ast_முனை* nodep, Ast_முனை_கோவை* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை replaceWChildBool(Ast_முனை* nodep, Ast_முனை_கோவை* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        எனில் (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } இல்லையெனில் {
            nodep->replaceWithKeepDType(புதிய AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    வெறுமை replaceWIteratedRhs(Ast_முனை_Triop* nodep) {
        எனில் (Ast_முனை* மாறிலி rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    வெறுமை replaceWIteratedThs(Ast_முனை_Triop* nodep) {
        எனில் (Ast_முனை* மாறிலி thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    வெறுமை replaceWLhs(Ast_முனை_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    வெறுமை replaceWLhs(Ast_முனை_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    வெறுமை replaceWRhs(Ast_முனை_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    வெறுமை replaceWLhsBool(Ast_முனை_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    வெறுமை replaceWRhsBool(Ast_முனை_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    வெறுமை replaceAsv(Ast_முனை_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_முனை_கோவை* மாறிலி ap = nodep->lhsp();
        Ast_முனை_Biop* மாறிலி rp = VN_AS(nodep->rhsp(), முனை_Biop);
        Ast_முனை_கோவை* மாறிலி bp = rp->lhsp();
        Ast_முனை_கோவை* மாறிலி cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        எனில் (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    வெறுமை replaceAsvLUp(Ast_முனை_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_முனை_Biop* மாறிலி lp = VN_AS(nodep->lhsp()->unlinkFrBack(), முனை_Biop);
        Ast_முனை_கோவை* மாறிலி llp = lp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி lrp = lp->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    வெறுமை replaceAsvRUp(Ast_முனை_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_முனை_கோவை* மாறிலி lp = nodep->lhsp()->unlinkFrBack();
        Ast_முனை_Biop* மாறிலி rp = VN_AS(nodep->rhsp()->unlinkFrBack(), முனை_Biop);
        Ast_முனை_கோவை* மாறிலி rlp = rp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    வெறுமை replaceAndOr(Ast_முனை_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_முனை_Biop* மாறிலி lp = VN_AS(nodep->lhsp()->unlinkFrBack(), முனை_Biop);
        Ast_முனை_கோவை* மாறிலி llp = lp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி lrp = lp->rhsp()->unlinkFrBack();
        Ast_முனை_Biop* மாறிலி rp = VN_AS(nodep->rhsp()->unlinkFrBack(), முனை_Biop);
        Ast_முனை_கோவை* மாறிலி rlp = rp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        எனில் (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(தள்ளு_Deletep(rp), rp);
            VL_DO_DANGLING(தள்ளு_Deletep(rlp), rlp);
        } இல்லையெனில் எனில் (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(தள்ளு_Deletep(rp), rp);
            VL_DO_DANGLING(தள்ளு_Deletep(lrp), lrp);
        } இல்லையெனில் {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    வெறுமை replaceShiftSame(Ast_முனை_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_முனை_Biop* மாறிலி lp = VN_AS(nodep->lhsp()->unlinkFrBack(), முனை_Biop);
        Ast_முனை_கோவை* மாறிலி llp = lp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி lrp = lp->rhsp()->unlinkFrBack();
        Ast_முனை_Biop* மாறிலி rp = VN_AS(nodep->rhsp()->unlinkFrBack(), முனை_Biop);
        Ast_முனை_கோவை* மாறிலி rlp = rp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(தள்ளு_Deletep(rp), rp);
        VL_DO_DANGLING(தள்ளு_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    வெறுமை replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* மாறிலி lselp = VN_AS(nodep->lhsp()->unlinkFrBack(), Sel);
        AstSel* மாறிலி rselp = VN_AS(nodep->rhsp()->unlinkFrBack(), Sel);
        மாறிலி முழு lstart = lselp->lsbConst();
        மாறிலி முழு lwidth = lselp->widthConst();
        மாறிலி முழு rstart = rselp->lsbConst();
        மாறிலி முழு rwidth = rselp->widthConst();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* மாறிலி newselp = புதிய AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlinkFrBack(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(தள்ளு_Deletep(lselp), lselp);
        VL_DO_DANGLING(தள்ளு_Deletep(rselp), rselp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_முனை_Biop* மாறிலி lp = VN_AS(nodep->lhsp(), முனை_Biop);
        Ast_முனை_Biop* மாறிலி rp = VN_AS(nodep->rhsp(), முனை_Biop);
        எனில் (concatMergeable(lp, rp, 0)) {
            Ast_முனை_கோவை* மாறிலி llp = lp->lhsp();
            Ast_முனை_கோவை* மாறிலி lrp = lp->rhsp();
            Ast_முனை_கோவை* மாறிலி rlp = rp->lhsp();
            Ast_முனை_கோவை* மாறிலி rrp = rp->rhsp();
            AstConcat* மாறிலி newlp = புதிய AstConcat{rlp->fileline(), llp->clone_மரம்_Pure(பொய்),
                                                   rlp->clone_மரம்_Pure(பொய்)};
            AstConcat* மாறிலி newrp = புதிய AstConcat{rrp->fileline(), lrp->clone_மரம்_Pure(பொய்),
                                                   rrp->clone_மரம்_Pure(பொய்)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(தள்ளு_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(தள்ளு_Deletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(தள்ளு_Deletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } இல்லையெனில் {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    வெறுமை replaceExtend(Ast_முனை* nodep, Ast_முனை_கோவை* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_முனை_கோவை*>(புதிய AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_முனை_கோவை*>(புதிய AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை replacePowShift(Ast_முனை_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* மாறிலி newp
            = புதிய AstShiftL{nodep->fileline(), புதிய AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        மாறிலி முழு amount = VN_AS(nodep->lhsp(), Const)->இலக்கம்().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_முனை_கோவை* மாறிலி opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* மாறிலி newp
            = புதிய AstShiftL{nodep->fileline(), opp, புதிய AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        மாறிலி முழு amount = VN_AS(nodep->rhsp(), Const)->இலக்கம்().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_முனை_கோவை* மாறிலி opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* மாறிலி newp
            = புதிய AstShiftR{nodep->fileline(), opp, புதிய AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        மாறிலி முழு amount = VN_AS(nodep->rhsp(), Const)->இலக்கம்().mostSetBitP1() - 1;  // 2^n->n+1
        V3_எண் mask{nodep, nodep->width()};
        mask.setMask(amount);
        Ast_முனை_கோவை* மாறிலி opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* மாறிலி newp
            = புதிய AstAnd{nodep->fileline(), opp, புதிய AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை replaceShiftOp(Ast_முனை_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        மாறிலி முழு width = nodep->width();
        மாறிலி முழு widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        Ast_முனை_Biop* மாறிலி lhsp = VN_AS(nodep->lhsp(), முனை_Biop);
        lhsp->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி shiftp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி ap = lhsp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி bp = lhsp->rhsp()->unlinkFrBack();
        Ast_முனை_Biop* மாறிலி shift1p = nodep;
        Ast_முனை_Biop* மாறிலி shift2p = nodep->clone_மரம்(உண்மை);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_மரம்_Pure(உண்மை));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_முனை_Biop* மாறிலி newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    வெறுமை replaceShiftShift(Ast_முனை_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_முனை_Biop* மாறிலி lhsp = VN_AS(nodep->lhsp(), முனை_Biop);
        lhsp->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி ap = lhsp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி shift1p = lhsp->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        எனில் (nodep->type() == lhsp->type()) {
            மாறிலி முழு shift1 = VN_AS(shift1p, Const)->toUInt();
            மாறிலி முழு shift2 = VN_AS(shift2p, Const)->toUInt();
            மாறிலி முழு newshift = shift1 + shift2;
            VL_DO_DANGLING(தள்ளு_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(தள்ளு_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(புதிய AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } இல்லையெனில் {
            // We know shift amounts are constant, but might be a mixed left/right shift
            முழு shift1 = VN_AS(shift1p, Const)->toUInt();
            எனில் (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            முழு shift2 = VN_AS(shift2p, Const)->toUInt();
            எனில் (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            மாறிலி முழு newshift = shift1 + shift2;
            VL_DO_DANGLING(தள்ளு_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(தள்ளு_Deletep(shift2p), shift2p);
            Ast_முனை_கோவை* newp;
            V3_எண் mask1{nodep, nodep->width()};
            V3_எண் ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            எனில் (shift1 < 0) {
                mask1.opShiftR(ones, V3_எண்(nodep, VL_IDATASIZE, -shift1));
            } இல்லையெனில் {
                mask1.opShiftL(ones, V3_எண்(nodep, VL_IDATASIZE, shift1));
            }
            V3_எண் mask{nodep, nodep->width()};
            எனில் (shift2 < 0) {
                mask.opShiftR(mask1, V3_எண்(nodep, VL_IDATASIZE, -shift2));
            } இல்லையெனில் {
                mask.opShiftL(mask1, V3_எண்(nodep, VL_IDATASIZE, shift2));
            }
            எனில் (newshift < 0) {
                newp = புதிய AstShiftR{nodep->fileline(), ap,
                                     புதிய AstConst(nodep->fileline(), -newshift)};
            } இல்லையெனில் {
                newp = புதிய AstShiftL{nodep->fileline(), ap,
                                     புதிய AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = புதிய AstAnd{nodep->fileline(), newp, புதிய AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(தள்ளு_Deletep(lhsp), lhsp);
    }

    தருக்க replaceAssignMultiSel(Ast_முனை_Assign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        எனில் (!m_modp) திரும்பு பொய்;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* மாறிலி sel1p = VN_CAST(nodep->lhsp(), Sel);
        எனில் (!sel1p) திரும்பு பொய்;
        Ast_முனை_Assign* மாறிலி nextp = VN_CAST(nodep->nextp(), முனை_Assign);
        எனில் (!nextp) திரும்பு பொய்;
        எனில் (nodep->type() != nextp->type()) திரும்பு பொய்;
        மாறிலி AstSel* மாறிலி sel2p = VN_CAST(nextp->lhsp(), Sel);
        எனில் (!sel2p) திரும்பு பொய்;
        AstVarRef* மாறிலி varref1p = VN_CAST(sel1p->fromp(), VarRef);
        எனில் (!varref1p) திரும்பு பொய்;
        மாறிலி AstVarRef* மாறிலி varref2p = VN_CAST(sel2p->fromp(), VarRef);
        எனில் (!varref2p) திரும்பு பொய்;
        எனில் (!varref1p->same_Gate_மரம்(varref2p)) திரும்பு பொய்;
        மாறிலி AstConst* மாறிலி con1p = VN_CAST(sel1p->lsbp(), Const);
        எனில் (!con1p) திரும்பு பொய்;
        மாறிலி AstConst* மாறிலி con2p = VN_CAST(sel2p->lsbp(), Const);
        எனில் (!con2p) திரும்பு பொய்;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        எனில் (!varNotReferenced(nodep->rhsp(), varref1p->varp())) திரும்பு பொய்;
        எனில் (!varNotReferenced(nextp->rhsp(), varref2p->varp())) திரும்பு பொய்;
        // If a variable is marked split_var, access to the variable should not be merged.
        எனில் (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) திரும்பு பொய்;
        // Swap?
        எனில் ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            திரும்பு பொய்;
        }
        மாறிலி தருக்க lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_முனை_கோவை* மாறிலி rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhs2p = nextp->rhsp()->unlinkFrBack();
        Ast_முனை_Assign* newp;
        எனில் (lsbFirstAssign) {
            newp = nodep->cloneType(புதிய AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    புதிய AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } இல்லையெனில் {
            newp = nodep->cloneType(புதிய AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    புதிய AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        VL_DO_DANGLING(தள்ளு_Deletep(nextp->unlinkFrBack()), nextp);
        திரும்பு உண்மை;
    }

    தருக்க replaceAssignSFormatF(Ast_முனை_Assign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        எனில் (!VN_IS(nodep, Assign)) திரும்பு பொய்;
        AstVarRef* மாறிலி lhsp = VN_CAST(nodep->lhsp(), VarRef);
        எனில் (!lhsp) திரும்பு பொய்;
        AstSFormatF* மாறிலி fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        எனில் (!fmtp) திரும்பு பொய்;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(புதிய AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }

    தருக்க varNotReferenced(Ast_முனை* nodep, AstVar* varp, முழு level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        எனில் (!nodep) திரும்பு உண்மை;
        எனில் (level > 2) திரும்பு பொய்;
        எனில் (!nodep->isPure()) திரும்பு பொய்;  // For example a $fgetc can't be reordered
        எனில் (VN_IS(nodep, முனை_Var_Ref) && VN_AS(nodep, முனை_Var_Ref)->varp() == varp) திரும்பு பொய்;
        திரும்பு (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    தருக்க replace_முனை_Assign(Ast_முனை_Assign* nodep) {
        எனில் (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            எனில் (VN_IS(nodep, AssignW)) {
                எனில் (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                திரும்பு பொய்;  // Don't delete the assign, or V3Gate will freak out
            } இல்லையெனில் {
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு உண்மை;
            }
        } இல்லையெனில் எனில் (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            தருக்க need_temp = பொய்;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            மாறிலி தருக்க waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            மாறிலி தருக்க needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            எனில் (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<size_t> lhsVarIds;
                nodep->lhsp()->foreach([&](மாறிலி AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](மாறிலி AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    எனில் (lhsVarIds.மொத்தம்(m_ids(refp->varp()))) need_temp = உண்மை;
                });
            }
            எனில் (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* மாறிலி tempPurep = புதிய AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* மாறிலி tempPureRefp
                    = புதிய AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                Ast_முனை_Assign* மாறிலி asnp
                    = VN_IS(nodep, AssignDly)
                          ? புதிய AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(புதிய AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                எனில் (waits) தள்ளு_Deletep(nodep->timingControlp()->unlinkFrBack());
            } இல்லையெனில் எனில் (need_temp) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } இல்லையெனில் {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* மாறிலி delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            எனில் (delayp && !delayp->lhsp()->isPure()) {
                File_வரி* மாறிலி flp = nodep->fileline();
                Ast_முனை_கோவை* மாறிலி valuep = delayp->lhsp()->unlinkFrBack();
                AstVar* மாறிலி tempp = புதிய AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswapNames.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempp);
                nodep->addHereThisAsNext(
                    புதிய AstAssign{flp, புதிய AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(புதிய AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_முனை_கோவை* மாறிலி lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhs2p = rhsp->clone_மரம்_Pure(பொய்);
            // Calc widths
            மாறிலி முழு lsb2 = 0;
            மாறிலி முழு msb2 = lsb2 + lc2p->width() - 1;
            மாறிலி முழு lsb1 = msb2 + 1;
            மாறிலி முழு msb1 = lsb1 + lc1p->width() - 1;
            UASSERT_OBJ(msb1 == (conp->width() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* மாறிலி sel1p = புதிய AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* மாறிலி sel2p = புதிய AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_முனை_Assign* newp = nullptr;
            எனில் (!need_temp) {
                Ast_முனை_Assign* மாறிலி asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_முனை_Assign* மாறிலி asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_முனை::addNext(newp, asn1ap);
                newp = Ast_முனை::addNext(newp, asn2ap);
            } இல்லையெனில் {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* மாறிலி temp1p
                    = புதிய AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* மாறிலி temp2p
                    = புதிய AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->addStmtsp(temp1p);
                m_modp->addStmtsp(temp2p);
                Ast_முனை_Assign* மாறிலி asn1ap = nodep->cloneType(
                    புதிய AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_முனை_Assign* மாறிலி asn2ap = nodep->cloneType(
                    புதிய AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_முனை_Assign* மாறிலி asn1bp = nodep->cloneType(
                    lc1p, புதிய AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_முனை_Assign* மாறிலி asn2bp = nodep->cloneType(
                    lc2p, புதிய AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_முனை::addNext(newp, asn1ap);
                newp = Ast_முனை::addNext(newp, asn2ap);
                newp = Ast_முனை::addNext(newp, asn1bp);
                newp = Ast_முனை::addNext(newp, asn2bp);
            }
            எனில் (debug() >= 9 && newp) newp->dump_மரம்_And_Next(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(தள்ளு_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            திரும்பு உண்மை;
        } இல்லையெனில் எனில் (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* மாறிலி cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            Ast_முனை_கோவை* srcp = cvtp->fromp()->unlinkFrBack();
            எனில் (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } இல்லையெனில் எனில் (Ast_முனை_Stream* மாறிலி streamp = VN_CAST(srcp, முனை_Stream)) {
                Ast_முனை_கோவை* மாறிலி streamSrcp = streamp->lhsp();
                எனில் (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    Ast_முனை_கோவை* மாறிலி packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(தள்ளு_Deletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            திரும்பு உண்மை;
        } இல்லையெனில் எனில் (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* மாறிலி streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            Ast_முனை_கோவை* srcp = streamp->lhsp()->unlinkFrBack();
            Ast_முனை_DType* மாறிலி srcDTypep = srcp->dtypep()->skipRefp();
            மாறிலி Ast_முனை_DType* மாறிலி dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            எனில் (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } இல்லையெனில் எனில் (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                எனில் (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    முழு srcElementBits = 0;
                    எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    முழு dstElementBits = 0;
                    எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = புதிய AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), பொய், 1,
                        dstElementBits,   srcElementBits};
                } இல்லையெனில் {
                    srcp = புதிய AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } இல்லையெனில் எனில் (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = புதிய AstCvtArrayToPacked{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                மாறிலி முழு packedBits = nodep->lhsp()->widthMin();
                மாறிலி முழு unpackBits
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->widthMin();
                மாறிலி uint32_t offset = packedBits > unpackBits ? packedBits - unpackBits : 0;
                srcp = புதிய AstShiftL{srcp->fileline(), srcp,
                                     புதிய AstConst{srcp->fileline(), offset}, packedBits};
            }
            எனில் (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                மாறிலி முழு sWidth = srcp->width();
                மாறிலி முழு dWidth = nodep->lhsp()->width();
                எனில் (sWidth < dWidth) {
                    AstExtend* மாறிலி extendp = புதிய AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = புதிய AstShiftL{
                        srcp->fileline(), extendp,
                        புதிய AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(தள்ளு_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            திரும்பு உண்மை;
        } இல்லையெனில் எனில் (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_முனை_கோவை* streamp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            Ast_முனை_DType* மாறிலி dstDTypep = dstp->dtypep()->skipRefp();
            Ast_முனை_கோவை* srcp = nodep->rhsp()->unlinkFrBack();
            மாறிலி Ast_முனை_DType* மாறிலி srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            எனில் ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                முழு blockSize = 1;
                எனில் (மாறிலி AstConst* மாறிலி constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    எனில் (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                முழு srcElementBits = 0;
                எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                முழு dstElementBits = 0;
                எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(புதிய AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, உண்மை,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(தள்ளு_Deletep(streamp), streamp);
                திரும்பு உண்மை;
            }
            எனில் (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            மாறிலி முழு sWidth = srcp->width();
            மாறிலி முழு dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            எனில் (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } இல்லையெனில் {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            எனில் (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = புதிய AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } இல்லையெனில் {
                எனில் (dWidth == 0) {
                    streamp = புதிய AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } இல்லையெனில் எனில் (sWidth >= dWidth) {
                    streamp = புதிய AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            திரும்பு உண்மை;
        } இல்லையெனில் எனில் (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_முனை_கோவை* மாறிலி streamp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            Ast_முனை_DType* மாறிலி dstDTypep = dstp->dtypep()->skipRefp();
            Ast_முனை_கோவை* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            எனில் (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_முனை_கோவை* origSrcp = srcp;
                எனில் (AstCvtArrayToPacked* மாறிலி cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                மாறிலி Ast_முனை_DType* மாறிலி origSrcDTypep = origSrcp->dtypep()->skipRefp();
                எனில் (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    முழு srcElementBits = 0;
                    எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    முழு dstElementBits = 0;
                    எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    எனில் (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(தள்ளு_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    தருக்க reverse = பொய்;
                    எனில் (மாறிலி AstUnpackArrayDType* மாறிலி unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(புதிய AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(தள்ளு_Deletep(streamp), streamp);
                    திரும்பு உண்மை;
                }
            }
            மாறிலி முழு sWidth = srcp->width();
            மாறிலி முழு dWidth = dstp->width();
            எனில் (VN_IS(dstDTypep, UnpackArrayDType)) {
                மாறிலி முழு dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                எனில் (sWidth > dstBitWidth) {
                    srcp
                        = புதிய AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = புதிய AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } இல்லையெனில் {
                எனில் (dWidth == 0) {
                    srcp = புதிய AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } இல்லையெனில் எனில் (sWidth >= dWidth) {
                    srcp = புதிய AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } இல்லையெனில் {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    எனில் (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* மாறிலி extendp = புதிய AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = புதிய AstShiftL{
                            srcp->fileline(), extendp,
                            புதிய AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } இல்லையெனில் {  // if it is a queue we dont need do do a shiftL
                        srcp = புதிய AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(தள்ளு_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            திரும்பு உண்மை;
        } இல்லையெனில் எனில் (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            Ast_முனை_கோவை* srcp = streamp->lhsp();
            மாறிலி Ast_முனை_DType* மாறிலி srcDTypep = srcp->dtypep()->skipRefp();
            Ast_முனை_DType* மாறிலி dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            எனில் (lowerAsFixedAggregate(srcDTypep)) {
                Ast_முனை_கோவை* மாறிலி packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } இல்லையெனில் எனில் ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                எனில் (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    முழு blockSize = 1;
                    எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        எனில் (VL_UNLIKELY(blockSize <= 0)) {
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
                    முழு srcElementBits = 0;
                    எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    முழு dstElementBits = 0;
                    எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = புதிய AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     உண்மை,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(தள்ளு_Deletep(streamp), streamp);
                } இல்லையெனில் {
                    streamp->lhsp(புதிய AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } இல்லையெனில் எனில் (m_doV && replaceAssignMultiSel(nodep)) {
            திரும்பு உண்மை;
        } இல்லையெனில் எனில் (replaceAssignSFormatF(nodep)) {
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    // Boolean replacements
    தருக்க operandBoolShift(மாறிலி Ast_முனை* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        எனில் (!VN_IS(nodep, And)) திரும்பு பொய்;
        எனில் (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) திரும்பு பொய்;
        எனில் (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) திரும்பு பொய்;
        மாறிலி AstShiftR* மாறிலி shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        எனில் (!VN_IS(shiftp->rhsp(), Const)) திரும்பு பொய்;
        எனில் (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }
    வெறுமை replaceBoolShift(Ast_முனை* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* மாறிலி andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_முனை_கோவை* மாறிலி fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* மாறிலி shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3_எண் val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->இலக்கம்(), shiftConstp->இலக்கம்());
        AstAnd* மாறிலி newp
            = புதிய AstAnd{nodep->fileline(), புதிய AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    வெறுமை replaceWithSimulation(Ast_முனை* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        எனில் (!simvis.optimizable()) {
            மாறிலி Ast_முனை* errorp = simvis.whyNotNodep();
            எனில் (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } இல்லையெனில் {
            // Fetch the result
            Ast_முனை* மாறிலி valuep = simvis.fetch_மதிப்பு_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_முனை* மாறிலி newp = valuep->clone_மரம்(பொய்);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = பொய்;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        எனில் (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = பொய்;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    வெறுமை swapSides(Ast_முனை_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    தருக்க containsMemberAccessRecurse(Ast_முனை* மாறிலி nodep) {
        எனில் (!nodep) திரும்பு பொய்;
        மாறிலி size_t id = m_ids(nodep);
        மாறிலி auto it = m_containsMemberAccess.find(id);
        எனில் (it != m_containsMemberAccess.end()) திரும்பு it->second;
        ++m_statMemberAccessVisits;
        தருக்க result = பொய்;
        எனில் (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = உண்மை;
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_FTask_Ref* மாறிலி funcRefp = VN_CAST(nodep, முனை_FTask_Ref)) {
            எனில் (containsMemberAccessRecurse(funcRefp->taskp())) result = உண்மை;
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_CCall* மாறிலி funcRefp = VN_CAST(nodep, முனை_CCall)) {
            எனில் (containsMemberAccessRecurse(funcRefp->funcp())) result = உண்மை;
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_FTask* மாறிலி funcp = VN_CAST(nodep, முனை_FTask)) {
            // Assume that it has a member access
            எனில் (funcp->recursive()) result = உண்மை;
        } இல்லையெனில் எனில் (மாறிலி AstCFunc* மாறிலி funcp = VN_CAST(nodep, CFunc)) {
            எனில் (funcp->recursive()) result = உண்மை;
        }
        எனில் (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        எனில் (!result && !VN_IS(nodep, முனை_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = உண்மை;
        }
        m_containsMemberAccess.emplace(id, result);
        திரும்பு result;
    }

    தருக்க matchBiopToBitwise(Ast_முனை_Biop* மாறிலி nodep) {
        எனில் (!m_convertLogicToBit) திரும்பு பொய்;
        எனில் (!nodep->lhsp()->width1()) திரும்பு பொய்;
        எனில் (!nodep->rhsp()->width1()) திரும்பு பொய்;
        எனில் (!nodep->isPure()) திரும்பு பொய்;
        எனில் (containsMemberAccessRecurse(nodep)) திரும்பு பொய்;
        திரும்பு உண்மை;
    }
    தருக்க matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* மாறிலி aRandp = VN_CAST(nodep->lhsp(), Rand);
        மாறிலி AstRand* மாறிலி bRandp = VN_CAST(nodep->rhsp(), Rand);
        எனில் (!aRandp || !bRandp) திரும்பு பொய்;
        எனில் (!aRandp->combinable(bRandp)) திரும்பு பொய்;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }
    தருக்க matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* மாறிலி aRandp = VN_CAST(nodep->fromp(), Rand);
        எனில் (!aRandp) திரும்பு பொய்;
        எனில் (aRandp->seedp()) திரும்பு பொய்;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }
    தருக்க matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        எனில் (மாறிலி AstConsArray* மாறிலி consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            எனில் (!(m_doExpensive || m_params)) திரும்பு பொய்;
            எனில் (!consp->is_Constant_மரம்()) திரும்பு பொய்;
        } இல்லையெனில் எனில் (!VN_IS(nodep->lhsp(), Const)) {
            திரும்பு பொய்;
        }
        replaceWithSimulation(nodep);
        திரும்பு உண்மை;
    }
    முழு operandConcatMove(மாறிலி AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        மாறிலி AstConcat* மாறிலி abConcp = VN_CAST(nodep->lhsp(), Concat);
        மாறிலி AstConcat* மாறிலி bcConcp = VN_CAST(nodep->rhsp(), Concat);
        எனில் (!abConcp && !bcConcp) திரும்பு 0;
        எனில் (bcConcp) {
            மாறிலி Ast_முனை_கோவை* மாறிலி ap = nodep->lhsp();
            மாறிலி Ast_முனை_கோவை* மாறிலி bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            எனில் (VL_BITBIT_I(ap->width() + bp->width()) == 0) திரும்பு 2;  // Transform 2: to abConc
        } இல்லையெனில் {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            எனில் (VL_BITBIT_I(nodep->lhsp()->width()) != 0) திரும்பு 1;  // Transform 1: to bcConc
        }
        திரும்பு 0;  // ok
    }
    வெறுமை moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        எனில் (operandConcatMove(nodep) > 1) {
            Ast_முனை_கோவை* மாறிலி ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* மாறிலி bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி bp = bcConcp->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* மாறிலி abConcp = புதிய AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            எனில் (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(தள்ளு_Deletep(bcConcp), bcConcp);
        } இல்லையெனில் {
            AstConcat* மாறிலி abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி ap = abConcp->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி bp = abConcp->rhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* மாறிலி bcConcp = புதிய AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            எனில் (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(தள்ளு_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    வெறுமை visit(AstConst*) override {}  // Already constant

    வெறுமை visit(AstCell* nodep) override {
        எனில் (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    வெறுமை visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        எனில் (AstConst* மாறிலி constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_இலக்கம்(nodep, AstMatchMasked::fold(constp->இலக்கம்(), nodep->matchp()->varp()));
        }
    }

    வெறுமை visit(AstPin* nodep) override { iterateChildren(nodep); }

    வெறுமை replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* மாறிலி newp = புதிய AstLogAnd{
            nodep->fileline(),
            புதிய AstLogOr{nodep->fileline(), புதிய AstLogNot{nodep->fileline(), lhsp}, rhsp},
            புதிய AstLogOr{nodep->fileline(),
                         புதிய AstLogNot{nodep->fileline(), rhsp->clone_மரம்_Pure(பொய்)},
                         lhsp->clone_மரம்_Pure(பொய்)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* மாறிலி belowp = VN_AS(nodep->fromp(), Sel);
        Ast_முனை_கோவை* மாறிலி fromp = belowp->fromp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி lsb1p = nodep->lsbp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_முனை_கோவை* newlsbp;
        எனில் (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = புதிய AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(தள்ளு_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(தள்ளு_Deletep(lsb2p), lsb2p);
        } இல்லையெனில் {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            மாறிலி Ast_முனை_கோவை* மாறிலி widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            Ast_முனை_கோவை* மாறிலி lhsp = widep->width() > lsb2p->width()
                                          ? புதிய AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_முனை_கோவை* மாறிலி rhsp = widep->width() > lsb1p->width()
                                          ? புதிய AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = புதிய AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* மாறிலி newp = புதிய AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* மாறிலி conp = VN_AS(nodep->fromp(), Concat);
        Ast_முனை_கோவை* மாறிலி conLhsp = conp->lhsp();
        Ast_முனை_கோவை* மாறிலி conRhsp = conp->rhsp();
        எனில் (static_cast<முழு>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* மாறிலி newp
                = புதிய AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } இல்லையெனில் எனில் (static_cast<முழு>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* மாறிலி newp
                = புதிய AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } இல்லையெனில் {
            // Yuk, split between the two
            conRhsp->unlinkFrBack();
            conLhsp->unlinkFrBack();
            AstConcat* மாறிலி newp
                = புதிய AstConcat{nodep->fileline(),
                                புதிய AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->width() + 1},
                                புதிய AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->width() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    தருக்க operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* மாறிலி repp = VN_AS(nodep->fromp(), Replicate);
        Ast_முனை_கோவை* மாறிலி fromp = repp->srcp();
        மாறிலி AstConst* மாறிலி lsbp = VN_CAST(nodep->lsbp(), Const);
        எனில் (!lsbp) திரும்பு பொய்;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        எனில் ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            திரும்பு பொய்;
        }
        //
        fromp->unlinkFrBack();
        AstSel* மாறிலி newp = புதிய AstSel{
            nodep->fileline(), fromp,
            புதிய AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }
    தருக்க operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* மாறிலி rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_முனை_கோவை* மாறிலி from2p = rep2p->srcp();
        மாறிலி AstConst* மாறிலி cnt1p = VN_CAST(nodep->countp(), Const);
        எனில் (!cnt1p) திரும்பு பொய்;
        மாறிலி AstConst* மாறிலி cnt2p = VN_CAST(rep2p->countp(), Const);
        எனில் (!cnt2p) திரும்பு பொய்;
        //
        from2p->unlinkFrBack();
        AstReplicate* மாறிலி newp
            = புதிய AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }
    தருக்க operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_முனை_கோவை* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_முனை_கோவை* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        எனில் (VN_IS(from1p, Replicate)) {
            மாறிலி AstConst* மாறிலி cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            எனில் (!cnt1p) திரும்பு பொய்;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        எனில் (VN_IS(from2p, Replicate)) {
            மாறிலி AstConst* மாறிலி cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            எனில் (!cnt2p) திரும்பு பொய்;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        எனில் (!operandsSame(from1p, from2p)) திரும்பு பொய்;
        //
        from1p->unlinkFrBack();
        AstReplicate* மாறிலி newp = புதிய AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }
    வெறுமை replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_முனை_Biop* மாறிலி fromp = VN_AS(nodep->fromp()->unlinkFrBack(), முனை_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_முனை_கோவை* மாறிலி lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_முனை_கோவை* மாறிலி bilhsp = fromp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            புதிய AstSel{nodep->fileline(), bilhsp, lsbp->clone_மரம்_Pure(உண்மை), nodep->widthConst()});
        fromp->rhsp(புதிய AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_முனை_Uniop* மாறிலி fromp = VN_AS(nodep->fromp()->unlinkFrBack(), முனை_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_முனை_கோவை* மாறிலி lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_முனை_கோவை* மாறிலி bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(புதிய AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        எனில் (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        எனில் (VN_IS(nodep->fromp(), Const)) {  // It did.
            எனில் (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } இல்லையெனில் {
                Ast_முனை* மாறிலி fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        இல்லையெனில் எனில் (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            மாறிலி AstConsArray* மாறிலி consp = VN_AS(nodep->fromp(), ConsArray);
            மாறிலி uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            Ast_முனை_கோவை* மாறிலி itemp = consp->valuep(bit);
            எனில் (VN_IS(itemp, Const)) {
                மாறிலி V3_எண்& இலக்கம் = VN_AS(itemp, Const)->இலக்கம்();
                VL_DO_DANGLING(replace_இலக்கம்(nodep, இலக்கம்), nodep);
            } இல்லையெனில் எனில் (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_மரம்(பொய்));
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        }
        m_selp = nullptr;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    வெறுமை visit(AstSliceSel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        எனில் (!m_required) திரும்பு;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    வெறுமை visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        எனில் (மாறிலி AstConsStruct* மாறிலி consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_முனை_கோவை* மாறிலி valuep = consp->memberValuep(nodep->name());
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_இலக்கம்(nodep, constp->இலக்கம்()), nodep);
            } இல்லையெனில் {
                nodep->replaceWith(valuep->clone_மரம்(பொய்));
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        }
    }

    வெறுமை visit(AstCAwait* nodep) override {
        m_hasJumpDelay = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        தருக்க did = பொய்;
        எனில் (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_முனை* மாறிலி valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            மாறிலி AstConsArray* மாறிலி consp = VN_CAST(valuep, ConsArray);
            மாறிலி Ast_முனை* மாறிலி selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            எனில் (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                எனில் (operandConst(valuep)) {
                    மாறிலி V3_எண்& இலக்கம் = VN_AS(valuep, Const)->இலக்கம்();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    மாறிலி string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_இலக்கம்(nodep, இலக்கம், origParamName), nodep);
                    did = உண்மை;
                } இல்லையெனில் எனில் (மாறிலி AstConst* மாறிலி itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_இலக்கம்(nodep, itemp->இலக்கம்()), nodep);
                    did = உண்மை;
                } இல்லையெனில் எனில் (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_முனை* மாறிலி newp = valuep->clone_மரம்(பொய்);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    did = உண்மை;
                } இல்லையெனில் எனில் (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_முனை* மாறிலி newp = valuep->clone_மரம்(பொய்);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    did = உண்மை;
                } இல்லையெனில் எனில் (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_முனை* மாறிலி newp = valuep->clone_மரம்(பொய்);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    did = உண்மை;
                }
            }
        }
        எனில் (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        iterateChildren(nodep);
        எனில் (!Ast_முனை::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    வெறுமை visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        தருக்க did = பொய்;
        எனில் (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            மாறிலி AstEnumItem* மாறிலி itemp = nodep->itemp();
            எனில் (m_recursing_Enum_பொருட்கள்.மொத்தம்(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } இல்லையெனில் {
                m_recursing_Enum_பொருட்கள்.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursing_Enum_பொருட்கள்.erase(itemp);
            }
            எனில் (AstConst* மாறிலி valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                மாறிலி V3_எண்& இலக்கம் = valuep->இலக்கம்();
                VL_DO_DANGLING(replace_இலக்கம்(nodep, இலக்கம்), nodep);
                did = உண்மை;
            }
        }
        எனில் (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    தருக்க only_Sen_Item_In_Sen_மரம்(மாறிலி AstSenItem* nodep) {
        // Only one if it's not in a list
        திரும்பு (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    வெறுமை visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        எனில் (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            எனில் (nodep->isClocked()) {  // A constant can never get a pos/negedge
                எனில் (only_Sen_Item_In_Sen_மரம்(nodep)) {
                    எனில் (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            புதிய AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } இல்லையெனில் {
                        nodep->replaceWith(புதிய AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                } இல்லையெனில் {
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } இல்லையெனில் {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(புதிய AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        } இல்லையெனில் எனில் (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_முனை* மாறிலி sensp = nodep->sensp();
            Ast_முனை* lastSensp = sensp;
            தருக்க invert = பொய்;
            போது (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            எனில் (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(sensp), sensp);
        }
    }

    வகுப்பு SenItemCmp final {
        நிலையான முழு cmp(மாறிலி Ast_முனை_கோவை* ap, மாறிலி Ast_முனை_கோவை* bp) {
            மாறிலி VNType aType = ap->type();
            மாறிலி VNType bType = bp->type();
            எனில் (aType != bType) திரும்பு static_cast<முழு>(bType) - static_cast<முழு>(aType);

            எனில் (மாறிலி AstVarRef* மாறிலி aRefp = VN_CAST(ap, VarRef)) {
                மாறிலி AstVarRef* மாறிலி bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                எனில் (aRefp->name() < bRefp->name()) திரும்பு -1;
                எனில் (aRefp->name() > bRefp->name()) திரும்பு 1;
                // But might be same name with different scopes
                எனில் (aRefp->varScopep() < bRefp->varScopep()) திரும்பு -1;
                எனில் (aRefp->varScopep() > bRefp->varScopep()) திரும்பு 1;
                // Or rarely, different data types
                எனில் (aRefp->dtypep() < bRefp->dtypep()) திரும்பு -1;
                எனில் (aRefp->dtypep() > bRefp->dtypep()) திரும்பு 1;
                திரும்பு 0;
            }

            எனில் (மாறிலி AstConst* மாறிலி aConstp = VN_CAST(ap, Const)) {
                மாறிலி AstConst* மாறிலி bConstp = VN_AS(bp, Const);
                எனில் (aConstp->இலக்கம்().isLtXZ(bConstp->இலக்கம்())) திரும்பு -1;
                எனில் (bConstp->இலக்கம்().isLtXZ(aConstp->இலக்கம்())) திரும்பு 1;
                திரும்பு 0;
            }

            எனில் (மாறிலி Ast_முனை_Biop* மாறிலி aBiOpp = VN_CAST(ap, முனை_Biop)) {
                மாறிலி Ast_முனை_Biop* மாறிலி bBiOpp = VN_AS(bp, முனை_Biop);
                // Compare RHSs first, as LHS might be const
                எனில் (மாறிலி முழு c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) திரும்பு c;
                திரும்பு cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            எனில் (மாறிலி AstCMethodHard* மாறிலி aCallp = VN_CAST(ap, CMethodHard)) {
                மாறிலி AstCMethodHard* மாறிலி bCallp = VN_AS(bp, CMethodHard);
                எனில் (aCallp->method() < bCallp->method()) திரும்பு -1;
                எனில் (aCallp->method() > bCallp->method()) திரும்பு 1;
                எனில் (மாறிலி முழு c = cmp(aCallp->fromp(), bCallp->fromp())) திரும்பு c;
                மாறிலி Ast_முனை_கோவை* aPinsp = aCallp->pinsp();
                மாறிலி Ast_முனை_கோவை* bPinsp = bCallp->pinsp();
                போது (aPinsp && bPinsp) {
                    எனில் (மாறிலி முழு c = cmp(aPinsp, bPinsp)) திரும்பு c;
                    aPinsp = VN_AS(aPinsp->nextp(), முனை_கோவை);
                    bPinsp = VN_AS(bPinsp->nextp(), முனை_கோவை);
                }
                திரும்பு aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            திரும்பு 0;
        }

    பொது:
        தருக்க operator()(மாறிலி AstSenItem* lhsp, மாறிலி AstSenItem* rhsp) மாறிலி {
            மாறிலி Ast_முனை_கோவை* மாறிலி lSensp = lhsp->sensp();
            மாறிலி Ast_முனை_கோவை* மாறிலி rSensp = rhsp->sensp();
            எனில் (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                எனில் (மாறிலி முழு c = cmp(lSensp, rSensp)) திரும்பு c < 0;
            } இல்லையெனில் எனில் (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                திரும்பு rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            திரும்பு lhsp->edgeType() < rhsp->edgeType();
        }
    };

    வெறுமை visit(Ast_Sen_மரம்* nodep) override {
        iterateChildren(nodep);
        எனில் (m_doExpensive) {
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
            க்கு (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                மாறிலி SenItemCmp cmp;
                எனில் (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    க்கு (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.தள்ளு_back(senp);
                    }
                    stable_sort(vec.begin(), vec.end(), SenItemCmp());
                    க்கு (மாறிலி auto& ip : vec) ip->unlinkFrBack();
                    க்கு (மாறிலி auto& ip : vec) nodep->addSensesp(ip);
                    நிறுத்து;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            க்கு (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                எனில் (!nextp) நிறுத்து;
                AstSenItem* மாறிலி lItemp = senp;
                AstSenItem* மாறிலி rItemp = nextp;
                மாறிலி Ast_முனை_கோவை* மாறிலி lSenp = lItemp->sensp();
                மாறிலி Ast_முனை_கோவை* மாறிலி rSenp = rItemp->sensp();
                எனில் (!lSenp || !rSenp) தொடர்;

                எனில் (lSenp->same_Gate_மரம்(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    எனில் (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    எனில் (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(தள்ளு_Deletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->timingControlp()) m_hasJumpDelay = உண்மை;
        எனில் (m_doNConst && replace_முனை_Assign(nodep)) திரும்பு;
    }
    வெறுமை visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    வெறுமை visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    வெறுமை visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        எனில் (m_doNConst && replace_முனை_Assign(nodep)) திரும்பு;
        // Process containing this AssignW as single body statement
        AstAlways* மாறிலி procp = VN_CAST(nodep->backp(), Always);
        எனில் (!procp || procp->stmtsp() != nodep || nodep->nextp()) திரும்பு;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(nodep->lhsp(), VarRef);
        எனில் (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->இலக்கம்().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_முனை_கோவை* மாறிலி exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            File_வரி* மாறிலி flp = nodep->fileline();
            AstInitial* மாறிலி newinitp = புதிய AstInitial{flp, புதிய AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(தள்ளு_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_முனை* மாறிலி initvaluep = exprp->clone_மரம்(பொய்);
            varrefp->varp()->isConst(உண்மை);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    வெறுமை visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        எனில் (!m_doNConst) திரும்பு;
        மாறிலி AstBasicDType* மாறிலி bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        எனில் (!bdtypep) திரும்பு;
        எனில் (!bdtypep->isZeroInit()) திரும்பு;
        AstConst* மாறிலி newp = புதிய AstConst{nodep->fileline(), V3_எண்{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* மாறிலி fromp = VN_CAST(nodep->fromp(), ConsArray);
        எனில் (!fromp) திரும்பு;
        // Fold the outermost dimension, converting the elements for any dimension under it
        மாறிலி AstUnpackArrayDType* மாறிலி toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        மாறிலி AstUnpackArrayDType* மாறிலி fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        மாறிலி தருக்க reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        மாறிலி uint64_t lastIndex = toArrayp->elementsConst() - 1;
        Ast_முனை_DType* மாறிலி subDTypep = toArrayp->subDTypep();
        மாறிலி AstUnpackArrayDType* மாறிலி subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        மாறிலி auto convert = [&](Ast_முனை_கோவை* valuep) -> Ast_முனை_கோவை* {
            valuep->unlinkFrBack();
            எனில் (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) திரும்பு valuep;
            திரும்பு புதிய AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* மாறிலி newp
            = புதிய AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : nullptr};
        க்கு (மாறிலி AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? lastIndex - itemp->index() : itemp->index(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    வெறுமை visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_முனை_Stream* streamp = nullptr;
        தருக்க isReverse = பொய்;
        எனில் (AstStreamL* மாறிலி streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = உண்மை;  // StreamL reverses the operation
        } இல்லையெனில் எனில் (AstStreamR* மாறிலி streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = பொய்;  // StreamR doesn't reverse the operation
        }
        எனில் (streamp) {
            Ast_முனை_கோவை* srcp = streamp->lhsp();
            மாறிலி Ast_முனை_DType* மாறிலி srcDTypep = srcp->dtypep()->skipRefp();
            Ast_முனை_DType* மாறிலி dstDTypep = nodep->dtypep()->skipRefp();
            எனில் (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                முழு blockSize = 1;
                எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    எனில் (VL_UNLIKELY(blockSize <= 0)) {
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
                முழு srcElementBits = 0;
                எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                முழு dstElementBits = 0;
                எனில் (மாறிலி Ast_முனை_DType* மாறிலி elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                Ast_முனை_கோவை* newp = புதிய AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(streamp), streamp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            }
        }
    }
    வெறுமை visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        எனில் (AstConcat* மாறிலி concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_வரி* மாறிலி flp = nodep->fileline();
            AstRelease* மாறிலி newLp = புதிய AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* மாறிலி newRp = புதிய AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    வெறுமை visit(Ast_முனை_If* nodep) override {
        iterateChildren(nodep);
        எனில் (m_doNConst) {
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->condp(), Const)) {
                Ast_முனை* keepp = nullptr;
                Ast_முனை* delp = nullptr;
                எனில் (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } இல்லையெனில் எனில் (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } இல்லையெனில் {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    திரும்பு;
                }
                எனில் (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    திரும்பு;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                எனில் (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } இல்லையெனில் {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_மரம்();
            } இல்லையெனில் எனில் (!Ast_முனை::afterCommentp(nodep->thensp())
                       && !Ast_முனை::afterCommentp(nodep->elsesp())) {
                எனில் (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, உண்மை);
                    Ast_முனை_Stmt* மாறிலி newp
                        = புதிய Ast_Stmt_கோவை{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                } இல்லையெனில் {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } இல்லையெனில் எனில் (!Ast_முனை::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_முனை_கோவை* மாறிலி condp = nodep->condp();
                Ast_முனை* மாறிலி elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                எனில் (nodep->thensp()) {  // Must have been comment
                    தள்ளு_Deletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(புதிய AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } இல்லையெனில் எனில் (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_முனை_கோவை* மாறிலி condp
                    = VN_AS(nodep->condp(), முனை_Uniop)->lhsp()->unlinkFrBackWithNext();
                Ast_முனை* மாறிலி thensp = nodep->thensp()->unlinkFrBackWithNext();
                Ast_முனை* மாறிலி elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* மாறிலி ifp = புதிய AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் எனில் (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_முனை_Assign* மாறிலி thensp = VN_AS(nodep->thensp(), முனை_Assign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_முனை_Assign* மாறிலி elsesp = VN_AS(nodep->elsesp(), முனை_Assign);
                thensp->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி condp = nodep->condp()->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி truep = thensp->rhsp()->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(புதிய AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் எனில் (பொய்  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_முனை_If* மாறிலி lowerIfp = VN_AS(nodep->thensp(), முனை_If);
                Ast_முனை_கோவை* மாறிலி condp = nodep->condp()->unlinkFrBack();
                Ast_முனை* மாறிலி lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                Ast_முனை_கோவை* மாறிலி lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(புதிய AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(தள்ளு_Deletep(lowerIfp), lowerIfp);
            } இல்லையெனில் {
                // Optimizations that don't reform the IF itself
                எனில் (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    வெறுமை visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        எனில் (stmtDisplayDisplay(nodep)) திரும்பு;
    }
    தருக்க stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        எனில் (!m_modp) திரும்பு பொய்;  // Don't optimize under single statement
        AstDisplay* மாறிலி prevp = VN_CAST(nodep->backp(), Display);
        எனில் (!prevp) திரும்பு பொய்;
        எனில் (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            திரும்பு பொய்;
        எனில் ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_மரம்(nodep->filep())))
            திரும்பு பொய்;
        எனில் (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            திரும்பு பொய்;
        AstSFormatF* மாறிலி pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* மாறிலி nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        எனில் (nformatp->scopeNamep() && pformatp->scopeNamep()) திரும்பு பொய்;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        எனில் (!m_doCpp && pformatp->exprsp()) திரும்பு பொய்;
        எனில் (!m_doCpp && nformatp->exprsp()) திரும்பு பொய்;
        எனில் (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) திரும்பு பொய்;
        எனில் (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) திரும்பு பொய்;
        // Avoid huge merges
        நிலையான constexpr முழு DISPLAY_MAX_MERGE_LENGTH = 500;
        எனில் (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            திரும்பு பொய்;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        எனில் (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        எனில் (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        எனில் (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        எனில் (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        எனில் (AstScopeName* மாறிலி scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        திரும்பு உண்மை;
    }
    வெறுமை visit(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterateAndNextNull(nodep->exprp());
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        எனில் (nodep->கோவை_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            எனில் (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->இலக்கம்().isFromString()) {
                AstConst* மாறிலி fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->இலக்கம்().toString());
                nodep->கோவை_Format_2(பொய்);
                VL_DO_DANGLING(தள்ளு_Deletep(fmtp), fmtp);
            }
        }
        எனில் (nodep->கோவை_Format_2()) திரும்பு;  // Runtime, unfortunately  (unless constify format later)
        தருக்க anyconst = பொய்;
        க்கு (Ast_முனை* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* மாறிலி fargp = VN_CAST(argp, SFormatArg);
            Ast_முனை* மாறிலி subargp = fargp ? fargp->exprp() : argp;
            எனில் (VN_IS(subargp, Const)) {
                anyconst = உண்மை;
                நிறுத்து;
            }
        }
        எனில் (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            தருக்க inPct = பொய்;
            Ast_முனை* argp = nodep->exprsp();
            மாறிலி string text = nodep->text();
            க்கு (எழுத்து ch : text) {
                எனில் (!inPct && ch == '%') {
                    inPct = உண்மை;
                    fmt = ch;
                } இல்லையெனில் எனில் (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } இல்லையெனில் எனில் (inPct) {
                    inPct = பொய்;
                    fmt += ch;
                    ch = std::tolower(ch);
                    தேர்வு (ch) {
                    நிலை '%': நிறுத்து;  // %% - still %%
                    நிலை 'm': நிறுத்து;  // %m - still %m - auto insert "name"
                    நிலை 'l': நிறுத்து;  // %l - still %l - auto insert "library"
                    நிலை 't':  // %t - don't know $timeformat so can't constify
                        எனில் (argp) argp = argp->nextp();
                        நிறுத்து;
                    இயல்பு:  // Most operators, just move to next argument
                        எனில் (argp) {
                            Ast_முனை* மாறிலி nextp = argp->nextp();
                            AstSFormatArg* மாறிலி fargp = VN_CAST(argp, SFormatArg);
                            Ast_முனை* மாறிலி subargp = fargp ? fargp->exprp() : argp;
                            மாறிலி VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            எனில் (VN_IS(subargp, Const)) {  // Convert it
                                மாறிலி string out
                                    = formatAttr.isEnum()
                                          ? const_இலக்கம்_V(subargp).displayedEnum(fargp, fmt)
                                          : const_இலக்கம்_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(தள்ளு_Deletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        நிறுத்து;
                    }  // switch
                    newFormat += fmt;
                } இல்லையெனில் {
                    newFormat += ch;
                }
            }
            எனில் (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        எனில் (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            திரும்பு;
        }
        எனில் (VN_IS(nodep->backp(), முனை_Assign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
    }
    வெறுமை visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        எனில் (!m_doNConst) திரும்பு;
        எனில் (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* மாறிலி fmtp = nodep->fmtp();
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp();
            AstBasicDType* மாறிலி basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            எனில் (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* மாறிலி strp
                    = புதிய AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(புதிய AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            }
        }
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        எனில் (nodep->recursive()) m_underRecFunc = உண்மை;
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = உண்மை;  // As don't analyze inside tasks for timing controls
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = உண்மை;  // As don't analyze inside tasks for timing controls
    }
    வெறுமை visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_முனை_FTask_Ref*>(nodep));
        எனில் (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    வெறுமை visit(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    வெறுமை visit(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        மாறிலி தருக்க oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = பொய்;
        m_hasLoopTest = பொய்;
        iterateChildren(nodep);
        மாறிலி தருக்க thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        எனில் (மாறிலி AstLoopTest* மாறிலி testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            எனில் (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
        }
        // If last statement always breaks, repalce loop with body
        எனில் (Ast_முனை* lastp = nodep->stmtsp()) {
            போது (Ast_முனை* மாறிலி nextp = lastp->nextp()) lastp = nextp;
            எனில் (AstLoopTest* மாறிலி testp = VN_CAST(lastp, LoopTest)) {
                எனில் (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(தள்ளு_Deletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
            }
        }
        // Warn on infinite loop
        எனில் (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, உண்மை);  // Complain once
        }
    }
    வெறுமை visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        எனில் (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }

        m_hasLoopTest = உண்மை;

        // If always breaks, subsequent statements are dead code, delete them
        எனில் (nodep->condp()->isZero()) {
            Ast_முனை* மாறிலி nextp = nodep->nextp();
            எனில் (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(தள்ளு_Deletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            திரும்பு;
        }

        எனில் (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    வெறுமை visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    வெறுமை visit(Ast_முனை_Pre_Sel*) override {}

    // Ignored, can eliminate early
    வெறுமை visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        எனில் (m_doNConst) VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        எனில் (!nodep->exprp()) {
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        // Remove if expression is trivially pure
        எனில் (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        // Remove calls to empty functions
        எனில் (மாறிலி AstCCall* மாறிலி callp = VN_CAST(nodep->exprp(), CCall)) {
            மாறிலி AstCFunc* மாறிலி funcp = callp->funcp();
            எனில் (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
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
    வெறுமை visit(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    வெறுமை visit(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        எனில் (nodep->nextp() && canDeleteStatements()) {
            தள்ளு_Deletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        எனில் (!nodep->nextp()) {
            க்கு (Ast_முனை* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                எனில் (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                    திரும்பு;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                எனில் (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) நிறுத்து;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = உண்மை;
    }

    வெறுமை visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        எனில் (AstLoopTest* மாறிலி testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        எனில் (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }

        // If no JumpGo points to this node, replace it with its body
        எனில் (!m_usedJumpBlocks.மொத்தம்(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    மரம்_SKIP_VISIT("ArraySel");
    மரம்_SKIP_VISIT("CAwait");

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
    வெறுமை visit(Ast_முனை* nodep) override {
        // Default: Just iterate
        எனில் (m_required) {
            எனில் (VN_IS(nodep, முனை_DType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } இல்லையெனில் எனில் (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } இல்லையெனில் எனில் (AstCellRef* மாறிலி crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                எனில் (Ast_முனை* மாறிலி newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(தள்ளு_Deletep(crp), crp);
                }
                திரும்பு;
            } இல்லையெனில் {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } இல்லையெனில் {
            எனில் (nodep->isTimingControl()) m_hasJumpDelay = உண்மை;
            // Calculate the width of this operation
            எனில் (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);
        }
    }

பொது:
    // Processing Mode Enum
    எண்ணிக்கை ProcMode : uint8_t {
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
    ConstVisitor(ProcMode pmode, தருக்க globalPass)
        : m_globalPass{globalPass}
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_global_Pass_இலக்கம்++)) : ""} {
        // clang-format off
        தேர்வு (pmode) {
        நிலை PROC_PARAMS_NOWARN:  m_doV = உண்மை;  m_doNConst = உண்மை; m_params = உண்மை;
                                  m_required = பொய்; நிறுத்து;
        நிலை PROC_PARAMS:         m_doV = உண்மை;  m_doNConst = உண்மை; m_params = உண்மை;
                                  m_required = உண்மை; நிறுத்து;
        நிலை PROC_GENERATE:       m_doV = உண்மை;  m_doNConst = உண்மை; m_params = உண்மை;
                                  m_required = உண்மை; m_doGenerate = உண்மை; நிறுத்து;
        நிலை PROC_LIVE:           நிறுத்து;
        நிலை PROC_V_WARN:         m_doV = உண்மை;  m_doNConst = உண்மை; m_warn = உண்மை; m_convertLogicToBit = உண்மை; நிறுத்து;
        நிலை PROC_V_NOWARN:       m_doV = உண்மை;  m_doNConst = உண்மை; நிறுத்து;
        நிலை PROC_V_EXPENSIVE:    m_doV = உண்மை;  m_doNConst = உண்மை; m_doExpensive = உண்மை; நிறுத்து;
        நிலை PROC_CPP:            m_doV = பொய்; m_doNConst = உண்மை; m_doCpp = உண்மை; நிறுத்து;
        இயல்பு:                  v3fatalSrc("Bad case"); நிறுத்து;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        எனில் (m_doCpp) {
            எனில் (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } இல்லையெனில் {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_stat_Cond_கோவை_Redundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_கோவை_Redundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_முனை* mainAcceptEdit(Ast_முனை* nodep) {
        VIsCached::clear_Cache_மரம்();  // Avoid using any stale isPure
        // Operate starting at a random place
        திரும்பு iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_இலக்கம் = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
வெறுமை V3Const::constifyParamsEdit(Ast_முனை* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ பொய்};
    // cppcheck-suppress constVariablePointer // edited below
    எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        எனில் (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } இல்லையெனில் {
        (வெறுமை)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
வெறுமை V3Const::constifyParamsNoWarnEdit(Ast_முனை* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ பொய்};
    // cppcheck-suppress constVariablePointer // edited below
    எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        எனில் (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } இல்லையெனில் {
        (வெறுமை)visitor.mainAcceptEdit(nodep);
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
வெறுமை V3Const::constifyGenerateParamsEdit(Ast_முனை* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ பொய்};
    // cppcheck-suppress constVariablePointer // edited below
    எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        எனில் (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } இல்லையெனில் {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

வெறுமை V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ உண்மை};
        (வெறுமை)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("const", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ உண்மை};
        (வெறுமை)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("const_cpp", 0, dump_மரம்_Either_Level() >= 3);
}

Ast_முனை* V3Const::constifyEdit(Ast_முனை* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ பொய்};
    nodep = visitor.mainAcceptEdit(nodep);
    திரும்பு nodep;
}

Ast_முனை* V3Const::constifyEditCpp(Ast_முனை* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ பொய்};
    nodep = visitor.mainAcceptEdit(nodep);
    திரும்பு nodep;
}

வெறுமை V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ உண்மை};
        (வெறுமை)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("const", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ உண்மை};
        (வெறுமை)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("const", 0, dump_மரம்_Either_Level() >= 3);
}

Ast_முனை* V3Const::constifyExpensiveEdit(Ast_முனை* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ பொய்};
    nodep = visitor.mainAcceptEdit(nodep);
    திரும்பு nodep;
}
