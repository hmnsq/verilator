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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Const.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Simulate.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3String.h"
#അന്തര്ഭാവ "V3UniqueNames.h"
#അന്തര്ഭാവ "V3Width.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <type_traits>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#define വൃക്ഷം_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

സ്ഥിതം തർക്കം isConst(സ്ഥിരം Ast_കണ്ണി* nodep, uint64_t v) {
    സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const);
    തിരികെ constp && constp->toUQuad() == v;
}

template <typename T>
സ്ഥിതം typename std::enable_if<std::is_integral<T>::മൂല്യം, തർക്കം>::type isPow2(T val) {
    തിരികെ (val & (val - 1)) == 0;
}

സ്ഥിതം പൂർണ്ണസംഖ്യ എണ്ണം_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#എങ്കിൽ defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    തിരികെ __builtin_ctzll(val);
#അല്ലെങ്കിൽ
    പൂർണ്ണസംഖ്യ bit = 0;
    val = ~val;
    ആകുമ്പോൾ (val & 1) {
        ++bit;
        val >>= 1;
    }
    തിരികെ bit;
#endif
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
വർഗ്ഗം VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4InUse> m_inuser4p;
    size_t m_nextId = 0;  // Id allocated most recently

പൊതു:
    // Return the unique id of the given node, allocating a new one if it has none yet
    size_t operator()(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        എങ്കിൽ (!nodep->user4p()) nodep->user4p(reinterpret_cast<ശൂന്യം*>(++m_nextId));
        തിരികെ reinterpret_cast<size_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
വർഗ്ഗം Const_Bit_Op_വൃക്ഷം_Visitor final : പൊതു VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<Ast_കണ്ണി_വ്യഞ്ജകം*, unsigned, തർക്കം>;

    വർഗ്ഗം LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        തർക്കം m_polarity = സത്യം;  // Invert result due to NOT
        പൂർണ്ണസംഖ്യ m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        പൂർണ്ണസംഖ്യ m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        പൂർണ്ണസംഖ്യ m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = nullptr;  // Leaf's variable reference
        സ്ഥിരം AstConst* m_constp = nullptr;  // Leaf's constant

    പൊതു:
        // CONSTRUCTORS
        LeafInfo() = സ്ഥിരസ്ഥിതി;
        LeafInfo(സ്ഥിരം LeafInfo& other) = സ്ഥിരസ്ഥിതി;
        LeafInfo& operator=(സ്ഥിരം LeafInfo& other) = സ്ഥിരസ്ഥിതി;
        LeafInfo(LeafInfo&& other) = സ്ഥിരസ്ഥിതി;
        LeafInfo& operator=(LeafInfo&& other) = സ്ഥിരസ്ഥിതി;
        explicit LeafInfo(പൂർണ്ണസംഖ്യ lsb)
            : m_lsb{lsb} {}

        // METHODS
        ശൂന്യം setLeaf(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->widthMin() - 1;
        }
        ശൂന്യം setLeaf(സ്ഥിരം AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->widthMin() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        ശൂന്യം updateBitRange(പൂർണ്ണസംഖ്യ newLsb, പൂർണ്ണസംഖ്യ newMsb) {
            എങ്കിൽ ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(സത്യം);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        ശൂന്യം updateBitRange(സ്ഥിരം AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->width() - 1));
        }
        ശൂന്യം updateBitRange(സ്ഥിരം AstShiftR* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->toUInt(), m_msb);
        }
        ശൂന്യം limitBitRangeToLsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        പൂർണ്ണസംഖ്യ wordIdx() സ്ഥിരം { തിരികെ m_wordIdx; }
        ശൂന്യം wordIdx(പൂർണ്ണസംഖ്യ i) { m_wordIdx = i; }
        തർക്കം polarity() സ്ഥിരം { തിരികെ m_polarity; }
        ശൂന്യം polarity(തർക്കം p) { m_polarity = p; }

        AstVarRef* refp() സ്ഥിരം { തിരികെ m_refp; }
        സ്ഥിരം AstConst* constp() സ്ഥിരം { തിരികെ m_constp; }
        തർക്കം missingWordSel() സ്ഥിരം {
            // When V3Expand is skipped, WordSel is not inserted.
            തിരികെ m_refp->isWide() && m_wordIdx == -1;
        }
        പൂർണ്ണസംഖ്യ lsb() സ്ഥിരം { തിരികെ m_lsb; }

        പൂർണ്ണസംഖ്യ msb() സ്ഥിരം { തിരികെ std::min(m_msb, varWidth() - 1); }
        പൂർണ്ണസംഖ്യ varWidth() സ്ഥിരം {
            UASSERT(m_refp, "m_refp should be set");
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = m_refp->varp()->widthMin();
            എങ്കിൽ (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                തിരികെ width;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (missingWordSel()) തിരികെ width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                സ്ഥിരം പൂർണ്ണസംഖ്യ bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                തിരികെ m_wordIdx == m_refp->width_വാക്കുകൾ() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    ഘടന BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        തർക്കം m_polarity = അസത്യം;  // Polarity the bit must have to match
        പൂർണ്ണസംഖ്യ m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(സ്ഥിരം LeafInfo& info, തർക്കം pol, പൂർണ്ണസംഖ്യ bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = സ്ഥിരസ്ഥിതി;
    };

    ഘടന Frozen_കണ്ണി_Info final {  // Context when a frozen node is found
        തർക്കം m_polarity;  // Polarity the frozen node must match
        പൂർണ്ണസംഖ്യ m_lsb;  // LSB position of the frozen node
        തർക്കം operator<(സ്ഥിരം Frozen_കണ്ണി_Info& other) സ്ഥിരം {
            എങ്കിൽ (m_lsb != other.m_lsb) തിരികെ m_lsb < other.m_lsb;
            തിരികെ m_polarity < other.m_polarity;
        }
    };

    വർഗ്ഗം Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_വൃക്ഷം_Visitor& m_visitor;  // Visitor whose state is saved and restored
        സ്ഥിരം size_t m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        സ്ഥിരം size_t m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        സ്ഥിരം unsigned m_ops;  // Saved m_visitor.m_ops to restore
        സ്ഥിരം തർക്കം m_polarity;  // Saved m_visitor.m_polarity to restore
        തർക്കം m_restore = സത്യം;  // Whether the destructor still needs to restore state

    പൊതു:
        explicit Restorer(Const_Bit_Op_വൃക്ഷം_Visitor& visitor)
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
            എങ്കിൽ (m_restore) restoreNow();
        }
        ശൂന്യം disableRestore() { m_restore = അസത്യം; }
        ശൂന്യം restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = അസത്യം;
        }
    };
    // Collect information for each Variable to transform as below
    വർഗ്ഗം VarInfo final {
        // MEMBERS
        പൂർണ്ണസംഖ്യ m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        സ്ഥിരം Const_Bit_Op_വൃക്ഷം_Visitor* സ്ഥിരം
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* സ്ഥിരം m_refp;  // Points the variable that this VarInfo covers
        സ്ഥിരം പൂർണ്ണസംഖ്യ m_width;  // Width of term this VarInfo refers to
        V3_സംഖ്യ m_bitPolarity;  // Coefficient of each bit

    പൊതു:
        // METHODS
        തർക്കം hasConstResult() സ്ഥിരം { തിരികെ m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        തർക്കം getConstResult() സ്ഥിരം {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            തിരികെ m_knownResult == 1;
        }
        സ്ഥിരം AstVarRef* refp() സ്ഥിരം { തിരികെ m_refp; }
        തർക്കം sameVarAs(സ്ഥിരം Ast_കണ്ണി_Var_Ref* otherp) സ്ഥിരം { തിരികെ m_refp->same_കണ്ണി(otherp); }
        ശൂന്യം setPolarity(തർക്കം compBit, പൂർണ്ണസംഖ്യ bit) {
            // Ignore if already determined a known reduction
            എങ്കിൽ (m_knownResult >= 0) തിരികെ;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            എങ്കിൽ (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } അല്ലെങ്കിൽ {  // The bit has already been marked with some polarity
                സ്ഥിരം തർക്കം sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                എങ്കിൽ (m_parentp->is_Xor_വൃക്ഷം()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } അല്ലെങ്കിൽ {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    എങ്കിൽ (sameFlag) തിരികെ;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->is_And_വൃക്ഷം() ? 0 : 1;
                    m_bitPolarity.setAllBitsX();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ResultTerm getResultTerm() സ്ഥിരം {
            UASSERT_OBJ(!hasConstResult(), m_refp, "getTerm on reduction that yields constant");
            File_വരി* സ്ഥിരം fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_കണ്ണി_വ്യഞ്ജകം* srcp = VN_CAST(m_refp->backp(), WordSel);
            എങ്കിൽ (!srcp) srcp = m_refp;
            srcp = srcp->clone_വൃക്ഷം(അസത്യം);

            // Signed variables might have redundant sign bits that need masking.
            സ്ഥിരം തർക്കം hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3_സംഖ്യ mask_അക്കം{srcp, m_width, 0};
            mask_അക്കം.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            സ്ഥിരം uint64_t maskVal = mask_അക്കം.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_കണ്ണി_വ്യഞ്ജകം* resultp = srcp;  // The tree for this term
            unsigned ops = 0;  // Number of ops in this term
            തർക്കം clean = അസത്യം;  // Whether the term is clean (has value 0 or 1)

            എങ്കിൽ (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                സ്ഥിരം പൂർണ്ണസംഖ്യ bit = എണ്ണം_Trailing_Zeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                എങ്കിൽ (bit != 0) {
                    resultp = പുതിയ AstShiftR{fl, resultp,
                                            പുതിയ AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                സ്ഥിരം തർക്കം negate = m_bitPolarity.bitIs0(bit);
                എങ്കിൽ (negate) {
                    resultp = പുതിയ AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } അല്ലെങ്കിൽ {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                സ്ഥിരം തർക്കം needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                എങ്കിൽ (needsMasking) {
                    resultp = പുതിയ AstAnd{fl, പുതിയ AstConst{fl, mask_അക്കം}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                എങ്കിൽ (m_parentp->is_Xor_വൃക്ഷം()) {
                    എങ്കിൽ (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        സ്ഥിരം പൂർണ്ണസംഖ്യ widthMin = mask_അക്കം.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = പുതിയ AstRedXor{fl, resultp};
                    ++ops;
                    clean = അസത്യം;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } അല്ലെങ്കിൽ എങ്കിൽ (m_parentp->is_And_വൃക്ഷം()) {
                    V3_സംഖ്യ comp_അക്കം{srcp, m_width, 0};
                    comp_അക്കം.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = പുതിയ AstEq{fl, പുതിയ AstConst{fl, comp_അക്കം}, resultp};
                    ++ops;
                    clean = സത്യം;
                } അല്ലെങ്കിൽ {  // Or
                    V3_സംഖ്യ comp_അക്കം{srcp, m_width, 0};
                    comp_അക്കം.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_അക്കം.opXor(V3_സംഖ്യ{comp_അക്കം}, mask_അക്കം);
                    resultp = പുതിയ AstNeq{fl, പുതിയ AstConst{fl, comp_അക്കം}, resultp};
                    ++ops;
                    clean = സത്യം;
                }
            }

            തിരികെ ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_വൃക്ഷം_Visitor* parent, AstVarRef* refp, പൂർണ്ണസംഖ്യ width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    തർക്കം m_failed = അസത്യം;
    തർക്കം m_polarity = സത്യം;  // Flip when AstNot comes
    unsigned m_ops;  // Number of operations such as And, Or, Xor, Sel...
    പൂർണ്ണസംഖ്യ m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം m_rootp;  // Root of this AST subtree

    std::vector<std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, Frozen_കണ്ണി_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<size_t, പൂർണ്ണസംഖ്യ> m_baseIdxs;

    // METHODS

    തർക്കം is_And_വൃക്ഷം() സ്ഥിരം { തിരികെ VN_IS(m_rootp, And); }
    തർക്കം is_Or_വൃക്ഷം() സ്ഥിരം { തിരികെ VN_IS(m_rootp, Or); }
    തർക്കം is_Xor_വൃക്ഷം() സ്ഥിരം { തിരികെ VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    എങ്കിൽ (setFailed(cond, #cond, nodep, __LINE__)) തിരികെ

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(സത്യം, reason, nodep, __LINE__)

    തർക്കം setFailed(തർക്കം fail, സ്ഥിരം അക്ഷരം* reason, സ്ഥിരം Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ വരി) {
        എങ്കിൽ (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << വരി << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = സത്യം;
        }
        തിരികെ m_failed;
    }
    ശൂന്യം incrOps(സ്ഥിരം Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ വരി) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << വരി);
    }
    VarInfo& getVarInfo(സ്ഥിരം LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_കണ്ണി* nodep = ref.refp()->varScopep();
        എങ്കിൽ (!nodep) nodep = ref.refp()->varp();  // Not scoped
        പൂർണ്ണസംഖ്യ& baseIdxr = m_baseIdxs[m_ids(nodep)];
        എങ്കിൽ (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            സ്ഥിരം പൂർണ്ണസംഖ്യ അക്കം_വാക്കുകൾ
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->width_വാക്കുകൾ() : 1;
            m_varInfos.resize(m_varInfos.size() + അക്കം_വാക്കുകൾ);
        }
        സ്ഥിരം size_t idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        എങ്കിൽ (!varInfop) {
            varInfop = പുതിയ VarInfo{ഇത്, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            എങ്കിൽ (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        തിരികെ *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(Ast_കണ്ണി* nodep, തർക്കം expectConst) {
        എങ്കിൽ (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) തിരികെ LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        തർക്കം ok = !m_failed;
        എങ്കിൽ (expectConst) {
            ok &= !info.refp() && info.constp();
        } അല്ലെങ്കിൽ {
            ok &= info.refp() && !info.constp();
        }
        തിരികെ ok ? info : LeafInfo{};
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    ശൂന്യം visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_leafp) m_leafp->updateBitRange(nodep);
    }
    ശൂന്യം visit(AstShiftR* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->toUInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->toUInt();
    }
    ശൂന്യം visit(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->widthMin() != 1, nodep);
        Ast_കണ്ണി* lhsp = nodep->lhsp();
        AstCCast* സ്ഥിരം castp = VN_CAST(lhsp, CCast);
        എങ്കിൽ (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_വൃക്ഷം() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        എങ്കിൽ (!is_Xor_വൃക്ഷം()) m_polarity = !m_polarity;
        എങ്കിൽ (m_leafp && castp) m_leafp->updateBitRange(castp);
        എങ്കിൽ (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    ശൂന്യം visit(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->toSInt());
        iterateConst(nodep->fromp());
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }
    ശൂന്യം visit(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }

    ശൂന്യം visit(AstRedXor* nodep) override {
        Restorer restorer{*ഇത്};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_കണ്ണി* lhsp = nodep->lhsp();
        സ്ഥിരം AstCCast* സ്ഥിരം castp = VN_CAST(lhsp, CCast);
        എങ്കിൽ (castp) lhsp = castp->lhsp();
        എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            സ്ഥിരം LeafInfo& mask = findLeaf(andp->lhsp(), സത്യം);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), അസത്യം);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            എങ്കിൽ (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            സ്ഥിരം V3_സംഖ്യ& mask_അക്കം = mask.constp()->അക്കം();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            സ്ഥിരം പൂർണ്ണസംഖ്യ maxBitIdx = std::min(ref.lsb() + mask_അക്കം.width(), ref.msb() + 1);
            വേണ്ടി (പൂർണ്ണസംഖ്യ bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ maskIdx = bitIdx - ref.lsb();
                എങ്കിൽ (mask_അക്കം.bitIs0(maskIdx)) തുടരുക;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, സത്യം, bitIdx);
            }
        } അല്ലെങ്കിൽ {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, അസത്യം);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            എങ്കിൽ (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            വേണ്ടി (പൂർണ്ണസംഖ്യ bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, സത്യം, bitIdx);
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_Biop* nodep) override {
        എങ്കിൽ (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*ഇത്};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            എങ്കിൽ (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_വൃക്ഷം(), nodep);
            incrOps(nodep, __LINE__);
            വേണ്ടി (സ്ഥിരം തർക്കം വലത് : {അസത്യം, സത്യം}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*ഇത്};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                Ast_കണ്ണി_വ്യഞ്ജകം* opp = വലത് ? nodep->rhsp() : nodep->lhsp();
                സ്ഥിരം തർക്കം origFailed = m_failed;
                iterateConst(opp);
                എങ്കിൽ (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    എങ്കിൽ (സ്ഥിരം AstCCast* സ്ഥിരം castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    സ്ഥിരം തർക്കം pol = is_Xor_വൃക്ഷം() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, Frozen_കണ്ണി_Info{pol, m_lsb});
                    m_failed = origFailed;
                    തുടരുക;
                }
                restorer.disableRestore();  // Now all checks passed
                എങ്കിൽ (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    എങ്കിൽ (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, is_Xor_വൃക്ഷം() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } അല്ലെങ്കിൽ എങ്കിൽ ((is_And_വൃക്ഷം() && leafInfo.polarity())
                               || (is_Or_വൃക്ഷം() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, സത്യം, 0);
                        m_bitPolarities.emplace_back(leafInfo, അസത്യം, 0);
                    }
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ ((is_And_വൃക്ഷം() && VN_IS(nodep, Eq)) || (is_Or_വൃക്ഷം() && VN_IS(nodep, Neq))) {
            Restorer restorer{*ഇത്};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            സ്ഥിരം Ast_കണ്ണി* lhsp = nodep->lhsp();
            എങ്കിൽ (സ്ഥിരം AstCCast* സ്ഥിരം castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            സ്ഥിരം V3_സംഖ്യ& comp_അക്കം = constp->അക്കം();

            auto setPolarities = [ഇത്, &comp_അക്കം](സ്ഥിരം LeafInfo& ref, സ്ഥിരം V3_സംഖ്യ* maskp) {
                സ്ഥിരം തർക്കം maskFlip = is_And_വൃക്ഷം() ^ ref.polarity();
                പൂർണ്ണസംഖ്യ constantWidth = comp_അക്കം.width();
                എങ്കിൽ (maskp) constantWidth = std::max(constantWidth, maskp->width());
                സ്ഥിരം പൂർണ്ണസംഖ്യ maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                വേണ്ടി (പൂർണ്ണസംഖ്യ bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ maskIdx = bitIdx - ref.lsb();
                    സ്ഥിരം തർക്കം mask0 = maskp && maskp->bitIs0(maskIdx);
                    സ്ഥിരം തർക്കം outOfRange = bitIdx > ref.msb();
                    എങ്കിൽ (mask0 || outOfRange) {  // RHS is 0
                        എങ്കിൽ (comp_അക്കം.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, സത്യം, 0);
                            m_bitPolarities.emplace_back(ref, അസത്യം, 0);
                            നിർത്തുക;
                        } അല്ലെങ്കിൽ {  // This bitIdx is irrelevant
                            തുടരുക;
                        }
                    }
                    സ്ഥിരം തർക്കം polarity = comp_അക്കം.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                സ്ഥിരം LeafInfo& mask = findLeaf(andp->lhsp(), സത്യം);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                സ്ഥിരം LeafInfo& ref = findLeaf(andp->rhsp(), അസത്യം);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                സ്ഥിരം V3_സംഖ്യ& mask_അക്കം = mask.constp()->അക്കം();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &mask_അക്കം);
            } അല്ലെങ്കിൽ {  // comp == v
                സ്ഥിരം LeafInfo& ref = findLeaf(nodep->rhsp(), അസത്യം);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } അല്ലെങ്കിൽ {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_വൃക്ഷം_Visitor(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, unsigned externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.തള്ളുക_back(nullptr);
        CONST_BITOP_RETURN_IF(!is_And_വൃക്ഷം() && !is_Or_വൃക്ഷം() && !is_Xor_വൃക്ഷം(), nodep);
        എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം biopp = VN_CAST(nodep, കണ്ണി_Biop)) {
            iterateConst(biopp);
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        വേണ്ടി (auto&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(is_Xor_വൃക്ഷം() || m_polarity, nodep, "must be the original polarity");
    }
    മായാ ~Const_Bit_Op_വൃക്ഷം_Visitor() = സ്ഥിരസ്ഥിതി;
#undef CONST_BITOP_RETURN_IF
#undef CONST_BITOP_SET_FAILED

പൊതു:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* simplify(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, പൂർണ്ണസംഖ്യ resultWidth, unsigned externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        സ്ഥിരം Const_Bit_Op_വൃക്ഷം_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        എങ്കിൽ (visitor.m_failed || visitor.m_varInfos.size() == 1) തിരികെ nullptr;

        // FileLine used for constructing all new nodes in this function
        File_വരി* സ്ഥിരം fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        unsigned resultOps = 0;
        തർക്കം hasCleanTerm = അസത്യം;
        തർക്കം hasDirtyTerm = അസത്യം;

        വേണ്ടി (auto&& v : visitor.m_varInfos) {
            എങ്കിൽ (!v) തുടരുക;  // Skip nullptr at m_varInfos[0]
            എങ്കിൽ (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_കണ്ണി_വ്യഞ്ജകം* resultp = nullptr;
                എങ്കിൽ (v->getConstResult()) {
                    UASSERT_OBJ(visitor.is_Or_വൃക്ഷം(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = പുതിയ AstConst{fl, AstConst::BitTrue{}};
                } അല്ലെങ്കിൽ എങ്കിൽ (visitor.is_And_വൃക്ഷം()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = പുതിയ AstConst{fl, AstConst::BitFalse{}};
                } അല്ലെങ്കിൽ {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    തുടരുക;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം termp : termps) VL_DO_DANGLING(termp->delete_വൃക്ഷം(), termp);
                തിരികെ resultp;
            }
            സ്ഥിരം ResultTerm result = v->getResultTerm();
            termps.തള്ളുക_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            എങ്കിൽ (std::get<2>(result)) {
                hasCleanTerm = സത്യം;
                UINFO(9, "Clean term: " << termps.back());
            } അല്ലെങ്കിൽ {
                hasDirtyTerm = സത്യം;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_കണ്ണി_Info, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>> frozenNodes;
        // Check if frozen terms are clean or not
        വേണ്ടി (സ്ഥിരം auto& frozenInfo : visitor.m_frozenNodes) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp = frozenInfo.first;
            // Comparison operators are clean
            എങ്കിൽ ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = സത്യം;
            } അല്ലെങ്കിൽ {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = സത്യം;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].തള്ളുക_back(termp);
        }

        // Figure out if a final negation is required
        സ്ഥിരം തർക്കം needsFlip = visitor.is_Xor_വൃക്ഷം() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        സ്ഥിരം തർക്കം needsCleaning = visitor.is_And_വൃക്ഷം() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        വേണ്ടി (സ്ഥിരം auto& lsbAndNodes : frozenNodes) {
            എങ്കിൽ (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            എങ്കിൽ (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        എങ്കിൽ (needsFlip) ++resultOps;
        // Add final cleaning AND
        എങ്കിൽ (needsCleaning) ++resultOps;

        എങ്കിൽ (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp : termps) termp->dump_വൃക്ഷം("-  Reduced term: ");
            വേണ്ടി (സ്ഥിരം std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, Frozen_കണ്ണി_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_വൃക്ഷം("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        എങ്കിൽ (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultp = needsFlip ? പുതിയ AstConst{fl, AstConst::BitTrue{}}
                                                   : പുതിയ AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            തിരികെ resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        എങ്കിൽ (visitor.m_ops <= resultOps) {
            വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം termp : termps) VL_DO_DANGLING(termp->delete_വൃക്ഷം(), termp);
            തിരികെ nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        സ്ഥിരം auto reduce = [&visitor, fl](Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            എങ്കിൽ (!lhsp) തിരികെ rhsp;
            എങ്കിൽ (visitor.is_And_വൃക്ഷം()) {
                തിരികെ പുതിയ AstAnd{fl, lhsp, rhsp};
            } അല്ലെങ്കിൽ എങ്കിൽ (visitor.is_Or_വൃക്ഷം()) {
                തിരികെ പുതിയ AstOr{fl, lhsp, rhsp};
            } അല്ലെങ്കിൽ {
                തിരികെ പുതിയ AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_കണ്ണി_വ്യഞ്ജകം* resultp = nullptr;
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        വേണ്ടി (auto&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_കണ്ണി_വ്യഞ്ജകം* termp = nullptr;
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            എങ്കിൽ (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_കണ്ണി_DType* സ്ഥിരം dtypep = termp->dtypep();
                termp = പുതിയ AstShiftR{termp->fileline(), termp,
                                      പുതിയ AstConst(termp->fileline(), AstConst::Widthed_മൂല്യം{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            എങ്കിൽ (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_കണ്ണി_DType* സ്ഥിരം dtypep = termp->dtypep();
                termp = പുതിയ AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        സ്ഥിരം പൂർണ്ണസംഖ്യ maskWidth
            = std::max(resultp->width(), resultWidth == 1 ? VL_IDATASIZE : resultWidth);

        // Apply final polarity flip
        എങ്കിൽ (needsFlip) {
            എങ്കിൽ (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = പുതിയ AstNot{fl, resultp};
            } അല്ലെങ്കിൽ {
                // Keep result clean by using XOR(1, _)
                AstConst* സ്ഥിരം maskp = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, maskWidth, 1};
                resultp = പുതിയ AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        എങ്കിൽ (needsCleaning) {
            AstConst* സ്ഥിരം maskp = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, maskWidth, 1};
            resultp = പുതിയ AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        എങ്കിൽ (resultp->width() != resultWidth) {
            resultp = പുതിയ AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        തിരികെ resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

വർഗ്ഗം ConstVisitor final : പൊതു VNVisitor {
    // CONSTANTS
    സ്ഥിതം constexpr unsigned CONCAT_MERGABLE_MAX_DEPTH = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ CONCAT_ASSIGN_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    തർക്കം m_params = അസത്യം;  // If true, propagate parameterized and true numbers only
    തർക്കം m_required = അസത്യം;  // If true, must become a constant
    തർക്കം m_wremove = സത്യം;  // Inside scope, no assignw removal
    തർക്കം m_warn = അസത്യം;  // Output warnings
    തർക്കം m_doExpensive = അസത്യം;  // Enable computationally expensive optimizations
    തർക്കം m_doCpp = അസത്യം;  // Enable late-stage C++ optimizations
    തർക്കം m_doNConst = അസത്യം;  // Enable non-constant-child simplifications
    തർക്കം m_doV = അസത്യം;  // Verilog, not C++ conversion
    തർക്കം m_doGenerate = അസത്യം;  // Postpone width checking inside generate
    തർക്കം m_convertLogicToBit = അസത്യം;  // Convert logical operators to bitwise
    തർക്കം m_hasJumpDelay = അസത്യം;  // JumpGo or Delay under this loop
    തർക്കം m_hasLoopTest = അസത്യം;  // Contains AstLoopTest
    തർക്കം m_underRecFunc = അസത്യം;  // Under a recursive function
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    സ്ഥിരം AstArraySel* m_selp = nullptr;  // Current select
    സ്ഥിരം AstScope* m_scopep = nullptr;  // Current scope
    സ്ഥിരം AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_വ്യഞ്ജകം_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_വ്യഞ്ജകം_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    സ്ഥിരം തർക്കം m_globalPass;  // ConstVisitor invoked as a global pass
    സ്ഥിതം uint32_t s_global_Pass_അക്കം;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<size_t, തർക്കം> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<സ്ഥിരം AstEnumItem*> m_recursing_Enum_ഇനങ്ങൾ;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    സ്ഥിതം തർക്കം canDeleteStatements() { തിരികെ v3Global.staticsLifted(); }

    ശൂന്യം deleteVarScopesUnder(Ast_കണ്ണി* subtreep) {
        എങ്കിൽ (!subtreep) തിരികെ;
        എങ്കിൽ (!m_scopep) തിരികെ;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.തിരുകുക(varp); });
        എങ്കിൽ (varps.empty()) തിരികെ;
        വേണ്ടി (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            എങ്കിൽ (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(തള്ളുക_Deletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3_സംഖ്യ const_അക്കം_V(Ast_കണ്ണി* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        സ്ഥിരം V3_സംഖ്യ& numc = VN_AS(nodep, Const)->അക്കം();
        തിരികെ !numc.is_സംഖ്യ() ? numc : V3_സംഖ്യ{nodep, nodep->widthMinV(), numc};
    }
    V3_സംഖ്യ to_അക്കം_C(Ast_കണ്ണി* nodep, സ്ഥിരം V3_സംഖ്യ& numv) {
        // Extend V width back to C width for given node
        തിരികെ !numv.is_സംഖ്യ() ? numv : V3_സംഖ്യ{nodep, nodep->width(), numv};
    }

    സ്ഥിതം തർക്കം lowerAsFixedAggregate(സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        തിരികെ dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp, സ്ഥിരം AstMemberDType* സ്ഥിരം itemp) {
        AstStructSel* സ്ഥിരം selp = പുതിയ AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        തിരികെ selp;
    }

    ശൂന്യം collectFixedAggregateTerms(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& termps,
                                    സ്ഥിരം തർക്കം packReal) {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = fromp->dtypep()->skipRefp();
        എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ idx : unpackDtypep->declRange().seq_ഇടത്_To_വലത്()) {
                AstArraySel* സ്ഥിരം selp
                    = പുതിയ AstArraySel{fromp->fileline(), fromp->clone_വൃക്ഷം_Pure(അസത്യം), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(fromp), fromp);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep
                   = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType)) {
            എങ്കിൽ (sdtypep->packed()) {
                termps.തള്ളുക_back(fromp);
                തിരികെ;
            }
            വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_വൃക്ഷം_Pure(അസത്യം), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(fromp), fromp);
        } അല്ലെങ്കിൽ എങ്കിൽ (packReal && dtypep->isDouble()) {
            termps.തള്ളുക_back(പുതിയ AstRealToBits{fromp->fileline(), fromp});
        } അല്ലെങ്കിൽ {
            termps.തള്ളുക_back(fromp);
        }
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* packFixedAggregate(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp) {
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> termps;
        collectFixedAggregateTerms(fromp, termps, സത്യം);
        UASSERT(!termps.empty(), "No stream terms");
        Ast_കണ്ണി_വ്യഞ്ജകം* resultp = termps[0];
        വേണ്ടി (size_t i = 1; i < termps.size(); ++i) {
            resultp = പുതിയ AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        തിരികെ resultp;
    }

    ശൂന്യം replaceAssignToFixedAggregate(Ast_കണ്ണി_Assign* സ്ഥിരം nodep, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dstp,
                                       Ast_കണ്ണി_വ്യഞ്ജകം* srcp) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ dstWidth = dstp->dtypep()->widthStream();
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> termps;
        collectFixedAggregateTerms(dstp, termps, അസത്യം);
        പൂർണ്ണസംഖ്യ srcWidth = srcp->width();
        എങ്കിൽ (srcWidth < dstWidth) {
            AstExtend* സ്ഥിരം extendp = പുതിയ AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = പുതിയ AstShiftL{
                extendp->fileline(), extendp,
                പുതിയ AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        Ast_കണ്ണി_Assign* newp = nullptr;
        പൂർണ്ണസംഖ്യ offset = 0;
        വേണ്ടി (size_t i = 0; i < termps.size(); ++i) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp = termps[i];
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = termp->dtypep()->widthStream();
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = srcWidth - offset - width;
            offset += width;
            Ast_കണ്ണി_വ്യഞ്ജകം* rhsp
                = പുതിയ AstSel{srcp->fileline(), srcp->clone_വൃക്ഷം_Pure(അസത്യം), lsb, width};
            എങ്കിൽ (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = പുതിയ AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            Ast_കണ്ണി_Assign* സ്ഥിരം assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_കണ്ണി::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(srcp), srcp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    തർക്കം operandConst(സ്ഥിരം Ast_കണ്ണി* nodep) { തിരികെ VN_IS(nodep, Const); }
    തർക്കം operandAsvConst(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        സ്ഥിരം Ast_കണ്ണി_Bi_Com_Asv* സ്ഥിരം bnodep = VN_CAST(nodep, കണ്ണി_Bi_Com_Asv);
        എങ്കിൽ (!bnodep) തിരികെ അസത്യം;
        എങ്കിൽ (!VN_IS(bnodep->lhsp(), Const)) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_Bi_Com_Asv* സ്ഥിരം rnodep = VN_CAST(bnodep->rhsp(), കണ്ണി_Bi_Com_Asv);
        എങ്കിൽ (!rnodep) തിരികെ അസത്യം;
        എങ്കിൽ (rnodep->type() != bnodep->type()) തിരികെ അസത്യം;
        എങ്കിൽ (rnodep->width() != bnodep->width()) തിരികെ അസത്യം;
        എങ്കിൽ (rnodep->lhsp()->width() != bnodep->lhsp()->width()) തിരികെ അസത്യം;
        എങ്കിൽ (!VN_IS(rnodep->lhsp(), Const)) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }
    തർക്കം operandAsvSame(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        സ്ഥിരം Ast_കണ്ണി_Bi_Com_Asv* സ്ഥിരം bnodep = VN_CAST(nodep, കണ്ണി_Bi_Com_Asv);
        എങ്കിൽ (!bnodep) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_Bi_Com_Asv* സ്ഥിരം rnodep = VN_CAST(bnodep->rhsp(), കണ്ണി_Bi_Com_Asv);
        എങ്കിൽ (!rnodep) തിരികെ അസത്യം;
        എങ്കിൽ (rnodep->type() != bnodep->type()) തിരികെ അസത്യം;
        എങ്കിൽ (rnodep->width() != bnodep->width()) തിരികെ അസത്യം;
        തിരികെ operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    തർക്കം operandAsvLUp(സ്ഥിരം Ast_കണ്ണി* nodep) {
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
        സ്ഥിരം Ast_കണ്ണി_Bi_Com_Asv* സ്ഥിരം bnodep = VN_CAST(nodep, കണ്ണി_Bi_Com_Asv);
        എങ്കിൽ (!bnodep) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_Bi_Com_Asv* സ്ഥിരം lnodep = VN_CAST(bnodep->lhsp(), കണ്ണി_Bi_Com_Asv);
        എങ്കിൽ (!lnodep) തിരികെ അസത്യം;
        എങ്കിൽ (lnodep->type() != bnodep->type()) തിരികെ അസത്യം;
        എങ്കിൽ (lnodep->width() != bnodep->width()) തിരികെ അസത്യം;
        തിരികെ VN_IS(lnodep->lhsp(), Const);
    }
    തർക്കം operandAsvRUp(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        സ്ഥിരം Ast_കണ്ണി_Bi_Com_Asv* സ്ഥിരം bnodep = VN_CAST(nodep, കണ്ണി_Bi_Com_Asv);
        എങ്കിൽ (!bnodep) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_Bi_Com_Asv* സ്ഥിരം rnodep = VN_CAST(bnodep->rhsp(), കണ്ണി_Bi_Com_Asv);
        എങ്കിൽ (!rnodep) തിരികെ അസത്യം;
        എങ്കിൽ (rnodep->type() != bnodep->type()) തിരികെ അസത്യം;
        എങ്കിൽ (rnodep->width() != bnodep->width()) തിരികെ അസത്യം;
        തിരികെ VN_IS(rnodep->lhsp(), Const);
    }
    സ്ഥിതം തർക്കം operandSubAdd(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം np = VN_CAST(nodep, കണ്ണി_Biop);
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം lp = VN_CAST(np->lhsp(), കണ്ണി_Biop);
        തിരികെ (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    തർക്കം matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* സ്ഥിരം constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        സ്ഥിരം auto isEqOrNeq = [](സ്ഥിരം Ast_കണ്ണി* nodep) -> തർക്കം {  //
            തിരികെ VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        എങ്കിൽ (isEqOrNeq(rhsp)) {
            ccastp = പുതിയ AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } അല്ലെങ്കിൽ എങ്കിൽ (AstCCast* സ്ഥിരം tmpp = VN_CAST(rhsp, CCast)) {
            എങ്കിൽ (isEqOrNeq(tmpp->lhsp())) {
                എങ്കിൽ (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } അല്ലെങ്കിൽ {
                    ccastp = പുതിയ AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        എങ്കിൽ (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(തള്ളുക_Deletep(andp), andp);
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം operandAndOrSame(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം np = VN_CAST(nodep, കണ്ണി_Biop);
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം lp = VN_CAST(np->lhsp(), കണ്ണി_Biop);
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം rp = VN_CAST(np->rhsp(), കണ്ണി_Biop);
        തിരികെ (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    തർക്കം matchOrAndNot(Ast_കണ്ണി_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* ap;
        Ast_കണ്ണി_Biop* andp;
        എങ്കിൽ (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } അല്ലെങ്കിൽ {
            തിരികെ അസത്യം;
        }
        സ്ഥിരം Ast_കണ്ണി_Uniop* notp;
        Ast_കണ്ണി_വ്യഞ്ജകം* cp;
        എങ്കിൽ (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } അല്ലെങ്കിൽ {
            തിരികെ അസത്യം;
        }
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bp = notp->lhsp();
        എങ്കിൽ (!operandsSame(ap, bp)) തിരികെ അസത്യം;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        എങ്കിൽ (nodep->rhsp()) {
            nodep->lhsp(cp);
        } അല്ലെങ്കിൽ {
            nodep->rhsp(cp);
        }
        തിരികെ സത്യം;
    }
    തർക്കം matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* സ്ഥിരം condp = VN_CAST(nodep->rhsp(), Cond);
        എങ്കിൽ (!condp) തിരികെ അസത്യം;
        എങ്കിൽ (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) തിരികെ അസത്യം;
        AstConst* സ്ഥിരം maskp = VN_CAST(nodep->lhsp(), Const);
        എങ്കിൽ (!maskp) തിരികെ അസത്യം;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* സ്ഥിരം newp = പുതിയ AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            പുതിയ AstAnd{nodep->fileline(), maskp->clone_വൃക്ഷം(അസത്യം), condp->thenp()->unlinkFrBack()},
            പുതിയ AstAnd{nodep->fileline(), maskp->clone_വൃക്ഷം(അസത്യം),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }

    തർക്കം matchCondCond(സ്ഥിരം AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        എങ്കിൽ (VN_IS(nodep->backp(), Cond)) തിരികെ അസത്യം;  // Was checked when visited parent
        എങ്കിൽ (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            തിരികെ അസത്യം;  // Short circuit
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> truesp;
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        തിരികെ അസത്യം;  // Can optimize further
    }
    ശൂന്യം matchCondCondRecurse(സ്ഥിരം AstCond* nodep, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& truesp,
                              std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        എങ്കിൽ (truesp.size() > 4 || falsesp.size() > 4) തിരികെ;
        എങ്കിൽ (!nodep->condp()->isPure()) തിരികെ;
        തർക്കം replaced = അസത്യം;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* condp : truesp) {
            എങ്കിൽ (replaced) നിർത്തുക;
            എങ്കിൽ (!operandsSame(nodep->condp(), condp)) തുടരുക;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_അക്കം(nodep->condp(), 1);
            replaced = സത്യം;
            ++m_stat_Cond_വ്യഞ്ജകം_Redundant;
        }
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* condp : falsesp) {
            എങ്കിൽ (replaced) നിർത്തുക;
            എങ്കിൽ (!operandsSame(nodep->condp(), condp)) തുടരുക;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = സത്യം;
            ++m_stat_Cond_വ്യഞ്ജകം_Redundant;
        }
        എങ്കിൽ (സ്ഥിരം AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            എങ്കിൽ (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            എങ്കിൽ (!replaced) truesp.എടുക്കുക_back();
        }
        എങ്കിൽ (സ്ഥിരം AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            എങ്കിൽ (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            എങ്കിൽ (!replaced) falsesp.എടുക്കുക_back();
        }
    }
    ശൂന്യം matchIfCondCond(സ്ഥിരം Ast_കണ്ണി_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        എങ്കിൽ (VN_IS(nodep->backp(), If)) തിരികെ;  // Was checked when visited parent
        എങ്കിൽ (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) തിരികെ;  // Short circuit
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> truesp;
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    ശൂന്യം matchIfCondCondRecurse(സ്ഥിരം Ast_കണ്ണി_If* nodep, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& truesp,
                                std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        എങ്കിൽ (truesp.size() > 4 || falsesp.size() > 4) തിരികെ;
        എങ്കിൽ (!nodep->condp()->isPure()) തിരികെ;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        എങ്കിൽ (VN_IS(nodep->condp(), Const)) തിരികെ;
        തർക്കം replaced = അസത്യം;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* condp : truesp) {
            എങ്കിൽ (replaced) നിർത്തുക;
            എങ്കിൽ (!operandsSame(nodep->condp(), condp)) തുടരുക;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_അക്കം(nodep->condp(), 1);
            replaced = സത്യം;
            ++m_stat_If_Cond_വ്യഞ്ജകം_Redundant;
        }
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* condp : falsesp) {
            എങ്കിൽ (replaced) നിർത്തുക;
            എങ്കിൽ (!operandsSame(nodep->condp(), condp)) തുടരുക;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = സത്യം;
            ++m_stat_If_Cond_വ്യഞ്ജകം_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_If* subIfp = VN_CAST(nodep->thensp(), കണ്ണി_If)) {
            എങ്കിൽ (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            എങ്കിൽ (!replaced) truesp.എടുക്കുക_back();
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_If* subIfp = VN_CAST(nodep->elsesp(), കണ്ണി_If)) {
            എങ്കിൽ (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            എങ്കിൽ (!replaced) falsesp.എടുക്കുക_back();
        }
    }

    ശൂന്യം matchIfSameLast(Ast_കണ്ണി_If* nodep) {
        എങ്കിൽ (!nodep->thensp()) തിരികെ;
        എങ്കിൽ (!nodep->elsesp()) തിരികെ;
        എങ്കിൽ (!canDeleteStatements()) തിരികെ;
        // Iterate bodies in reverse
        Ast_കണ്ണി* tp = nodep->thensp()->lastp();
        Ast_കണ്ണി* ep = nodep->elsesp()->lastp();
        ആകുമ്പോൾ (tp && ep && tp->same_വൃക്ഷം(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_കണ്ണി* സ്ഥിരം tPrevp = tp->prevp();
            Ast_കണ്ണി* സ്ഥിരം ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(ep->unlinkFrBack()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    തർക്കം matchMaskedOr(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(nodep->lhsp(), Const);
        സ്ഥിരം uint32_t significantBits = constp->അക്കം().widthToFit();

        AstOr* സ്ഥിരം orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        സ്ഥിരം auto checkBottomClear = [=](സ്ഥിരം Ast_കണ്ണി* nodep) -> തർക്കം {
            എങ്കിൽ (സ്ഥിരം AstShiftL* സ്ഥിരം shiftp = VN_CAST(nodep, ShiftL)) {
                എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം scp = VN_CAST(shiftp->rhsp(), Const)) {
                    തിരികെ scp->അക്കം().toUInt() >= significantBits;
                }
            }
            തിരികെ അസത്യം;
        };

        സ്ഥിരം തർക്കം orLIsRedundant = checkBottomClear(orp->lhsp());
        സ്ഥിരം തർക്കം orRIsRedundant = checkBottomClear(orp->rhsp());

        എങ്കിൽ (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                പുതിയ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(orp), orp);
            തിരികെ അസത്യം;  // input node is still valid, keep going
        } അല്ലെങ്കിൽ എങ്കിൽ (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(orp), orp);
            തിരികെ അസത്യം;  // input node is still valid, keep going
        } അല്ലെങ്കിൽ {
            തിരികെ അസത്യം;
        }
    }
    തർക്കം matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        സ്ഥിരം auto checkMask = [nodep, ഇത്](സ്ഥിരം V3_സംഖ്യ& mask) -> തർക്കം {
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(nodep->lhsp(), Const);
            എങ്കിൽ (constp->അക്കം().isCaseEq(mask)) {
                Ast_കണ്ണി* സ്ഥിരം rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ സത്യം;
            }
            തിരികെ അസത്യം;
        };

        // Check if masking is redundant
        എങ്കിൽ (സ്ഥിരം AstShiftR* സ്ഥിരം shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_സംഖ്യ maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->അക്കം().toUInt());
                തിരികെ checkMask(maskLo);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstShiftL* സ്ഥിരം shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_സംഖ്യ mask{nodep, nodep->width()};
                സ്ഥിരം uint32_t shiftAmount = scp->അക്കം().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                തിരികെ checkMask(mask);
            }
        }
        തിരികെ അസത്യം;
    }

    തർക്കം matchMaskedZero(സ്ഥിരം AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        സ്ഥിരം AstConst* സ്ഥിരം maskp = VN_AS(nodep->lhsp(), Const);
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp();
        സ്ഥിരം uint32_t msbP1 = maskp->അക്കം().mostSetBitP1();
        എങ്കിൽ (!msbP1) തിരികെ അസത്യം;  // Don't rewrite, separate rule matches for this
        സ്ഥിരം uint32_t msb = msbP1 - 1;
        സ്ഥിരം uint32_t lsb = maskp->അക്കം().leastSetBitP1() - 1;

        എങ്കിൽ (സ്ഥിരം AstShiftL* സ്ഥിരം shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            എങ്കിൽ (AstConst* സ്ഥിരം scp = VN_CAST(shiftp->rhsp(), Const)) {
                തിരികെ scp->അക്കം().fitsInUInt()  //
                       && (scp->അക്കം().toUInt() > msb);
            }
        }
        എങ്കിൽ (സ്ഥിരം AstShiftR* സ്ഥിരം shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            എങ്കിൽ (AstConst* സ്ഥിരം scp = VN_CAST(shiftp->rhsp(), Const)) {
                തിരികെ scp->അക്കം().fitsInUInt()
                       && (lsb + scp->അക്കം().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        എങ്കിൽ (സ്ഥിരം AstMul* സ്ഥിരം mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            എങ്കിൽ (AstConst* സ്ഥിരം cp = VN_CAST(mulp->lhsp(), Const)) {
                തിരികെ cp->അക്കം().leastSetBitP1() > msb + 1;
            }
        }
        എങ്കിൽ (സ്ഥിരം AstExtend* സ്ഥിരം extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            തിരികെ lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        തിരികെ അസത്യം;
    }

    തർക്കം match_Bit_Op_വൃക്ഷം(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (nodep->widthMin() != 1) തിരികെ അസത്യം;
        എങ്കിൽ (!v3Global.opt.f_Const_Bit_Op_വൃക്ഷം()) തിരികെ അസത്യം;

        സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->width();
        Ast_കണ്ണി_വ്യഞ്ജകം* rootp = nodep;
        unsigned externalOps = 0;
        // Reach past a plain making AND
        എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം andp = VN_CAST(nodep, And)) {
            എങ്കിൽ (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        എങ്കിൽ (rootp->type() == nodep->backp()->type()) തിരികെ അസത്യം;

        std::string debugPrefix;
        എങ്കിൽ (debug() >= 9) {  // LCOV_EXCL_START
            സ്ഥിതം പൂർണ്ണസംഖ്യ s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dump_വൃക്ഷം(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = Const_Bit_Op_വൃക്ഷം_Visitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        എങ്കിൽ (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }

        എങ്കിൽ (debug() >= 9) {  // LCOV_EXCL_START
            എങ്കിൽ (newp) {
                newp->dump_വൃക്ഷം(debugPrefix + "RESULT: ");
            } അല്ലെങ്കിൽ {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        തിരികെ newp;
    }
    സ്ഥിതം തർക്കം operandShiftSame(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം np = VN_AS(nodep, കണ്ണി_Biop);
        {
            സ്ഥിരം AstShiftL* സ്ഥിരം lp = VN_CAST(np->lhsp(), ShiftL);
            സ്ഥിരം AstShiftL* സ്ഥിരം rp = VN_CAST(np->rhsp(), ShiftL);
            എങ്കിൽ (lp && rp) {
                തിരികെ (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            സ്ഥിരം AstShiftR* സ്ഥിരം lp = VN_CAST(np->lhsp(), ShiftR);
            സ്ഥിരം AstShiftR* സ്ഥിരം rp = VN_CAST(np->rhsp(), ShiftR);
            എങ്കിൽ (lp && rp) {
                തിരികെ (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        തിരികെ അസത്യം;
    }
    തർക്കം operandHugeShiftL(സ്ഥിരം Ast_കണ്ണി_Biop* nodep) {
        തിരികെ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->അക്കം().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->അക്കം().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    തർക്കം operandHugeShiftR(സ്ഥിരം Ast_കണ്ണി_Biop* nodep) {
        തിരികെ (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->അക്കം().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->അക്കം().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    തർക്കം operandIsTwo(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const);
        എങ്കിൽ (!constp) തിരികെ അസത്യം;  // not constant
        എങ്കിൽ (constp->അക്കം().isFourState()) തിരികെ അസത്യം;  // four-state
        എങ്കിൽ (nodep->width() > VL_QUADSIZE) തിരികെ അസത്യം;  // too wide
        എങ്കിൽ (nodep->isSigned() && constp->അക്കം().isNegative()) തിരികെ അസത്യം;  // signed and negative
        തിരികെ constp->toUQuad() == 2;
    }
    തർക്കം operandIsTwostate(സ്ഥിരം Ast_കണ്ണി* nodep) {
        തിരികെ (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->അക്കം().isFourState());
    }
    തർക്കം operandIsPowTwo(സ്ഥിരം Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!operandIsTwostate(nodep)) തിരികെ അസത്യം;
        തിരികെ (1 == VN_AS(nodep, Const)->അക്കം().എണ്ണം_Ones());
    }
    തർക്കം operandShiftOp(സ്ഥിരം Ast_കണ്ണി_Biop* nodep) {
        എങ്കിൽ (!VN_IS(nodep->rhsp(), Const)) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), കണ്ണി_Biop);
        എങ്കിൽ (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) തിരികെ അസത്യം;
        എങ്കിൽ (nodep->width() != lhsp->width()) തിരികെ അസത്യം;
        എങ്കിൽ (nodep->width() != lhsp->lhsp()->width()) തിരികെ അസത്യം;
        എങ്കിൽ (nodep->width() != lhsp->rhsp()->width()) തിരികെ അസത്യം;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        എങ്കിൽ (VN_IS(lhsp->lhsp(), Const)) തിരികെ സത്യം;
        എങ്കിൽ (VN_IS(lhsp->rhsp(), Const)) തിരികെ സത്യം;
        // - Const shift on either side
        എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം llp = VN_CAST(lhsp->lhsp(), കണ്ണി_Biop)) {
            തിരികെ (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം lrp = VN_CAST(lhsp->rhsp(), കണ്ണി_Biop)) {
            തിരികെ (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        തിരികെ അസത്യം;
    }
    തർക്കം operandShiftShift(സ്ഥിരം Ast_കണ്ണി_Biop* nodep) {
        // We could add a AND though.
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), കണ്ണി_Biop);
        എങ്കിൽ (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) തിരികെ അസത്യം;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        എങ്കിൽ (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) തിരികെ അസത്യം;
        എങ്കിൽ (VN_AS(nodep->rhsp(), Const)->അക്കം().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->അക്കം().isFourState())
            തിരികെ അസത്യം;
        എങ്കിൽ (nodep->width() != lhsp->width()) തിരികെ അസത്യം;
        എങ്കിൽ (nodep->width() != lhsp->lhsp()->width()) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }
    തർക്കം operandWordOOB(സ്ഥിരം AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        തിരികെ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), കണ്ണി_Var_Ref)
                && VN_AS(nodep->fromp(), കണ്ണി_Var_Ref)->access().isReadOnly()
                && (static_cast<പൂർണ്ണസംഖ്യ>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), കണ്ണി_Var_Ref)->varp()->width_വാക്കുകൾ()));
    }
    തർക്കം operandSelFull(സ്ഥിരം AstSel* nodep) {
        തിരികെ (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<പൂർണ്ണസംഖ്യ>(nodep->widthConst()) == nodep->fromp()->width());
    }
    തർക്കം operandSelExtend(AstSel* nodep) {
        എങ്കിൽ (!m_doV) തിരികെ അസത്യം;
        AstExtend* സ്ഥിരം extendp = VN_CAST(nodep->fromp(), Extend);
        എങ്കിൽ (!extendp) തിരികെ അസത്യം;
        AstConst* സ്ഥിരം lsbp = VN_CAST(nodep->lsbp(), Const);
        എങ്കിൽ (!lsbp) തിരികെ അസത്യം;
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->widthConst();
        സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = lsbp->toSInt();
        സ്ഥിരം പൂർണ്ണസംഖ്യ msb = lsb + width - 1;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = extendp->lhsp();
        എങ്കിൽ (!lhsp->isPure()) തിരികെ അസത്യം;

        // Selecting the entire extended expression, replace with that
        എങ്കിൽ (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ സത്യം;
        }
        // Select entirely in the extended part - replace with zero
        എങ്കിൽ (lsb >= lhsp->width()) {
            replaceZero(nodep);
            തിരികെ സത്യം;
        }
        // Select entirely in the extended expression - replace with select from that
        എങ്കിൽ (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(പുതിയ AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ സത്യം;
        }
        // Select straddles both sides, but is just a shorter extend
        എങ്കിൽ (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(പുതിയ AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ സത്യം;
        }

        തിരികെ അസത്യം;
    }
    തർക്കം operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_കണ്ണി_Biop* സ്ഥിരം bip = VN_CAST(nodep->fromp(), കണ്ണി_Biop);
        എങ്കിൽ (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) തിരികെ അസത്യം;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bilhsp = bip->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(പുതിയ AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(പുതിയ AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        തിരികെ സത്യം;
    }
    തർക്കം operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        സ്ഥിരം AstShiftR* സ്ഥിരം shiftp = VN_CAST(nodep->fromp(), ShiftR);
        എങ്കിൽ (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            തിരികെ അസത്യം;
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ap = shiftp->lhsp();
        സ്ഥിരം AstConst* സ്ഥിരം bp = VN_AS(shiftp->rhsp(), Const);
        സ്ഥിരം AstConst* സ്ഥിരം lp = VN_AS(nodep->lsbp(), Const);
        എങ്കിൽ (bp->isWide() || bp->അക്കം().isFourState() || bp->അക്കം().isNegative() || lp->isWide()
            || lp->അക്കം().isFourState() || lp->അക്കം().isNegative()) {
            തിരികെ അസത്യം;
        }
        സ്ഥിരം പൂർണ്ണസംഖ്യ newLsb = lp->toSInt() + bp->toSInt();
        എങ്കിൽ (newLsb + nodep->widthConst() > ap->width()) തിരികെ അസത്യം;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* സ്ഥിരം newp
            = പുതിയ AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }

    തർക്കം operandBiExtendConstShrink(Ast_കണ്ണി_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* സ്ഥിരം extendp = VN_CAST(nodep->rhsp(), Extend);
        എങ്കിൽ (!extendp) തിരികെ അസത്യം;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം smallerp = extendp->lhsp();
        സ്ഥിരം പൂർണ്ണസംഖ്യ subsize = smallerp->width();
        AstConst* സ്ഥിരം constp = VN_CAST(nodep->lhsp(), Const);
        എങ്കിൽ (!constp) തിരികെ അസത്യം;
        എങ്കിൽ (!constp->അക്കം().isBitsZero(constp->width() - 1, subsize)) തിരികെ അസത്യം;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        സ്ഥിരം V3_സംഖ്യ അക്കം{constp, subsize, constp->അക്കം()};
        nodep->lhsp(പുതിയ AstConst{constp->fileline(), അക്കം});
        VL_DO_DANGLING(തള്ളുക_Deletep(constp), constp);
        തിരികെ അസത്യം;  // input node is still valid, keep going
    }
    തർക്കം operandBiExtendConstOver(സ്ഥിരം Ast_കണ്ണി_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        സ്ഥിരം AstExtend* സ്ഥിരം extendp = VN_CAST(nodep->rhsp(), Extend);
        എങ്കിൽ (!extendp) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം smallerp = extendp->lhsp();
        സ്ഥിരം പൂർണ്ണസംഖ്യ subsize = smallerp->width();
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->lhsp(), Const);
        എങ്കിൽ (!constp) തിരികെ അസത്യം;
        എങ്കിൽ (constp->അക്കം().isBitsZero(constp->width() - 1, subsize)) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }

    // Extraction checks
    തർക്കം warnSelect(AstSel* nodep) {
        എങ്കിൽ (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        സ്ഥിരം തർക്കം doit = സത്യം;
        എങ്കിൽ (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            എങ്കിൽ (VN_AS(nodep->lsbp(), Const)->അക്കം().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                എങ്കിൽ (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } അല്ലെങ്കിൽ {
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
        തിരികെ അസത്യം;  // Not a transform, so NOP
    }

    സ്ഥിതം തർക്കം operandsSame(സ്ഥിരം Ast_കണ്ണി* node1p, സ്ഥിരം Ast_കണ്ണി* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം const1p = VN_CAST(node1p, Const)) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം const2p = VN_CAST(node2p, Const)) {
                തിരികെ V3_സംഖ്യ{node1p->fileline(), 1, 0}
                    .opEq(const1p->അക്കം(), const2p->അക്കം())
                    .isEqOne();
            }
        }
        എങ്കിൽ (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            തിരികെ node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം and1p = VN_CAST(node1p, And)) {
            എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം and2p = VN_CAST(node2p, And)) {
                എങ്കിൽ (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), കണ്ണി_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), കണ്ണി_Var_Ref))
                    തിരികെ node1p->same_Gate_വൃക്ഷം(node2p);
            }
        }
        തിരികെ അസത്യം;
    }
    തർക്കം ifSameAssign(സ്ഥിരം Ast_കണ്ണി_If* nodep) {
        സ്ഥിരം Ast_കണ്ണി_Assign* സ്ഥിരം thensp = VN_CAST(nodep->thensp(), കണ്ണി_Assign);
        സ്ഥിരം Ast_കണ്ണി_Assign* സ്ഥിരം elsesp = VN_CAST(nodep->elsesp(), കണ്ണി_Assign);
        എങ്കിൽ (!thensp || thensp->nextp()) തിരികെ അസത്യം;  // Must be SINGLE statement
        എങ്കിൽ (!elsesp || elsesp->nextp()) തിരികെ അസത്യം;
        എങ്കിൽ (thensp->type() != elsesp->type()) തിരികെ അസത്യം;  // Can't mix an assigndly with assign
        എങ്കിൽ (!thensp->lhsp()->same_Gate_വൃക്ഷം(elsesp->lhsp())) തിരികെ അസത്യം;
        എങ്കിൽ (!thensp->rhsp()->gate_വൃക്ഷം()) തിരികെ അസത്യം;
        എങ്കിൽ (!elsesp->rhsp()->gate_വൃക്ഷം()) തിരികെ അസത്യം;
        // Must not create an expression with unpacked array type
        എങ്കിൽ (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) തിരികെ അസത്യം;
        എങ്കിൽ (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) തിരികെ അസത്യം;
        എങ്കിൽ (m_underRecFunc) തിരികെ അസത്യം;  // This optimization may lead to infinite recursion
        എങ്കിൽ (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            എങ്കിൽ (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) തിരികെ അസത്യം;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        തിരികെ !VN_IS(thensp->rhsp(), കണ്ണി_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), കണ്ണി_FTask_Ref)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    തർക്കം operandIfIf(സ്ഥിരം Ast_കണ്ണി_If* nodep) {
        എങ്കിൽ (nodep->elsesp()) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_If* സ്ഥിരം lowerIfp = VN_CAST(nodep->thensp(), കണ്ണി_If);
        എങ്കിൽ (!lowerIfp || lowerIfp->nextp()) തിരികെ അസത്യം;
        എങ്കിൽ (nodep->type() != lowerIfp->type()) തിരികെ അസത്യം;
        എങ്കിൽ (Ast_കണ്ണി::afterCommentp(lowerIfp->elsesp())) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }
    തർക്കം ifConcatMergeableBiop(സ്ഥിരം Ast_കണ്ണി* nodep) {
        തിരികെ (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    തർക്കം ifAdjacentSel(സ്ഥിരം AstSel* lhsp, സ്ഥിരം AstSel* rhsp) {
        എങ്കിൽ (!v3Global.opt.fAssemble()) തിരികെ അസത്യം;  // opt disabled
        എങ്കിൽ (!lhsp || !rhsp) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം lfromp = lhsp->fromp();
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം rfromp = rhsp->fromp();
        എങ്കിൽ (!lfromp || !rfromp || !lfromp->same_Gate_വൃക്ഷം(rfromp)) തിരികെ അസത്യം;
        സ്ഥിരം AstConst* സ്ഥിരം lstart = VN_CAST(lhsp->lsbp(), Const);
        സ്ഥിരം AstConst* സ്ഥിരം rstart = VN_CAST(rhsp->lsbp(), Const);
        എങ്കിൽ (!lstart || !rstart) തിരികെ അസത്യം;  // too complicated
        സ്ഥിരം പൂർണ്ണസംഖ്യ rend = (rstart->toSInt() + rhsp->widthConst());
        തിരികെ (rend == lstart->toSInt());
    }
    തർക്കം ifMergeAdjacent(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        എങ്കിൽ (!v3Global.opt.fAssemble()) തിരികെ അസത്യം;  // opt disabled
        // Two same varref
        എങ്കിൽ (operandsSame(lhsp, rhsp)) തിരികെ സത്യം;
        സ്ഥിരം AstSel* സ്ഥിരം lselp = VN_CAST(lhsp, Sel);
        സ്ഥിരം AstSel* സ്ഥിരം rselp = VN_CAST(rhsp, Sel);
        എങ്കിൽ (!lselp && !rselp) തിരികെ അസത്യം;
        എങ്കിൽ (lselp && !VN_IS(lselp->lsbp(), Const)) തിരികെ അസത്യം;
        എങ്കിൽ (rselp && !VN_IS(rselp->lsbp(), Const)) തിരികെ അസത്യം;
        // a[i:j] {a[j-1:k], b}
        എങ്കിൽ (lselp && !rselp && VN_IS(rhsp, Concat))
            തിരികെ ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        എങ്കിൽ (rselp && !lselp && VN_IS(lhsp, Concat))
            തിരികെ ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_വൃക്ഷം(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_വൃക്ഷം(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        എങ്കിൽ (!lfromp || !rfromp || !lfromp->same_Gate_വൃക്ഷം(rfromp)) തിരികെ അസത്യം;

        സ്ഥിരം int32_t lstart = lselp ? lselp->lsbConst() : 0;
        സ്ഥിരം int32_t rstart = rselp ? rselp->lsbConst() : 0;
        സ്ഥിരം int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        എങ്കിൽ (rend == lstart) തിരികെ സത്യം;
        // a[i:0] a[msb:j]
        എങ്കിൽ (rend == rfromp->width() && lstart == 0) തിരികെ സത്യം;
        തിരികെ അസത്യം;
    }
    തർക്കം concatMergeable(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, unsigned depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        എങ്കിൽ (!v3Global.opt.fAssemble()) തിരികെ അസത്യം;  // opt disabled
        എങ്കിൽ (lhsp->type() != rhsp->type()) തിരികെ അസത്യം;
        എങ്കിൽ (!ifConcatMergeableBiop(lhsp)) തിരികെ അസത്യം;
        എങ്കിൽ (depth > CONCAT_MERGABLE_MAX_DEPTH) തിരികെ അസത്യം;  // As worse case O(n^2) algorithm

        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം lp = VN_CAST(lhsp, കണ്ണി_Biop);
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം rp = VN_CAST(rhsp, കണ്ണി_Biop);
        എങ്കിൽ (!lp || !rp) തിരികെ അസത്യം;
        // {a[]&b[], a[]&b[]}
        സ്ഥിരം തർക്കം lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        സ്ഥിരം തർക്കം rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        എങ്കിൽ (lad && rad) തിരികെ സത്യം;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        എങ്കിൽ (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) തിരികെ സത്യം;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        എങ്കിൽ (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) തിരികെ സത്യം;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        എങ്കിൽ (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    സ്ഥിതം തർക്കം operandsSameWidth(സ്ഥിരം Ast_കണ്ണി* lhsp, സ്ഥിരം Ast_കണ്ണി* rhsp) {
        തിരികെ lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    ശൂന്യം replace_അക്കം(Ast_കണ്ണി* oldp, സ്ഥിരം V3_സംഖ്യ& അക്കം, സ്ഥിരം string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->അക്കം().isFourState()), oldp,
                    "Already constant??");
        AstConst* സ്ഥിരം newp = പുതിയ AstConst{oldp->fileline(), അക്കം};
        എങ്കിൽ (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(തള്ളുക_Deletep(oldp), oldp);
    }
    ശൂന്യം replace_അക്കം(Ast_കണ്ണി* nodep, uint32_t val) {
        സ്ഥിരം V3_സംഖ്യ അക്കം{nodep, nodep->width(), val};
        VL_DO_DANGLING(replace_അക്കം(nodep, അക്കം), nodep);
    }
    ശൂന്യം replace_അക്കം_Signed(Ast_കണ്ണി_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        എങ്കിൽ (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_അക്കം(nodep, val), nodep);
    }
    ശൂന്യം replace_അക്കം_Limited(Ast_കണ്ണി_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        എങ്കിൽ (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_അക്കം(nodep, val), nodep);
    }
    ശൂന്യം replaceZero(Ast_കണ്ണി* nodep) { VL_DO_DANGLING(replace_അക്കം(nodep, 0), nodep); }
    ശൂന്യം replaceZeroChkPure(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        എങ്കിൽ (checkp->isPure()) {
            VL_DO_DANGLING(replace_അക്കം(nodep, 0), nodep);
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstAnd{nodep->fileline(), പുതിയ AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം replaceAllOnes(Ast_കണ്ണി* nodep) {
        V3_സംഖ്യ ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replace_അക്കം(nodep, ones), nodep);
    }
    ശൂന്യം replaceConst(Ast_കണ്ണി_Uniop* nodep) {
        V3_സംഖ്യ numv{nodep, nodep->widthMinV()};
        nodep->സംഖ്യ_Operate(numv, const_അക്കം_V(nodep->lhsp()));
        സ്ഥിരം V3_സംഖ്യ& അക്കം = to_അക്കം_C(nodep, numv);
        UINFO(4, "UNICONST -> " << അക്കം);
        VL_DO_DANGLING(replace_അക്കം(nodep, അക്കം), nodep);
    }
    ശൂന്യം replaceConst(Ast_കണ്ണി_Biop* nodep) {
        V3_സംഖ്യ numv{nodep, nodep->widthMinV()};
        nodep->സംഖ്യ_Operate(numv, const_അക്കം_V(nodep->lhsp()), const_അക്കം_V(nodep->rhsp()));
        സ്ഥിരം V3_സംഖ്യ& അക്കം = to_അക്കം_C(nodep, numv);
        UINFO(4, "BICONST -> " << അക്കം);
        VL_DO_DANGLING(replace_അക്കം(nodep, അക്കം), nodep);
    }
    ശൂന്യം replaceConst(Ast_കണ്ണി_Triop* nodep) {
        V3_സംഖ്യ numv{nodep, nodep->widthMinV()};
        nodep->സംഖ്യ_Operate(numv, const_അക്കം_V(nodep->lhsp()), const_അക്കം_V(nodep->rhsp()),
                             const_അക്കം_V(nodep->thsp()));
        സ്ഥിരം V3_സംഖ്യ& അക്കം = to_അക്കം_C(nodep, numv);
        UINFO(4, "TRICONST -> " << അക്കം);
        VL_DO_DANGLING(replace_അക്കം(nodep, അക്കം), nodep);
    }
    ശൂന്യം replaceConst(Ast_കണ്ണി_Quadop* nodep) {
        V3_സംഖ്യ numv{nodep, nodep->widthMinV()};
        nodep->സംഖ്യ_Operate(numv, const_അക്കം_V(nodep->lhsp()), const_അക്കം_V(nodep->rhsp()),
                             const_അക്കം_V(nodep->thsp()), const_അക്കം_V(nodep->fhsp()));
        സ്ഥിരം V3_സംഖ്യ& അക്കം = to_അക്കം_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << അക്കം);
        VL_DO_DANGLING(replace_അക്കം(nodep, അക്കം), nodep);
    }

    ശൂന്യം replaceConstString(Ast_കണ്ണി* oldp, സ്ഥിരം string& അക്കം) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstConst{oldp->fileline(), AstConst::String{}, അക്കം};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    ശൂന്യം replaceWChild(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം replaceWChildBool(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        എങ്കിൽ (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } അല്ലെങ്കിൽ {
            nodep->replaceWithKeepDType(പുതിയ AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    ശൂന്യം replaceWIteratedRhs(Ast_കണ്ണി_Triop* nodep) {
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    ശൂന്യം replaceWIteratedThs(Ast_കണ്ണി_Triop* nodep) {
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    ശൂന്യം replaceWLhs(Ast_കണ്ണി_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ശൂന്യം replaceWLhs(Ast_കണ്ണി_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    ശൂന്യം replaceWRhs(Ast_കണ്ണി_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    ശൂന്യം replaceWLhsBool(Ast_കണ്ണി_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    ശൂന്യം replaceWRhsBool(Ast_കണ്ണി_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    ശൂന്യം replaceAsv(Ast_കണ്ണി_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ap = nodep->lhsp();
        Ast_കണ്ണി_Biop* സ്ഥിരം rp = VN_AS(nodep->rhsp(), കണ്ണി_Biop);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bp = rp->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        എങ്കിൽ (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    ശൂന്യം replaceAsvLUp(Ast_കണ്ണി_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_കണ്ണി_Biop* സ്ഥിരം lp = VN_AS(nodep->lhsp()->unlinkFrBack(), കണ്ണി_Biop);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം llp = lp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lrp = lp->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ശൂന്യം replaceAsvRUp(Ast_കണ്ണി_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lp = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_Biop* സ്ഥിരം rp = VN_AS(nodep->rhsp()->unlinkFrBack(), കണ്ണി_Biop);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rlp = rp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    ശൂന്യം replaceAndOr(Ast_കണ്ണി_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_കണ്ണി_Biop* സ്ഥിരം lp = VN_AS(nodep->lhsp()->unlinkFrBack(), കണ്ണി_Biop);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം llp = lp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lrp = lp->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_Biop* സ്ഥിരം rp = VN_AS(nodep->rhsp()->unlinkFrBack(), കണ്ണി_Biop);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rlp = rp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        എങ്കിൽ (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(തള്ളുക_Deletep(rp), rp);
            VL_DO_DANGLING(തള്ളുക_Deletep(rlp), rlp);
        } അല്ലെങ്കിൽ എങ്കിൽ (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(തള്ളുക_Deletep(rp), rp);
            VL_DO_DANGLING(തള്ളുക_Deletep(lrp), lrp);
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    ശൂന്യം replaceShiftSame(Ast_കണ്ണി_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_കണ്ണി_Biop* സ്ഥിരം lp = VN_AS(nodep->lhsp()->unlinkFrBack(), കണ്ണി_Biop);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം llp = lp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lrp = lp->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_Biop* സ്ഥിരം rp = VN_AS(nodep->rhsp()->unlinkFrBack(), കണ്ണി_Biop);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rlp = rp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(തള്ളുക_Deletep(rp), rp);
        VL_DO_DANGLING(തള്ളുക_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    ശൂന്യം replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* സ്ഥിരം lselp = VN_AS(nodep->lhsp()->unlinkFrBack(), Sel);
        AstSel* സ്ഥിരം rselp = VN_AS(nodep->rhsp()->unlinkFrBack(), Sel);
        സ്ഥിരം പൂർണ്ണസംഖ്യ lstart = lselp->lsbConst();
        സ്ഥിരം പൂർണ്ണസംഖ്യ lwidth = lselp->widthConst();
        സ്ഥിരം പൂർണ്ണസംഖ്യ rstart = rselp->lsbConst();
        സ്ഥിരം പൂർണ്ണസംഖ്യ rwidth = rselp->widthConst();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* സ്ഥിരം newselp = പുതിയ AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlinkFrBack(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(തള്ളുക_Deletep(lselp), lselp);
        VL_DO_DANGLING(തള്ളുക_Deletep(rselp), rselp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_കണ്ണി_Biop* സ്ഥിരം lp = VN_AS(nodep->lhsp(), കണ്ണി_Biop);
        Ast_കണ്ണി_Biop* സ്ഥിരം rp = VN_AS(nodep->rhsp(), കണ്ണി_Biop);
        എങ്കിൽ (concatMergeable(lp, rp, 0)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം llp = lp->lhsp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lrp = lp->rhsp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rlp = rp->lhsp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rrp = rp->rhsp();
            AstConcat* സ്ഥിരം newlp = പുതിയ AstConcat{rlp->fileline(), llp->clone_വൃക്ഷം_Pure(അസത്യം),
                                                   rlp->clone_വൃക്ഷം_Pure(അസത്യം)};
            AstConcat* സ്ഥിരം newrp = പുതിയ AstConcat{rrp->fileline(), lrp->clone_വൃക്ഷം_Pure(അസത്യം),
                                                   rrp->clone_വൃക്ഷം_Pure(അസത്യം)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(തള്ളുക_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(തള്ളുക_Deletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(തള്ളുക_Deletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    ശൂന്യം replaceExtend(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം replacePowShift(Ast_കണ്ണി_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* സ്ഥിരം newp
            = പുതിയ AstShiftL{nodep->fileline(), പുതിയ AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        സ്ഥിരം പൂർണ്ണസംഖ്യ amount = VN_AS(nodep->lhsp(), Const)->അക്കം().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* സ്ഥിരം newp
            = പുതിയ AstShiftL{nodep->fileline(), opp, പുതിയ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        സ്ഥിരം പൂർണ്ണസംഖ്യ amount = VN_AS(nodep->rhsp(), Const)->അക്കം().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* സ്ഥിരം newp
            = പുതിയ AstShiftR{nodep->fileline(), opp, പുതിയ AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        സ്ഥിരം പൂർണ്ണസംഖ്യ amount = VN_AS(nodep->rhsp(), Const)->അക്കം().mostSetBitP1() - 1;  // 2^n->n+1
        V3_സംഖ്യ mask{nodep, nodep->width()};
        mask.setMask(amount);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* സ്ഥിരം newp
            = പുതിയ AstAnd{nodep->fileline(), opp, പുതിയ AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം replaceShiftOp(Ast_കണ്ണി_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->width();
        സ്ഥിരം പൂർണ്ണസംഖ്യ widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        Ast_കണ്ണി_Biop* സ്ഥിരം lhsp = VN_AS(nodep->lhsp(), കണ്ണി_Biop);
        lhsp->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം shiftp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ap = lhsp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bp = lhsp->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_Biop* സ്ഥിരം shift1p = nodep;
        Ast_കണ്ണി_Biop* സ്ഥിരം shift2p = nodep->clone_വൃക്ഷം(സത്യം);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_വൃക്ഷം_Pure(സത്യം));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_കണ്ണി_Biop* സ്ഥിരം newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    ശൂന്യം replaceShiftShift(Ast_കണ്ണി_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_കണ്ണി_Biop* സ്ഥിരം lhsp = VN_AS(nodep->lhsp(), കണ്ണി_Biop);
        lhsp->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ap = lhsp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം shift1p = lhsp->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        എങ്കിൽ (nodep->type() == lhsp->type()) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ shift1 = VN_AS(shift1p, Const)->toUInt();
            സ്ഥിരം പൂർണ്ണസംഖ്യ shift2 = VN_AS(shift2p, Const)->toUInt();
            സ്ഥിരം പൂർണ്ണസംഖ്യ newshift = shift1 + shift2;
            VL_DO_DANGLING(തള്ളുക_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(തള്ളുക_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(പുതിയ AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } അല്ലെങ്കിൽ {
            // We know shift amounts are constant, but might be a mixed left/right shift
            പൂർണ്ണസംഖ്യ shift1 = VN_AS(shift1p, Const)->toUInt();
            എങ്കിൽ (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            പൂർണ്ണസംഖ്യ shift2 = VN_AS(shift2p, Const)->toUInt();
            എങ്കിൽ (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            സ്ഥിരം പൂർണ്ണസംഖ്യ newshift = shift1 + shift2;
            VL_DO_DANGLING(തള്ളുക_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(തള്ളുക_Deletep(shift2p), shift2p);
            Ast_കണ്ണി_വ്യഞ്ജകം* newp;
            V3_സംഖ്യ mask1{nodep, nodep->width()};
            V3_സംഖ്യ ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            എങ്കിൽ (shift1 < 0) {
                mask1.opShiftR(ones, V3_സംഖ്യ(nodep, VL_IDATASIZE, -shift1));
            } അല്ലെങ്കിൽ {
                mask1.opShiftL(ones, V3_സംഖ്യ(nodep, VL_IDATASIZE, shift1));
            }
            V3_സംഖ്യ mask{nodep, nodep->width()};
            എങ്കിൽ (shift2 < 0) {
                mask.opShiftR(mask1, V3_സംഖ്യ(nodep, VL_IDATASIZE, -shift2));
            } അല്ലെങ്കിൽ {
                mask.opShiftL(mask1, V3_സംഖ്യ(nodep, VL_IDATASIZE, shift2));
            }
            എങ്കിൽ (newshift < 0) {
                newp = പുതിയ AstShiftR{nodep->fileline(), ap,
                                     പുതിയ AstConst(nodep->fileline(), -newshift)};
            } അല്ലെങ്കിൽ {
                newp = പുതിയ AstShiftL{nodep->fileline(), ap,
                                     പുതിയ AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = പുതിയ AstAnd{nodep->fileline(), newp, പുതിയ AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(lhsp), lhsp);
    }

    തർക്കം replaceAssignMultiSel(Ast_കണ്ണി_Assign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        എങ്കിൽ (!m_modp) തിരികെ അസത്യം;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* സ്ഥിരം sel1p = VN_CAST(nodep->lhsp(), Sel);
        എങ്കിൽ (!sel1p) തിരികെ അസത്യം;
        Ast_കണ്ണി_Assign* സ്ഥിരം nextp = VN_CAST(nodep->nextp(), കണ്ണി_Assign);
        എങ്കിൽ (!nextp) തിരികെ അസത്യം;
        എങ്കിൽ (nodep->type() != nextp->type()) തിരികെ അസത്യം;
        സ്ഥിരം AstSel* സ്ഥിരം sel2p = VN_CAST(nextp->lhsp(), Sel);
        എങ്കിൽ (!sel2p) തിരികെ അസത്യം;
        AstVarRef* സ്ഥിരം varref1p = VN_CAST(sel1p->fromp(), VarRef);
        എങ്കിൽ (!varref1p) തിരികെ അസത്യം;
        സ്ഥിരം AstVarRef* സ്ഥിരം varref2p = VN_CAST(sel2p->fromp(), VarRef);
        എങ്കിൽ (!varref2p) തിരികെ അസത്യം;
        എങ്കിൽ (!varref1p->same_Gate_വൃക്ഷം(varref2p)) തിരികെ അസത്യം;
        സ്ഥിരം AstConst* സ്ഥിരം con1p = VN_CAST(sel1p->lsbp(), Const);
        എങ്കിൽ (!con1p) തിരികെ അസത്യം;
        സ്ഥിരം AstConst* സ്ഥിരം con2p = VN_CAST(sel2p->lsbp(), Const);
        എങ്കിൽ (!con2p) തിരികെ അസത്യം;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        എങ്കിൽ (!varNotReferenced(nodep->rhsp(), varref1p->varp())) തിരികെ അസത്യം;
        എങ്കിൽ (!varNotReferenced(nextp->rhsp(), varref2p->varp())) തിരികെ അസത്യം;
        // If a variable is marked split_var, access to the variable should not be merged.
        എങ്കിൽ (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) തിരികെ അസത്യം;
        // Swap?
        എങ്കിൽ ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            തിരികെ അസത്യം;
        }
        സ്ഥിരം തർക്കം lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhs2p = nextp->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_Assign* newp;
        എങ്കിൽ (lsbFirstAssign) {
            newp = nodep->cloneType(പുതിയ AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    പുതിയ AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } അല്ലെങ്കിൽ {
            newp = nodep->cloneType(പുതിയ AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    പുതിയ AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        VL_DO_DANGLING(തള്ളുക_Deletep(nextp->unlinkFrBack()), nextp);
        തിരികെ സത്യം;
    }

    തർക്കം replaceAssignSFormatF(Ast_കണ്ണി_Assign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        എങ്കിൽ (!VN_IS(nodep, Assign)) തിരികെ അസത്യം;
        AstVarRef* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), VarRef);
        എങ്കിൽ (!lhsp) തിരികെ അസത്യം;
        AstSFormatF* സ്ഥിരം fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        എങ്കിൽ (!fmtp) തിരികെ അസത്യം;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(പുതിയ AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }

    തർക്കം varNotReferenced(Ast_കണ്ണി* nodep, AstVar* varp, പൂർണ്ണസംഖ്യ level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        എങ്കിൽ (!nodep) തിരികെ സത്യം;
        എങ്കിൽ (level > 2) തിരികെ അസത്യം;
        എങ്കിൽ (!nodep->isPure()) തിരികെ അസത്യം;  // For example a $fgetc can't be reordered
        എങ്കിൽ (VN_IS(nodep, കണ്ണി_Var_Ref) && VN_AS(nodep, കണ്ണി_Var_Ref)->varp() == varp) തിരികെ അസത്യം;
        തിരികെ (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    തർക്കം replace_കണ്ണി_Assign(Ast_കണ്ണി_Assign* nodep) {
        എങ്കിൽ (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            എങ്കിൽ (VN_IS(nodep, AssignW)) {
                എങ്കിൽ (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                തിരികെ അസത്യം;  // Don't delete the assign, or V3Gate will freak out
            } അല്ലെങ്കിൽ {
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ സത്യം;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            തർക്കം need_temp = അസത്യം;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            സ്ഥിരം തർക്കം waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            സ്ഥിരം തർക്കം needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            എങ്കിൽ (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<size_t> lhsVarIds;
                nodep->lhsp()->foreach([&](സ്ഥിരം AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](സ്ഥിരം AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    എങ്കിൽ (lhsVarIds.എണ്ണം(m_ids(refp->varp()))) need_temp = സത്യം;
                });
            }
            എങ്കിൽ (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* സ്ഥിരം tempPurep = പുതിയ AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* സ്ഥിരം tempPureRefp
                    = പുതിയ AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                Ast_കണ്ണി_Assign* സ്ഥിരം asnp
                    = VN_IS(nodep, AssignDly)
                          ? പുതിയ AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(പുതിയ AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                എങ്കിൽ (waits) തള്ളുക_Deletep(nodep->timingControlp()->unlinkFrBack());
            } അല്ലെങ്കിൽ എങ്കിൽ (need_temp) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } അല്ലെങ്കിൽ {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* സ്ഥിരം delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            എങ്കിൽ (delayp && !delayp->lhsp()->isPure()) {
                File_വരി* സ്ഥിരം flp = nodep->fileline();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = delayp->lhsp()->unlinkFrBack();
                AstVar* സ്ഥിരം tempp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswapNames.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempp);
                nodep->addHereThisAsNext(
                    പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(പുതിയ AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhs2p = rhsp->clone_വൃക്ഷം_Pure(അസത്യം);
            // Calc widths
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb2 = 0;
            സ്ഥിരം പൂർണ്ണസംഖ്യ msb2 = lsb2 + lc2p->width() - 1;
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb1 = msb2 + 1;
            സ്ഥിരം പൂർണ്ണസംഖ്യ msb1 = lsb1 + lc1p->width() - 1;
            UASSERT_OBJ(msb1 == (conp->width() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* സ്ഥിരം sel1p = പുതിയ AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* സ്ഥിരം sel2p = പുതിയ AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_കണ്ണി_Assign* newp = nullptr;
            എങ്കിൽ (!need_temp) {
                Ast_കണ്ണി_Assign* സ്ഥിരം asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_കണ്ണി_Assign* സ്ഥിരം asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_കണ്ണി::addNext(newp, asn1ap);
                newp = Ast_കണ്ണി::addNext(newp, asn2ap);
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* സ്ഥിരം temp1p
                    = പുതിയ AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* സ്ഥിരം temp2p
                    = പുതിയ AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->addStmtsp(temp1p);
                m_modp->addStmtsp(temp2p);
                Ast_കണ്ണി_Assign* സ്ഥിരം asn1ap = nodep->cloneType(
                    പുതിയ AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_കണ്ണി_Assign* സ്ഥിരം asn2ap = nodep->cloneType(
                    പുതിയ AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_കണ്ണി_Assign* സ്ഥിരം asn1bp = nodep->cloneType(
                    lc1p, പുതിയ AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_കണ്ണി_Assign* സ്ഥിരം asn2bp = nodep->cloneType(
                    lc2p, പുതിയ AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_കണ്ണി::addNext(newp, asn1ap);
                newp = Ast_കണ്ണി::addNext(newp, asn2ap);
                newp = Ast_കണ്ണി::addNext(newp, asn1bp);
                newp = Ast_കണ്ണി::addNext(newp, asn2bp);
            }
            എങ്കിൽ (debug() >= 9 && newp) newp->dump_വൃക്ഷം_And_Next(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(തള്ളുക_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* സ്ഥിരം cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* srcp = cvtp->fromp()->unlinkFrBack();
            എങ്കിൽ (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_Stream* സ്ഥിരം streamp = VN_CAST(srcp, കണ്ണി_Stream)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം streamSrcp = streamp->lhsp();
                എങ്കിൽ (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* സ്ഥിരം streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* srcp = streamp->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_DType* സ്ഥിരം srcDTypep = srcp->dtypep()->skipRefp();
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            എങ്കിൽ (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                എങ്കിൽ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    പൂർണ്ണസംഖ്യ srcElementBits = 0;
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    പൂർണ്ണസംഖ്യ dstElementBits = 0;
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = പുതിയ AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), അസത്യം, 1,
                        dstElementBits,   srcElementBits};
                } അല്ലെങ്കിൽ {
                    srcp = പുതിയ AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = പുതിയ AstCvtArrayToPacked{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                സ്ഥിരം പൂർണ്ണസംഖ്യ packedBits = nodep->lhsp()->widthMin();
                സ്ഥിരം പൂർണ്ണസംഖ്യ unpackBits
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->widthMin();
                സ്ഥിരം uint32_t offset = packedBits > unpackBits ? packedBits - unpackBits : 0;
                srcp = പുതിയ AstShiftL{srcp->fileline(), srcp,
                                     പുതിയ AstConst{srcp->fileline(), offset}, packedBits};
            }
            എങ്കിൽ (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ sWidth = srcp->width();
                സ്ഥിരം പൂർണ്ണസംഖ്യ dWidth = nodep->lhsp()->width();
                എങ്കിൽ (sWidth < dWidth) {
                    AstExtend* സ്ഥിരം extendp = പുതിയ AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = പുതിയ AstShiftL{
                        srcp->fileline(), extendp,
                        പുതിയ AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(തള്ളുക_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_കണ്ണി_വ്യഞ്ജകം* streamp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_DType* സ്ഥിരം dstDTypep = dstp->dtypep()->skipRefp();
            Ast_കണ്ണി_വ്യഞ്ജകം* srcp = nodep->rhsp()->unlinkFrBack();
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            എങ്കിൽ ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                പൂർണ്ണസംഖ്യ blockSize = 1;
                എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    എങ്കിൽ (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                പൂർണ്ണസംഖ്യ srcElementBits = 0;
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                പൂർണ്ണസംഖ്യ dstElementBits = 0;
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(പുതിയ AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, സത്യം,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(തള്ളുക_Deletep(streamp), streamp);
                തിരികെ സത്യം;
            }
            എങ്കിൽ (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            സ്ഥിരം പൂർണ്ണസംഖ്യ sWidth = srcp->width();
            സ്ഥിരം പൂർണ്ണസംഖ്യ dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            എങ്കിൽ (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } അല്ലെങ്കിൽ {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            എങ്കിൽ (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = പുതിയ AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (dWidth == 0) {
                    streamp = പുതിയ AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } അല്ലെങ്കിൽ എങ്കിൽ (sWidth >= dWidth) {
                    streamp = പുതിയ AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം streamp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_DType* സ്ഥിരം dstDTypep = dstp->dtypep()->skipRefp();
            Ast_കണ്ണി_വ്യഞ്ജകം* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            എങ്കിൽ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* origSrcp = srcp;
                എങ്കിൽ (AstCvtArrayToPacked* സ്ഥിരം cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം origSrcDTypep = origSrcp->dtypep()->skipRefp();
                എങ്കിൽ (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    പൂർണ്ണസംഖ്യ srcElementBits = 0;
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    പൂർണ്ണസംഖ്യ dstElementBits = 0;
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    എങ്കിൽ (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(തള്ളുക_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    തർക്കം reverse = അസത്യം;
                    എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(പുതിയ AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(തള്ളുക_Deletep(streamp), streamp);
                    തിരികെ സത്യം;
                }
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ sWidth = srcp->width();
            സ്ഥിരം പൂർണ്ണസംഖ്യ dWidth = dstp->width();
            എങ്കിൽ (VN_IS(dstDTypep, UnpackArrayDType)) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                എങ്കിൽ (sWidth > dstBitWidth) {
                    srcp
                        = പുതിയ AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = പുതിയ AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (dWidth == 0) {
                    srcp = പുതിയ AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } അല്ലെങ്കിൽ എങ്കിൽ (sWidth >= dWidth) {
                    srcp = പുതിയ AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } അല്ലെങ്കിൽ {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    എങ്കിൽ (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* സ്ഥിരം extendp = പുതിയ AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = പുതിയ AstShiftL{
                            srcp->fileline(), extendp,
                            പുതിയ AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } അല്ലെങ്കിൽ {  // if it is a queue we dont need do do a shiftL
                        srcp = പുതിയ AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(തള്ളുക_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            Ast_കണ്ണി_വ്യഞ്ജകം* srcp = streamp->lhsp();
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം srcDTypep = srcp->dtypep()->skipRefp();
            Ast_കണ്ണി_DType* സ്ഥിരം dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            എങ്കിൽ (lowerAsFixedAggregate(srcDTypep)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } അല്ലെങ്കിൽ എങ്കിൽ ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                എങ്കിൽ (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    പൂർണ്ണസംഖ്യ blockSize = 1;
                    എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        എങ്കിൽ (VL_UNLIKELY(blockSize <= 0)) {
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
                    പൂർണ്ണസംഖ്യ srcElementBits = 0;
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    പൂർണ്ണസംഖ്യ dstElementBits = 0;
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = പുതിയ AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     സത്യം,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(streamp), streamp);
                } അല്ലെങ്കിൽ {
                    streamp->lhsp(പുതിയ AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (m_doV && replaceAssignMultiSel(nodep)) {
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (replaceAssignSFormatF(nodep)) {
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // Boolean replacements
    തർക്കം operandBoolShift(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        എങ്കിൽ (!VN_IS(nodep, And)) തിരികെ അസത്യം;
        എങ്കിൽ (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) തിരികെ അസത്യം;
        എങ്കിൽ (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) തിരികെ അസത്യം;
        സ്ഥിരം AstShiftR* സ്ഥിരം shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        എങ്കിൽ (!VN_IS(shiftp->rhsp(), Const)) തിരികെ അസത്യം;
        എങ്കിൽ (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }
    ശൂന്യം replaceBoolShift(Ast_കണ്ണി* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* സ്ഥിരം andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* സ്ഥിരം shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3_സംഖ്യ val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->അക്കം(), shiftConstp->അക്കം());
        AstAnd* സ്ഥിരം newp
            = പുതിയ AstAnd{nodep->fileline(), പുതിയ AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    ശൂന്യം replaceWithSimulation(Ast_കണ്ണി* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        എങ്കിൽ (!simvis.optimizable()) {
            സ്ഥിരം Ast_കണ്ണി* errorp = simvis.whyNotNodep();
            എങ്കിൽ (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } അല്ലെങ്കിൽ {
            // Fetch the result
            Ast_കണ്ണി* സ്ഥിരം valuep = simvis.fetch_മൂല്യം_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_കണ്ണി* സ്ഥിരം newp = valuep->clone_വൃക്ഷം(അസത്യം);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = അസത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = അസത്യം;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ശൂന്യം swapSides(Ast_കണ്ണി_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    തർക്കം containsMemberAccessRecurse(Ast_കണ്ണി* സ്ഥിരം nodep) {
        എങ്കിൽ (!nodep) തിരികെ അസത്യം;
        സ്ഥിരം size_t id = m_ids(nodep);
        സ്ഥിരം auto it = m_containsMemberAccess.find(id);
        എങ്കിൽ (it != m_containsMemberAccess.end()) തിരികെ it->second;
        ++m_statMemberAccessVisits;
        തർക്കം result = അസത്യം;
        എങ്കിൽ (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_FTask_Ref* സ്ഥിരം funcRefp = VN_CAST(nodep, കണ്ണി_FTask_Ref)) {
            എങ്കിൽ (containsMemberAccessRecurse(funcRefp->taskp())) result = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_CCall* സ്ഥിരം funcRefp = VN_CAST(nodep, കണ്ണി_CCall)) {
            എങ്കിൽ (containsMemberAccessRecurse(funcRefp->funcp())) result = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം funcp = VN_CAST(nodep, കണ്ണി_FTask)) {
            // Assume that it has a member access
            എങ്കിൽ (funcp->recursive()) result = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCFunc* സ്ഥിരം funcp = VN_CAST(nodep, CFunc)) {
            എങ്കിൽ (funcp->recursive()) result = സത്യം;
        }
        എങ്കിൽ (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        എങ്കിൽ (!result && !VN_IS(nodep, കണ്ണി_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = സത്യം;
        }
        m_containsMemberAccess.emplace(id, result);
        തിരികെ result;
    }

    തർക്കം matchBiopToBitwise(Ast_കണ്ണി_Biop* സ്ഥിരം nodep) {
        എങ്കിൽ (!m_convertLogicToBit) തിരികെ അസത്യം;
        എങ്കിൽ (!nodep->lhsp()->width1()) തിരികെ അസത്യം;
        എങ്കിൽ (!nodep->rhsp()->width1()) തിരികെ അസത്യം;
        എങ്കിൽ (!nodep->isPure()) തിരികെ അസത്യം;
        എങ്കിൽ (containsMemberAccessRecurse(nodep)) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }
    തർക്കം matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* സ്ഥിരം aRandp = VN_CAST(nodep->lhsp(), Rand);
        സ്ഥിരം AstRand* സ്ഥിരം bRandp = VN_CAST(nodep->rhsp(), Rand);
        എങ്കിൽ (!aRandp || !bRandp) തിരികെ അസത്യം;
        എങ്കിൽ (!aRandp->combinable(bRandp)) തിരികെ അസത്യം;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }
    തർക്കം matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* സ്ഥിരം aRandp = VN_CAST(nodep->fromp(), Rand);
        എങ്കിൽ (!aRandp) തിരികെ അസത്യം;
        എങ്കിൽ (aRandp->seedp()) തിരികെ അസത്യം;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }
    തർക്കം matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        എങ്കിൽ (സ്ഥിരം AstConsArray* സ്ഥിരം consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            എങ്കിൽ (!(m_doExpensive || m_params)) തിരികെ അസത്യം;
            എങ്കിൽ (!consp->is_Constant_വൃക്ഷം()) തിരികെ അസത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(nodep->lhsp(), Const)) {
            തിരികെ അസത്യം;
        }
        replaceWithSimulation(nodep);
        തിരികെ സത്യം;
    }
    പൂർണ്ണസംഖ്യ operandConcatMove(സ്ഥിരം AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        സ്ഥിരം AstConcat* സ്ഥിരം abConcp = VN_CAST(nodep->lhsp(), Concat);
        സ്ഥിരം AstConcat* സ്ഥിരം bcConcp = VN_CAST(nodep->rhsp(), Concat);
        എങ്കിൽ (!abConcp && !bcConcp) തിരികെ 0;
        എങ്കിൽ (bcConcp) {
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ap = nodep->lhsp();
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            എങ്കിൽ (VL_BITBIT_I(ap->width() + bp->width()) == 0) തിരികെ 2;  // Transform 2: to abConc
        } അല്ലെങ്കിൽ {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            എങ്കിൽ (VL_BITBIT_I(nodep->lhsp()->width()) != 0) തിരികെ 1;  // Transform 1: to bcConc
        }
        തിരികെ 0;  // ok
    }
    ശൂന്യം moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        എങ്കിൽ (operandConcatMove(nodep) > 1) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* സ്ഥിരം bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bp = bcConcp->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* സ്ഥിരം abConcp = പുതിയ AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            എങ്കിൽ (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(തള്ളുക_Deletep(bcConcp), bcConcp);
        } അല്ലെങ്കിൽ {
            AstConcat* സ്ഥിരം abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ap = abConcp->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bp = abConcp->rhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* സ്ഥിരം bcConcp = പുതിയ AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            എങ്കിൽ (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(തള്ളുക_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    ശൂന്യം visit(AstConst*) override {}  // Already constant

    ശൂന്യം visit(AstCell* nodep) override {
        എങ്കിൽ (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    ശൂന്യം visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_അക്കം(nodep, AstMatchMasked::fold(constp->അക്കം(), nodep->matchp()->varp()));
        }
    }

    ശൂന്യം visit(AstPin* nodep) override { iterateChildren(nodep); }

    ശൂന്യം replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* സ്ഥിരം newp = പുതിയ AstLogAnd{
            nodep->fileline(),
            പുതിയ AstLogOr{nodep->fileline(), പുതിയ AstLogNot{nodep->fileline(), lhsp}, rhsp},
            പുതിയ AstLogOr{nodep->fileline(),
                         പുതിയ AstLogNot{nodep->fileline(), rhsp->clone_വൃക്ഷം_Pure(അസത്യം)},
                         lhsp->clone_വൃക്ഷം_Pure(അസത്യം)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* സ്ഥിരം belowp = VN_AS(nodep->fromp(), Sel);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = belowp->fromp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsb1p = nodep->lsbp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_കണ്ണി_വ്യഞ്ജകം* newlsbp;
        എങ്കിൽ (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = പുതിയ AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(തള്ളുക_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(തള്ളുക_Deletep(lsb2p), lsb2p);
        } അല്ലെങ്കിൽ {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = widep->width() > lsb2p->width()
                                          ? പുതിയ AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = widep->width() > lsb1p->width()
                                          ? പുതിയ AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = പുതിയ AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* സ്ഥിരം newp = പുതിയ AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* സ്ഥിരം conp = VN_AS(nodep->fromp(), Concat);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം conLhsp = conp->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം conRhsp = conp->rhsp();
        എങ്കിൽ (static_cast<പൂർണ്ണസംഖ്യ>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* സ്ഥിരം newp
                = പുതിയ AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } അല്ലെങ്കിൽ എങ്കിൽ (static_cast<പൂർണ്ണസംഖ്യ>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* സ്ഥിരം newp
                = പുതിയ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } അല്ലെങ്കിൽ {
            // Yuk, split between the two
            conRhsp->unlinkFrBack();
            conLhsp->unlinkFrBack();
            AstConcat* സ്ഥിരം newp
                = പുതിയ AstConcat{nodep->fileline(),
                                പുതിയ AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->width() + 1},
                                പുതിയ AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->width() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    തർക്കം operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* സ്ഥിരം repp = VN_AS(nodep->fromp(), Replicate);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = repp->srcp();
        സ്ഥിരം AstConst* സ്ഥിരം lsbp = VN_CAST(nodep->lsbp(), Const);
        എങ്കിൽ (!lsbp) തിരികെ അസത്യം;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        എങ്കിൽ ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            തിരികെ അസത്യം;
        }
        //
        fromp->unlinkFrBack();
        AstSel* സ്ഥിരം newp = പുതിയ AstSel{
            nodep->fileline(), fromp,
            പുതിയ AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }
    തർക്കം operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* സ്ഥിരം rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം from2p = rep2p->srcp();
        സ്ഥിരം AstConst* സ്ഥിരം cnt1p = VN_CAST(nodep->countp(), Const);
        എങ്കിൽ (!cnt1p) തിരികെ അസത്യം;
        സ്ഥിരം AstConst* സ്ഥിരം cnt2p = VN_CAST(rep2p->countp(), Const);
        എങ്കിൽ (!cnt2p) തിരികെ അസത്യം;
        //
        from2p->unlinkFrBack();
        AstReplicate* സ്ഥിരം newp
            = പുതിയ AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }
    തർക്കം operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_കണ്ണി_വ്യഞ്ജകം* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_കണ്ണി_വ്യഞ്ജകം* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        എങ്കിൽ (VN_IS(from1p, Replicate)) {
            സ്ഥിരം AstConst* സ്ഥിരം cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            എങ്കിൽ (!cnt1p) തിരികെ അസത്യം;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        എങ്കിൽ (VN_IS(from2p, Replicate)) {
            സ്ഥിരം AstConst* സ്ഥിരം cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            എങ്കിൽ (!cnt2p) തിരികെ അസത്യം;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        എങ്കിൽ (!operandsSame(from1p, from2p)) തിരികെ അസത്യം;
        //
        from1p->unlinkFrBack();
        AstReplicate* സ്ഥിരം newp = പുതിയ AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }
    ശൂന്യം replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_കണ്ണി_Biop* സ്ഥിരം fromp = VN_AS(nodep->fromp()->unlinkFrBack(), കണ്ണി_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bilhsp = fromp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            പുതിയ AstSel{nodep->fileline(), bilhsp, lsbp->clone_വൃക്ഷം_Pure(സത്യം), nodep->widthConst()});
        fromp->rhsp(പുതിയ AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_കണ്ണി_Uniop* സ്ഥിരം fromp = VN_AS(nodep->fromp()->unlinkFrBack(), കണ്ണി_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(പുതിയ AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        എങ്കിൽ (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        എങ്കിൽ (VN_IS(nodep->fromp(), Const)) {  // It did.
            എങ്കിൽ (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } അല്ലെങ്കിൽ {
                Ast_കണ്ണി* സ്ഥിരം fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            സ്ഥിരം AstConsArray* സ്ഥിരം consp = VN_AS(nodep->fromp(), ConsArray);
            സ്ഥിരം uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം itemp = consp->valuep(bit);
            എങ്കിൽ (VN_IS(itemp, Const)) {
                സ്ഥിരം V3_സംഖ്യ& അക്കം = VN_AS(itemp, Const)->അക്കം();
                VL_DO_DANGLING(replace_അക്കം(nodep, അക്കം), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_വൃക്ഷം(അസത്യം));
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        }
        m_selp = nullptr;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    ശൂന്യം visit(AstSliceSel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        എങ്കിൽ (!m_required) തിരികെ;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    ശൂന്യം visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        എങ്കിൽ (സ്ഥിരം AstConsStruct* സ്ഥിരം consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = consp->memberValuep(nodep->name());
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_അക്കം(nodep, constp->അക്കം()), nodep);
            } അല്ലെങ്കിൽ {
                nodep->replaceWith(valuep->clone_വൃക്ഷം(അസത്യം));
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        }
    }

    ശൂന്യം visit(AstCAwait* nodep) override {
        m_hasJumpDelay = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        തർക്കം did = അസത്യം;
        എങ്കിൽ (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_കണ്ണി* സ്ഥിരം valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            സ്ഥിരം AstConsArray* സ്ഥിരം consp = VN_CAST(valuep, ConsArray);
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            എങ്കിൽ (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                എങ്കിൽ (operandConst(valuep)) {
                    സ്ഥിരം V3_സംഖ്യ& അക്കം = VN_AS(valuep, Const)->അക്കം();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    സ്ഥിരം string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_അക്കം(nodep, അക്കം, origParamName), nodep);
                    did = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_അക്കം(nodep, itemp->അക്കം()), nodep);
                    did = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_കണ്ണി* സ്ഥിരം newp = valuep->clone_വൃക്ഷം(അസത്യം);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    did = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_കണ്ണി* സ്ഥിരം newp = valuep->clone_വൃക്ഷം(അസത്യം);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    did = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_കണ്ണി* സ്ഥിരം newp = valuep->clone_വൃക്ഷം(അസത്യം);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    did = സത്യം;
                }
            }
        }
        എങ്കിൽ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!Ast_കണ്ണി::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    ശൂന്യം visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        തർക്കം did = അസത്യം;
        എങ്കിൽ (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            സ്ഥിരം AstEnumItem* സ്ഥിരം itemp = nodep->itemp();
            എങ്കിൽ (m_recursing_Enum_ഇനങ്ങൾ.എണ്ണം(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } അല്ലെങ്കിൽ {
                m_recursing_Enum_ഇനങ്ങൾ.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursing_Enum_ഇനങ്ങൾ.erase(itemp);
            }
            എങ്കിൽ (AstConst* സ്ഥിരം valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                സ്ഥിരം V3_സംഖ്യ& അക്കം = valuep->അക്കം();
                VL_DO_DANGLING(replace_അക്കം(nodep, അക്കം), nodep);
                did = സത്യം;
            }
        }
        എങ്കിൽ (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    തർക്കം only_Sen_Item_In_Sen_വൃക്ഷം(സ്ഥിരം AstSenItem* nodep) {
        // Only one if it's not in a list
        തിരികെ (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    ശൂന്യം visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            എങ്കിൽ (nodep->isClocked()) {  // A constant can never get a pos/negedge
                എങ്കിൽ (only_Sen_Item_In_Sen_വൃക്ഷം(nodep)) {
                    എങ്കിൽ (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            പുതിയ AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } അല്ലെങ്കിൽ {
                        nodep->replaceWith(പുതിയ AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                } അല്ലെങ്കിൽ {
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } അല്ലെങ്കിൽ {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(പുതിയ AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_കണ്ണി* സ്ഥിരം sensp = nodep->sensp();
            Ast_കണ്ണി* lastSensp = sensp;
            തർക്കം invert = അസത്യം;
            ആകുമ്പോൾ (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            എങ്കിൽ (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(sensp), sensp);
        }
    }

    വർഗ്ഗം SenItemCmp final {
        സ്ഥിതം പൂർണ്ണസംഖ്യ cmp(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* ap, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* bp) {
            സ്ഥിരം VNType aType = ap->type();
            സ്ഥിരം VNType bType = bp->type();
            എങ്കിൽ (aType != bType) തിരികെ static_cast<പൂർണ്ണസംഖ്യ>(bType) - static_cast<പൂർണ്ണസംഖ്യ>(aType);

            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം aRefp = VN_CAST(ap, VarRef)) {
                സ്ഥിരം AstVarRef* സ്ഥിരം bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                എങ്കിൽ (aRefp->name() < bRefp->name()) തിരികെ -1;
                എങ്കിൽ (aRefp->name() > bRefp->name()) തിരികെ 1;
                // But might be same name with different scopes
                എങ്കിൽ (aRefp->varScopep() < bRefp->varScopep()) തിരികെ -1;
                എങ്കിൽ (aRefp->varScopep() > bRefp->varScopep()) തിരികെ 1;
                // Or rarely, different data types
                എങ്കിൽ (aRefp->dtypep() < bRefp->dtypep()) തിരികെ -1;
                എങ്കിൽ (aRefp->dtypep() > bRefp->dtypep()) തിരികെ 1;
                തിരികെ 0;
            }

            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം aConstp = VN_CAST(ap, Const)) {
                സ്ഥിരം AstConst* സ്ഥിരം bConstp = VN_AS(bp, Const);
                എങ്കിൽ (aConstp->അക്കം().isLtXZ(bConstp->അക്കം())) തിരികെ -1;
                എങ്കിൽ (bConstp->അക്കം().isLtXZ(aConstp->അക്കം())) തിരികെ 1;
                തിരികെ 0;
            }

            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം aBiOpp = VN_CAST(ap, കണ്ണി_Biop)) {
                സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം bBiOpp = VN_AS(bp, കണ്ണി_Biop);
                // Compare RHSs first, as LHS might be const
                എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) തിരികെ c;
                തിരികെ cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            എങ്കിൽ (സ്ഥിരം AstCMethodHard* സ്ഥിരം aCallp = VN_CAST(ap, CMethodHard)) {
                സ്ഥിരം AstCMethodHard* സ്ഥിരം bCallp = VN_AS(bp, CMethodHard);
                എങ്കിൽ (aCallp->method() < bCallp->method()) തിരികെ -1;
                എങ്കിൽ (aCallp->method() > bCallp->method()) തിരികെ 1;
                എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ c = cmp(aCallp->fromp(), bCallp->fromp())) തിരികെ c;
                സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* aPinsp = aCallp->pinsp();
                സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* bPinsp = bCallp->pinsp();
                ആകുമ്പോൾ (aPinsp && bPinsp) {
                    എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ c = cmp(aPinsp, bPinsp)) തിരികെ c;
                    aPinsp = VN_AS(aPinsp->nextp(), കണ്ണി_വ്യഞ്ജകം);
                    bPinsp = VN_AS(bPinsp->nextp(), കണ്ണി_വ്യഞ്ജകം);
                }
                തിരികെ aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            തിരികെ 0;
        }

    പൊതു:
        തർക്കം operator()(സ്ഥിരം AstSenItem* lhsp, സ്ഥിരം AstSenItem* rhsp) സ്ഥിരം {
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lSensp = lhsp->sensp();
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rSensp = rhsp->sensp();
            എങ്കിൽ (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ c = cmp(lSensp, rSensp)) തിരികെ c < 0;
            } അല്ലെങ്കിൽ എങ്കിൽ (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                തിരികെ rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            തിരികെ lhsp->edgeType() < rhsp->edgeType();
        }
    };

    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_doExpensive) {
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
            വേണ്ടി (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                സ്ഥിരം SenItemCmp cmp;
                എങ്കിൽ (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    വേണ്ടി (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.തള്ളുക_back(senp);
                    }
                    stable_sort(vec.begin(), vec.end(), SenItemCmp());
                    വേണ്ടി (സ്ഥിരം auto& ip : vec) ip->unlinkFrBack();
                    വേണ്ടി (സ്ഥിരം auto& ip : vec) nodep->addSensesp(ip);
                    നിർത്തുക;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            വേണ്ടി (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                എങ്കിൽ (!nextp) നിർത്തുക;
                AstSenItem* സ്ഥിരം lItemp = senp;
                AstSenItem* സ്ഥിരം rItemp = nextp;
                സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lSenp = lItemp->sensp();
                സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rSenp = rItemp->sensp();
                എങ്കിൽ (!lSenp || !rSenp) തുടരുക;

                എങ്കിൽ (lSenp->same_Gate_വൃക്ഷം(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    എങ്കിൽ (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    എങ്കിൽ (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(തള്ളുക_Deletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->timingControlp()) m_hasJumpDelay = സത്യം;
        എങ്കിൽ (m_doNConst && replace_കണ്ണി_Assign(nodep)) തിരികെ;
    }
    ശൂന്യം visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ശൂന്യം visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    ശൂന്യം visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_doNConst && replace_കണ്ണി_Assign(nodep)) തിരികെ;
        // Process containing this AssignW as single body statement
        AstAlways* സ്ഥിരം procp = VN_CAST(nodep->backp(), Always);
        എങ്കിൽ (!procp || procp->stmtsp() != nodep || nodep->nextp()) തിരികെ;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(nodep->lhsp(), VarRef);
        എങ്കിൽ (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->അക്കം().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            AstInitial* സ്ഥിരം newinitp = പുതിയ AstInitial{flp, പുതിയ AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(തള്ളുക_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_കണ്ണി* സ്ഥിരം initvaluep = exprp->clone_വൃക്ഷം(അസത്യം);
            varrefp->varp()->isConst(സത്യം);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    ശൂന്യം visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!m_doNConst) തിരികെ;
        സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        എങ്കിൽ (!bdtypep) തിരികെ;
        എങ്കിൽ (!bdtypep->isZeroInit()) തിരികെ;
        AstConst* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), V3_സംഖ്യ{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* സ്ഥിരം fromp = VN_CAST(nodep->fromp(), ConsArray);
        എങ്കിൽ (!fromp) തിരികെ;
        // Fold the outermost dimension, converting the elements for any dimension under it
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        സ്ഥിരം തർക്കം reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        സ്ഥിരം uint64_t lastIndex = toArrayp->elementsConst() - 1;
        Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = toArrayp->subDTypep();
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        സ്ഥിരം auto convert = [&](Ast_കണ്ണി_വ്യഞ്ജകം* valuep) -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            valuep->unlinkFrBack();
            എങ്കിൽ (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) തിരികെ valuep;
            തിരികെ പുതിയ AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* സ്ഥിരം newp
            = പുതിയ AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : nullptr};
        വേണ്ടി (സ്ഥിരം AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? lastIndex - itemp->index() : itemp->index(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    ശൂന്യം visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_കണ്ണി_Stream* streamp = nullptr;
        തർക്കം isReverse = അസത്യം;
        എങ്കിൽ (AstStreamL* സ്ഥിരം streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = സത്യം;  // StreamL reverses the operation
        } അല്ലെങ്കിൽ എങ്കിൽ (AstStreamR* സ്ഥിരം streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = അസത്യം;  // StreamR doesn't reverse the operation
        }
        എങ്കിൽ (streamp) {
            Ast_കണ്ണി_വ്യഞ്ജകം* srcp = streamp->lhsp();
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം srcDTypep = srcp->dtypep()->skipRefp();
            Ast_കണ്ണി_DType* സ്ഥിരം dstDTypep = nodep->dtypep()->skipRefp();
            എങ്കിൽ (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                പൂർണ്ണസംഖ്യ blockSize = 1;
                എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    എങ്കിൽ (VL_UNLIKELY(blockSize <= 0)) {
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
                പൂർണ്ണസംഖ്യ srcElementBits = 0;
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                പൂർണ്ണസംഖ്യ dstElementBits = 0;
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* newp = പുതിയ AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(streamp), streamp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }
        }
    }
    ശൂന്യം visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        എങ്കിൽ (AstConcat* സ്ഥിരം concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            AstRelease* സ്ഥിരം newLp = പുതിയ AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* സ്ഥിരം newRp = പുതിയ AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_doNConst) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->condp(), Const)) {
                Ast_കണ്ണി* keepp = nullptr;
                Ast_കണ്ണി* delp = nullptr;
                എങ്കിൽ (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } അല്ലെങ്കിൽ എങ്കിൽ (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } അല്ലെങ്കിൽ {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    തിരികെ;
                }
                എങ്കിൽ (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    തിരികെ;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                എങ്കിൽ (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } അല്ലെങ്കിൽ {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_വൃക്ഷം();
            } അല്ലെങ്കിൽ എങ്കിൽ (!Ast_കണ്ണി::afterCommentp(nodep->thensp())
                       && !Ast_കണ്ണി::afterCommentp(nodep->elsesp())) {
                എങ്കിൽ (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, സത്യം);
                    Ast_കണ്ണി_Stmt* സ്ഥിരം newp
                        = പുതിയ Ast_Stmt_വ്യഞ്ജകം{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                } അല്ലെങ്കിൽ {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (!Ast_കണ്ണി::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = nodep->condp();
                Ast_കണ്ണി* സ്ഥിരം elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                എങ്കിൽ (nodep->thensp()) {  // Must have been comment
                    തള്ളുക_Deletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(പുതിയ AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } അല്ലെങ്കിൽ എങ്കിൽ (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp
                    = VN_AS(nodep->condp(), കണ്ണി_Uniop)->lhsp()->unlinkFrBackWithNext();
                Ast_കണ്ണി* സ്ഥിരം thensp = nodep->thensp()->unlinkFrBackWithNext();
                Ast_കണ്ണി* സ്ഥിരം elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* സ്ഥിരം ifp = പുതിയ AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_കണ്ണി_Assign* സ്ഥിരം thensp = VN_AS(nodep->thensp(), കണ്ണി_Assign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_കണ്ണി_Assign* സ്ഥിരം elsesp = VN_AS(nodep->elsesp(), കണ്ണി_Assign);
                thensp->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = nodep->condp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം truep = thensp->rhsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(പുതിയ AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (അസത്യം  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_കണ്ണി_If* സ്ഥിരം lowerIfp = VN_AS(nodep->thensp(), കണ്ണി_If);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = nodep->condp()->unlinkFrBack();
                Ast_കണ്ണി* സ്ഥിരം lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(പുതിയ AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(തള്ളുക_Deletep(lowerIfp), lowerIfp);
            } അല്ലെങ്കിൽ {
                // Optimizations that don't reform the IF itself
                എങ്കിൽ (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    ശൂന്യം visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        എങ്കിൽ (stmtDisplayDisplay(nodep)) തിരികെ;
    }
    തർക്കം stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        എങ്കിൽ (!m_modp) തിരികെ അസത്യം;  // Don't optimize under single statement
        AstDisplay* സ്ഥിരം prevp = VN_CAST(nodep->backp(), Display);
        എങ്കിൽ (!prevp) തിരികെ അസത്യം;
        എങ്കിൽ (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            തിരികെ അസത്യം;
        എങ്കിൽ ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_വൃക്ഷം(nodep->filep())))
            തിരികെ അസത്യം;
        എങ്കിൽ (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            തിരികെ അസത്യം;
        AstSFormatF* സ്ഥിരം pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* സ്ഥിരം nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        എങ്കിൽ (nformatp->scopeNamep() && pformatp->scopeNamep()) തിരികെ അസത്യം;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        എങ്കിൽ (!m_doCpp && pformatp->exprsp()) തിരികെ അസത്യം;
        എങ്കിൽ (!m_doCpp && nformatp->exprsp()) തിരികെ അസത്യം;
        എങ്കിൽ (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) തിരികെ അസത്യം;
        എങ്കിൽ (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) തിരികെ അസത്യം;
        // Avoid huge merges
        സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ DISPLAY_MAX_MERGE_LENGTH = 500;
        എങ്കിൽ (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            തിരികെ അസത്യം;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        എങ്കിൽ (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        എങ്കിൽ (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        എങ്കിൽ (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        എങ്കിൽ (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        എങ്കിൽ (AstScopeName* സ്ഥിരം scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        തിരികെ സത്യം;
    }
    ശൂന്യം visit(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterateAndNextNull(nodep->exprp());
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        എങ്കിൽ (nodep->വ്യഞ്ജകം_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            എങ്കിൽ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->അക്കം().isFromString()) {
                AstConst* സ്ഥിരം fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->അക്കം().toString());
                nodep->വ്യഞ്ജകം_Format_2(അസത്യം);
                VL_DO_DANGLING(തള്ളുക_Deletep(fmtp), fmtp);
            }
        }
        എങ്കിൽ (nodep->വ്യഞ്ജകം_Format_2()) തിരികെ;  // Runtime, unfortunately  (unless constify format later)
        തർക്കം anyconst = അസത്യം;
        വേണ്ടി (Ast_കണ്ണി* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* സ്ഥിരം fargp = VN_CAST(argp, SFormatArg);
            Ast_കണ്ണി* സ്ഥിരം subargp = fargp ? fargp->exprp() : argp;
            എങ്കിൽ (VN_IS(subargp, Const)) {
                anyconst = സത്യം;
                നിർത്തുക;
            }
        }
        എങ്കിൽ (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            തർക്കം inPct = അസത്യം;
            Ast_കണ്ണി* argp = nodep->exprsp();
            സ്ഥിരം string text = nodep->text();
            വേണ്ടി (അക്ഷരം ch : text) {
                എങ്കിൽ (!inPct && ch == '%') {
                    inPct = സത്യം;
                    fmt = ch;
                } അല്ലെങ്കിൽ എങ്കിൽ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } അല്ലെങ്കിൽ എങ്കിൽ (inPct) {
                    inPct = അസത്യം;
                    fmt += ch;
                    ch = std::tolower(ch);
                    തിരഞ്ഞെടുപ്പ് (ch) {
                    അവസ്ഥ '%': നിർത്തുക;  // %% - still %%
                    അവസ്ഥ 'm': നിർത്തുക;  // %m - still %m - auto insert "name"
                    അവസ്ഥ 'l': നിർത്തുക;  // %l - still %l - auto insert "library"
                    അവസ്ഥ 't':  // %t - don't know $timeformat so can't constify
                        എങ്കിൽ (argp) argp = argp->nextp();
                        നിർത്തുക;
                    സ്ഥിരസ്ഥിതി:  // Most operators, just move to next argument
                        എങ്കിൽ (argp) {
                            Ast_കണ്ണി* സ്ഥിരം nextp = argp->nextp();
                            AstSFormatArg* സ്ഥിരം fargp = VN_CAST(argp, SFormatArg);
                            Ast_കണ്ണി* സ്ഥിരം subargp = fargp ? fargp->exprp() : argp;
                            സ്ഥിരം VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            എങ്കിൽ (VN_IS(subargp, Const)) {  // Convert it
                                സ്ഥിരം string out
                                    = formatAttr.isEnum()
                                          ? const_അക്കം_V(subargp).displayedEnum(fargp, fmt)
                                          : const_അക്കം_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(തള്ളുക_Deletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        നിർത്തുക;
                    }  // switch
                    newFormat += fmt;
                } അല്ലെങ്കിൽ {
                    newFormat += ch;
                }
            }
            എങ്കിൽ (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        എങ്കിൽ (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            തിരികെ;
        }
        എങ്കിൽ (VN_IS(nodep->backp(), കണ്ണി_Assign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
    }
    ശൂന്യം visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!m_doNConst) തിരികെ;
        എങ്കിൽ (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* സ്ഥിരം fmtp = nodep->fmtp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
            AstBasicDType* സ്ഥിരം basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            എങ്കിൽ (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* സ്ഥിരം strp
                    = പുതിയ AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(പുതിയ AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        എങ്കിൽ (nodep->recursive()) m_underRecFunc = സത്യം;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = സത്യം;  // As don't analyze inside tasks for timing controls
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = സത്യം;  // As don't analyze inside tasks for timing controls
    }
    ശൂന്യം visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_കണ്ണി_FTask_Ref*>(nodep));
        എങ്കിൽ (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    ശൂന്യം visit(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        സ്ഥിരം തർക്കം oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = അസത്യം;
        m_hasLoopTest = അസത്യം;
        iterateChildren(nodep);
        സ്ഥിരം തർക്കം thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        എങ്കിൽ (സ്ഥിരം AstLoopTest* സ്ഥിരം testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            എങ്കിൽ (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
        }
        // If last statement always breaks, repalce loop with body
        എങ്കിൽ (Ast_കണ്ണി* lastp = nodep->stmtsp()) {
            ആകുമ്പോൾ (Ast_കണ്ണി* സ്ഥിരം nextp = lastp->nextp()) lastp = nextp;
            എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(lastp, LoopTest)) {
                എങ്കിൽ (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(തള്ളുക_Deletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
            }
        }
        // Warn on infinite loop
        എങ്കിൽ (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, സത്യം);  // Complain once
        }
    }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        എങ്കിൽ (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }

        m_hasLoopTest = സത്യം;

        // If always breaks, subsequent statements are dead code, delete them
        എങ്കിൽ (nodep->condp()->isZero()) {
            Ast_കണ്ണി* സ്ഥിരം nextp = nodep->nextp();
            എങ്കിൽ (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(തള്ളുക_Deletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            തിരികെ;
        }

        എങ്കിൽ (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    ശൂന്യം visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    ശൂന്യം visit(Ast_കണ്ണി_Pre_Sel*) override {}

    // Ignored, can eliminate early
    ശൂന്യം visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_doNConst) VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        എങ്കിൽ (!nodep->exprp()) {
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        // Remove if expression is trivially pure
        എങ്കിൽ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        // Remove calls to empty functions
        എങ്കിൽ (സ്ഥിരം AstCCall* സ്ഥിരം callp = VN_CAST(nodep->exprp(), CCall)) {
            സ്ഥിരം AstCFunc* സ്ഥിരം funcp = callp->funcp();
            എങ്കിൽ (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
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
    ശൂന്യം visit(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    ശൂന്യം visit(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        എങ്കിൽ (nodep->nextp() && canDeleteStatements()) {
            തള്ളുക_Deletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        എങ്കിൽ (!nodep->nextp()) {
            വേണ്ടി (Ast_കണ്ണി* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                എങ്കിൽ (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                    തിരികെ;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                എങ്കിൽ (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) നിർത്തുക;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = സത്യം;
    }

    ശൂന്യം visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        എങ്കിൽ (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }

        // If no JumpGo points to this node, replace it with its body
        എങ്കിൽ (!m_usedJumpBlocks.എണ്ണം(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    വൃക്ഷം_SKIP_VISIT("ArraySel");
    വൃക്ഷം_SKIP_VISIT("CAwait");

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
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        // Default: Just iterate
        എങ്കിൽ (m_required) {
            എങ്കിൽ (VN_IS(nodep, കണ്ണി_DType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstCellRef* സ്ഥിരം crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(തള്ളുക_Deletep(crp), crp);
                }
                തിരികെ;
            } അല്ലെങ്കിൽ {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->isTimingControl()) m_hasJumpDelay = സത്യം;
            // Calculate the width of this operation
            എങ്കിൽ (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);
        }
    }

പൊതു:
    // Processing Mode Enum
    എണ്ണൽ ProcMode : uint8_t {
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
    ConstVisitor(ProcMode pmode, തർക്കം globalPass)
        : m_globalPass{globalPass}
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_global_Pass_അക്കം++)) : ""} {
        // clang-format off
        തിരഞ്ഞെടുപ്പ് (pmode) {
        അവസ്ഥ PROC_PARAMS_NOWARN:  m_doV = സത്യം;  m_doNConst = സത്യം; m_params = സത്യം;
                                  m_required = അസത്യം; നിർത്തുക;
        അവസ്ഥ PROC_PARAMS:         m_doV = സത്യം;  m_doNConst = സത്യം; m_params = സത്യം;
                                  m_required = സത്യം; നിർത്തുക;
        അവസ്ഥ PROC_GENERATE:       m_doV = സത്യം;  m_doNConst = സത്യം; m_params = സത്യം;
                                  m_required = സത്യം; m_doGenerate = സത്യം; നിർത്തുക;
        അവസ്ഥ PROC_LIVE:           നിർത്തുക;
        അവസ്ഥ PROC_V_WARN:         m_doV = സത്യം;  m_doNConst = സത്യം; m_warn = സത്യം; m_convertLogicToBit = സത്യം; നിർത്തുക;
        അവസ്ഥ PROC_V_NOWARN:       m_doV = സത്യം;  m_doNConst = സത്യം; നിർത്തുക;
        അവസ്ഥ PROC_V_EXPENSIVE:    m_doV = സത്യം;  m_doNConst = സത്യം; m_doExpensive = സത്യം; നിർത്തുക;
        അവസ്ഥ PROC_CPP:            m_doV = അസത്യം; m_doNConst = സത്യം; m_doCpp = സത്യം; നിർത്തുക;
        സ്ഥിരസ്ഥിതി:                  v3fatalSrc("Bad case"); നിർത്തുക;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        എങ്കിൽ (m_doCpp) {
            എങ്കിൽ (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } അല്ലെങ്കിൽ {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_stat_Cond_വ്യഞ്ജകം_Redundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_വ്യഞ്ജകം_Redundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_കണ്ണി* mainAcceptEdit(Ast_കണ്ണി* nodep) {
        VIsCached::clear_Cache_വൃക്ഷം();  // Avoid using any stale isPure
        // Operate starting at a random place
        തിരികെ iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_അക്കം = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
ശൂന്യം V3Const::constifyParamsEdit(Ast_കണ്ണി* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ അസത്യം};
    // cppcheck-suppress constVariablePointer // edited below
    എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        എങ്കിൽ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } അല്ലെങ്കിൽ {
        (ശൂന്യം)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
ശൂന്യം V3Const::constifyParamsNoWarnEdit(Ast_കണ്ണി* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ അസത്യം};
    // cppcheck-suppress constVariablePointer // edited below
    എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        എങ്കിൽ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } അല്ലെങ്കിൽ {
        (ശൂന്യം)visitor.mainAcceptEdit(nodep);
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
ശൂന്യം V3Const::constifyGenerateParamsEdit(Ast_കണ്ണി* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ അസത്യം};
    // cppcheck-suppress constVariablePointer // edited below
    എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        എങ്കിൽ (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } അല്ലെങ്കിൽ {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

ശൂന്യം V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ സത്യം};
        (ശൂന്യം)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("const", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ സത്യം};
        (ശൂന്യം)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("const_cpp", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

Ast_കണ്ണി* V3Const::constifyEdit(Ast_കണ്ണി* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ അസത്യം};
    nodep = visitor.mainAcceptEdit(nodep);
    തിരികെ nodep;
}

Ast_കണ്ണി* V3Const::constifyEditCpp(Ast_കണ്ണി* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ അസത്യം};
    nodep = visitor.mainAcceptEdit(nodep);
    തിരികെ nodep;
}

ശൂന്യം V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ സത്യം};
        (ശൂന്യം)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("const", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ സത്യം};
        (ശൂന്യം)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("const", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

Ast_കണ്ണി* V3Const::constifyExpensiveEdit(Ast_കണ്ണി* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ അസത്യം};
    nodep = visitor.mainAcceptEdit(nodep);
    തിരികെ nodep;
}
