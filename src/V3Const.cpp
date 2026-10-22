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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Const.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Simulate.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3String.h"
#અન્તર્ભાવ "V3UniqueNames.h"
#અન્તર્ભાવ "V3Width.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <type_traits>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

#define વૃક્ષ_SKIP_VISIT(...)
#define TREEOP1(...)
#define TREEOPA(...)
#define TREEOP(...)
#define TREEOPS(...)
#define TREEOPC(...)
#define TREEOPV(...)

//######################################################################
// Utilities

સ્થિર તાર્કિક isConst(અચલ Ast_ગાંઠ* nodep, uint64_t v) {
    અચલ AstConst* અચલ constp = VN_CAST(nodep, Const);
    પાછા constp && constp->toUQuad() == v;
}

template <typename T>
સ્થિર typename std::enable_if<std::is_integral<T>::કિંમત, તાર્કિક>::type isPow2(T val) {
    પાછા (val & (val - 1)) == 0;
}

સ્થિર પૂર્ણાંક ગણતરી_Trailing_Zeroes(uint64_t val) {
    UASSERT(val, "countTrailingZeroes argument must be non-zero");
#જો defined(__GNUC__) && !defined(VL_NO_BUILTINS)
    પાછા __builtin_ctzll(val);
#નહીંતર
    પૂર્ણાંક bit = 0;
    val = ~val;
    જ્યારે (val & 1) {
        ++bit;
        val >>= 1;
    }
    પાછા bit;
#endif
}

// Allocates a unique id for each AstNode it is applied to, held in user4.
વર્ગ VNIdAllocator final {
    // NODE STATE
    // AstNode::user4p -> size_t. Unique id of the node (0/nullptr if not allocated yet)

    // MEMBERS
    // Claimed lazily on first use TODO: fix conflict with V3Param user4 slot
    std::unique_ptr<VNUser4InUse> m_inuser4p;
    size_t m_nextId = 0;  // Id allocated most recently

જાહેર:
    // Return the unique id of the given node, allocating a new one if it has none yet
    size_t operator()(Ast_ગાંઠ* nodep) {
        જો (!m_inuser4p) m_inuser4p = std::make_unique<VNUser4InUse>();  // Claim on first use
        જો (!nodep->user4p()) nodep->user4p(reinterpret_cast<શૂન્ય*>(++m_nextId));
        પાછા reinterpret_cast<size_t>(nodep->user4p());
    }
};

// This visitor can be used in the post-expanded Ast from V3Expand, where the Ast satisfies:
// - Constants are 64 bit at most (because words are accessed via AstWordSel)
// - Variables are scoped.
વર્ગ Const_Bit_Op_વૃક્ષ_Visitor final : જાહેર VNVisitorConst {
    // TYPES

    // Holds a node to be added as a term in the reduction tree, it's equivalent op count, and a
    // bool indicating if the term is clean (0/1 value, or if the top bits might be dirty)
    using ResultTerm = std::tuple<Ast_ગાંઠ_પદાવલિ*, unsigned, તાર્કિક>;

    વર્ગ LeafInfo final {  // Leaf node (either AstConst or AstVarRef)
        // MEMBERS
        તાર્કિક m_polarity = સાચું;  // Invert result due to NOT
        પૂર્ણાંક m_lsb = 0;  // LSB of actually used bit of m_refp->varp()
        પૂર્ણાંક m_msb = 0;  // MSB of actually used bit of m_refp->varp()
        પૂર્ણાંક m_wordIdx = -1;  // -1 means AstWordSel is not used.
        AstVarRef* m_refp = nullptr;  // Leaf's variable reference
        અચલ AstConst* m_constp = nullptr;  // Leaf's constant

    જાહેર:
        // CONSTRUCTORS
        LeafInfo() = મૂળભૂત;
        LeafInfo(અચલ LeafInfo& other) = મૂળભૂત;
        LeafInfo& operator=(અચલ LeafInfo& other) = મૂળભૂત;
        LeafInfo(LeafInfo&& other) = મૂળભૂત;
        LeafInfo& operator=(LeafInfo&& other) = મૂળભૂત;
        explicit LeafInfo(પૂર્ણાંક lsb)
            : m_lsb{lsb} {}

        // METHODS
        શૂન્ય setLeaf(AstVarRef* refp) {
            UASSERT_OBJ(!m_refp && !m_constp, refp, "Must be called just once");
            m_refp = refp;
            m_msb = refp->varp()->widthMin() - 1;
        }
        શૂન્ય setLeaf(અચલ AstConst* constp) {
            UASSERT_OBJ(!m_refp && !m_constp, constp, "Must be called just once");
            m_constp = constp;
            m_msb = constp->widthMin() - 1;
        }
        // updateBitRange(), limitBitRangeToLsb(), and polarity() must be called during ascending
        // back to the root.
        શૂન્ય updateBitRange(પૂર્ણાંક newLsb, પૂર્ણાંક newMsb) {
            જો ((m_lsb <= m_msb && newLsb > newMsb) || (m_lsb > m_msb && m_lsb < newLsb)) {
                // When the new bit range is out of m_refp, clear polarity because nodes below is
                // shifted out to zero.
                // This kind of clear may happen several times. e.g. (!(1'b1 >> 1)) >> 1
                polarity(સાચું);
            }
            m_lsb = newLsb;
            m_msb = newMsb;
        }
        શૂન્ય updateBitRange(અચલ AstCCast* castp) {
            updateBitRange(m_lsb, std::min(m_msb, m_lsb + castp->width() - 1));
        }
        શૂન્ય updateBitRange(અચલ AstShiftR* shiftp) {
            updateBitRange(m_lsb + VN_AS(shiftp->rhsp(), Const)->toUInt(), m_msb);
        }
        શૂન્ય limitBitRangeToLsb() { updateBitRange(m_lsb, std::min(m_msb, m_lsb)); }
        પૂર્ણાંક wordIdx() અચલ { પાછા m_wordIdx; }
        શૂન્ય wordIdx(પૂર્ણાંક i) { m_wordIdx = i; }
        તાર્કિક polarity() અચલ { પાછા m_polarity; }
        શૂન્ય polarity(તાર્કિક p) { m_polarity = p; }

        AstVarRef* refp() અચલ { પાછા m_refp; }
        અચલ AstConst* constp() અચલ { પાછા m_constp; }
        તાર્કિક missingWordSel() અચલ {
            // When V3Expand is skipped, WordSel is not inserted.
            પાછા m_refp->isWide() && m_wordIdx == -1;
        }
        પૂર્ણાંક lsb() અચલ { પાછા m_lsb; }

        પૂર્ણાંક msb() અચલ { પાછા std::min(m_msb, varWidth() - 1); }
        પૂર્ણાંક varWidth() અચલ {
            UASSERT(m_refp, "m_refp should be set");
            અચલ પૂર્ણાંક width = m_refp->varp()->widthMin();
            જો (!m_refp->isWide()) {
                UASSERT_OBJ(m_wordIdx == -1, m_refp, "Bad word index into non-wide");
                પાછા width;
            } નહીંતર {
                જો (missingWordSel()) પાછા width;
                UASSERT_OBJ(m_wordIdx >= 0, m_refp, "Bad word index into wide");
                અચલ પૂર્ણાંક bitsInMSW = VL_BITBIT_E(width) ? VL_BITBIT_E(width) : VL_EDATASIZE;
                પાછા m_wordIdx == m_refp->width_શબ્દો() - 1 ? bitsInMSW : VL_EDATASIZE;
            }
        }
    };

    સંરચના BitPolarityEntry final {  // Found bit polarity during iterate()
        LeafInfo m_info;  // Leaf (variable or constant) bit polarity was found on
        તાર્કિક m_polarity = ખોટું;  // Polarity the bit must have to match
        પૂર્ણાંક m_bit = 0;  // Bit index within the leaf that was tested
        BitPolarityEntry(અચલ LeafInfo& info, તાર્કિક pol, પૂર્ણાંક bit)
            : m_info{info}
            , m_polarity{pol}
            , m_bit{bit} {}
        BitPolarityEntry() = મૂળભૂત;
    };

    સંરચના Frozen_ગાંઠ_Info final {  // Context when a frozen node is found
        તાર્કિક m_polarity;  // Polarity the frozen node must match
        પૂર્ણાંક m_lsb;  // LSB position of the frozen node
        તાર્કિક operator<(અચલ Frozen_ગાંઠ_Info& other) અચલ {
            જો (m_lsb != other.m_lsb) પાછા m_lsb < other.m_lsb;
            પાછા m_polarity < other.m_polarity;
        }
    };

    વર્ગ Restorer final {  // Restore the original state unless disableRestore() is called
        Const_Bit_Op_વૃક્ષ_Visitor& m_visitor;  // Visitor whose state is saved and restored
        અચલ size_t m_polaritiesSize;  // Saved m_visitor.m_bitPolarities size to truncate back to
        અચલ size_t m_frozenSize;  // Saved m_visitor.m_frozenNodes size to truncate back to
        અચલ unsigned m_ops;  // Saved m_visitor.m_ops to restore
        અચલ તાર્કિક m_polarity;  // Saved m_visitor.m_polarity to restore
        તાર્કિક m_restore = સાચું;  // Whether the destructor still needs to restore state

    જાહેર:
        explicit Restorer(Const_Bit_Op_વૃક્ષ_Visitor& visitor)
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
            જો (m_restore) restoreNow();
        }
        શૂન્ય disableRestore() { m_restore = ખોટું; }
        શૂન્ય restoreNow() {
            UASSERT(m_restore, "Can be called just once");
            m_visitor.m_bitPolarities.resize(m_polaritiesSize);
            m_visitor.m_frozenNodes.resize(m_frozenSize);
            m_visitor.m_ops = m_ops;
            m_visitor.m_polarity = m_polarity;
            m_restore = ખોટું;
        }
    };
    // Collect information for each Variable to transform as below
    વર્ગ VarInfo final {
        // MEMBERS
        પૂર્ણાંક m_knownResult = -1;  // -1: result is not known, 0 or 1: result of this tree
        અચલ Const_Bit_Op_વૃક્ષ_Visitor* અચલ
            m_parentp;  // ConstBitOpTreeVisitor holding this VarInfo
        AstVarRef* અચલ m_refp;  // Points the variable that this VarInfo covers
        અચલ પૂર્ણાંક m_width;  // Width of term this VarInfo refers to
        V3_સંખ્યા m_bitPolarity;  // Coefficient of each bit

    જાહેર:
        // METHODS
        તાર્કિક hasConstResult() અચલ { પાછા m_knownResult >= 0 || m_bitPolarity.isAllX(); }
        // The constant result. Only valid if hasConstResult() returned true.
        તાર્કિક getConstResult() અચલ {
            // Note that this condition covers m_knownResult == -1 but m_bitPolarity.isAllX(),
            // in which case the result is 0
            પાછા m_knownResult == 1;
        }
        અચલ AstVarRef* refp() અચલ { પાછા m_refp; }
        તાર્કિક sameVarAs(અચલ Ast_ગાંઠ_Var_Ref* otherp) અચલ { પાછા m_refp->same_ગાંઠ(otherp); }
        શૂન્ય setPolarity(તાર્કિક compBit, પૂર્ણાંક bit) {
            // Ignore if already determined a known reduction
            જો (m_knownResult >= 0) પાછા;
            UASSERT_OBJ(bit < m_width, m_refp,
                        "Bit index out of range: " << bit << " width: " << m_width);
            જો (m_bitPolarity.bitIsX(bit)) {  // The bit is not yet marked with either polarity
                m_bitPolarity.setBit(bit, compBit);
            } નહીંતર {  // The bit has already been marked with some polarity
                અચલ તાર્કિક sameFlag = m_bitPolarity.bitIs1(bit) == compBit;
                જો (m_parentp->is_Xor_વૃક્ષ()) {
                    UASSERT_OBJ(compBit && sameFlag, m_refp, "Only true is set in Xor tree");
                    // a ^ a ^ b == b so we can ignore a
                    m_bitPolarity.setBit(bit, 'x');
                } નહીંતર {  // And, Or
                    // Can ignore this nodep as the bit is already marked with the same polarity
                    જો (sameFlag) પાછા;  // a & a == a, b | b == b
                    // Otherwise result is constant (a & ~a == 0) or (a | ~a == 1)
                    m_knownResult = m_parentp->is_And_વૃક્ષ() ? 0 : 1;
                    m_bitPolarity.setAllBitsX();  // The variable is not referred anymore
                }
            }
        }

        // Return reduction term for this VarInfo, together with the number of ops in the term,
        // and a boolean indicating if the term is clean (1-bit vs multi-bit value)
        ResultTerm getResultTerm() અચલ {
            UASSERT_OBJ(!hasConstResult(), m_refp, "getTerm on reduction that yields constant");
            File_લીટી* અચલ fl = m_refp->fileline();

            // Get the term we are referencing (the WordSel, if wide, otherwise just the VarRef)
            Ast_ગાંઠ_પદાવલિ* srcp = VN_CAST(m_refp->backp(), WordSel);
            જો (!srcp) srcp = m_refp;
            srcp = srcp->clone_વૃક્ષ(ખોટું);

            // Signed variables might have redundant sign bits that need masking.
            અચલ તાર્કિક hasRedundantSignBits
                = m_refp->varp()->dtypep()->isSigned()
                  && (m_refp->isWide() ? (m_width != VL_EDATASIZE)
                                       : (m_width < 8 || !isPow2(m_width)));

            // Get the mask that selects the bits that are relevant in this term
            V3_સંખ્યા mask_અંક{srcp, m_width, 0};
            mask_અંક.opBitsNonXZ(m_bitPolarity);  // 'x' -> 0, 0->1, 1->1
            અચલ uint64_t maskVal = mask_અંક.toUQuad();
            UASSERT_OBJ(maskVal != 0, m_refp,
                        "Should have been recognized as having const 0 result");

            // Parts of the return value
            Ast_ગાંઠ_પદાવલિ* resultp = srcp;  // The tree for this term
            unsigned ops = 0;  // Number of ops in this term
            તાર્કિક clean = ખોટું;  // Whether the term is clean (has value 0 or 1)

            જો (isPow2(maskVal)) {
                // If we only want a single bit, shift it out instead of a masked compare. Shifts
                // don't go through the flags register on x86 and are hence faster. This is also
                // always fewer or same ops as mask and compare, but with shorter instructions on
                // x86.

                // Find the index of the bit we want.
                અચલ પૂર્ણાંક bit = ગણતરી_Trailing_Zeroes(maskVal);
                // If we want something other than the bottom bit, shift it out
                જો (bit != 0) {
                    resultp = નવું AstShiftR{fl, resultp,
                                            નવું AstConst{fl, static_cast<uint32_t>(bit)}, m_width};
                    ++ops;
                }
                // Negate it if necessary
                અચલ તાર્કિક negate = m_bitPolarity.bitIs0(bit);
                જો (negate) {
                    resultp = નવું AstNot{fl, resultp};
                    ++ops;
                }
                // Clean if MSB of unsigned value, and not negated
                clean = (bit == m_width - 1) && !hasRedundantSignBits && !negate;
            } નહીંતર {
                // We want multiple bits. Go ahead and extract them.

                // Check if masking is required, and if so apply it
                અચલ તાર્કિક needsMasking = maskVal != VL_MASK_Q(m_width) || hasRedundantSignBits;
                જો (needsMasking) {
                    resultp = નવું AstAnd{fl, નવું AstConst{fl, mask_અંક}, resultp};
                    ++ops;
                }

                // Create the sub-expression for this term
                જો (m_parentp->is_Xor_વૃક્ષ()) {
                    જો (needsMasking) {
                        // Reduce the masked term to the minimum known width,
                        // to use the smallest RedXor formula
                        અચલ પૂર્ણાંક widthMin = mask_અંક.widthToFit();
                        resultp->dtypeChgWidth(widthMin, widthMin);
                    }
                    resultp = નવું AstRedXor{fl, resultp};
                    ++ops;
                    clean = ખોટું;
                    // VL_REDXOR_* returns IData, set width accordingly to avoid unnecessary casts
                    resultp->dtypeChgWidth(VL_IDATASIZE, 1);
                } નહીંતર જો (m_parentp->is_And_વૃક્ષ()) {
                    V3_સંખ્યા comp_અંક{srcp, m_width, 0};
                    comp_અંક.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    resultp = નવું AstEq{fl, નવું AstConst{fl, comp_અંક}, resultp};
                    ++ops;
                    clean = સાચું;
                } નહીંતર {  // Or
                    V3_સંખ્યા comp_અંક{srcp, m_width, 0};
                    comp_અંક.opBitsOne(m_bitPolarity);  // 'x'->0, 0->0, 1->1
                    comp_અંક.opXor(V3_સંખ્યા{comp_અંક}, mask_અંક);
                    resultp = નવું AstNeq{fl, નવું AstConst{fl, comp_અંક}, resultp};
                    ++ops;
                    clean = સાચું;
                }
            }

            પાછા ResultTerm{resultp, ops, clean};
        }

        // CONSTRUCTORS
        VarInfo(Const_Bit_Op_વૃક્ષ_Visitor* parent, AstVarRef* refp, પૂર્ણાંક width)
            : m_parentp{parent}
            , m_refp{refp}
            , m_width{width}
            , m_bitPolarity{refp, m_width} {
            m_bitPolarity.setAllBitsX();
        }
    };

    // MEMBERS
    તાર્કિક m_failed = ખોટું;
    તાર્કિક m_polarity = સાચું;  // Flip when AstNot comes
    unsigned m_ops;  // Number of operations such as And, Or, Xor, Sel...
    પૂર્ણાંક m_lsb = 0;  // Current LSB
    LeafInfo* m_leafp = nullptr;  // AstConst or AstVarRef that currently looking for
    અચલ Ast_ગાંઠ_પદાવલિ* અચલ m_rootp;  // Root of this AST subtree

    std::vector<std::pair<Ast_ગાંઠ_પદાવલિ*, Frozen_ગાંઠ_Info>>
        m_frozenNodes;  // Nodes that cannot be optimized
    std::vector<BitPolarityEntry> m_bitPolarities;  // Polarity of bits found during iterate()
    std::vector<std::unique_ptr<VarInfo>> m_varInfos;  // VarInfo for each variable, [0] is nullptr
    VNIdAllocator& m_ids;  // Node id allocator, owned by ConstVisitor
    // Base index of m_varInfos that points VarInfo, keyed by AstVarScope/AstVarRef id,
    // zero means not set yet.
    std::unordered_map<size_t, પૂર્ણાંક> m_baseIdxs;

    // METHODS

    તાર્કિક is_And_વૃક્ષ() અચલ { પાછા VN_IS(m_rootp, And); }
    તાર્કિક is_Or_વૃક્ષ() અચલ { પાછા VN_IS(m_rootp, Or); }
    તાર્કિક is_Xor_વૃક્ષ() અચલ { પાછા VN_IS(m_rootp, Xor) || VN_IS(m_rootp, RedXor); }

#define CONST_BITOP_RETURN_IF(cond, nodep) \
    જો (setFailed(cond, #cond, nodep, __LINE__)) પાછા

#define CONST_BITOP_SET_FAILED(reason, nodep) setFailed(સાચું, reason, nodep, __LINE__)

    તાર્કિક setFailed(તાર્કિક fail, અચલ અક્ષર* reason, અચલ Ast_ગાંઠ* nodep, પૂર્ણાંક લીટી) {
        જો (fail && !m_failed) {
            UINFO(9, "cannot optimize " << m_rootp << " reason:" << reason << " called from line:"
                                        << લીટી << " when checking:" << nodep);
            // UINFOTREE(9, m_rootp, "", "root");
            m_failed = સાચું;
        }
        પાછા m_failed;
    }
    શૂન્ય incrOps(અચલ Ast_ગાંઠ* nodep, પૂર્ણાંક લીટી) {
        ++m_ops;
        UINFO(9, "Increment to " << m_ops << " " << nodep << " called from line " << લીટી);
    }
    VarInfo& getVarInfo(અચલ LeafInfo& ref) {
        UASSERT_OBJ(ref.refp(), m_rootp, "null varref in And/Or/Xor optimization");
        Ast_ગાંઠ* nodep = ref.refp()->varScopep();
        જો (!nodep) nodep = ref.refp()->varp();  // Not scoped
        પૂર્ણાંક& baseIdxr = m_baseIdxs[m_ids(nodep)];
        જો (baseIdxr == 0) {  // Not set yet
            baseIdxr = m_varInfos.size();
            અચલ પૂર્ણાંક અંક_શબ્દો
                = ref.refp()->dtypep()->isWide() ? ref.refp()->dtypep()->width_શબ્દો() : 1;
            m_varInfos.resize(m_varInfos.size() + અંક_શબ્દો);
        }
        અચલ size_t idx = baseIdxr + std::max(0, ref.wordIdx());
        VarInfo* varInfop = m_varInfos[idx].get();
        જો (!varInfop) {
            varInfop = નવું VarInfo{આ, ref.refp(), ref.varWidth()};
            m_varInfos[idx].reset(varInfop);
            જો (ref.missingWordSel()) {
                // ConstBitOpTreeVisitor makes some constants for masks and its type is uint64_t.
                // That's why V3Expand, that inserts WordSel, is needed.
                CONST_BITOP_SET_FAILED("V3Expand is skipped", ref.refp());
            }
        } નહીંતર {
            જો (!varInfop->sameVarAs(ref.refp()))
                CONST_BITOP_SET_FAILED("different var (scope?)", ref.refp());
        }
        પાછા *varInfop;
    }

    // Traverse down to see AstConst or AstVarRef
    LeafInfo findLeaf(Ast_ગાંઠ* nodep, તાર્કિક expectConst) {
        જો (!nodep->dtypep()->skipRefp()->isIntegralOrPacked()) પાછા LeafInfo{};
        LeafInfo info{m_lsb};
        {
            VL_RESTORER(m_leafp);
            m_leafp = &info;
            iterateConst(nodep);
        }

        તાર્કિક ok = !m_failed;
        જો (expectConst) {
            ok &= !info.refp() && info.constp();
        } નહીંતર {
            ok &= info.refp() && !info.constp();
        }
        પાછા ok ? info : LeafInfo{};
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { CONST_BITOP_SET_FAILED("Hit unexpected op", nodep); }
    શૂન્ય visit(AstCCast* nodep) override {
        iterateChildrenConst(nodep);
        જો (m_leafp) m_leafp->updateBitRange(nodep);
    }
    શૂન્ય visit(AstShiftR* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        અચલ AstConst* અચલ constp = VN_CAST(nodep->rhsp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->rhsp());
        m_lsb += constp->toUInt();
        incrOps(nodep, __LINE__);
        iterateConst(nodep->lhsp());
        m_leafp->updateBitRange(nodep);
        m_lsb -= constp->toUInt();
    }
    શૂન્ય visit(AstNot* nodep) override {
        CONST_BITOP_RETURN_IF(nodep->widthMin() != 1, nodep);
        Ast_ગાંઠ* lhsp = nodep->lhsp();
        AstCCast* અચલ castp = VN_CAST(lhsp, CCast);
        જો (castp) lhsp = castp->lhsp();
        CONST_BITOP_RETURN_IF(!is_Xor_વૃક્ષ() && !VN_IS(lhsp, VarRef) && !VN_IS(lhsp, ShiftR), lhsp);
        incrOps(nodep, __LINE__);
        m_polarity = !m_polarity;
        iterateChildrenConst(nodep);
        // Don't restore m_polarity for Xor as it counts parity of the entire tree
        જો (!is_Xor_વૃક્ષ()) m_polarity = !m_polarity;
        જો (m_leafp && castp) m_leafp->updateBitRange(castp);
        જો (m_leafp) m_leafp->polarity(!m_leafp->polarity());
    }
    શૂન્ય visit(AstWordSel* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        અચલ AstConst* અચલ constp = VN_CAST(nodep->bitp(), Const);
        CONST_BITOP_RETURN_IF(!constp, nodep->bitp());
        UASSERT_OBJ(m_leafp->wordIdx() == -1, nodep, "Unexpected nested WordSel");
        m_leafp->wordIdx(constp->toSInt());
        iterateConst(nodep->fromp());
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }
    શૂન્ય visit(AstConst* nodep) override {
        CONST_BITOP_RETURN_IF(!m_leafp, nodep);
        m_leafp->setLeaf(nodep);
    }

    શૂન્ય visit(AstRedXor* nodep) override {
        Restorer restorer{*આ};
        CONST_BITOP_RETURN_IF(!VN_IS(m_rootp, Xor), nodep);
        Ast_ગાંઠ* lhsp = nodep->lhsp();
        અચલ AstCCast* અચલ castp = VN_CAST(lhsp, CCast);
        જો (castp) lhsp = castp->lhsp();
        જો (અચલ AstAnd* અચલ andp = VN_CAST(lhsp, And)) {  // '^(mask & leaf)'
            CONST_BITOP_RETURN_IF(!andp, lhsp);

            અચલ LeafInfo& mask = findLeaf(andp->lhsp(), સાચું);
            CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

            LeafInfo ref = findLeaf(andp->rhsp(), ખોટું);
            CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());
            જો (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all subtree succeeded

            અચલ V3_સંખ્યા& mask_અંક = mask.constp()->અંક();

            incrOps(nodep, __LINE__);
            incrOps(andp, __LINE__);

            // Mark all bits checked in this reduction
            અચલ પૂર્ણાંક maxBitIdx = std::min(ref.lsb() + mask_અંક.width(), ref.msb() + 1);
            માટે (પૂર્ણાંક bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                અચલ પૂર્ણાંક maskIdx = bitIdx - ref.lsb();
                જો (mask_અંક.bitIs0(maskIdx)) ચાલુ;
                // Set true, m_polarity takes care of the entire parity
                m_bitPolarities.emplace_back(ref, સાચું, bitIdx);
            }
        } નહીંતર {  // '^leaf'
            LeafInfo ref = findLeaf(lhsp, ખોટું);
            CONST_BITOP_RETURN_IF(!ref.refp(), lhsp);
            જો (castp) ref.updateBitRange(castp);

            restorer.disableRestore();  // Now all checks passed

            incrOps(nodep, __LINE__);

            // Mark all bits checked by this comparison
            માટે (પૂર્ણાંક bitIdx = ref.lsb(); bitIdx <= ref.msb(); ++bitIdx) {
                m_bitPolarities.emplace_back(ref, સાચું, bitIdx);
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_Biop* nodep) override {
        જો (VN_IS(nodep, And) && isConst(nodep->lhsp(), 1)) {  // 1 & _
            // Always reach past a plain making AND
            Restorer restorer{*આ};
            incrOps(nodep, __LINE__);
            iterateConst(nodep->rhsp());
            જો (m_leafp) m_leafp->limitBitRangeToLsb();
            CONST_BITOP_RETURN_IF(m_failed, nodep->rhsp());
            restorer.disableRestore();  // Now all checks passed
        } નહીંતર જો (nodep->type() == m_rootp->type()) {  // And, Or, Xor
            // subtree under NOT can be optimized only in XOR tree.
            CONST_BITOP_RETURN_IF(!m_polarity && !is_Xor_વૃક્ષ(), nodep);
            incrOps(nodep, __LINE__);
            માટે (અચલ તાર્કિક જમણું : {ખોટું, સાચું}) {
                VL_RESTORER(m_leafp);
                Restorer restorer{*આ};
                LeafInfo leafInfo{m_lsb};
                // cppcheck-suppress danglingLifetime
                m_leafp = &leafInfo;
                Ast_ગાંઠ_પદાવલિ* opp = જમણું ? nodep->rhsp() : nodep->lhsp();
                અચલ તાર્કિક origFailed = m_failed;
                iterateConst(opp);
                જો (leafInfo.constp() || m_failed) {
                    // Revert changes in leaf
                    restorer.restoreNow();
                    // Reach past a cast then add to frozen nodes to be added to final reduction
                    જો (અચલ AstCCast* અચલ castp = VN_CAST(opp, CCast)) opp = castp->lhsp();
                    અચલ તાર્કિક pol = is_Xor_વૃક્ષ() || m_polarity;  // Only AND/OR tree needs polarity
                    UASSERT_OBJ(pol, nodep, "AND/OR tree expects m_polarity==true");
                    m_frozenNodes.emplace_back(opp, Frozen_ગાંઠ_Info{pol, m_lsb});
                    m_failed = origFailed;
                    ચાલુ;
                }
                restorer.disableRestore();  // Now all checks passed
                જો (leafInfo.refp()) {
                    // The conditional on the lsb being in range is necessary for some degenerate
                    // case, e.g.: (IData)((QData)wide[0] >> 32), or <1-bit-var> >> 1, which is
                    // just zero
                    જો (leafInfo.lsb() <= leafInfo.msb()) {
                        m_bitPolarities.emplace_back(leafInfo, is_Xor_વૃક્ષ() || leafInfo.polarity(),
                                                     leafInfo.lsb());
                    } નહીંતર જો ((is_And_વૃક્ષ() && leafInfo.polarity())
                               || (is_Or_વૃક્ષ() && !leafInfo.polarity())) {
                        // If there is a constant 0 term in an And tree or 1 term in an Or tree, we
                        // must include it. Fudge this by adding a bit with both polarities, which
                        // will simplify to zero or one respectively.
                        // Note that Xor tree does not need this kind of care, polarity of Xor tree
                        // is already cared when visitin AstNot. Taking xor with 1'b0 is nop.
                        m_bitPolarities.emplace_back(leafInfo, સાચું, 0);
                        m_bitPolarities.emplace_back(leafInfo, ખોટું, 0);
                    }
                }
            }
        } નહીંતર જો ((is_And_વૃક્ષ() && VN_IS(nodep, Eq)) || (is_Or_વૃક્ષ() && VN_IS(nodep, Neq))) {
            Restorer restorer{*આ};
            CONST_BITOP_RETURN_IF(!m_polarity, nodep);
            CONST_BITOP_RETURN_IF(m_lsb, nodep);  // the result of EQ/NE is 1 bit width
            અચલ Ast_ગાંઠ* lhsp = nodep->lhsp();
            જો (અચલ AstCCast* અચલ castp = VN_CAST(lhsp, CCast)) lhsp = castp->lhsp();
            અચલ AstConst* અચલ constp = VN_CAST(lhsp, Const);
            CONST_BITOP_RETURN_IF(!constp, nodep->lhsp());

            અચલ V3_સંખ્યા& comp_અંક = constp->અંક();

            auto setPolarities = [આ, &comp_અંક](અચલ LeafInfo& ref, અચલ V3_સંખ્યા* maskp) {
                અચલ તાર્કિક maskFlip = is_And_વૃક્ષ() ^ ref.polarity();
                પૂર્ણાંક constantWidth = comp_અંક.width();
                જો (maskp) constantWidth = std::max(constantWidth, maskp->width());
                અચલ પૂર્ણાંક maxBitIdx = std::max(ref.lsb() + constantWidth, ref.msb() + 1);
                // Mark all bits checked by this comparison
                માટે (પૂર્ણાંક bitIdx = ref.lsb(); bitIdx < maxBitIdx; ++bitIdx) {
                    અચલ પૂર્ણાંક maskIdx = bitIdx - ref.lsb();
                    અચલ તાર્કિક mask0 = maskp && maskp->bitIs0(maskIdx);
                    અચલ તાર્કિક outOfRange = bitIdx > ref.msb();
                    જો (mask0 || outOfRange) {  // RHS is 0
                        જો (comp_અંક.bitIs1(maskIdx)) {
                            // LHS is 1
                            // And tree: 1 == 0 => always false, set v && !v
                            // Or tree : 1 != 0 => always true, set v || !v
                            m_bitPolarities.emplace_back(ref, સાચું, 0);
                            m_bitPolarities.emplace_back(ref, ખોટું, 0);
                            તોડો;
                        } નહીંતર {  // This bitIdx is irrelevant
                            ચાલુ;
                        }
                    }
                    અચલ તાર્કિક polarity = comp_અંક.bitIs1(maskIdx) != maskFlip;
                    m_bitPolarities.emplace_back(ref, polarity, bitIdx);
                }
            };

            જો (અચલ AstAnd* અચલ andp = VN_CAST(nodep->rhsp(), And)) {  // comp == (mask & v)
                અચલ LeafInfo& mask = findLeaf(andp->lhsp(), સાચું);
                CONST_BITOP_RETURN_IF(!mask.constp() || mask.lsb() != 0, andp->lhsp());

                અચલ LeafInfo& ref = findLeaf(andp->rhsp(), ખોટું);
                CONST_BITOP_RETURN_IF(!ref.refp(), andp->rhsp());

                restorer.disableRestore();  // Now all checks passed

                અચલ V3_સંખ્યા& mask_અંક = mask.constp()->અંક();

                incrOps(nodep, __LINE__);
                incrOps(andp, __LINE__);

                setPolarities(ref, &mask_અંક);
            } નહીંતર {  // comp == v
                અચલ LeafInfo& ref = findLeaf(nodep->rhsp(), ખોટું);
                CONST_BITOP_RETURN_IF(!ref.refp(), nodep->rhsp());

                restorer.disableRestore();  // Now all checks passed

                incrOps(nodep, __LINE__);

                setPolarities(ref, nullptr);
            }
        } નહીંતર {
            CONST_BITOP_SET_FAILED("Mixture of different ops cannot be optimized", nodep);
        }
    }

    // CONSTRUCTORS
    Const_Bit_Op_વૃક્ષ_Visitor(Ast_ગાંઠ_પદાવલિ* nodep, unsigned externalOps, VNIdAllocator& ids)
        : m_ops{externalOps}
        , m_rootp{nodep}
        , m_ids{ids} {
        // Fill nullptr at [0] because a base index of 0 means not set yet
        m_varInfos.ધકેલો_back(nullptr);
        CONST_BITOP_RETURN_IF(!is_And_વૃક્ષ() && !is_Or_વૃક્ષ() && !is_Xor_વૃક્ષ(), nodep);
        જો (Ast_ગાંઠ_Biop* અચલ biopp = VN_CAST(nodep, ગાંઠ_Biop)) {
            iterateConst(biopp);
        } નહીંતર {
            UASSERT_OBJ(VN_IS(nodep, RedXor), nodep, "Must be RedXor");
            incrOps(nodep, __LINE__);
            iterateChildrenConst(nodep);
        }
        માટે (auto&& entry : m_bitPolarities) {
            getVarInfo(entry.m_info).setPolarity(entry.m_polarity, entry.m_bit);
        }
        UASSERT_OBJ(is_Xor_વૃક્ષ() || m_polarity, nodep, "must be the original polarity");
    }
    આભાસી ~Const_Bit_Op_વૃક્ષ_Visitor() = મૂળભૂત;
#undef CONST_BITOP_RETURN_IF
#undef CONST_BITOP_SET_FAILED

જાહેર:
    // Transform as below.
    // v[0] & v[1] => 2'b11 == (2'b11 & v)
    // v[0] | v[1] => 2'b00 != (2'b11 & v)
    // v[0] ^ v[1] => ^{2'b11 & v}
    // (3'b011 == (3'b011 & v)) & v[2]  => 3'b111 == (3'b111 & v)
    // (3'b000 != (3'b011 & v)) | v[2]  => 3'b000 != (3'b111 & v)
    // Reduction ops are transformed in the same way.
    // &{v[0], v[1]} => 2'b11 == (2'b11 & v)
    સ્થિર Ast_ગાંઠ_પદાવલિ* simplify(Ast_ગાંઠ_પદાવલિ* nodep, પૂર્ણાંક resultWidth, unsigned externalOps,
                                 VDouble0& reduction, VNIdAllocator& ids) {
        UASSERT_OBJ(1 <= resultWidth && resultWidth <= 64, nodep, "resultWidth out of range");

        // Walk tree, gathering all terms referenced in expression
        અચલ Const_Bit_Op_વૃક્ષ_Visitor visitor{nodep, externalOps, ids};

        // If failed on root node is not optimizable, or there are no variable terms, then done
        જો (visitor.m_failed || visitor.m_varInfos.size() == 1) પાછા nullptr;

        // FileLine used for constructing all new nodes in this function
        File_લીટી* અચલ fl = nodep->fileline();

        // Get partial result each term referenced, count total number of ops and keep track of
        // whether we have clean/dirty terms. visitor.m_varInfos appears in deterministic order,
        // so the optimized tree is deterministic as well.

        std::vector<Ast_ગાંઠ_પદાવલિ*> termps;
        termps.reserve(visitor.m_varInfos.size() - 1);
        unsigned resultOps = 0;
        તાર્કિક hasCleanTerm = ખોટું;
        તાર્કિક hasDirtyTerm = ખોટું;

        માટે (auto&& v : visitor.m_varInfos) {
            જો (!v) ચાલુ;  // Skip nullptr at m_varInfos[0]
            જો (v->hasConstResult()) {
                // If a constant term is known, we can either drop it or the whole tree is constant
                Ast_ગાંઠ_પદાવલિ* resultp = nullptr;
                જો (v->getConstResult()) {
                    UASSERT_OBJ(visitor.is_Or_વૃક્ષ(), nodep,
                                "Only OR tree can yield known 1 result");
                    UINFO(9, "OR tree with const 1 term: " << v->refp());
                    // Known 1 bit in OR tree, whole result is 1
                    resultp = નવું AstConst{fl, AstConst::BitTrue{}};
                } નહીંતર જો (visitor.is_And_વૃક્ષ()) {
                    UINFO(9, "AND tree with const 0 term: " << v->refp());
                    // Known 0 bit in AND tree, whole result is 0
                    resultp = નવું AstConst{fl, AstConst::BitFalse{}};
                } નહીંતર {
                    // Known 0 bit in OR or XOR tree. Ignore it.
                    ચાલુ;
                }
                // Set width and widthMin precisely
                resultp->dtypeChgWidth(resultWidth, 1);
                માટે (Ast_ગાંઠ* અચલ termp : termps) VL_DO_DANGLING(termp->delete_વૃક્ષ(), termp);
                પાછા resultp;
            }
            અચલ ResultTerm result = v->getResultTerm();
            termps.ધકેલો_back(std::get<0>(result));
            resultOps += std::get<1>(result);
            જો (std::get<2>(result)) {
                hasCleanTerm = સાચું;
                UINFO(9, "Clean term: " << termps.back());
            } નહીંતર {
                hasDirtyTerm = સાચું;
                UINFO(9, "Dirty term: " << termps.back());
            }
        }

        // Group by FrozenNodeInfo
        std::map<Frozen_ગાંઠ_Info, std::vector<Ast_ગાંઠ_પદાવલિ*>> frozenNodes;
        // Check if frozen terms are clean or not
        માટે (અચલ auto& frozenInfo : visitor.m_frozenNodes) {
            Ast_ગાંઠ_પદાવલિ* અચલ termp = frozenInfo.first;
            // Comparison operators are clean
            જો ((VN_IS(termp, Eq) || VN_IS(termp, Neq) || VN_IS(termp, Lt) || VN_IS(termp, Lte)
                 || VN_IS(termp, Gt) || VN_IS(termp, Gte))
                && frozenInfo.second.m_lsb == 0) {
                hasCleanTerm = સાચું;
            } નહીંતર {
                // Otherwise, conservatively assume the frozen term is dirty
                hasDirtyTerm = સાચું;
                UINFO(9, "Dirty frozen term: " << termp);
            }
            frozenNodes[frozenInfo.second].ધકેલો_back(termp);
        }

        // Figure out if a final negation is required
        અચલ તાર્કિક needsFlip = visitor.is_Xor_વૃક્ષ() && !visitor.m_polarity;

        // Figure out if the final tree needs cleaning
        અચલ તાર્કિક needsCleaning = visitor.is_And_વૃક્ષ() ? !hasCleanTerm : hasDirtyTerm;

        // Add size of reduction tree to op count
        resultOps += termps.size() - 1;
        માટે (અચલ auto& lsbAndNodes : frozenNodes) {
            જો (lsbAndNodes.first.m_lsb > 0) ++resultOps;  // Needs AstShiftR
            જો (!lsbAndNodes.first.m_polarity) ++resultOps;  // Needs AstNot
            resultOps += lsbAndNodes.second.size();
        }
        // Add final polarity flip in Xor tree
        જો (needsFlip) ++resultOps;
        // Add final cleaning AND
        જો (needsCleaning) ++resultOps;

        જો (debug() >= 9) {  // LCOV_EXCL_START
            cout << "-  Bitop tree considered:\n";
            માટે (અચલ Ast_ગાંઠ_પદાવલિ* અચલ termp : termps) termp->dump_વૃક્ષ("-  Reduced term: ");
            માટે (અચલ std::pair<Ast_ગાંઠ_પદાવલિ*, Frozen_ગાંઠ_Info>& termp : visitor.m_frozenNodes) {
                termp.first->dump_વૃક્ષ("-  Frozen term with lsb "
                                      + std::to_string(termp.second.m_lsb) + " polarity "
                                      + std::to_string(termp.second.m_polarity) + ": ");
            }
            cout << "-  Needs flipping: " << needsFlip << "\n";
            cout << "-  Needs cleaning: " << needsCleaning << "\n";
            cout << "-  Size: " << resultOps << " input size: " << visitor.m_ops << "\n";
        }  // LCOV_EXCL_STOP

        // Sometimes we have no terms left after ignoring redundant terms
        // (all of which were zeroes)
        જો (termps.empty() && visitor.m_frozenNodes.empty()) {
            reduction += visitor.m_ops;
            Ast_ગાંઠ_પદાવલિ* અચલ resultp = needsFlip ? નવું AstConst{fl, AstConst::BitTrue{}}
                                                   : નવું AstConst{fl, AstConst::BitFalse{}};
            resultp->dtypeChgWidth(resultWidth, 1);
            પાછા resultp;
        }

        // Only substitute the result if beneficial as determined by operation count
        જો (visitor.m_ops <= resultOps) {
            માટે (Ast_ગાંઠ* અચલ termp : termps) VL_DO_DANGLING(termp->delete_વૃક્ષ(), termp);
            પાછા nullptr;
        }

        // Update statistics
        reduction += visitor.m_ops - resultOps;

        // Reduction op to combine terms
        અચલ auto reduce = [&visitor, fl](Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) -> Ast_ગાંઠ_પદાવલિ* {
            જો (!lhsp) પાછા rhsp;
            જો (visitor.is_And_વૃક્ષ()) {
                પાછા નવું AstAnd{fl, lhsp, rhsp};
            } નહીંતર જો (visitor.is_Or_વૃક્ષ()) {
                પાછા નવું AstOr{fl, lhsp, rhsp};
            } નહીંતર {
                પાછા નવું AstXor{fl, lhsp, rhsp};
            }
        };

        // Compute result by reducing all terms
        Ast_ગાંઠ_પદાવલિ* resultp = nullptr;
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ termp : termps) {  //
            resultp = reduce(resultp, termp);
        }
        // Add any frozen terms to the reduction
        માટે (auto&& nodes : frozenNodes) {
            // nodes.second has same lsb and polarity
            Ast_ગાંઠ_પદાવલિ* termp = nullptr;
            માટે (Ast_ગાંઠ_પદાવલિ* અચલ itemp : nodes.second) {
                termp = reduce(termp, itemp->unlinkFrBack());
            }
            જો (nodes.first.m_lsb > 0) {  // LSB is not 0, so shiftR
                Ast_ગાંઠ_DType* અચલ dtypep = termp->dtypep();
                termp = નવું AstShiftR{termp->fileline(), termp,
                                      નવું AstConst(termp->fileline(), AstConst::Widthed_કિંમત{},
                                                   termp->width(), nodes.first.m_lsb)};
                termp->dtypep(dtypep);
            }
            જો (!nodes.first.m_polarity) {  // Polarity is inverted, so append Not
                Ast_ગાંઠ_DType* અચલ dtypep = termp->dtypep();
                termp = નવું AstNot{termp->fileline(), termp};
                termp->dtypep(dtypep);
            }
            resultp = reduce(resultp, termp);
        }

        // Set width of masks to expected result width. This is required to prevent later removal
        // of the masking node e.g. by the "AND with all ones" rule. If the result width happens
        // to be 1, we still need to ensure the AstAnd is not dropped, so use a wider mask in this
        // special case.
        અચલ પૂર્ણાંક maskWidth
            = std::max(resultp->width(), resultWidth == 1 ? VL_IDATASIZE : resultWidth);

        // Apply final polarity flip
        જો (needsFlip) {
            જો (needsCleaning) {
                // Cleaning will be added below. Use a NOT which is a byte shorter on x86
                resultp = નવું AstNot{fl, resultp};
            } નહીંતર {
                // Keep result clean by using XOR(1, _)
                AstConst* અચલ maskp = નવું AstConst{fl, AstConst::Widthed_કિંમત{}, maskWidth, 1};
                resultp = નવું AstXor{fl, maskp, resultp};
            }
        }

        // Apply final cleaning
        જો (needsCleaning) {
            AstConst* અચલ maskp = નવું AstConst{fl, AstConst::Widthed_કિંમત{}, maskWidth, 1};
            resultp = નવું AstAnd{fl, maskp, resultp};
        }

        // Cast back to original size if required
        જો (resultp->width() != resultWidth) {
            resultp = નવું AstCCast{fl, resultp, resultWidth, 1};
        }

        // Set width and widthMin precisely
        resultp->dtypeChgWidth(resultWidth, 1);

        પાછા resultp;
    }
};

//######################################################################
// Const state, as a visitor of each AstNode

વર્ગ ConstVisitor final : જાહેર VNVisitor {
    // CONSTANTS
    સ્થિર constexpr unsigned CONCAT_MERGABLE_MAX_DEPTH = 10;  // Limit alg recursion
    // Largest right-hand side that is copied to each part of a split concatenation assignment
    સ્થિર constexpr પૂર્ણાંક CONCAT_ASSIGN_MAX_RHS_NODES = 16;

    // NODE STATE
    // See VNIdAllocator. All per node state below is held in side tables keyed on the
    // node ids it hands out, as node pointers are not stable keys within V3Const.

    // STATE
    VNIdAllocator m_ids;  // Node id allocator
    તાર્કિક m_params = ખોટું;  // If true, propagate parameterized and true numbers only
    તાર્કિક m_required = ખોટું;  // If true, must become a constant
    તાર્કિક m_wremove = સાચું;  // Inside scope, no assignw removal
    તાર્કિક m_warn = ખોટું;  // Output warnings
    તાર્કિક m_doExpensive = ખોટું;  // Enable computationally expensive optimizations
    તાર્કિક m_doCpp = ખોટું;  // Enable late-stage C++ optimizations
    તાર્કિક m_doNConst = ખોટું;  // Enable non-constant-child simplifications
    તાર્કિક m_doV = ખોટું;  // Verilog, not C++ conversion
    તાર્કિક m_doGenerate = ખોટું;  // Postpone width checking inside generate
    તાર્કિક m_convertLogicToBit = ખોટું;  // Convert logical operators to bitwise
    તાર્કિક m_hasJumpDelay = ખોટું;  // JumpGo or Delay under this loop
    તાર્કિક m_hasLoopTest = ખોટું;  // Contains AstLoopTest
    તાર્કિક m_underRecFunc = ખોટું;  // Under a recursive function
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    અચલ AstArraySel* m_selp = nullptr;  // Current select
    અચલ AstScope* m_scopep = nullptr;  // Current scope
    અચલ AstAttrOf* m_attrp = nullptr;  // Current attribute
    VDouble0 m_statBitOpReduction;  // Ops reduced in ConstBitOpTreeVisitor
    VDouble0 m_statConcatMerge;  // Concat merges
    VDouble0 m_stat_Cond_પદાવલિ_Redundant;  // Conditional repeated expressions
    VDouble0 m_stat_If_Cond_પદાવલિ_Redundant;  // Conditional repeated expressions
    VDouble0 m_statMemberAccessVisits;  // Nodes visited by containsMemberAccessRecurse
    અચલ તાર્કિક m_globalPass;  // ConstVisitor invoked as a global pass
    સ્થિર uint32_t s_global_Pass_અંક;  // Counts number of times ConstVisitor invoked as global pass
    V3UniqueNames m_concswapNames;  // For generating unique temporary variable names
    // Caches results of matchBiopToBitwise, keyed on node id
    std::unordered_map<size_t, તાર્કિક> m_containsMemberAccess;
    // Items of enum currently being recursed into. Cannot use VNIdAllocator, as this runs in
    // every mode, including those invoked under a pass holding user4 (V3Param). Node pointers
    // are safe as keys here, unlike elsewhere in V3Const: folding the item value below does
    // free nodes whose addresses are then reused, but AstEnumItem is only ever constructed
    // while parsing (verilog.y and V3LinkParse), so a reused address can never become one,
    // and every key looked up is an AstEnumItem.
    std::unordered_set<અચલ AstEnumItem*> m_recursing_Enum_વસ્તુઓ;
    std::unordered_set<AstJumpBlock*> m_usedJumpBlocks;  // JumpBlocks used by some JumpGo

    // METHODS

    // Return true if statements may be deleted. Until statics are lifted, a variable declared
    // in a statement might be referenced from elsewhere (e.g. hierarchically).
    સ્થિર તાર્કિક canDeleteStatements() { પાછા v3Global.staticsLifted(); }

    શૂન્ય deleteVarScopesUnder(Ast_ગાંઠ* subtreep) {
        જો (!subtreep) પાછા;
        જો (!m_scopep) પાછા;
        std::unordered_set<AstVar*> varps;
        subtreep->foreachAndNext([&](AstVar* varp) { varps.ઉમેરો(varp); });
        જો (varps.empty()) પાછા;
        માટે (AstVarScope *vscp = m_scopep->varsp(), *nextp; vscp; vscp = nextp) {
            nextp = VN_AS(vscp->nextp(), VarScope);
            જો (varps.find(vscp->varp()) != varps.end()) {
                VL_DO_DANGLING(ધકેલો_Deletep(vscp->unlinkFrBack()), vscp);
            }
        }
    }

    V3_સંખ્યા const_અંક_V(Ast_ગાંઠ* nodep) {
        // Contract C width to V width (if needed, else just direct copy)
        // The upper zeros in the C representation can otherwise cause
        // wrong results in some operations, e.g. MulS
        અચલ V3_સંખ્યા& numc = VN_AS(nodep, Const)->અંક();
        પાછા !numc.is_સંખ્યા() ? numc : V3_સંખ્યા{nodep, nodep->widthMinV(), numc};
    }
    V3_સંખ્યા to_અંક_C(Ast_ગાંઠ* nodep, અચલ V3_સંખ્યા& numv) {
        // Extend V width back to C width for given node
        પાછા !numv.is_સંખ્યા() ? numv : V3_સંખ્યા{nodep, nodep->width(), numv};
    }

    સ્થિર તાર્કિક lowerAsFixedAggregate(અચલ Ast_ગાંઠ_DType* અચલ dtypep) {
        પાછા dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }

    AstStructSel* newStructSel(Ast_ગાંઠ_પદાવલિ* અચલ fromp, અચલ AstMemberDType* અચલ itemp) {
        AstStructSel* અચલ selp = નવું AstStructSel{fromp->fileline(), fromp, itemp->name()};
        selp->dtypeFrom(itemp->dtypep());
        પાછા selp;
    }

    શૂન્ય collectFixedAggregateTerms(Ast_ગાંઠ_પદાવલિ* અચલ fromp, std::vector<Ast_ગાંઠ_પદાવલિ*>& termps,
                                    અચલ તાર્કિક packReal) {
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = fromp->dtypep()->skipRefp();
        જો (અચલ AstUnpackArrayDType* અચલ unpackDtypep = VN_CAST(dtypep, UnpackArrayDType)) {
            માટે (અચલ પૂર્ણાંક idx : unpackDtypep->declRange().seq_ડાબું_To_જમણું()) {
                AstArraySel* અચલ selp
                    = નવું AstArraySel{fromp->fileline(), fromp->clone_વૃક્ષ_Pure(ખોટું), idx};
                collectFixedAggregateTerms(selp, termps, packReal);
            }
            VL_DO_DANGLING(ધકેલો_Deletep(fromp), fromp);
        } નહીંતર જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep
                   = VN_CAST(dtypep, ગાંઠ_UOr_Struct_DType)) {
            જો (sdtypep->packed()) {
                termps.ધકેલો_back(fromp);
                પાછા;
            }
            માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                collectFixedAggregateTerms(newStructSel(fromp->clone_વૃક્ષ_Pure(ખોટું), itemp),
                                           termps, packReal);
            }
            VL_DO_DANGLING(ધકેલો_Deletep(fromp), fromp);
        } નહીંતર જો (packReal && dtypep->isDouble()) {
            termps.ધકેલો_back(નવું AstRealToBits{fromp->fileline(), fromp});
        } નહીંતર {
            termps.ધકેલો_back(fromp);
        }
    }

    Ast_ગાંઠ_પદાવલિ* packFixedAggregate(Ast_ગાંઠ_પદાવલિ* અચલ fromp) {
        std::vector<Ast_ગાંઠ_પદાવલિ*> termps;
        collectFixedAggregateTerms(fromp, termps, સાચું);
        UASSERT(!termps.empty(), "No stream terms");
        Ast_ગાંઠ_પદાવલિ* resultp = termps[0];
        માટે (size_t i = 1; i < termps.size(); ++i) {
            resultp = નવું AstConcat{resultp->fileline(), resultp, termps[i]};
        }
        પાછા resultp;
    }

    શૂન્ય replaceAssignToFixedAggregate(Ast_ગાંઠ_Assign* અચલ nodep, Ast_ગાંઠ_પદાવલિ* અચલ dstp,
                                       Ast_ગાંઠ_પદાવલિ* srcp) {
        અચલ પૂર્ણાંક dstWidth = dstp->dtypep()->widthStream();
        std::vector<Ast_ગાંઠ_પદાવલિ*> termps;
        collectFixedAggregateTerms(dstp, termps, ખોટું);
        પૂર્ણાંક srcWidth = srcp->width();
        જો (srcWidth < dstWidth) {
            AstExtend* અચલ extendp = નવું AstExtend{srcp->fileline(), srcp, dstWidth};
            srcp = નવું AstShiftL{
                extendp->fileline(), extendp,
                નવું AstConst{extendp->fileline(), static_cast<uint32_t>(dstWidth - srcWidth)},
                dstWidth};
            srcWidth = dstWidth;
        }
        Ast_ગાંઠ_Assign* newp = nullptr;
        પૂર્ણાંક offset = 0;
        માટે (size_t i = 0; i < termps.size(); ++i) {
            Ast_ગાંઠ_પદાવલિ* અચલ termp = termps[i];
            અચલ પૂર્ણાંક width = termp->dtypep()->widthStream();
            અચલ પૂર્ણાંક lsb = srcWidth - offset - width;
            offset += width;
            Ast_ગાંઠ_પદાવલિ* rhsp
                = નવું AstSel{srcp->fileline(), srcp->clone_વૃક્ષ_Pure(ખોટું), lsb, width};
            જો (termp->dtypep()->skipRefp()->isDouble()) {
                rhsp = નવું AstBitsToRealD{rhsp->fileline(), rhsp};
            }
            Ast_ગાંઠ_Assign* અચલ assignp = nodep->cloneType(termp, rhsp);
            assignp->dtypeFrom(termp);
            newp = Ast_ગાંઠ::addNext(newp, assignp);
        }
        nodep->addNextHere(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(srcp), srcp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    તાર્કિક operandConst(અચલ Ast_ગાંઠ* nodep) { પાછા VN_IS(nodep, Const); }
    તાર્કિક operandAsvConst(અચલ Ast_ગાંઠ* nodep) {
        // BIASV(CONST, BIASV(CONST,...)) -> BIASV( BIASV_CONSTED(a,b), ...)
        અચલ Ast_ગાંઠ_Bi_Com_Asv* અચલ bnodep = VN_CAST(nodep, ગાંઠ_Bi_Com_Asv);
        જો (!bnodep) પાછા ખોટું;
        જો (!VN_IS(bnodep->lhsp(), Const)) પાછા ખોટું;
        અચલ Ast_ગાંઠ_Bi_Com_Asv* અચલ rnodep = VN_CAST(bnodep->rhsp(), ગાંઠ_Bi_Com_Asv);
        જો (!rnodep) પાછા ખોટું;
        જો (rnodep->type() != bnodep->type()) પાછા ખોટું;
        જો (rnodep->width() != bnodep->width()) પાછા ખોટું;
        જો (rnodep->lhsp()->width() != bnodep->lhsp()->width()) પાછા ખોટું;
        જો (!VN_IS(rnodep->lhsp(), Const)) પાછા ખોટું;
        પાછા સાચું;
    }
    તાર્કિક operandAsvSame(અચલ Ast_ગાંઠ* nodep) {
        // BIASV(SAMEa, BIASV(SAMEb,...)) -> BIASV( BIASV(SAMEa,SAMEb), ...)
        અચલ Ast_ગાંઠ_Bi_Com_Asv* અચલ bnodep = VN_CAST(nodep, ગાંઠ_Bi_Com_Asv);
        જો (!bnodep) પાછા ખોટું;
        અચલ Ast_ગાંઠ_Bi_Com_Asv* અચલ rnodep = VN_CAST(bnodep->rhsp(), ગાંઠ_Bi_Com_Asv);
        જો (!rnodep) પાછા ખોટું;
        જો (rnodep->type() != bnodep->type()) પાછા ખોટું;
        જો (rnodep->width() != bnodep->width()) પાછા ખોટું;
        પાછા operandsSame(bnodep->lhsp(), rnodep->lhsp());
    }
    તાર્કિક operandAsvLUp(અચલ Ast_ગાંઠ* nodep) {
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
        અચલ Ast_ગાંઠ_Bi_Com_Asv* અચલ bnodep = VN_CAST(nodep, ગાંઠ_Bi_Com_Asv);
        જો (!bnodep) પાછા ખોટું;
        અચલ Ast_ગાંઠ_Bi_Com_Asv* અચલ lnodep = VN_CAST(bnodep->lhsp(), ગાંઠ_Bi_Com_Asv);
        જો (!lnodep) પાછા ખોટું;
        જો (lnodep->type() != bnodep->type()) પાછા ખોટું;
        જો (lnodep->width() != bnodep->width()) પાછા ખોટું;
        પાછા VN_IS(lnodep->lhsp(), Const);
    }
    તાર્કિક operandAsvRUp(અચલ Ast_ગાંઠ* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr)) ?
        અચલ Ast_ગાંઠ_Bi_Com_Asv* અચલ bnodep = VN_CAST(nodep, ગાંઠ_Bi_Com_Asv);
        જો (!bnodep) પાછા ખોટું;
        અચલ Ast_ગાંઠ_Bi_Com_Asv* અચલ rnodep = VN_CAST(bnodep->rhsp(), ગાંઠ_Bi_Com_Asv);
        જો (!rnodep) પાછા ખોટું;
        જો (rnodep->type() != bnodep->type()) પાછા ખોટું;
        જો (rnodep->width() != bnodep->width()) પાછા ખોટું;
        પાછા VN_IS(rnodep->lhsp(), Const);
    }
    સ્થિર તાર્કિક operandSubAdd(અચલ Ast_ગાંઠ* nodep) {
        // SUB( ADD(CONSTx,y), CONSTz) -> ADD(SUB(CONSTx,CONSTz), y)
        અચલ Ast_ગાંઠ_Biop* અચલ np = VN_CAST(nodep, ગાંઠ_Biop);
        અચલ Ast_ગાંઠ_Biop* અચલ lp = VN_CAST(np->lhsp(), ગાંઠ_Biop);
        પાછા (lp && VN_IS(lp->lhsp(), Const) && VN_IS(np->rhsp(), Const)
                && lp->width() == np->width());
    }
    તાર્કિક matchRedundantClean(AstAnd* andp) {
        // Remove And with constant one inserted by V3Clean
        // 1 & (a == b)  -> (IData)(a == b)
        // When bool is casted to int, the value is either 0 or 1
        AstConst* અચલ constp = VN_AS(andp->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isOne(), andp->lhsp(), "TRREEOPC must meet this condition");
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = andp->rhsp();
        AstCCast* ccastp = nullptr;
        અચલ auto isEqOrNeq = [](અચલ Ast_ગાંઠ* nodep) -> તાર્કિક {  //
            પાછા VN_IS(nodep, Eq) || VN_IS(nodep, Neq);
        };
        જો (isEqOrNeq(rhsp)) {
            ccastp = નવું AstCCast{andp->fileline(), rhsp->unlinkFrBack(), andp};
        } નહીંતર જો (AstCCast* અચલ tmpp = VN_CAST(rhsp, CCast)) {
            જો (isEqOrNeq(tmpp->lhsp())) {
                જો (tmpp->width() == andp->width()) {
                    tmpp->unlinkFrBack();
                    ccastp = tmpp;
                } નહીંતર {
                    ccastp = નવું AstCCast{andp->fileline(), tmpp->lhsp()->unlinkFrBack(), andp};
                }
            }
        }
        જો (ccastp) {
            andp->replaceWithKeepDType(ccastp);
            VL_DO_DANGLING(ધકેલો_Deletep(andp), andp);
            પાછા સાચું;
        }
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક operandAndOrSame(અચલ Ast_ગાંઠ* nodep) {
        // OR( AND(VAL,x), AND(VAL,y)) -> AND(VAL,OR(x,y))
        // OR( AND(x,VAL), AND(y,VAL)) -> AND(OR(x,y),VAL)
        અચલ Ast_ગાંઠ_Biop* અચલ np = VN_CAST(nodep, ગાંઠ_Biop);
        અચલ Ast_ગાંઠ_Biop* અચલ lp = VN_CAST(np->lhsp(), ગાંઠ_Biop);
        અચલ Ast_ગાંઠ_Biop* અચલ rp = VN_CAST(np->rhsp(), ગાંઠ_Biop);
        પાછા (lp && rp && lp->width() == rp->width() && lp->type() == rp->type()
                && (operandsSame(lp->lhsp(), rp->lhsp()) || operandsSame(lp->rhsp(), rp->rhsp())));
    }
    તાર્કિક matchOrAndNot(Ast_ગાંઠ_Biop* nodep) {
        // AstOr{$a, AstAnd{AstNot{$b}, $c}} if $a.width1, $a==$b => AstOr{$a,$c}
        // Someday we'll sort the biops completely and this can be simplified
        // This often results from our simplified clock generation:
        // if (rst) ... else if (enable)... -> OR(rst,AND(!rst,enable))
        અચલ Ast_ગાંઠ_પદાવલિ* ap;
        Ast_ગાંઠ_Biop* andp;
        જો (VN_IS(nodep->lhsp(), And)) {
            andp = VN_AS(nodep->lhsp(), And);
            ap = nodep->rhsp();
        } નહીંતર જો (VN_IS(nodep->rhsp(), And)) {
            andp = VN_AS(nodep->rhsp(), And);
            ap = nodep->lhsp();
        } નહીંતર {
            પાછા ખોટું;
        }
        અચલ Ast_ગાંઠ_Uniop* notp;
        Ast_ગાંઠ_પદાવલિ* cp;
        જો (VN_IS(andp->lhsp(), Not)) {
            notp = VN_AS(andp->lhsp(), Not);
            cp = andp->rhsp();
        } નહીંતર જો (VN_IS(andp->rhsp(), Not)) {
            notp = VN_AS(andp->rhsp(), Not);
            cp = andp->lhsp();
        } નહીંતર {
            પાછા ખોટું;
        }
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ bp = notp->lhsp();
        જો (!operandsSame(ap, bp)) પાછા ખોટું;
        // Do it
        cp->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(andp->unlinkFrBack()), andp);
        VL_DANGLING(notp);
        // Replace whichever branch is now dangling
        જો (nodep->rhsp()) {
            nodep->lhsp(cp);
        } નહીંતર {
            nodep->rhsp(cp);
        }
        પાછા સાચું;
    }
    તાર્કિક matchAndCond(AstAnd* nodep) {
        // Push down a AND into conditional, when one side of conditional is constant
        // (otherwise we'd be trading one operation for two operations)
        // V3Clean often makes this pattern, as it postpones the AND until
        // as high as possible, which is usually the right choice, except for this.
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstCond* અચલ condp = VN_CAST(nodep->rhsp(), Cond);
        જો (!condp) પાછા ખોટું;
        જો (!VN_IS(condp->thenp(), Const) && !VN_IS(condp->elsep(), Const)) પાછા ખોટું;
        AstConst* અચલ maskp = VN_CAST(nodep->lhsp(), Const);
        જો (!maskp) પાછા ખોટું;
        UINFO(4, "AND(CONSTm, CONDcond(c, i, e))->CONDcond(c, AND(m,i), AND(m, e)) " << nodep);
        AstCond* અચલ newp = નવું AstCond{
            condp->fileline(), condp->condp()->unlinkFrBack(),
            નવું AstAnd{nodep->fileline(), maskp->clone_વૃક્ષ(ખોટું), condp->thenp()->unlinkFrBack()},
            નવું AstAnd{nodep->fileline(), maskp->clone_વૃક્ષ(ખોટું),
                       condp->elsep()->unlinkFrBack()}};
        newp->thenp()->dtypeFrom(nodep);  // As And might have been to change widths
        newp->elsep()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }

    તાર્કિક matchCondCond(અચલ AstCond* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        જો (VN_IS(nodep->backp(), Cond)) પાછા ખોટું;  // Was checked when visited parent
        જો (!VN_IS(nodep->thenp(), Cond) && !VN_IS(nodep->elsep(), Cond))
            પાછા ખોટું;  // Short circuit
        std::vector<Ast_ગાંઠ_પદાવલિ*> truesp;
        std::vector<Ast_ગાંઠ_પદાવલિ*> falsesp;
        matchCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
        પાછા ખોટું;  // Can optimize further
    }
    શૂન્ય matchCondCondRecurse(અચલ AstCond* nodep, std::vector<Ast_ગાંઠ_પદાવલિ*>& truesp,
                              std::vector<Ast_ગાંઠ_પદાવલિ*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        જો (truesp.size() > 4 || falsesp.size() > 4) પાછા;
        જો (!nodep->condp()->isPure()) પાછા;
        તાર્કિક replaced = ખોટું;
        માટે (અચલ Ast_ગાંઠ_પદાવલિ* condp : truesp) {
            જો (replaced) તોડો;
            જો (!operandsSame(nodep->condp(), condp)) ચાલુ;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_અંક(nodep->condp(), 1);
            replaced = સાચું;
            ++m_stat_Cond_પદાવલિ_Redundant;
        }
        માટે (અચલ Ast_ગાંઠ_પદાવલિ* condp : falsesp) {
            જો (replaced) તોડો;
            જો (!operandsSame(nodep->condp(), condp)) ચાલુ;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = સાચું;
            ++m_stat_Cond_પદાવલિ_Redundant;
        }
        જો (અચલ AstCond* subCondp = VN_CAST(nodep->thenp(), Cond)) {
            જો (!replaced) truesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            જો (!replaced) truesp.કાઢો_back();
        }
        જો (અચલ AstCond* subCondp = VN_CAST(nodep->elsep(), Cond)) {
            જો (!replaced) falsesp.emplace_back(nodep->condp());
            matchCondCondRecurse(subCondp, truesp /*ref*/, falsesp /*ref*/);
            જો (!replaced) falsesp.કાઢો_back();
        }
    }
    શૂન્ય matchIfCondCond(અચલ Ast_ગાંઠ_If* nodep) {
        // Same condition on either leg of a condition means that
        // expression is either always true or always false
        જો (VN_IS(nodep->backp(), If)) પાછા;  // Was checked when visited parent
        જો (!VN_IS(nodep->thensp(), If) && !VN_IS(nodep->elsesp(), If)) પાછા;  // Short circuit
        std::vector<Ast_ગાંઠ_પદાવલિ*> truesp;
        std::vector<Ast_ગાંઠ_પદાવલિ*> falsesp;
        matchIfCondCondRecurse(nodep, truesp /*ref*/, falsesp /*ref*/);
    }
    શૂન્ય matchIfCondCondRecurse(અચલ Ast_ગાંઠ_If* nodep, std::vector<Ast_ગાંઠ_પદાવલિ*>& truesp,
                                std::vector<Ast_ગાંઠ_પદાવલિ*>& falsesp) {
        // Avoid O(n^2) compares
        // Could reduce cost with hash table, but seems unlikely to be worth cost
        જો (truesp.size() > 4 || falsesp.size() > 4) પાછા;
        જો (!nodep->condp()->isPure()) પાછા;
        // A constant condition is folded by visit(AstNodeIf), once statements can be deleted
        જો (VN_IS(nodep->condp(), Const)) પાછા;
        તાર્કિક replaced = ખોટું;
        માટે (અચલ Ast_ગાંઠ_પદાવલિ* condp : truesp) {
            જો (replaced) તોડો;
            જો (!operandsSame(nodep->condp(), condp)) ચાલુ;
            UINFO(9, "COND(c, CONDb(c, tt, tf), f) -> CONDb(1, tt, tf) " << nodep);
            replace_અંક(nodep->condp(), 1);
            replaced = સાચું;
            ++m_stat_If_Cond_પદાવલિ_Redundant;
        }
        માટે (અચલ Ast_ગાંઠ_પદાવલિ* condp : falsesp) {
            જો (replaced) તોડો;
            જો (!operandsSame(nodep->condp(), condp)) ચાલુ;
            UINFO(9, "COND(c, t, CONDb(c, ft, ff)) -> CONDb(0, ft, ff) " << nodep);
            replaceZero(nodep->condp());
            replaced = સાચું;
            ++m_stat_If_Cond_પદાવલિ_Redundant;
        }
        // We only check the first statement of parent IF is an If
        // So we don't need to check for effects in the executing thensp/elsesp
        // altering the child't condition.  e.g. 'if (x) begin x=1; if (x) end'
        જો (અચલ Ast_ગાંઠ_If* subIfp = VN_CAST(nodep->thensp(), ગાંઠ_If)) {
            જો (!replaced) truesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            જો (!replaced) truesp.કાઢો_back();
        }
        જો (અચલ Ast_ગાંઠ_If* subIfp = VN_CAST(nodep->elsesp(), ગાંઠ_If)) {
            જો (!replaced) falsesp.emplace_back(nodep->condp());
            matchIfCondCondRecurse(subIfp, truesp /*ref*/, falsesp /*ref*/);
            જો (!replaced) falsesp.કાઢો_back();
        }
    }

    શૂન્ય matchIfSameLast(Ast_ગાંઠ_If* nodep) {
        જો (!nodep->thensp()) પાછા;
        જો (!nodep->elsesp()) પાછા;
        જો (!canDeleteStatements()) પાછા;
        // Iterate bodies in reverse
        Ast_ગાંઠ* tp = nodep->thensp()->lastp();
        Ast_ગાંઠ* ep = nodep->elsesp()->lastp();
        જ્યારે (tp && ep && tp->same_વૃક્ષ(ep)) {
            UASSERT_OBJ(!tp->nextp(), tp, "Expected no nextp");
            UASSERT_OBJ(!ep->nextp(), ep, "Expected no nextp");
            Ast_ગાંઠ* અચલ tPrevp = tp->prevp();
            Ast_ગાંઠ* અચલ ePrevp = ep->prevp();
            nodep->addNextHere(tp->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(ep->unlinkFrBack()), ep);
            tp = tPrevp;
            ep = ePrevp;
        }
    }

    તાર્કિક matchMaskedOr(AstAnd* nodep) {
        // Masking an OR with terms that have no bits set under the mask is replaced with masking
        // only the remaining terms. Canonical example as generated by V3Expand is:
        // 0xff & (a << 8 | b >> 24) --> 0xff & (b >> 24)

        // Compute how many significant bits are in the mask
        અચલ AstConst* અચલ constp = VN_AS(nodep->lhsp(), Const);
        અચલ uint32_t significantBits = constp->અંક().widthToFit();

        AstOr* અચલ orp = VN_AS(nodep->rhsp(), Or);

        // Predicate for checking whether the bottom 'significantBits' bits of the given expression
        // are all zeroes.
        અચલ auto checkBottomClear = [=](અચલ Ast_ગાંઠ* nodep) -> તાર્કિક {
            જો (અચલ AstShiftL* અચલ shiftp = VN_CAST(nodep, ShiftL)) {
                જો (અચલ AstConst* અચલ scp = VN_CAST(shiftp->rhsp(), Const)) {
                    પાછા scp->અંક().toUInt() >= significantBits;
                }
            }
            પાછા ખોટું;
        };

        અચલ તાર્કિક orLIsRedundant = checkBottomClear(orp->lhsp());
        અચલ તાર્કિક orRIsRedundant = checkBottomClear(orp->rhsp());

        જો (orLIsRedundant && orRIsRedundant) {
            nodep->replaceWithKeepDType(
                નવું AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા સાચું;
        } નહીંતર જો (orLIsRedundant) {
            orp->replaceWithKeepDType(orp->rhsp()->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(orp), orp);
            પાછા ખોટું;  // input node is still valid, keep going
        } નહીંતર જો (orRIsRedundant) {
            orp->replaceWithKeepDType(orp->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(orp), orp);
            પાછા ખોટું;  // input node is still valid, keep going
        } નહીંતર {
            પાછા ખોટું;
        }
    }
    તાર્કિક matchMaskedShift(AstAnd* nodep) {
        // Drop redundant masking of right shift result. E.g: 0xff & ((uint32_t)a >> 24). This
        // commonly appears after V3Expand and the simplification in matchMaskedOr. Similarly,
        // drop redundant masking of left shift result. E.g.: 0xff000000 & ((uint32_t)a << 24).

        અચલ auto checkMask = [nodep, આ](અચલ V3_સંખ્યા& mask) -> તાર્કિક {
            અચલ AstConst* અચલ constp = VN_AS(nodep->lhsp(), Const);
            જો (constp->અંક().isCaseEq(mask)) {
                Ast_ગાંઠ* અચલ rhsp = nodep->rhsp();
                rhsp->unlinkFrBack();
                nodep->replaceWithKeepDType(rhsp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા સાચું;
            }
            પાછા ખોટું;
        };

        // Check if masking is redundant
        જો (અચલ AstShiftR* અચલ shiftp = VN_CAST(nodep->rhsp(), ShiftR)) {
            જો (અચલ AstConst* અચલ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_સંખ્યા maskLo{nodep, nodep->width()};
                maskLo.setMask(nodep->width() - scp->અંક().toUInt());
                પાછા checkMask(maskLo);
            }
        } નહીંતર જો (અચલ AstShiftL* અચલ shiftp = VN_CAST(nodep->rhsp(), ShiftL)) {
            જો (અચલ AstConst* અચલ scp = VN_CAST(shiftp->rhsp(), Const)) {
                // Check if mask is full over the non-zero bits
                V3_સંખ્યા mask{nodep, nodep->width()};
                અચલ uint32_t shiftAmount = scp->અંક().toUInt();
                mask.setMask(nodep->width() - shiftAmount, shiftAmount);
                પાછા checkMask(mask);
            }
        }
        પાછા ખોટું;
    }

    તાર્કિક matchMaskedZero(અચલ AstAnd* nodep) {
        // Turn masking of known zero bits into constant zero. Commonly appears after V3Expand.
        અચલ AstConst* અચલ maskp = VN_AS(nodep->lhsp(), Const);
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp();
        અચલ uint32_t msbP1 = maskp->અંક().mostSetBitP1();
        જો (!msbP1) પાછા ખોટું;  // Don't rewrite, separate rule matches for this
        અચલ uint32_t msb = msbP1 - 1;
        અચલ uint32_t lsb = maskp->અંક().leastSetBitP1() - 1;

        જો (અચલ AstShiftL* અચલ shiftp = VN_CAST(rhsp, ShiftL)) {
            // 'a << S' forces the low S bits to zero
            જો (AstConst* અચલ scp = VN_CAST(shiftp->rhsp(), Const)) {
                પાછા scp->અંક().fitsInUInt()  //
                       && (scp->અંક().toUInt() > msb);
            }
        }
        જો (અચલ AstShiftR* અચલ shiftp = VN_CAST(rhsp, ShiftR)) {
            // 'a >> S' forces the high S bits to zero. Check against the width of the shifted
            // operand, V3Expand can create shifts wider than their inputs. Must use width(),
            // not widthMin(), as bits above widthMin() are not guaranteed to be zero.
            જો (AstConst* અચલ scp = VN_CAST(shiftp->rhsp(), Const)) {
                પાછા scp->અંક().fitsInUInt()
                       && (lsb + scp->અંક().toUInt()
                           >= static_cast<uint32_t>(shiftp->lhsp()->width()));
            }
        }
        જો (અચલ AstMul* અચલ mulp = VN_CAST(rhsp, Mul)) {
            // 'C * a' forces the low N bits to zero where 'C' has low zero bits
            જો (AstConst* અચલ cp = VN_CAST(mulp->lhsp(), Const)) {
                પાછા cp->અંક().leastSetBitP1() > msb + 1;
            }
        }
        જો (અચલ AstExtend* અચલ extendp = VN_CAST(rhsp, Extend)) {
            // Zero-extension forces the bits above the source width to zero
            પાછા lsb >= static_cast<uint32_t>(extendp->lhsp()->width());
        }

        પાછા ખોટું;
    }

    તાર્કિક match_Bit_Op_વૃક્ષ(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (nodep->widthMin() != 1) પાછા ખોટું;
        જો (!v3Global.opt.f_Const_Bit_Op_વૃક્ષ()) પાછા ખોટું;

        અચલ પૂર્ણાંક width = nodep->width();
        Ast_ગાંઠ_પદાવલિ* rootp = nodep;
        unsigned externalOps = 0;
        // Reach past a plain making AND
        જો (અચલ AstAnd* અચલ andp = VN_CAST(nodep, And)) {
            જો (isConst(andp->lhsp(), 1)) {
                rootp = andp->rhsp();
                externalOps = 1;
            }
        }

        // Only optimize if rootp is in fact the root of a tree of identical
        // operations, otherwise analysis on an eventually unoptimizable unablanced
        // tree can go O(N^2) as we would re-analyze every time as we move up the
        // tree, repeatedly re-discover the sub-tree is not optimizable.
        જો (rootp->type() == nodep->backp()->type()) પાછા ખોટું;

        std::string debugPrefix;
        જો (debug() >= 9) {  // LCOV_EXCL_START
            સ્થિર પૂર્ણાંક s_c = 0;
            debugPrefix = "-  matchBitOpTree[" + std::to_string(++s_c) + "] ";
            nodep->dump_વૃક્ષ(debugPrefix + "INPUT: ");
        }  // LCOV_EXCL_STOP

        Ast_ગાંઠ_પદાવલિ* અચલ newp = Const_Bit_Op_વૃક્ષ_Visitor::simplify(rootp, width, externalOps,
                                                                  m_statBitOpReduction, m_ids);

        જો (newp) {
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Transformed leaf of bit tree to " << newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }

        જો (debug() >= 9) {  // LCOV_EXCL_START
            જો (newp) {
                newp->dump_વૃક્ષ(debugPrefix + "RESULT: ");
            } નહીંતર {
                cout << debugPrefix << "not replaced\n";
            }
        }  // LCOV_EXCL_STOP

        પાછા newp;
    }
    સ્થિર તાર્કિક operandShiftSame(અચલ Ast_ગાંઠ* nodep) {
        અચલ Ast_ગાંઠ_Biop* અચલ np = VN_AS(nodep, ગાંઠ_Biop);
        {
            અચલ AstShiftL* અચલ lp = VN_CAST(np->lhsp(), ShiftL);
            અચલ AstShiftL* અચલ rp = VN_CAST(np->rhsp(), ShiftL);
            જો (lp && rp) {
                પાછા (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        {
            અચલ AstShiftR* અચલ lp = VN_CAST(np->lhsp(), ShiftR);
            અચલ AstShiftR* અચલ rp = VN_CAST(np->rhsp(), ShiftR);
            જો (lp && rp) {
                પાછા (lp->width() == rp->width() && lp->lhsp()->width() == rp->lhsp()->width()
                        && operandsSame(lp->rhsp(), rp->rhsp()));
            }
        }
        પાછા ખોટું;
    }
    તાર્કિક operandHugeShiftL(અચલ Ast_ગાંઠ_Biop* nodep) {
        પાછા (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->અંક().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->અંક().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->width())))
                && nodep->lhsp()->isPure());
    }
    તાર્કિક operandHugeShiftR(અચલ Ast_ગાંઠ_Biop* nodep) {
        પાછા (VN_IS(nodep->rhsp(), Const) && !VN_AS(nodep->rhsp(), Const)->અંક().isFourState()
                && (!VN_AS(nodep->rhsp(), Const)->અંક().fitsInUInt()  // > 2^32 shift
                    || (VN_AS(nodep->rhsp(), Const)->toUInt()
                        >= static_cast<uint32_t>(nodep->lhsp()->width())))
                && nodep->lhsp()->isPure()
                && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)));
    }
    તાર્કિક operandIsTwo(અચલ Ast_ગાંઠ* nodep) {
        અચલ AstConst* અચલ constp = VN_CAST(nodep, Const);
        જો (!constp) પાછા ખોટું;  // not constant
        જો (constp->અંક().isFourState()) પાછા ખોટું;  // four-state
        જો (nodep->width() > VL_QUADSIZE) પાછા ખોટું;  // too wide
        જો (nodep->isSigned() && constp->અંક().isNegative()) પાછા ખોટું;  // signed and negative
        પાછા constp->toUQuad() == 2;
    }
    તાર્કિક operandIsTwostate(અચલ Ast_ગાંઠ* nodep) {
        પાછા (VN_IS(nodep, Const) && !VN_AS(nodep, Const)->અંક().isFourState());
    }
    તાર્કિક operandIsPowTwo(અચલ Ast_ગાંઠ* nodep) {
        જો (!operandIsTwostate(nodep)) પાછા ખોટું;
        પાછા (1 == VN_AS(nodep, Const)->અંક().ગણતરી_Ones());
    }
    તાર્કિક operandShiftOp(અચલ Ast_ગાંઠ_Biop* nodep) {
        જો (!VN_IS(nodep->rhsp(), Const)) પાછા ખોટું;
        અચલ Ast_ગાંઠ_Biop* અચલ lhsp = VN_CAST(nodep->lhsp(), ગાંઠ_Biop);
        જો (!lhsp || !(VN_IS(lhsp, And) || VN_IS(lhsp, Or) || VN_IS(lhsp, Xor))) પાછા ખોટું;
        જો (nodep->width() != lhsp->width()) પાછા ખોટું;
        જો (nodep->width() != lhsp->lhsp()->width()) પાછા ખોટું;
        જો (nodep->width() != lhsp->rhsp()->width()) પાછા ખોટું;

        // Only do it if we can fruther reduce one of the sides if pulling the shift through:
        // - Either operands are constants
        જો (VN_IS(lhsp->lhsp(), Const)) પાછા સાચું;
        જો (VN_IS(lhsp->rhsp(), Const)) પાછા સાચું;
        // - Const shift on either side
        જો (Ast_ગાંઠ_Biop* અચલ llp = VN_CAST(lhsp->lhsp(), ગાંઠ_Biop)) {
            પાછા (VN_IS(llp, ShiftL) || VN_IS(llp, ShiftR)) && VN_IS(llp->rhsp(), Const);
        }
        જો (Ast_ગાંઠ_Biop* અચલ lrp = VN_CAST(lhsp->rhsp(), ગાંઠ_Biop)) {
            પાછા (VN_IS(lrp, ShiftL) || VN_IS(lrp, ShiftR)) && VN_IS(lrp->rhsp(), Const);
        }
        // Otherwise we would increase logic size
        પાછા ખોટું;
    }
    તાર્કિક operandShiftShift(અચલ Ast_ગાંઠ_Biop* nodep) {
        // We could add a AND though.
        અચલ Ast_ગાંઠ_Biop* અચલ lhsp = VN_CAST(nodep->lhsp(), ગાંઠ_Biop);
        જો (!lhsp || !(VN_IS(lhsp, ShiftL) || VN_IS(lhsp, ShiftR))) પાછા ખોટું;
        // We can only get rid of a<<b>>c or a<<b<<c, with constant b & c
        // because bits may be masked in that process, or (b+c) may exceed the word width.
        જો (!(VN_IS(nodep->rhsp(), Const) && VN_IS(lhsp->rhsp(), Const))) પાછા ખોટું;
        જો (VN_AS(nodep->rhsp(), Const)->અંક().isFourState()
            || VN_AS(lhsp->rhsp(), Const)->અંક().isFourState())
            પાછા ખોટું;
        જો (nodep->width() != lhsp->width()) પાછા ખોટું;
        જો (nodep->width() != lhsp->lhsp()->width()) પાછા ખોટું;
        પાછા સાચું;
    }
    તાર્કિક operandWordOOB(અચલ AstWordSel* nodep) {
        // V3Expand may make a arraysel that exceeds the bounds of the array
        // It was an expression, then got constified.  In reality, the WordSel
        // must be wrapped in a Cond, that will be false.
        પાછા (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ગાંઠ_Var_Ref)
                && VN_AS(nodep->fromp(), ગાંઠ_Var_Ref)->access().isReadOnly()
                && (static_cast<પૂર્ણાંક>(VN_AS(nodep->bitp(), Const)->toUInt())
                    >= VN_AS(nodep->fromp(), ગાંઠ_Var_Ref)->varp()->width_શબ્દો()));
    }
    તાર્કિક operandSelFull(અચલ AstSel* nodep) {
        પાછા (VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0
                && static_cast<પૂર્ણાંક>(nodep->widthConst()) == nodep->fromp()->width());
    }
    તાર્કિક operandSelExtend(AstSel* nodep) {
        જો (!m_doV) પાછા ખોટું;
        AstExtend* અચલ extendp = VN_CAST(nodep->fromp(), Extend);
        જો (!extendp) પાછા ખોટું;
        AstConst* અચલ lsbp = VN_CAST(nodep->lsbp(), Const);
        જો (!lsbp) પાછા ખોટું;
        અચલ પૂર્ણાંક width = nodep->widthConst();
        અચલ પૂર્ણાંક lsb = lsbp->toSInt();
        અચલ પૂર્ણાંક msb = lsb + width - 1;
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = extendp->lhsp();
        જો (!lhsp->isPure()) પાછા ખોટું;

        // Selecting the entire extended expression, replace with that
        જો (lsb == 0 && msb == lhsp->width() - 1) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(lhsp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા સાચું;
        }
        // Select entirely in the extended part - replace with zero
        જો (lsb >= lhsp->width()) {
            replaceZero(nodep);
            પાછા સાચું;
        }
        // Select entirely in the extended expression - replace with select from that
        જો (msb < lhsp->width()) {
            lhsp->unlinkFrBack();
            lsbp->unlinkFrBack();
            nodep->replaceWithKeepDType(નવું AstSel{nodep->fileline(), lhsp, lsbp, width});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા સાચું;
        }
        // Select straddles both sides, but is just a shorter extend
        જો (lsb == 0) {
            lhsp->unlinkFrBack();
            nodep->replaceWithKeepDType(નવું AstExtend{nodep->fileline(), lhsp});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા સાચું;
        }

        પાછા ખોટું;
    }
    તાર્કિક operandSelBiLower(AstSel* nodep) {
        // SEL(ADD(a,b),(width-1),0) -> ADD(SEL(a),SEL(b))
        // Add or any operation which doesn't care if we discard top bits
        Ast_ગાંઠ_Biop* અચલ bip = VN_CAST(nodep->fromp(), ગાંઠ_Biop);
        જો (!(m_doV && bip && VN_IS(nodep->lsbp(), Const) && nodep->lsbConst() == 0)) પાછા ખોટું;
        UINFOTREE(9, nodep, "", "SEL(BI)-in");
        Ast_ગાંઠ_પદાવલિ* અચલ bilhsp = bip->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ birhsp = bip->rhsp()->unlinkFrBack();
        bip->lhsp(નવું AstSel{nodep->fileline(), bilhsp, 0, nodep->widthConst()});
        bip->rhsp(નવું AstSel{nodep->fileline(), birhsp, 0, nodep->widthConst()});
        UINFOTREE(9, bip, "", "SEL(BI)-ou");
        VL_DO_DANGLING(replaceWChild(nodep, bip), nodep);
        પાછા સાચું;
    }
    તાર્કિક operandSelShiftLower(AstSel* nodep) {
        // AND({a}, SHIFTR({b}, {c})) is often shorthand in C for Verilog {b}[{c} :+ {a}]
        // becomes thought other optimizations
        // SEL(SHIFTR({a},{b}),{lsb},{width}) -> SEL({a},{lsb+b},{width})
        અચલ AstShiftR* અચલ shiftp = VN_CAST(nodep->fromp(), ShiftR);
        જો (!(m_doV && shiftp && VN_IS(shiftp->rhsp(), Const) && VN_IS(nodep->lsbp(), Const))) {
            પાછા ખોટું;
        }
        Ast_ગાંઠ_પદાવલિ* અચલ ap = shiftp->lhsp();
        અચલ AstConst* અચલ bp = VN_AS(shiftp->rhsp(), Const);
        અચલ AstConst* અચલ lp = VN_AS(nodep->lsbp(), Const);
        જો (bp->isWide() || bp->અંક().isFourState() || bp->અંક().isNegative() || lp->isWide()
            || lp->અંક().isFourState() || lp->અંક().isNegative()) {
            પાછા ખોટું;
        }
        અચલ પૂર્ણાંક newLsb = lp->toSInt() + bp->toSInt();
        જો (newLsb + nodep->widthConst() > ap->width()) પાછા ખોટું;
        //
        UINFO(9, "SEL(SHIFTR(a,b),l,w) -> SEL(a,l+b,w)");
        UINFOTREE(9, nodep, "", "SEL(SH)-in");
        AstSel* અચલ newp
            = નવું AstSel{nodep->fileline(), ap->unlinkFrBack(), newLsb, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        UINFOTREE(9, newp, "", "SEL(SH)-ou");
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }

    તાર્કિક operandBiExtendConstShrink(Ast_ગાંઠ_Biop* nodep) {
        // Loop unrolling favors standalone compares
        // EQ(const{width32}, EXTEND(xx{width3})) -> EQ(const{3}, xx{3})
        // The constant must have zero bits (+ 1 if signed) or compare
        // would be incorrect. See also operandBiExtendConst
        AstExtend* અચલ extendp = VN_CAST(nodep->rhsp(), Extend);
        જો (!extendp) પાછા ખોટું;
        Ast_ગાંઠ_પદાવલિ* અચલ smallerp = extendp->lhsp();
        અચલ પૂર્ણાંક subsize = smallerp->width();
        AstConst* અચલ constp = VN_CAST(nodep->lhsp(), Const);
        જો (!constp) પાછા ખોટું;
        જો (!constp->અંક().isBitsZero(constp->width() - 1, subsize)) પાછા ખોટું;
        //
        UINFOTREE(9, nodep, "", "BI(EXTEND)-in");
        smallerp->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(extendp->unlinkFrBack()), extendp);  // aka nodep->lhsp.
        nodep->rhsp(smallerp);

        constp->unlinkFrBack();
        અચલ V3_સંખ્યા અંક{constp, subsize, constp->અંક()};
        nodep->lhsp(નવું AstConst{constp->fileline(), અંક});
        VL_DO_DANGLING(ધકેલો_Deletep(constp), constp);
        પાછા ખોટું;  // input node is still valid, keep going
    }
    તાર્કિક operandBiExtendConstOver(અચલ Ast_ગાંઠ_Biop* nodep) {
        // EQ(const{width32}, EXTEND(xx{width3})) -> constant
        // When the constant has non-zero bits above the extend it's a constant.
        // Avoids compiler warning
        અચલ AstExtend* અચલ extendp = VN_CAST(nodep->rhsp(), Extend);
        જો (!extendp) પાછા ખોટું;
        અચલ Ast_ગાંઠ* અચલ smallerp = extendp->lhsp();
        અચલ પૂર્ણાંક subsize = smallerp->width();
        અચલ AstConst* અચલ constp = VN_CAST(nodep->lhsp(), Const);
        જો (!constp) પાછા ખોટું;
        જો (constp->અંક().isBitsZero(constp->width() - 1, subsize)) પાછા ખોટું;
        પાછા સાચું;
    }

    // Extraction checks
    તાર્કિક warnSelect(AstSel* nodep) {
        જો (m_doGenerate) {
            // Never checked yet
            V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);  // May need "constifying"
        }
        // Find range of dtype we are selecting from
        // Similar code in V3Unknown::AstSel
        અચલ તાર્કિક doit = સાચું;
        જો (m_warn && VN_IS(nodep->lsbp(), Const) && doit) {
            અચલ પૂર્ણાંક maxDeclBit = nodep->declRange().hiMaxSelect() * nodep->declElWidth()
                                   + (nodep->declElWidth() - 1);
            જો (VN_AS(nodep->lsbp(), Const)->અંક().isFourState()) {
                nodep->v3error("Selection index is constantly unknown or tristated: "
                               "lsb="
                               << nodep->lsbp()->name() << " width=" << nodep->widthConst());
                // Replacing nodep will make a mess above, so we replace the offender
                replaceZero(nodep->lsbp());
            } નહીંતર જો (nodep->declRange().ranged()
                       && (nodep->msbConst() > maxDeclBit || nodep->lsbConst() > maxDeclBit)) {
                // See also warning in V3Width
                // Must adjust by element width as declRange() is in number of elements
                string msbLsbProtected;
                જો (nodep->declElWidth() == 0) {
                    msbLsbProtected = "(nodep->declElWidth() == 0) "
                                      + std::to_string(nodep->msbConst()) + ":"
                                      + std::to_string(nodep->lsbConst());
                } નહીંતર {
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
        પાછા ખોટું;  // Not a transform, so NOP
    }

    સ્થિર તાર્કિક operandsSame(અચલ Ast_ગાંઠ* node1p, અચલ Ast_ગાંઠ* node2p) {
        // For now we just detect constants & simple vars, though it could be more generic
        જો (અચલ AstConst* અચલ const1p = VN_CAST(node1p, Const)) {
            જો (અચલ AstConst* અચલ const2p = VN_CAST(node2p, Const)) {
                પાછા V3_સંખ્યા{node1p->fileline(), 1, 0}
                    .opEq(const1p->અંક(), const2p->અંક())
                    .isEqOne();
            }
        }
        જો (VN_IS(node1p, VarRef) && VN_IS(node2p, VarRef)) {
            // Avoid comparing widthMin's, which results in lost optimization attempts
            // If cleanup sameGateTree to be smarter, this can be restored.
            // return node1p->sameGateTree(node2p);
            પાછા node1p->isSame(node2p);
        }
        // Pattern created by coverage-line/branch; avoid compiler tautological-compare warning
        જો (અચલ AstAnd* અચલ and1p = VN_CAST(node1p, And)) {
            જો (અચલ AstAnd* અચલ and2p = VN_CAST(node2p, And)) {
                જો (VN_IS(and1p->lhsp(), Const) && VN_IS(and1p->rhsp(), ગાંઠ_Var_Ref)
                    && VN_IS(and2p->lhsp(), Const) && VN_IS(and2p->rhsp(), ગાંઠ_Var_Ref))
                    પાછા node1p->same_Gate_વૃક્ષ(node2p);
            }
        }
        પાછા ખોટું;
    }
    તાર્કિક ifSameAssign(અચલ Ast_ગાંઠ_If* nodep) {
        અચલ Ast_ગાંઠ_Assign* અચલ thensp = VN_CAST(nodep->thensp(), ગાંઠ_Assign);
        અચલ Ast_ગાંઠ_Assign* અચલ elsesp = VN_CAST(nodep->elsesp(), ગાંઠ_Assign);
        જો (!thensp || thensp->nextp()) પાછા ખોટું;  // Must be SINGLE statement
        જો (!elsesp || elsesp->nextp()) પાછા ખોટું;
        જો (thensp->type() != elsesp->type()) પાછા ખોટું;  // Can't mix an assigndly with assign
        જો (!thensp->lhsp()->same_Gate_વૃક્ષ(elsesp->lhsp())) પાછા ખોટું;
        જો (!thensp->rhsp()->gate_વૃક્ષ()) પાછા ખોટું;
        જો (!elsesp->rhsp()->gate_વૃક્ષ()) પાછા ખોટું;
        // Must not create an expression with unpacked array type
        જો (VN_IS(thensp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) પાછા ખોટું;
        જો (VN_IS(elsesp->rhsp()->dtypep()->skipRefp(), UnpackArrayDType)) પાછા ખોટું;
        જો (m_underRecFunc) પાછા ખોટું;  // This optimization may lead to infinite recursion
        જો (m_doCpp) {  // Do not combine ifs cretaed by V3Premit
            જો (VN_IS(thensp->lhsp(), VarRef) && thensp->lhsp()->isWide()) પાછા ખોટું;
        }
        // Only do it if not calls and both pure, otherwise undoes V3LiftExpr
        પાછા !VN_IS(thensp->rhsp(), ગાંઠ_FTask_Ref)  //
               && !VN_IS(elsesp->rhsp(), ગાંઠ_FTask_Ref)  //
               && thensp->rhsp()->isPure()  //
               && elsesp->rhsp()->isPure();
    }
    તાર્કિક operandIfIf(અચલ Ast_ગાંઠ_If* nodep) {
        જો (nodep->elsesp()) પાછા ખોટું;
        અચલ Ast_ગાંઠ_If* અચલ lowerIfp = VN_CAST(nodep->thensp(), ગાંઠ_If);
        જો (!lowerIfp || lowerIfp->nextp()) પાછા ખોટું;
        જો (nodep->type() != lowerIfp->type()) પાછા ખોટું;
        જો (Ast_ગાંઠ::afterCommentp(lowerIfp->elsesp())) પાછા ખોટું;
        પાછા સાચું;
    }
    તાર્કિક ifConcatMergeableBiop(અચલ Ast_ગાંઠ* nodep) {
        પાછા (VN_IS(nodep, And) || VN_IS(nodep, Or) || VN_IS(nodep, Xor));
    }
    તાર્કિક ifAdjacentSel(અચલ AstSel* lhsp, અચલ AstSel* rhsp) {
        જો (!v3Global.opt.fAssemble()) પાછા ખોટું;  // opt disabled
        જો (!lhsp || !rhsp) પાછા ખોટું;
        અચલ Ast_ગાંઠ* અચલ lfromp = lhsp->fromp();
        અચલ Ast_ગાંઠ* અચલ rfromp = rhsp->fromp();
        જો (!lfromp || !rfromp || !lfromp->same_Gate_વૃક્ષ(rfromp)) પાછા ખોટું;
        અચલ AstConst* અચલ lstart = VN_CAST(lhsp->lsbp(), Const);
        અચલ AstConst* અચલ rstart = VN_CAST(rhsp->lsbp(), Const);
        જો (!lstart || !rstart) પાછા ખોટું;  // too complicated
        અચલ પૂર્ણાંક rend = (rstart->toSInt() + rhsp->widthConst());
        પાછા (rend == lstart->toSInt());
    }
    તાર્કિક ifMergeAdjacent(અચલ Ast_ગાંઠ_પદાવલિ* lhsp, અચલ Ast_ગાંઠ_પદાવલિ* rhsp) {
        // Called by concatmergeable to determine if {lhsp, rhsp} make sense
        જો (!v3Global.opt.fAssemble()) પાછા ખોટું;  // opt disabled
        // Two same varref
        જો (operandsSame(lhsp, rhsp)) પાછા સાચું;
        અચલ AstSel* અચલ lselp = VN_CAST(lhsp, Sel);
        અચલ AstSel* અચલ rselp = VN_CAST(rhsp, Sel);
        જો (!lselp && !rselp) પાછા ખોટું;
        જો (lselp && !VN_IS(lselp->lsbp(), Const)) પાછા ખોટું;
        જો (rselp && !VN_IS(rselp->lsbp(), Const)) પાછા ખોટું;
        // a[i:j] {a[j-1:k], b}
        જો (lselp && !rselp && VN_IS(rhsp, Concat))
            પાછા ifMergeAdjacent(lhsp, VN_AS(rhsp, Concat)->lhsp());
        // {b, a[j:k]} a[k-1:i]
        જો (rselp && !lselp && VN_IS(lhsp, Concat))
            પાછા ifMergeAdjacent(VN_AS(lhsp, Concat)->rhsp(), rhsp);
        // a a[msb:j]
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ lfromp = lselp                                ? lselp->fromp()
                                          : lhsp->same_Gate_વૃક્ષ(rselp->fromp()) ? lhsp
                                                                               : nullptr;
        // a[i:0] a
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ rfromp = rselp                                ? rselp->fromp()
                                          : rhsp->same_Gate_વૃક્ષ(lselp->fromp()) ? rhsp
                                                                               : nullptr;
        જો (!lfromp || !rfromp || !lfromp->same_Gate_વૃક્ષ(rfromp)) પાછા ખોટું;

        અચલ int32_t lstart = lselp ? lselp->lsbConst() : 0;
        અચલ int32_t rstart = rselp ? rselp->lsbConst() : 0;
        અચલ int32_t rend = rstart + (rselp ? rselp->widthConst() : rhsp->width());
        // a[i:j] a[j-1:k]
        જો (rend == lstart) પાછા સાચું;
        // a[i:0] a[msb:j]
        જો (rend == rfromp->width() && lstart == 0) પાછા સાચું;
        પાછા ખોટું;
    }
    તાર્કિક concatMergeable(અચલ Ast_ગાંઠ_પદાવલિ* lhsp, અચલ Ast_ગાંઠ_પદાવલિ* rhsp, unsigned depth) {
        // Determine if {a OP b, c OP d} => {a, c} OP {b, d} is advantageous
        જો (!v3Global.opt.fAssemble()) પાછા ખોટું;  // opt disabled
        જો (lhsp->type() != rhsp->type()) પાછા ખોટું;
        જો (!ifConcatMergeableBiop(lhsp)) પાછા ખોટું;
        જો (depth > CONCAT_MERGABLE_MAX_DEPTH) પાછા ખોટું;  // As worse case O(n^2) algorithm

        અચલ Ast_ગાંઠ_Biop* અચલ lp = VN_CAST(lhsp, ગાંઠ_Biop);
        અચલ Ast_ગાંઠ_Biop* અચલ rp = VN_CAST(rhsp, ગાંઠ_Biop);
        જો (!lp || !rp) પાછા ખોટું;
        // {a[]&b[], a[]&b[]}
        અચલ તાર્કિક lad = ifMergeAdjacent(lp->lhsp(), rp->lhsp());
        અચલ તાર્કિક rad = ifMergeAdjacent(lp->rhsp(), rp->rhsp());
        જો (lad && rad) પાછા સાચું;
        // {a[] & b[]&c[], a[] & b[]&c[]}
        જો (lad && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) પાછા સાચું;
        // {a[]&b[] & c[], a[]&b[] & c[]}
        જો (rad && concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)) પાછા સાચું;
        // {(a[]&b[])&(c[]&d[]), (a[]&b[])&(c[]&d[])}
        જો (concatMergeable(lp->lhsp(), rp->lhsp(), depth + 1)
            && concatMergeable(lp->rhsp(), rp->rhsp(), depth + 1)) {
            પાછા સાચું;
        }
        પાછા ખોટું;
    }
    સ્થિર તાર્કિક operandsSameWidth(અચલ Ast_ગાંઠ* lhsp, અચલ Ast_ગાંઠ* rhsp) {
        પાછા lhsp->width() == rhsp->width();
    }

    //----------------------------------------
    // Constant Replacement functions.
    // These all take a node, delete its tree, and replaces it with a constant

    શૂન્ય replace_અંક(Ast_ગાંઠ* oldp, અચલ V3_સંખ્યા& અંક, અચલ string& origParamName = "") {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        UASSERT_OBJ(!(VN_IS(oldp, Const) && !VN_AS(oldp, Const)->અંક().isFourState()), oldp,
                    "Already constant??");
        AstConst* અચલ newp = નવું AstConst{oldp->fileline(), અંક};
        જો (!origParamName.empty()) newp->origParamName(origParamName);
        oldp->replaceWithKeepDType(newp);
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        VL_DO_DANGLING(ધકેલો_Deletep(oldp), oldp);
    }
    શૂન્ય replace_અંક(Ast_ગાંઠ* nodep, uint32_t val) {
        અચલ V3_સંખ્યા અંક{nodep, nodep->width(), val};
        VL_DO_DANGLING(replace_અંક(nodep, અંક), nodep);
    }
    શૂન્ય replace_અંક_Signed(Ast_ગાંઠ_Biop* nodep, uint32_t val) {
        // We allow both sides to be constant, as one may have come from
        // parameter propagation, etc.
        જો (m_warn && !(VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const))) {
            nodep->v3warn(UNSIGNED, "Comparison is constant due to unsigned arithmetic");
        }
        VL_DO_DANGLING(replace_અંક(nodep, val), nodep);
    }
    શૂન્ય replace_અંક_Limited(Ast_ગાંઠ_Biop* nodep, uint32_t val) {
        // Avoids gcc warning about same
        જો (m_warn) nodep->v3warn(CMPCONST, "Comparison is constant due to limited range");
        VL_DO_DANGLING(replace_અંક(nodep, val), nodep);
    }
    શૂન્ય replaceZero(Ast_ગાંઠ* nodep) { VL_DO_DANGLING(replace_અંક(nodep, 0), nodep); }
    શૂન્ય replaceZeroChkPure(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* checkp) {
        // For example, "0 * n" -> 0 if n has no side effects
        // Else strength reduce it to 0 & n.
        // If ever change the operation note AstAnd rule specially ignores this created pattern
        જો (checkp->isPure()) {
            VL_DO_DANGLING(replace_અંક(nodep, 0), nodep);
        } નહીંતર {
            Ast_ગાંઠ* અચલ newp = નવું AstAnd{nodep->fileline(), નવું AstConst{nodep->fileline(), 0},
                                             checkp->unlinkFrBack()};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય replaceAllOnes(Ast_ગાંઠ* nodep) {
        V3_સંખ્યા ones{nodep, nodep->width(), 0};
        ones.setMask(nodep->width());
        VL_DO_DANGLING(replace_અંક(nodep, ones), nodep);
    }
    શૂન્ય replaceConst(Ast_ગાંઠ_Uniop* nodep) {
        V3_સંખ્યા numv{nodep, nodep->widthMinV()};
        nodep->સંખ્યા_Operate(numv, const_અંક_V(nodep->lhsp()));
        અચલ V3_સંખ્યા& અંક = to_અંક_C(nodep, numv);
        UINFO(4, "UNICONST -> " << અંક);
        VL_DO_DANGLING(replace_અંક(nodep, અંક), nodep);
    }
    શૂન્ય replaceConst(Ast_ગાંઠ_Biop* nodep) {
        V3_સંખ્યા numv{nodep, nodep->widthMinV()};
        nodep->સંખ્યા_Operate(numv, const_અંક_V(nodep->lhsp()), const_અંક_V(nodep->rhsp()));
        અચલ V3_સંખ્યા& અંક = to_અંક_C(nodep, numv);
        UINFO(4, "BICONST -> " << અંક);
        VL_DO_DANGLING(replace_અંક(nodep, અંક), nodep);
    }
    શૂન્ય replaceConst(Ast_ગાંઠ_Triop* nodep) {
        V3_સંખ્યા numv{nodep, nodep->widthMinV()};
        nodep->સંખ્યા_Operate(numv, const_અંક_V(nodep->lhsp()), const_અંક_V(nodep->rhsp()),
                             const_અંક_V(nodep->thsp()));
        અચલ V3_સંખ્યા& અંક = to_અંક_C(nodep, numv);
        UINFO(4, "TRICONST -> " << અંક);
        VL_DO_DANGLING(replace_અંક(nodep, અંક), nodep);
    }
    શૂન્ય replaceConst(Ast_ગાંઠ_Quadop* nodep) {
        V3_સંખ્યા numv{nodep, nodep->widthMinV()};
        nodep->સંખ્યા_Operate(numv, const_અંક_V(nodep->lhsp()), const_અંક_V(nodep->rhsp()),
                             const_અંક_V(nodep->thsp()), const_અંક_V(nodep->fhsp()));
        અચલ V3_સંખ્યા& અંક = to_અંક_C(nodep, numv);
        UINFO(4, "QUADCONST -> " << અંક);
        VL_DO_DANGLING(replace_અંક(nodep, અંક), nodep);
    }

    શૂન્ય replaceConstString(Ast_ગાંઠ* oldp, અચલ string& અંક) {
        // Replace oldp node with a constant set to specified value
        UASSERT(oldp, "Null old");
        Ast_ગાંઠ* અચલ newp = નવું AstConst{oldp->fileline(), AstConst::String{}, અંક};
        UINFOTREE(6, oldp, "", "const_old");
        UINFOTREE(6, newp, "", "_new");
        oldp->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(oldp), oldp);
    }
    //----------------------------------------
    // Replacement functions.
    // These all take a node and replace it with something else

    શૂન્ય replaceWChild(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* childp) {
        // NODE(..., CHILD(...)) -> CHILD(...)
        childp->unlinkFrBackWithNext();
        // If replacing a SEL for example, the data type comes from the parent (is less wide).
        // This may adversely affect the operation of the node being replaced.
        nodep->replaceWithKeepDType(childp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય replaceWChildBool(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* childp) {
        // NODE(..., CHILD(...)) -> REDOR(CHILD(...))
        childp->unlinkFrBack();
        જો (childp->width1()) {
            nodep->replaceWithKeepDType(childp);
        } નહીંતર {
            nodep->replaceWithKeepDType(નવું AstRedOr{childp->fileline(), childp});
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    //! Replace a ternary node with its RHS after iterating
    //! Used with short-circuiting, where the RHS has not yet been iterated.
    શૂન્ય replaceWIteratedRhs(Ast_ગાંઠ_Triop* nodep) {
        જો (Ast_ગાંઠ* અચલ rhsp = nodep->rhsp()) iterateAndNextNull(rhsp);
        replaceWChild(nodep, nodep->rhsp());  // May have changed
    }

    //! Replace a ternary node with its THS after iterating
    //! Used with short-circuiting, where the THS has not yet been iterated.
    શૂન્ય replaceWIteratedThs(Ast_ગાંઠ_Triop* nodep) {
        જો (Ast_ગાંઠ* અચલ thsp = nodep->thsp()) iterateAndNextNull(thsp);
        replaceWChild(nodep, nodep->thsp());  // May have changed
    }
    શૂન્ય replaceWLhs(Ast_ગાંઠ_Uniop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    શૂન્ય replaceWLhs(Ast_ગાંઠ_Biop* nodep) {
        // Keep LHS, remove RHS
        replaceWChild(nodep, nodep->lhsp());
    }
    શૂન્ય replaceWRhs(Ast_ગાંઠ_Biop* nodep) {
        // Keep RHS, remove LHS
        replaceWChild(nodep, nodep->rhsp());
    }
    શૂન્ય replaceWLhsBool(Ast_ગાંઠ_Biop* nodep) { replaceWChildBool(nodep, nodep->lhsp()); }
    શૂન્ય replaceWRhsBool(Ast_ગાંઠ_Biop* nodep) { replaceWChildBool(nodep, nodep->rhsp()); }
    શૂન્ય replaceAsv(Ast_ગાંઠ_Biop* nodep) {
        // BIASV(CONSTa, BIASV(CONSTb, c)) -> BIASV( BIASV_CONSTED(a,b), c)
        // BIASV(SAMEa,  BIASV(SAMEb, c))  -> BIASV( BIASV(SAMEa,SAMEb), c)
        // UINFOTREE(1, nodep, "", "repAsvConst_old");
        Ast_ગાંઠ_પદાવલિ* અચલ ap = nodep->lhsp();
        Ast_ગાંઠ_Biop* અચલ rp = VN_AS(nodep->rhsp(), ગાંઠ_Biop);
        Ast_ગાંઠ_પદાવલિ* અચલ bp = rp->lhsp();
        Ast_ગાંઠ_પદાવલિ* અચલ cp = rp->rhsp();
        ap->unlinkFrBack();
        bp->unlinkFrBack();
        cp->unlinkFrBack();
        rp->unlinkFrBack();
        nodep->lhsp(rp);
        nodep->rhsp(cp);
        rp->lhsp(ap);
        rp->rhsp(bp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        જો (VN_IS(rp->lhsp(), Const) && VN_IS(rp->rhsp(), Const)) replaceConst(rp);
        iterate(nodep);  // Proceed to fixed point
    }
    શૂન્ય replaceAsvLUp(Ast_ગાંઠ_Biop* nodep) {
        // BIASV(BIASV(CONSTll,lr),r) -> BIASV(CONSTll,BIASV(lr,r))
        Ast_ગાંઠ_Biop* અચલ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ગાંઠ_Biop);
        Ast_ગાંઠ_પદાવલિ* અચલ llp = lp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rp = nodep->rhsp()->unlinkFrBack();
        nodep->lhsp(llp);
        nodep->rhsp(lp);
        lp->lhsp(lrp);
        lp->rhsp(rp);
        lp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    શૂન્ય replaceAsvRUp(Ast_ગાંઠ_Biop* nodep) {
        // BIASV(l,BIASV(CONSTrl,rr)) -> BIASV(CONSTrl,BIASV(l,rr))
        Ast_ગાંઠ_પદાવલિ* અચલ lp = nodep->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_Biop* અચલ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ગાંઠ_Biop);
        Ast_ગાંઠ_પદાવલિ* અચલ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rrp = rp->rhsp()->unlinkFrBack();
        nodep->lhsp(rlp);
        nodep->rhsp(rp);
        rp->lhsp(lp);
        rp->rhsp(rrp);
        rp->dtypeFrom(nodep);  // Upper widthMin more likely correct
        iterate(nodep);  // Proceed to fixed point
    }
    શૂન્ય replaceAndOr(Ast_ગાંઠ_Biop* nodep) {
        //  OR  (AND (CONSTll,lr), AND(CONSTrl==ll,rr))    -> AND (CONSTll, OR(lr,rr))
        //  OR  (AND (CONSTll,lr), AND(CONSTrl,    rr=lr)) -> AND (OR(ll,rl), rr)
        // nodep ^lp  ^llp   ^lrp  ^rp  ^rlp       ^rrp
        // (Or/And may also be reversed)
        Ast_ગાંઠ_Biop* અચલ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ગાંઠ_Biop);
        Ast_ગાંઠ_પદાવલિ* અચલ llp = lp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_Biop* અચલ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ગાંઠ_Biop);
        Ast_ગાંઠ_પદાવલિ* અચલ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        જો (operandsSame(llp, rlp)) {
            lp->lhsp(llp);
            lp->rhsp(nodep);
            lp->dtypeFrom(nodep);
            nodep->lhsp(lrp);
            nodep->rhsp(rrp);
            VL_DO_DANGLING(ધકેલો_Deletep(rp), rp);
            VL_DO_DANGLING(ધકેલો_Deletep(rlp), rlp);
        } નહીંતર જો (operandsSame(lrp, rrp)) {
            lp->lhsp(nodep);
            lp->rhsp(rrp);
            lp->dtypeFrom(nodep);
            nodep->lhsp(llp);
            nodep->rhsp(rlp);
            VL_DO_DANGLING(ધકેલો_Deletep(rp), rp);
            VL_DO_DANGLING(ધકેલો_Deletep(lrp), lrp);
        } નહીંતર {
            nodep->v3fatalSrc("replaceAndOr on something operandAndOrSame shouldn't have matched");
        }
        // UINFOTREE(1, nodep, "", "repAndOr_new");
    }
    શૂન્ય replaceShiftSame(Ast_ગાંઠ_Biop* nodep) {
        // Or(Shift(ll,CONSTlr),Shift(rl,CONSTrr==lr)) -> Shift(Or(ll,rl),CONSTlr)
        // (Or/And may also be reversed)
        Ast_ગાંઠ_Biop* અચલ lp = VN_AS(nodep->lhsp()->unlinkFrBack(), ગાંઠ_Biop);
        Ast_ગાંઠ_પદાવલિ* અચલ llp = lp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ lrp = lp->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_Biop* અચલ rp = VN_AS(nodep->rhsp()->unlinkFrBack(), ગાંઠ_Biop);
        Ast_ગાંઠ_પદાવલિ* અચલ rlp = rp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rrp = rp->rhsp()->unlinkFrBack();
        nodep->replaceWithKeepDType(lp);
        lp->lhsp(nodep);
        lp->rhsp(lrp);
        nodep->lhsp(llp);
        nodep->rhsp(rlp);
        nodep->dtypep(llp->dtypep());  // dtype of Biop is before shift.
        VL_DO_DANGLING(ધકેલો_Deletep(rp), rp);
        VL_DO_DANGLING(ધકેલો_Deletep(rrp), rrp);
        // UINFOTREE(1, nodep, "", "repShiftSame_new");
    }
    શૂન્ય replaceConcatSel(AstConcat* nodep) {
        // {a[1], a[0]} -> a[1:0]
        AstSel* અચલ lselp = VN_AS(nodep->lhsp()->unlinkFrBack(), Sel);
        AstSel* અચલ rselp = VN_AS(nodep->rhsp()->unlinkFrBack(), Sel);
        અચલ પૂર્ણાંક lstart = lselp->lsbConst();
        અચલ પૂર્ણાંક lwidth = lselp->widthConst();
        અચલ પૂર્ણાંક rstart = rselp->lsbConst();
        અચલ પૂર્ણાંક rwidth = rselp->widthConst();

        UASSERT_OBJ((rstart + rwidth) == lstart, nodep,
                    "tried to merge two selects which are not adjacent");
        AstSel* અચલ newselp = નવું AstSel{
            lselp->fromp()->fileline(), rselp->fromp()->unlinkFrBack(), rstart, lwidth + rwidth};
        UINFO(5, "merged two adjacent sel " << lselp << " and " << rselp << " to one " << newselp);

        nodep->replaceWithKeepDType(newselp);
        VL_DO_DANGLING(ધકેલો_Deletep(lselp), lselp);
        VL_DO_DANGLING(ધકેલો_Deletep(rselp), rselp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય replaceConcatMerge(AstConcat* nodep) {
        // {llp OP lrp, rlp OP rrp} => {llp, rlp} OP {lrp, rrp}, where OP = AND/OR/XOR
        Ast_ગાંઠ_Biop* અચલ lp = VN_AS(nodep->lhsp(), ગાંઠ_Biop);
        Ast_ગાંઠ_Biop* અચલ rp = VN_AS(nodep->rhsp(), ગાંઠ_Biop);
        જો (concatMergeable(lp, rp, 0)) {
            Ast_ગાંઠ_પદાવલિ* અચલ llp = lp->lhsp();
            Ast_ગાંઠ_પદાવલિ* અચલ lrp = lp->rhsp();
            Ast_ગાંઠ_પદાવલિ* અચલ rlp = rp->lhsp();
            Ast_ગાંઠ_પદાવલિ* અચલ rrp = rp->rhsp();
            AstConcat* અચલ newlp = નવું AstConcat{rlp->fileline(), llp->clone_વૃક્ષ_Pure(ખોટું),
                                                   rlp->clone_વૃક્ષ_Pure(ખોટું)};
            AstConcat* અચલ newrp = નવું AstConcat{rrp->fileline(), lrp->clone_વૃક્ષ_Pure(ખોટું),
                                                   rrp->clone_વૃક્ષ_Pure(ખોટું)};
            // use the lhs to replace the parent concat
            llp->replaceWith(newlp);
            VL_DO_DANGLING(ધકેલો_Deletep(llp), llp);
            lrp->replaceWith(newrp);
            VL_DO_DANGLING(ધકેલો_Deletep(lrp), lrp);
            lp->dtypeChgWidthSigned(newlp->width(), newlp->width(), VSigning::UNSIGNED);
            UINFO(5, "merged " << nodep);
            ++m_statConcatMerge;
            VL_DO_DANGLING(ધકેલો_Deletep(rp->unlinkFrBack()), rp);
            nodep->replaceWithKeepDType(lp->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            iterate(lp->lhsp());
            iterate(lp->rhsp());
        } નહીંતર {
            nodep->v3fatalSrc("tried to merge two Concat which are not adjacent");
        }
    }
    શૂન્ય replaceExtend(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* arg0p) {
        // -> EXTEND(nodep)
        // like a AstExtend{$rhsp}, but we need to set the width correctly from base node
        arg0p->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ newp
            = (VN_IS(nodep, ExtendS)
                   ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstExtendS{nodep->fileline(), arg0p})
                   : static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstExtend{nodep->fileline(), arg0p}));
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય replacePowShift(Ast_ગાંઠ_Biop* nodep) {  // Pow or PowS
        UINFO(5, "POW(2,b)->SHIFTL(1,b) " << nodep);
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* અચલ newp
            = નવું AstShiftL{nodep->fileline(), નવું AstConst{nodep->fileline(), 1}, rhsp};
        newp->lhsp()->dtypeFrom(nodep);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય replaceMulShift(AstMul* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "MUL(2^n,b)->SHIFTL(b,n) " << nodep);
        અચલ પૂર્ણાંક amount = VN_AS(nodep->lhsp(), Const)->અંક().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_ગાંઠ_પદાવલિ* અચલ opp = nodep->rhsp()->unlinkFrBack();
        AstShiftL* અચલ newp
            = નવું AstShiftL{nodep->fileline(), opp, નવું AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય replaceDivShift(AstDiv* nodep) {  // Mul, but not MulS as not simple shift
        UINFO(5, "DIV(b,2^n)->SHIFTR(b,n) " << nodep);
        અચલ પૂર્ણાંક amount = VN_AS(nodep->rhsp(), Const)->અંક().mostSetBitP1() - 1;  // 2^n->n+1
        Ast_ગાંઠ_પદાવલિ* અચલ opp = nodep->lhsp()->unlinkFrBack();
        AstShiftR* અચલ newp
            = નવું AstShiftR{nodep->fileline(), opp, નવું AstConst(nodep->fileline(), amount)};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય replaceModAnd(AstModDiv* nodep) {  // Mod, but not ModS as not simple shift
        UINFO(5, "MOD(b,2^n)->AND(b,2^n-1) " << nodep);
        અચલ પૂર્ણાંક amount = VN_AS(nodep->rhsp(), Const)->અંક().mostSetBitP1() - 1;  // 2^n->n+1
        V3_સંખ્યા mask{nodep, nodep->width()};
        mask.setMask(amount);
        Ast_ગાંઠ_પદાવલિ* અચલ opp = nodep->lhsp()->unlinkFrBack();
        AstAnd* અચલ newp
            = નવું AstAnd{nodep->fileline(), opp, નવું AstConst{nodep->fileline(), mask}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય replaceShiftOp(Ast_ગાંઠ_Biop* nodep) {
        UINFO(5, "SHIFT(AND(a,b),CONST) with a/b special ->AND(SHIFT(a,CONST),SHIFT(b,CONST)) "
                     << nodep);
        અચલ પૂર્ણાંક width = nodep->width();
        અચલ પૂર્ણાંક widthMin = nodep->widthMin();
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        Ast_ગાંઠ_Biop* અચલ lhsp = VN_AS(nodep->lhsp(), ગાંઠ_Biop);
        lhsp->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ shiftp = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ ap = lhsp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ bp = lhsp->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_Biop* અચલ shift1p = nodep;
        Ast_ગાંઠ_Biop* અચલ shift2p = nodep->clone_વૃક્ષ(સાચું);
        shift1p->lhsp(ap);
        shift1p->rhsp(shiftp->clone_વૃક્ષ_Pure(સાચું));
        shift2p->lhsp(bp);
        shift2p->rhsp(shiftp);
        Ast_ગાંઠ_Biop* અચલ newp = lhsp;
        newp->lhsp(shift1p);
        newp->rhsp(shift2p);
        newp->dtypeChgWidth(width, widthMin);  // The new AND must have width of the original SHIFT
        handle.relink(newp);
        iterate(newp);  // Further reduce, either node may have more reductions.
    }
    શૂન્ય replaceShiftShift(Ast_ગાંઠ_Biop* nodep) {
        UINFO(4, "SHIFT(SHIFT(a,s1),s2)->SHIFT(a,ADD(s1,s2)) " << nodep);
        UINFOTREE(9, nodep, "", "repShiftShift_old");
        Ast_ગાંઠ_Biop* અચલ lhsp = VN_AS(nodep->lhsp(), ગાંઠ_Biop);
        lhsp->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ ap = lhsp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ shift1p = lhsp->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ shift2p = nodep->rhsp()->unlinkFrBack();
        // Shift1p and shift2p may have different sizes, both are
        // self-determined so sum with infinite width
        જો (nodep->type() == lhsp->type()) {
            અચલ પૂર્ણાંક shift1 = VN_AS(shift1p, Const)->toUInt();
            અચલ પૂર્ણાંક shift2 = VN_AS(shift2p, Const)->toUInt();
            અચલ પૂર્ણાંક newshift = shift1 + shift2;
            VL_DO_DANGLING(ધકેલો_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ધકેલો_Deletep(shift2p), shift2p);
            nodep->lhsp(ap);
            nodep->rhsp(નવું AstConst(nodep->fileline(), newshift));
            iterate(nodep);  // Further reduce, either node may have more reductions.
        } નહીંતર {
            // We know shift amounts are constant, but might be a mixed left/right shift
            પૂર્ણાંક shift1 = VN_AS(shift1p, Const)->toUInt();
            જો (VN_IS(lhsp, ShiftR)) shift1 = -shift1;
            પૂર્ણાંક shift2 = VN_AS(shift2p, Const)->toUInt();
            જો (VN_IS(nodep, ShiftR)) shift2 = -shift2;
            અચલ પૂર્ણાંક newshift = shift1 + shift2;
            VL_DO_DANGLING(ધકેલો_Deletep(shift1p), shift1p);
            VL_DO_DANGLING(ધકેલો_Deletep(shift2p), shift2p);
            Ast_ગાંઠ_પદાવલિ* newp;
            V3_સંખ્યા mask1{nodep, nodep->width()};
            V3_સંખ્યા ones{nodep, nodep->width()};
            ones.setMask(nodep->width());
            જો (shift1 < 0) {
                mask1.opShiftR(ones, V3_સંખ્યા(nodep, VL_IDATASIZE, -shift1));
            } નહીંતર {
                mask1.opShiftL(ones, V3_સંખ્યા(nodep, VL_IDATASIZE, shift1));
            }
            V3_સંખ્યા mask{nodep, nodep->width()};
            જો (shift2 < 0) {
                mask.opShiftR(mask1, V3_સંખ્યા(nodep, VL_IDATASIZE, -shift2));
            } નહીંતર {
                mask.opShiftL(mask1, V3_સંખ્યા(nodep, VL_IDATASIZE, shift2));
            }
            જો (newshift < 0) {
                newp = નવું AstShiftR{nodep->fileline(), ap,
                                     નવું AstConst(nodep->fileline(), -newshift)};
            } નહીંતર {
                newp = નવું AstShiftL{nodep->fileline(), ap,
                                     નવું AstConst(nodep->fileline(), newshift)};
            }
            newp->dtypeFrom(nodep);
            newp = નવું AstAnd{nodep->fileline(), newp, નવું AstConst{nodep->fileline(), mask}};
            nodep->replaceWithKeepDType(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            // UINFOTREE(1, newp, "", "repShiftShift_new");
            iterate(newp);  // Further reduce, either node may have more reductions.
        }
        VL_DO_DANGLING(ધકેલો_Deletep(lhsp), lhsp);
    }

    તાર્કિક replaceAssignMultiSel(Ast_ગાંઠ_Assign* nodep) {
        // Multiple assignments to sequential bits can be concated
        // ASSIGN(SEL(a),aq), ASSIGN(SEL(a+1),bq) -> ASSIGN(SEL(a:b),CONCAT(aq,bq)
        // ie. assign var[2]=a, assign var[3]=b -> assign var[3:2]={b,a}

        // Skip if we're not const'ing an entire module (IE doing only one assign, etc)
        જો (!m_modp) પાછા ખોટું;
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* અચલ sel1p = VN_CAST(nodep->lhsp(), Sel);
        જો (!sel1p) પાછા ખોટું;
        Ast_ગાંઠ_Assign* અચલ nextp = VN_CAST(nodep->nextp(), ગાંઠ_Assign);
        જો (!nextp) પાછા ખોટું;
        જો (nodep->type() != nextp->type()) પાછા ખોટું;
        અચલ AstSel* અચલ sel2p = VN_CAST(nextp->lhsp(), Sel);
        જો (!sel2p) પાછા ખોટું;
        AstVarRef* અચલ varref1p = VN_CAST(sel1p->fromp(), VarRef);
        જો (!varref1p) પાછા ખોટું;
        અચલ AstVarRef* અચલ varref2p = VN_CAST(sel2p->fromp(), VarRef);
        જો (!varref2p) પાછા ખોટું;
        જો (!varref1p->same_Gate_વૃક્ષ(varref2p)) પાછા ખોટું;
        અચલ AstConst* અચલ con1p = VN_CAST(sel1p->lsbp(), Const);
        જો (!con1p) પાછા ખોટું;
        અચલ AstConst* અચલ con2p = VN_CAST(sel2p->lsbp(), Const);
        જો (!con2p) પાછા ખોટું;
        // We need to make sure there's no self-references involved in either
        // assignment.  For speed, we only look 3 deep, then give up.
        જો (!varNotReferenced(nodep->rhsp(), varref1p->varp())) પાછા ખોટું;
        જો (!varNotReferenced(nextp->rhsp(), varref2p->varp())) પાછા ખોટું;
        // If a variable is marked split_var, access to the variable should not be merged.
        જો (varref1p->varp()->attrSplitVar() || varref2p->varp()->attrSplitVar()) પાછા ખોટું;
        // Swap?
        જો ((con1p->toSInt() != con2p->toSInt() + sel2p->width())
            && (con2p->toSInt() != con1p->toSInt() + sel1p->width())) {
            પાછા ખોટું;
        }
        અચલ તાર્કિક lsbFirstAssign = (con1p->toUInt() < con2p->toUInt());
        UINFO(4, "replaceAssignMultiSel " << nodep);
        UINFO(4, "                   && " << nextp);
        // UINFOTREE(1, nodep, "", "comb1");
        // UINFOTREE(1, nextp, "", "comb2");
        Ast_ગાંઠ_પદાવલિ* અચલ rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rhs2p = nextp->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_Assign* newp;
        જો (lsbFirstAssign) {
            newp = nodep->cloneType(નવું AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel1p->lsbConst(), sel1p->width() + sel2p->width()},
                                    નવું AstConcat{rhs1p->fileline(), rhs2p, rhs1p});
        } નહીંતર {
            newp = nodep->cloneType(નવું AstSel{sel1p->fileline(), varref1p->unlinkFrBack(),
                                               sel2p->lsbConst(), sel1p->width() + sel2p->width()},
                                    નવું AstConcat{rhs1p->fileline(), rhs1p, rhs2p});
        }
        // UINFOTREE(1, pnewp, "", "conew");
        nodep->replaceWith(newp);  // dypep intentionally changing
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        VL_DO_DANGLING(ધકેલો_Deletep(nextp->unlinkFrBack()), nextp);
        પાછા સાચું;
    }

    તાર્કિક replaceAssignSFormatF(Ast_ગાંઠ_Assign* nodep) {
        // Rewrite 'x = sformatf(...)' into 'sformat(x, ...)', which is more efficient
        // at the call site as it does not need to check if 'x' needs to be freed first.
        // This is a somewhat common pattern after lowering assertions.
        જો (!VN_IS(nodep, Assign)) પાછા ખોટું;
        AstVarRef* અચલ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        જો (!lhsp) પાછા ખોટું;
        AstSFormatF* અચલ fmtp = VN_CAST(nodep->rhsp(), SFormatF);
        જો (!fmtp) પાછા ખોટું;
        lhsp->unlinkFrBack();
        fmtp->unlinkFrBack();
        nodep->replaceWith(નવું AstSFormat{nodep->fileline(), fmtp, lhsp});
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }

    તાર્કિક varNotReferenced(Ast_ગાંઠ* nodep, AstVar* varp, પૂર્ણાંક level = 0) {
        // Return true if varp never referenced under node.
        // Return false if referenced, or tree too deep to be worth it, or side effects
        જો (!nodep) પાછા સાચું;
        જો (level > 2) પાછા ખોટું;
        જો (!nodep->isPure()) પાછા ખોટું;  // For example a $fgetc can't be reordered
        જો (VN_IS(nodep, ગાંઠ_Var_Ref) && VN_AS(nodep, ગાંઠ_Var_Ref)->varp() == varp) પાછા ખોટું;
        પાછા (varNotReferenced(nodep->nextp(), varp, level + 1)
                && varNotReferenced(nodep->op1p(), varp, level + 1)
                && varNotReferenced(nodep->op2p(), varp, level + 1)
                && varNotReferenced(nodep->op3p(), varp, level + 1)
                && varNotReferenced(nodep->op4p(), varp, level + 1));
    }

    તાર્કિક replace_ગાંઠ_Assign(Ast_ગાંઠ_Assign* nodep) {
        જો (VN_IS(nodep->lhsp(), VarRef) && VN_IS(nodep->rhsp(), VarRef)
            && VN_AS(nodep->lhsp(), VarRef)->sameNoLvalue(VN_AS(nodep->rhsp(), VarRef))
            && !VN_IS(nodep, AssignDly)) {
            // X = X.  Quite pointless, though X <= X may override another earlier assignment
            જો (VN_IS(nodep, AssignW)) {
                જો (!v3Global.opt.pedantic())
                    nodep->v3error(
                        "Wire inputs its own output, creating circular logic (wire x=x)");
                પાછા ખોટું;  // Don't delete the assign, or V3Gate will freak out
            } નહીંતર {
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા સાચું;
            }
        } નહીંતર જો (m_doV && VN_IS(nodep->lhsp(), Concat)) {
            તાર્કિક need_temp = ખોટું;
            // A blocking assignment with an intra-assignment timing control evaluates its value,
            // then waits once (IEEE 1800-2023 9.4.5), here to assign a temporary for the parts
            અચલ તાર્કિક waits = VN_IS(nodep, Assign) && nodep->timingControlp();
            // Splitting copies the RHS once per part, so a large RHS would make the tree
            // grow with the product of the number of parts and the RHS size
            અચલ તાર્કિક needTempPure = !nodep->rhsp()->isPure() || waits
                                      || nodep->rhsp()->isLargerThan(CONCAT_ASSIGN_MAX_RHS_NODES);
            જો (m_warn && !VN_IS(nodep, AssignDly)
                && !needTempPure) {  // Is same var on LHS and RHS?
                // If the rhs is not pure, we need a temporary variable anyway
                std::unordered_set<size_t> lhsVarIds;
                nodep->lhsp()->foreach([&](અચલ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    lhsVarIds.emplace(m_ids(refp->varp()));
                });
                nodep->rhsp()->foreach([&](અચલ AstVarRef* refp) {
                    UASSERT_OBJ(refp->varp(), refp, "Unlinked VarRef");
                    જો (lhsVarIds.ગણતરી(m_ids(refp->varp()))) need_temp = સાચું;
                });
            }
            જો (needTempPure) {
                // if the RHS is impure or large we need to create a temporary variable for it,
                // because further handling involves copying of the RHS.
                UINFO(4, "  ASSITEMPPURE " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp,rhs),
                //                                ASSIGN(lc1,SEL(temp,{size1})),
                //                                ASSIGN(lc2,SEL(temp,{size2}))

                Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
                AstVar* અચલ tempPurep = નવું AstVar{rhsp->fileline(), VVarType::BLOCKTEMP,
                                                     m_concswapNames.get(rhsp), rhsp->dtypep()};
                // Static, as read right after written, also in a fork
                tempPurep->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempPurep);
                AstVarRef* અચલ tempPureRefp
                    = નવું AstVarRef{rhsp->fileline(), tempPurep, VAccess::WRITE};
                Ast_ગાંઠ_Assign* અચલ asnp
                    = VN_IS(nodep, AssignDly)
                          ? નવું AstAssign{nodep->fileline(), tempPureRefp, rhsp}
                          : nodep->cloneType(tempPureRefp, rhsp);
                nodep->addHereThisAsNext(asnp);
                nodep->rhsp(નવું AstVarRef{rhsp->fileline(), tempPurep, VAccess::READ});
                // Only the assignment of the temporary waits
                જો (waits) ધકેલો_Deletep(nodep->timingControlp()->unlinkFrBack());
            } નહીંતર જો (need_temp) {
                // The first time we constify, there may be the same variable on the LHS
                // and RHS.  In that case, we must use temporaries, or {a,b}={b,a} will break.
                UINFO(4, "  ASSITEMP " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(temp1,SEL(rhs,{size})),
                //                                ASSIGN(temp2,SEL(newrhs,{size}))
                //                                ASSIGN(lc1,temp1),
                //                                ASSIGN(lc2,temp2)
            } નહીંતર {
                UINFO(4, "  ASSI " << nodep);
                // ASSIGN(CONCAT(lc1,lc2),rhs) -> ASSIGN(lc1,SEL(rhs,{size})),
                //                                ASSIGN(lc2,SEL(newrhs,{size}))
            }
            // The NBA evaluates its intra-assignment delay as it executes (IEEE 1800-2023 4.9.4),
            // so once for the NBAs to all parts
            AstDelay* અચલ delayp
                = VN_IS(nodep, AssignDly) ? VN_CAST(nodep->timingControlp(), Delay) : nullptr;
            જો (delayp && !delayp->lhsp()->isPure()) {
                File_લીટી* અચલ flp = nodep->fileline();
                Ast_ગાંઠ_પદાવલિ* અચલ valuep = delayp->lhsp()->unlinkFrBack();
                AstVar* અચલ tempp = નવું AstVar{flp, VVarType::BLOCKTEMP,
                                                 m_concswapNames.get(valuep), valuep->dtypep()};
                // Static, as read right after written, when the NBAs to the parts start waiting
                tempp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(tempp);
                nodep->addHereThisAsNext(
                    નવું AstAssign{flp, નવું AstVarRef{flp, tempp, VAccess::WRITE}, valuep});
                delayp->lhsp(નવું AstVarRef{flp, tempp, VAccess::READ});
            }
            UINFOTREE(9, nodep, "", "Ass_old");
            // Unlink the stuff
            Ast_ગાંઠ_પદાવલિ* અચલ lc1p = VN_AS(nodep->lhsp(), Concat)->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ lc2p = VN_AS(nodep->lhsp(), Concat)->rhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ conp = VN_AS(nodep->lhsp(), Concat)->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhs2p = rhsp->clone_વૃક્ષ_Pure(ખોટું);
            // Calc widths
            અચલ પૂર્ણાંક lsb2 = 0;
            અચલ પૂર્ણાંક msb2 = lsb2 + lc2p->width() - 1;
            અચલ પૂર્ણાંક lsb1 = msb2 + 1;
            અચલ પૂર્ણાંક msb1 = lsb1 + lc1p->width() - 1;
            UASSERT_OBJ(msb1 == (conp->width() - 1), nodep, "Width calc mismatch");
            // Form ranges
            AstSel* અચલ sel1p = નવું AstSel{conp->fileline(), rhsp, lsb1, msb1 - lsb1 + 1};
            AstSel* અચલ sel2p = નવું AstSel{conp->fileline(), rhs2p, lsb2, msb2 - lsb2 + 1};
            // Make new assigns of same flavor as old one
            //*** Not cloneTree; just one node.
            Ast_ગાંઠ_Assign* newp = nullptr;
            જો (!need_temp) {
                Ast_ગાંઠ_Assign* અચલ asn1ap = nodep->cloneType(lc1p, sel1p);
                Ast_ગાંઠ_Assign* અચલ asn2ap = nodep->cloneType(lc2p, sel2p);
                asn1ap->dtypeFrom(sel1p);
                asn2ap->dtypeFrom(sel2p);
                newp = Ast_ગાંઠ::addNext(newp, asn1ap);
                newp = Ast_ગાંઠ::addNext(newp, asn2ap);
            } નહીંતર {
                UASSERT_OBJ(m_modp, nodep, "Not under module");
                UASSERT_OBJ(m_globalPass, nodep,
                            "Should not reach here when not invoked on whole AstNetlist");
                // We could create just one temp variable, but we'll get better optimization
                // if we make one per term.
                AstVar* અચલ temp1p
                    = નવું AstVar{sel1p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel1p), VFlagLogicPacked{}, msb1 - lsb1 + 1};
                AstVar* અચલ temp2p
                    = નવું AstVar{sel2p->fileline(), VVarType::BLOCKTEMP,
                                 m_concswapNames.get(sel2p), VFlagLogicPacked{}, msb2 - lsb2 + 1};
                m_modp->addStmtsp(temp1p);
                m_modp->addStmtsp(temp2p);
                Ast_ગાંઠ_Assign* અચલ asn1ap = nodep->cloneType(
                    નવું AstVarRef{sel1p->fileline(), temp1p, VAccess::WRITE}, sel1p);
                Ast_ગાંઠ_Assign* અચલ asn2ap = nodep->cloneType(
                    નવું AstVarRef{sel2p->fileline(), temp2p, VAccess::WRITE}, sel2p);
                Ast_ગાંઠ_Assign* અચલ asn1bp = nodep->cloneType(
                    lc1p, નવું AstVarRef{sel1p->fileline(), temp1p, VAccess::READ});
                Ast_ગાંઠ_Assign* અચલ asn2bp = nodep->cloneType(
                    lc2p, નવું AstVarRef{sel2p->fileline(), temp2p, VAccess::READ});
                asn1ap->dtypeFrom(temp1p);
                asn1bp->dtypeFrom(temp1p);
                asn2ap->dtypeFrom(temp2p);
                asn2bp->dtypeFrom(temp2p);
                // This order matters
                newp = Ast_ગાંઠ::addNext(newp, asn1ap);
                newp = Ast_ગાંઠ::addNext(newp, asn2ap);
                newp = Ast_ગાંઠ::addNext(newp, asn1bp);
                newp = Ast_ગાંઠ::addNext(newp, asn2bp);
            }
            જો (debug() >= 9 && newp) newp->dump_વૃક્ષ_And_Next(cout, "-     _new: ");
            nodep->addNextHere(newp);
            // Cleanup
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            VL_DO_DANGLING(ધકેલો_Deletep(conp), conp);
            // Further reduce, either node may have more reductions.
            પાછા સાચું;
        } નહીંતર જો (m_doV && VN_IS(nodep->rhsp(), CvtPackedToArray)
                   && lowerAsFixedAggregate(nodep->lhsp()->dtypep())) {
            AstCvtPackedToArray* અચલ cvtp
                = VN_AS(nodep->rhsp(), CvtPackedToArray)->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* srcp = cvtp->fromp()->unlinkFrBack();
            જો (lowerAsFixedAggregate(srcp->dtypep())) {
                srcp = packFixedAggregate(srcp);
            } નહીંતર જો (Ast_ગાંઠ_Stream* અચલ streamp = VN_CAST(srcp, ગાંઠ_Stream)) {
                Ast_ગાંઠ_પદાવલિ* અચલ streamSrcp = streamp->lhsp();
                જો (lowerAsFixedAggregate(streamSrcp->dtypep())) {
                    Ast_ગાંઠ_પદાવલિ* અચલ packedp = packFixedAggregate(streamSrcp->unlinkFrBack());
                    streamp->lhsp(packedp);
                    streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                       VSigning::UNSIGNED,
                                                       packedp->isFourstateOrNoDtype());
                }
            }
            VL_DO_DANGLING(ધકેલો_Deletep(cvtp), cvtp);
            replaceAssignToFixedAggregate(nodep, nodep->lhsp()->unlinkFrBack(), srcp);
            પાછા સાચું;
        } નહીંતર જો (m_doV && VN_IS(nodep->rhsp(), StreamR)
                   && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
            // The right-streaming operator on rhs of assignment does not
            // change the order of bits. Eliminate stream but keep its lhsp.
            // Add a cast if needed.
            AstStreamR* અચલ streamp = VN_AS(nodep->rhsp(), StreamR)->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* srcp = streamp->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_DType* અચલ srcDTypep = srcp->dtypep()->skipRefp();
            અચલ Ast_ગાંઠ_DType* અચલ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            જો (lowerAsFixedAggregate(srcDTypep)) {
                srcp = packFixedAggregate(srcp);
            } નહીંતર જો (VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)) {
                જો (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    પૂર્ણાંક srcElementBits = 0;
                    જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    પૂર્ણાંક dstElementBits = 0;
                    જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    srcp = નવું AstCvtArrayToArray{
                        srcp->fileline(), srcp,          nodep->dtypep(), ખોટું, 1,
                        dstElementBits,   srcElementBits};
                } નહીંતર {
                    srcp = નવું AstCvtArrayToPacked{srcp->fileline(), srcp, nodep->dtypep()};
                }
            } નહીંતર જો (VN_IS(srcDTypep, UnpackArrayDType)) {
                srcp = નવું AstCvtArrayToPacked{srcp->fileline(), srcp, srcDTypep};
                // Handling the case where lhs is wider than rhs by inserting zeros. StreamL does
                // not require this, since the left streaming operator implicitly handles this.
                અચલ પૂર્ણાંક packedBits = nodep->lhsp()->widthMin();
                અચલ પૂર્ણાંક unpackBits
                    = srcDTypep->arrayUnpackedElements() * srcDTypep->subDTypep()->widthMin();
                અચલ uint32_t offset = packedBits > unpackBits ? packedBits - unpackBits : 0;
                srcp = નવું AstShiftL{srcp->fileline(), srcp,
                                     નવું AstConst{srcp->fileline(), offset}, packedBits};
            }
            જો (!VN_IS(dstDTypep, UnpackArrayDType) && !VN_IS(dstDTypep, QueueDType)
                && !VN_IS(dstDTypep, DynArrayDType)) {
                અચલ પૂર્ણાંક sWidth = srcp->width();
                અચલ પૂર્ણાંક dWidth = nodep->lhsp()->width();
                જો (sWidth < dWidth) {
                    AstExtend* અચલ extendp = નવું AstExtend{srcp->fileline(), srcp, dWidth};
                    srcp = નવું AstShiftL{
                        srcp->fileline(), extendp,
                        નવું AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                        dWidth};
                }
            }
            nodep->rhsp(srcp);
            VL_DO_DANGLING(ધકેલો_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            પાછા સાચું;
        } નહીંતર જો (m_doV && VN_IS(nodep->lhsp(), StreamL)) {
            // Push the stream operator to the rhs of the assignment statement
            Ast_ગાંઠ_પદાવલિ* streamp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ dstp = VN_AS(streamp, StreamL)->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_DType* અચલ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ગાંઠ_પદાવલિ* srcp = nodep->rhsp()->unlinkFrBack();
            અચલ Ast_ગાંઠ_DType* અચલ srcDTypep = srcp->dtypep()->skipRefp();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamL reverses, so reverse=true)
            જો ((VN_IS(srcDTypep, UnpackArrayDType) || VN_IS(srcDTypep, QueueDType)
                 || VN_IS(srcDTypep, DynArrayDType))
                && (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType))) {
                પૂર્ણાંક blockSize = 1;
                જો (અચલ AstConst* અચલ constp
                    = VN_CAST(VN_AS(streamp, StreamL)->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    જો (VL_UNLIKELY(blockSize <= 0)) blockSize = 1;
                }
                પૂર્ણાંક srcElementBits = 0;
                જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                પૂર્ણાંક dstElementBits = 0;
                જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                nodep->lhsp(dstp);
                nodep->rhsp(નવું AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, સાચું,
                                                   blockSize, dstElementBits, srcElementBits});
                nodep->dtypep(dstDTypep);
                VL_DO_DANGLING(ધકેલો_Deletep(streamp), streamp);
                પાછા સાચું;
            }
            જો (lowerAsFixedAggregate(srcDTypep)) srcp = packFixedAggregate(srcp);
            અચલ પૂર્ણાંક sWidth = srcp->width();
            અચલ પૂર્ણાંક dWidth = dstp->width();
            // Connect the rhs to the stream operator and update its width
            VN_AS(streamp, StreamL)->lhsp(srcp);
            જો (VN_IS(srcDTypep, DynArrayDType) || VN_IS(srcDTypep, QueueDType)
                || VN_IS(srcDTypep, UnpackArrayDType)) {
                streamp->dtypeSetStream();
            } નહીંતર {
                streamp->dtypeSetBitOrLogicUnsized(srcp->width(), srcp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   srcp->isFourstateOrNoDtype());
            }
            જો (VN_IS(dstDTypep, UnpackArrayDType)) {
                streamp = નવું AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
            } નહીંતર {
                જો (dWidth == 0) {
                    streamp = નવું AstCvtPackedToArray{nodep->fileline(), streamp, dstDTypep};
                } નહીંતર જો (sWidth >= dWidth) {
                    streamp = નવું AstSel{streamp->fileline(), streamp, sWidth - dWidth, dWidth};
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(streamp);
            nodep->dtypep(dstDTypep);
            પાછા સાચું;
        } નહીંતર જો (m_doV && VN_IS(nodep->lhsp(), StreamR)) {
            // The right stream operator on lhs of assignment statement does
            // not reorder bits. However, if the rhs is wider than the lhs,
            // then we select bits from the left-most, not the right-most.
            Ast_ગાંઠ_પદાવલિ* અચલ streamp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ dstp = VN_AS(streamp, StreamR)->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_DType* અચલ dstDTypep = dstp->dtypep()->skipRefp();
            Ast_ગાંઠ_પદાવલિ* srcp = nodep->rhsp()->unlinkFrBack();
            // Handle unpacked/queue/dynarray source -> queue/dynarray dest via
            // CvtArrayToArray (StreamR does not reverse, so reverse=false).
            // V3Width may have wrapped the source in CvtArrayToPacked; unwrap it.
            જો (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                Ast_ગાંઠ_પદાવલિ* origSrcp = srcp;
                જો (AstCvtArrayToPacked* અચલ cvtp = VN_CAST(srcp, CvtArrayToPacked)) {
                    origSrcp = cvtp->fromp();
                }
                અચલ Ast_ગાંઠ_DType* અચલ origSrcDTypep = origSrcp->dtypep()->skipRefp();
                જો (VN_IS(origSrcDTypep, UnpackArrayDType) || VN_IS(origSrcDTypep, QueueDType)
                    || VN_IS(origSrcDTypep, DynArrayDType)) {
                    પૂર્ણાંક srcElementBits = 0;
                    જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = origSrcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    પૂર્ણાંક dstElementBits = 0;
                    જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    જો (VN_IS(srcp, CvtArrayToPacked)) {
                        origSrcp = VN_AS(srcp, CvtArrayToPacked)->fromp()->unlinkFrBack();
                        VL_DO_DANGLING(ધકેલો_Deletep(srcp), srcp);
                        srcp = origSrcp;
                    }
                    // Descending unpacked arrays need element reversal
                    તાર્કિક reverse = ખોટું;
                    જો (અચલ AstUnpackArrayDType* અચલ unpackDtp
                        = VN_CAST(origSrcDTypep, UnpackArrayDType)) {
                        reverse = !unpackDtp->declRange().ascending();
                    }
                    nodep->lhsp(dstp);
                    nodep->rhsp(નવું AstCvtArrayToArray{srcp->fileline(), srcp, dstDTypep, reverse,
                                                       1, dstElementBits, srcElementBits});
                    nodep->dtypep(dstDTypep);
                    VL_DO_DANGLING(ધકેલો_Deletep(streamp), streamp);
                    પાછા સાચું;
                }
            }
            અચલ પૂર્ણાંક sWidth = srcp->width();
            અચલ પૂર્ણાંક dWidth = dstp->width();
            જો (VN_IS(dstDTypep, UnpackArrayDType)) {
                અચલ પૂર્ણાંક dstBitWidth
                    = dWidth * VN_AS(dstDTypep, UnpackArrayDType)->arrayUnpackedElements();
                // Handling the case where rhs is wider than lhs. StreamL does not require this
                // since the combination of the left streaming operation and the implicit
                // truncation in VL_ASSIGN_UNPACK automatically selects the left-most bits.
                જો (sWidth > dstBitWidth) {
                    srcp
                        = નવું AstSel{streamp->fileline(), srcp, sWidth - dstBitWidth, dstBitWidth};
                }
                srcp = નવું AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
            } નહીંતર {
                જો (dWidth == 0) {
                    srcp = નવું AstCvtPackedToArray{nodep->fileline(), srcp, dstDTypep};
                } નહીંતર જો (sWidth >= dWidth) {
                    srcp = નવું AstSel{streamp->fileline(), srcp, sWidth - dWidth, dWidth};
                } નહીંતર {
                    // Source narrower than destination: left-justify by shifting left.
                    // The right stream operator packs left-to-right, so remaining
                    // LSBs are zero-filled (IEEE 1800-2023 11.4.14.3).
                    જો (!VN_IS(srcp->dtypep()->skipRefp(), QueueDType)) {
                        AstExtend* અચલ extendp = નવું AstExtend{srcp->fileline(), srcp, dWidth};
                        srcp = નવું AstShiftL{
                            srcp->fileline(), extendp,
                            નવું AstConst{srcp->fileline(), static_cast<uint32_t>(dWidth - sWidth)},
                            dWidth};
                    } નહીંતર {  // if it is a queue we dont need do do a shiftL
                        srcp = નવું AstExtend{srcp->fileline(), srcp};
                    }
                }
            }
            nodep->lhsp(dstp);
            nodep->rhsp(srcp);
            nodep->dtypep(dstDTypep);
            VL_DO_DANGLING(ધકેલો_Deletep(streamp), streamp);
            // Further reduce, any of the nodes may have more reductions.
            પાછા સાચું;
        } નહીંતર જો (m_doV && VN_IS(nodep->rhsp(), StreamL)) {
            AstStreamL* streamp = VN_AS(nodep->rhsp(), StreamL);
            Ast_ગાંઠ_પદાવલિ* srcp = streamp->lhsp();
            અચલ Ast_ગાંઠ_DType* અચલ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ગાંઠ_DType* અચલ dstDTypep = nodep->lhsp()->dtypep()->skipRefp();
            જો (lowerAsFixedAggregate(srcDTypep)) {
                Ast_ગાંઠ_પદાવલિ* અચલ packedp = packFixedAggregate(srcp->unlinkFrBack());
                streamp->lhsp(packedp);
                streamp->dtypeSetBitOrLogicUnsized(packedp->width(), packedp->widthMin(),
                                                   VSigning::UNSIGNED,
                                                   packedp->isFourstateOrNoDtype());
                srcp = packedp;
            } નહીંતર જો ((VN_IS(srcDTypep, QueueDType) || VN_IS(srcDTypep, DynArrayDType)
                        || VN_IS(srcDTypep, UnpackArrayDType))) {
                જો (VN_IS(dstDTypep, QueueDType) || VN_IS(dstDTypep, DynArrayDType)) {
                    પૂર્ણાંક blockSize = 1;
                    જો (અચલ AstConst* અચલ constp = VN_CAST(streamp->rhsp(), Const)) {
                        blockSize = constp->toSInt();
                        જો (VL_UNLIKELY(blockSize <= 0)) {
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
                    પૂર્ણાંક srcElementBits = 0;
                    જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = srcDTypep->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    પૂર્ણાંક dstElementBits = 0;
                    જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = dstDTypep->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    streamp->unlinkFrBack();
                    srcp = નવું AstCvtArrayToArray{
                        srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     સાચું,
                        blockSize,        dstElementBits,       srcElementBits};
                    nodep->rhsp(srcp);
                    VL_DO_DANGLING(ધકેલો_Deletep(streamp), streamp);
                } નહીંતર {
                    streamp->lhsp(નવું AstCvtArrayToPacked{srcp->fileline(), srcp->unlinkFrBack(),
                                                          dstDTypep});
                    streamp->dtypeFrom(dstDTypep);
                }
            }
        } નહીંતર જો (m_doV && replaceAssignMultiSel(nodep)) {
            પાછા સાચું;
        } નહીંતર જો (replaceAssignSFormatF(nodep)) {
            પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // Boolean replacements
    તાર્કિક operandBoolShift(અચલ Ast_ગાંઠ* nodep) {
        // boolean test of AND(const,SHIFTR(x,const)) -> test of AND(SHIFTL(x,const), x)
        જો (!VN_IS(nodep, And)) પાછા ખોટું;
        જો (!VN_IS(VN_AS(nodep, And)->lhsp(), Const)) પાછા ખોટું;
        જો (!VN_IS(VN_AS(nodep, And)->rhsp(), ShiftR)) પાછા ખોટું;
        અચલ AstShiftR* અચલ shiftp = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR);
        જો (!VN_IS(shiftp->rhsp(), Const)) પાછા ખોટું;
        જો (static_cast<uint32_t>(nodep->width()) <= VN_AS(shiftp->rhsp(), Const)->toUInt()) {
            પાછા ખોટું;
        }
        પાછા સાચું;
    }
    શૂન્ય replaceBoolShift(Ast_ગાંઠ* nodep) {
        UINFOTREE(9, nodep, "", "bshft_old");
        AstConst* અચલ andConstp = VN_AS(VN_AS(nodep, And)->lhsp(), Const);
        Ast_ગાંઠ_પદાવલિ* અચલ fromp
            = VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->lhsp()->unlinkFrBack();
        AstConst* અચલ shiftConstp
            = VN_AS(VN_AS(VN_AS(nodep, And)->rhsp(), ShiftR)->rhsp(), Const);
        V3_સંખ્યા val{andConstp, andConstp->width()};
        val.opShiftL(andConstp->અંક(), shiftConstp->અંક());
        AstAnd* અચલ newp
            = નવું AstAnd{nodep->fileline(), નવું AstConst{nodep->fileline(), val}, fromp};
        // widthMin no longer applicable if different C-expanded width
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        UINFOTREE(9, newp, "", "_new");
    }

    શૂન્ય replaceWithSimulation(Ast_ગાંઠ* nodep) {
        SimulateVisitor simvis;
        // Run it - may be unoptimizable due to large for loop, etc
        simvis.mainParamEmulate(nodep);
        જો (!simvis.optimizable()) {
            અચલ Ast_ગાંઠ* errorp = simvis.whyNotNodep();
            જો (!errorp) errorp = nodep;
            nodep->v3error("Expecting expression to be constant, but can't determine constant for "
                           << nodep->prettyTypeName() << '\n'
                           << errorp->warnOther() << "... Location of non-constant "
                           << errorp->prettyTypeName() << ": " << simvis.whyNotMessage());
            VL_DO_DANGLING(replaceZero(nodep), nodep);
        } નહીંતર {
            // Fetch the result
            Ast_ગાંઠ* અચલ valuep = simvis.fetch_કિંમત_Null(nodep);  // valuep is owned by Simulate
            UASSERT_OBJ(valuep, nodep, "No value returned from simulation");
            // Replace it
            Ast_ગાંઠ* અચલ newp = valuep->clone_વૃક્ષ(ખોટું);
            newp->fileline(nodep->fileline());
            nodep->replaceWithKeepDType(newp);
            UINFO(4, "Simulate->" << newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }

    //----------------------------------------

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        iterateChildrenBackwardsConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_concswapNames.reset();
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        // No ASSIGNW removals under funcs, we've long eliminated INITIALs
        // (We should perhaps rename the assignw's to just assigns)
        VL_RESTORER(m_wremove);
        m_wremove = ખોટું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCLocalScope* nodep) override {
        iterateChildren(nodep);
        જો (!nodep->stmtsp()) {
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(AstScope* nodep) override {
        // No ASSIGNW removals under scope, we've long eliminated INITIALs
        VL_RESTORER(m_wremove);
        VL_RESTORER(m_scopep);
        m_wremove = ખોટું;
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    શૂન્ય swapSides(Ast_ગાંઠ_Bi_Com* nodep) {
        // COMMUTATIVE({a},CONST) -> COMMUTATIVE(CONST,{a})
        // This simplifies later optimizations
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBackWithNext();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBackWithNext();
        nodep->lhsp(rhsp);
        nodep->rhsp(lhsp);
        iterate(nodep);  // Again?
    }

    તાર્કિક containsMemberAccessRecurse(Ast_ગાંઠ* અચલ nodep) {
        જો (!nodep) પાછા ખોટું;
        અચલ size_t id = m_ids(nodep);
        અચલ auto it = m_containsMemberAccess.find(id);
        જો (it != m_containsMemberAccess.end()) પાછા it->second;
        ++m_statMemberAccessVisits;
        તાર્કિક result = ખોટું;
        જો (VN_IS(nodep, MemberSel) || VN_IS(nodep, MethodCall) || VN_IS(nodep, CMethodCall)) {
            result = સાચું;
        } નહીંતર જો (અચલ Ast_ગાંઠ_FTask_Ref* અચલ funcRefp = VN_CAST(nodep, ગાંઠ_FTask_Ref)) {
            જો (containsMemberAccessRecurse(funcRefp->taskp())) result = સાચું;
        } નહીંતર જો (અચલ Ast_ગાંઠ_CCall* અચલ funcRefp = VN_CAST(nodep, ગાંઠ_CCall)) {
            જો (containsMemberAccessRecurse(funcRefp->funcp())) result = સાચું;
        } નહીંતર જો (અચલ Ast_ગાંઠ_FTask* અચલ funcp = VN_CAST(nodep, ગાંઠ_FTask)) {
            // Assume that it has a member access
            જો (funcp->recursive()) result = સાચું;
        } નહીંતર જો (અચલ AstCFunc* અચલ funcp = VN_CAST(nodep, CFunc)) {
            જો (funcp->recursive()) result = સાચું;
        }
        જો (!result) {
            result = containsMemberAccessRecurse(nodep->op1p())
                     || containsMemberAccessRecurse(nodep->op2p())
                     || containsMemberAccessRecurse(nodep->op3p())
                     || containsMemberAccessRecurse(nodep->op4p());
        }
        જો (!result && !VN_IS(nodep, ગાંઠ_FTask)
            && !VN_IS(nodep, CFunc)  // don't enter into next function
            && containsMemberAccessRecurse(nodep->nextp())) {
            result = સાચું;
        }
        m_containsMemberAccess.emplace(id, result);
        પાછા result;
    }

    તાર્કિક matchBiopToBitwise(Ast_ગાંઠ_Biop* અચલ nodep) {
        જો (!m_convertLogicToBit) પાછા ખોટું;
        જો (!nodep->lhsp()->width1()) પાછા ખોટું;
        જો (!nodep->rhsp()->width1()) પાછા ખોટું;
        જો (!nodep->isPure()) પાછા ખોટું;
        જો (containsMemberAccessRecurse(nodep)) પાછા ખોટું;
        પાછા સાચું;
    }
    તાર્કિક matchConcatRand(AstConcat* nodep) {
        //    CONCAT(RAND, RAND) - created by Chisel code
        AstRand* અચલ aRandp = VN_CAST(nodep->lhsp(), Rand);
        અચલ AstRand* અચલ bRandp = VN_CAST(nodep->rhsp(), Rand);
        જો (!aRandp || !bRandp) પાછા ખોટું;
        જો (!aRandp->combinable(bRandp)) પાછા ખોટું;
        UINFO(4, "Concat(Rand,Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }
    તાર્કિક matchSelRand(AstSel* nodep) {
        //    SEL(RAND) - created by Chisel code
        AstRand* અચલ aRandp = VN_CAST(nodep->fromp(), Rand);
        જો (!aRandp) પાછા ખોટું;
        જો (aRandp->seedp()) પાછા ખોટું;
        UINFO(4, "Sel(Rand) => Rand: " << nodep);
        nodep->replaceWithKeepDType(aRandp->unlinkFrBack());
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }
    તાર્કિક matchToStringNConst(AstToStringN* nodep) {
        iterateChildren(nodep);
        જો (અચલ AstConsArray* અચલ consp = VN_CAST(nodep->lhsp(), ConsArray)) {
            જો (!(m_doExpensive || m_params)) પાછા ખોટું;
            જો (!consp->is_Constant_વૃક્ષ()) પાછા ખોટું;
        } નહીંતર જો (!VN_IS(nodep->lhsp(), Const)) {
            પાછા ખોટું;
        }
        replaceWithSimulation(nodep);
        પાછા સાચું;
    }
    પૂર્ણાંક operandConcatMove(અચલ AstConcat* nodep) {
        //    CONCAT under concat  (See moveConcat)
        // Return value: true indicates to do it; 2 means move to LHS
        અચલ AstConcat* અચલ abConcp = VN_CAST(nodep->lhsp(), Concat);
        અચલ AstConcat* અચલ bcConcp = VN_CAST(nodep->rhsp(), Concat);
        જો (!abConcp && !bcConcp) પાછા 0;
        જો (bcConcp) {
            અચલ Ast_ગાંઠ_પદાવલિ* અચલ ap = nodep->lhsp();
            અચલ Ast_ગાંઠ_પદાવલિ* અચલ bp = bcConcp->lhsp();
            // If a+b == 32,64,96 etc, then we want to have a+b together on LHS
            જો (VL_BITBIT_I(ap->width() + bp->width()) == 0) પાછા 2;  // Transform 2: to abConc
        } નહીંતર {  // abConcp
            // Unless lhs is already 32 bits due to above, reorder it
            જો (VL_BITBIT_I(nodep->lhsp()->width()) != 0) પાછા 1;  // Transform 1: to bcConc
        }
        પાછા 0;  // ok
    }
    શૂન્ય moveConcat(AstConcat* nodep) {
        //    1: CONCAT(CONCAT({a},{b}),{c})  -> CONCAT({a},CONCAT({b}, {c}))
        // or 2: CONCAT({a}, CONCAT({b},{c})) -> CONCAT(CONCAT({a},{b}),{c})
        // Because the lhs of a concat needs a shift but the rhs doesn't,
        // putting additional CONCATs on the RHS leads to fewer assembler operations.
        // However, we'll end up with lots of wide moves if we make huge trees
        // like that, so on 32 bit boundaries, we'll do the opposite form.
        UINFO(4, "Move concat: " << nodep);
        જો (operandConcatMove(nodep) > 1) {
            Ast_ગાંઠ_પદાવલિ* અચલ ap = nodep->lhsp()->unlinkFrBack();
            AstConcat* અચલ bcConcp = VN_AS(nodep->rhsp(), Concat);
            bcConcp->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ bp = bcConcp->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ cp = bcConcp->rhsp()->unlinkFrBack();
            AstConcat* અચલ abConcp = નવું AstConcat{bcConcp->fileline(), ap, bp};
            nodep->lhsp(abConcp);
            nodep->rhsp(cp);
            // If bp was a concat, then we have this exact same form again!
            // Recurse rather then calling node->iterate to prevent 2^n recursion!
            જો (operandConcatMove(abConcp)) moveConcat(abConcp);
            VL_DO_DANGLING(ધકેલો_Deletep(bcConcp), bcConcp);
        } નહીંતર {
            AstConcat* અચલ abConcp = VN_AS(nodep->lhsp(), Concat);
            abConcp->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ ap = abConcp->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ bp = abConcp->rhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ cp = nodep->rhsp()->unlinkFrBack();
            AstConcat* અચલ bcConcp = નવું AstConcat{abConcp->fileline(), bp, cp};
            nodep->lhsp(ap);
            nodep->rhsp(bcConcp);
            જો (operandConcatMove(bcConcp)) moveConcat(bcConcp);
            VL_DO_DANGLING(ધકેલો_Deletep(abConcp), abConcp);
        }
    }

    // Special cases
    શૂન્ય visit(AstConst*) override {}  // Already constant

    શૂન્ય visit(AstCell* nodep) override {
        જો (m_params) {
            iterateAndNextNull(nodep->paramsp());
        } નહીંતર {
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override { iterateChildren(nodep); }

    શૂન્ય visit(AstMatchMasked* nodep) override {
        // Do not iterate the tables, they must be constant pool entries
        iterate(nodep->lhsp());
        જો (AstConst* અચલ constp = VN_CAST(nodep->lhsp(), Const)) {
            replace_અંક(nodep, AstMatchMasked::fold(constp->અંક(), nodep->matchp()->varp()));
        }
    }

    શૂન્ય visit(AstPin* nodep) override { iterateChildren(nodep); }

    શૂન્ય replaceLogEq(AstLogEq* nodep) {
        // LOGEQ(a,b) => AstLogAnd{AstLogOr{AstLogNot{a},b},AstLogOr{AstLogNot{b},a}}
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
        // Do exactly as IEEE says, might result in extra terms, so in future may do differently
        AstLogAnd* અચલ newp = નવું AstLogAnd{
            nodep->fileline(),
            નવું AstLogOr{nodep->fileline(), નવું AstLogNot{nodep->fileline(), lhsp}, rhsp},
            નવું AstLogOr{nodep->fileline(),
                         નવું AstLogNot{nodep->fileline(), rhsp->clone_વૃક્ષ_Pure(ખોટું)},
                         lhsp->clone_વૃક્ષ_Pure(ખોટું)}};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય replaceSelSel(AstSel* nodep) {
        // SEL(SEL({x},a,b),c,d) => SEL({x},a+c,d)
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSel* અચલ belowp = VN_AS(nodep->fromp(), Sel);
        Ast_ગાંઠ_પદાવલિ* અચલ fromp = belowp->fromp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ lsb1p = nodep->lsbp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ lsb2p = belowp->lsbp()->unlinkFrBack();
        // Eliminate lower range
        UINFO(4, "Elim Lower range: " << nodep);
        Ast_ગાંઠ_પદાવલિ* newlsbp;
        જો (VN_IS(lsb1p, Const) && VN_IS(lsb2p, Const)) {
            newlsbp = નવું AstConst{lsb1p->fileline(),
                                   VN_AS(lsb1p, Const)->toUInt() + VN_AS(lsb2p, Const)->toUInt()};
            VL_DO_DANGLING(ધકેલો_Deletep(lsb1p), lsb1p);
            VL_DO_DANGLING(ધકેલો_Deletep(lsb2p), lsb2p);
        } નહીંતર {
            // Width is important, we need the width of the fromp's expression, not the
            // potentially smaller lsb1p's width, but don't insert a redundant AstExtend.
            // Note that due to some sloppiness in earlier passes, lsb1p might actually be wider,
            // so extend to the wider type.
            અચલ Ast_ગાંઠ_પદાવલિ* અચલ widep = lsb1p->width() > lsb2p->width() ? lsb1p : lsb2p;
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = widep->width() > lsb2p->width()
                                          ? નવું AstExtend{lsb2p->fileline(), lsb2p}
                                          : lsb2p;
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = widep->width() > lsb1p->width()
                                          ? નવું AstExtend{lsb1p->fileline(), lsb1p}
                                          : lsb1p;
            lhsp->dtypeFrom(widep);
            rhsp->dtypeFrom(widep);
            newlsbp = નવું AstAdd{lsb1p->fileline(), lhsp, rhsp};
            newlsbp->dtypeFrom(widep);
        }
        AstSel* અચલ newp = નવું AstSel{nodep->fileline(), fromp, newlsbp, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય replaceSelConcat(AstSel* nodep) {
        // SEL(CONCAT(a,b),c,d) => SEL(a or b, . .)
        AstConcat* અચલ conp = VN_AS(nodep->fromp(), Concat);
        Ast_ગાંઠ_પદાવલિ* અચલ conLhsp = conp->lhsp();
        Ast_ગાંઠ_પદાવલિ* અચલ conRhsp = conp->rhsp();
        જો (static_cast<પૂર્ણાંક>(nodep->lsbConst()) >= conRhsp->width()) {
            conLhsp->unlinkFrBack();
            AstSel* અચલ newp
                = નવું AstSel{nodep->fileline(), conLhsp, nodep->lsbConst() - conRhsp->width(),
                             nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } નહીંતર જો (static_cast<પૂર્ણાંક>(nodep->msbConst()) < conRhsp->width()) {
            conRhsp->unlinkFrBack();
            AstSel* અચલ newp
                = નવું AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(), nodep->widthConst()};
            nodep->replaceWithKeepDType(newp);
        } નહીંતર {
            // Yuk, split between the two
            conRhsp->unlinkFrBack();
            conLhsp->unlinkFrBack();
            AstConcat* અચલ newp
                = નવું AstConcat{nodep->fileline(),
                                નવું AstSel{nodep->fileline(), conLhsp, 0,
                                           nodep->msbConst() - conRhsp->width() + 1},
                                નવું AstSel{nodep->fileline(), conRhsp, nodep->lsbConst(),
                                           conRhsp->width() - nodep->lsbConst()}};
            nodep->replaceWithKeepDType(newp);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    તાર્કિક operandSelReplicate(AstSel* nodep) {
        // SEL(REPLICATE(from,rep),lsb,width) => SEL(from,0,width) as long
        // as SEL's width <= b's width
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* અચલ repp = VN_AS(nodep->fromp(), Replicate);
        Ast_ગાંઠ_પદાવલિ* અચલ fromp = repp->srcp();
        અચલ AstConst* અચલ lsbp = VN_CAST(nodep->lsbp(), Const);
        જો (!lsbp) પાછા ખોટું;
        UASSERT_OBJ(fromp->width(), nodep, "Not widthed");
        જો ((lsbp->toUInt() / fromp->width())
            != ((lsbp->toUInt() + nodep->width() - 1) / fromp->width())) {
            પાછા ખોટું;
        }
        //
        fromp->unlinkFrBack();
        AstSel* અચલ newp = નવું AstSel{
            nodep->fileline(), fromp,
            નવું AstConst{lsbp->fileline(), lsbp->toUInt() % fromp->width()}, nodep->widthConst()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }
    તાર્કિક operandRepRep(AstReplicate* nodep) {
        // REPLICATE(REPLICATE2(from2,cnt2),cnt1) => REPLICATE(from2,(cnt1+cnt2))
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstReplicate* અચલ rep2p = VN_AS(nodep->srcp(), Replicate);
        Ast_ગાંઠ_પદાવલિ* અચલ from2p = rep2p->srcp();
        અચલ AstConst* અચલ cnt1p = VN_CAST(nodep->countp(), Const);
        જો (!cnt1p) પાછા ખોટું;
        અચલ AstConst* અચલ cnt2p = VN_CAST(rep2p->countp(), Const);
        જો (!cnt2p) પાછા ખોટું;
        //
        from2p->unlinkFrBack();
        AstReplicate* અચલ newp
            = નવું AstReplicate{nodep->fileline(), from2p, cnt1p->toUInt() * cnt2p->toUInt()};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }
    તાર્કિક operandConcatSame(AstConcat* nodep) {
        // CONCAT(fromp,fromp) -> REPLICATE(fromp,1+1)
        // CONCAT(REP(fromp,cnt1),fromp) -> REPLICATE(fromp,cnt1+1)
        // CONCAT(fromp,REP(fromp,cnt1)) -> REPLICATE(fromp,1+cnt1)
        // CONCAT(REP(fromp,cnt1),REP(fromp,cnt2)) -> REPLICATE(fromp,cnt1+cnt2)
        Ast_ગાંઠ_પદાવલિ* from1p = nodep->lhsp();
        uint32_t cnt1 = 1;
        Ast_ગાંઠ_પદાવલિ* from2p = nodep->rhsp();
        uint32_t cnt2 = 1;
        જો (VN_IS(from1p, Replicate)) {
            અચલ AstConst* અચલ cnt1p = VN_CAST(VN_CAST(from1p, Replicate)->countp(), Const);
            જો (!cnt1p) પાછા ખોટું;
            from1p = VN_AS(from1p, Replicate)->srcp();
            cnt1 = cnt1p->toUInt();
        }
        જો (VN_IS(from2p, Replicate)) {
            અચલ AstConst* અચલ cnt2p = VN_CAST(VN_CAST(from2p, Replicate)->countp(), Const);
            જો (!cnt2p) પાછા ખોટું;
            from2p = VN_AS(from2p, Replicate)->srcp();
            cnt2 = cnt2p->toUInt();
        }
        જો (!operandsSame(from1p, from2p)) પાછા ખોટું;
        //
        from1p->unlinkFrBack();
        AstReplicate* અચલ newp = નવું AstReplicate{nodep->fileline(), from1p, cnt1 + cnt2};
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }
    શૂન્ય replaceSelIntoBiop(AstSel* nodep) {
        // SEL(BUFIF1(a,b),1,bit) => BUFIF1(SEL(a,1,bit),SEL(b,1,bit))
        Ast_ગાંઠ_Biop* અચલ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ગાંઠ_Biop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ગાંઠ_પદાવલિ* અચલ lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_ગાંઠ_પદાવલિ* અચલ bilhsp = fromp->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ birhsp = fromp->rhsp()->unlinkFrBack();
        //
        fromp->lhsp(
            નવું AstSel{nodep->fileline(), bilhsp, lsbp->clone_વૃક્ષ_Pure(સાચું), nodep->widthConst()});
        fromp->rhsp(નવું AstSel{nodep->fileline(), birhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય replaceSelIntoUniop(AstSel* nodep) {
        // SEL(NOT(a),1,bit) => NOT(SEL(a,bit))
        Ast_ગાંઠ_Uniop* અચલ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ગાંઠ_Uniop);
        UASSERT_OBJ(fromp, nodep, "Called on non-biop");
        Ast_ગાંઠ_પદાવલિ* અચલ lsbp = nodep->lsbp()->unlinkFrBack();
        //
        Ast_ગાંઠ_પદાવલિ* અચલ bilhsp = fromp->lhsp()->unlinkFrBack();
        //
        fromp->lhsp(નવું AstSel{nodep->fileline(), bilhsp, lsbp, nodep->widthConst()});
        nodep->replaceWithKeepDType(fromp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->bitp());
        જો (VN_IS(nodep->bitp(), Const)
            && VN_IS(nodep->fromp(), VarRef)
            // Need to make sure it's an array object so don't mis-allow a constant (bug509.)
            && VN_AS(nodep->fromp(), VarRef)->varp()
            && VN_IS(VN_AS(nodep->fromp(), VarRef)->varp()->valuep(), ConsArray)) {
            m_selp = nodep;  // Ask visit(AstVarRef) to replace varref with const
        }
        iterateAndNextNull(nodep->fromp());
        જો (VN_IS(nodep->fromp(), Const)) {  // It did.
            જો (!m_selp) {
                nodep->v3error("Illegal assignment of constant to unpacked array");
            } નહીંતર {
                Ast_ગાંઠ* અચલ fromp = nodep->fromp()->unlinkFrBack();
                nodep->replaceWithKeepDType(fromp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        }
        // Handle ARRAYSEL directly on ConsArray (not through VarRef)
        નહીંતર જો (VN_IS(nodep->bitp(), Const) && VN_IS(nodep->fromp(), ConsArray)) {
            અચલ AstConsArray* અચલ consp = VN_AS(nodep->fromp(), ConsArray);
            અચલ uint32_t bit = VN_AS(nodep->bitp(), Const)->toUInt();
            Ast_ગાંઠ_પદાવલિ* અચલ itemp = consp->valuep(bit);
            જો (VN_IS(itemp, Const)) {
                અચલ V3_સંખ્યા& અંક = VN_AS(itemp, Const)->અંક();
                VL_DO_DANGLING(replace_અંક(nodep, અંક), nodep);
            } નહીંતર જો (m_params && itemp) {
                // An aggregate element, e.g. a structure in an array parameter
                nodep->replaceWithKeepDType(itemp->clone_વૃક્ષ(ખોટું));
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        }
        m_selp = nullptr;
    }

    // Evaluate a slice of an unpacked array.  If constantification is
    // required (m_required=true), call replaceWithSimulation() to compute
    // the slice via simulation.  Otherwise just iterate the children.
    શૂન્ય visit(AstSliceSel* nodep) override {
        // First constify or width any child nodes
        iterateChildren(nodep);
        જો (!m_required) પાછા;  // Do nothing unless we are in parameter mode
        // Fallback to simulation: this will invoke SimulateVisitor::visit(AstSliceSel*)
        replaceWithSimulation(nodep);
    }

    શૂન્ય visit(AstStructSel* nodep) override {
        iterateChildren(nodep);

        જો (અચલ AstConsStruct* અચલ consp = VN_CAST(nodep->fromp(), ConsStruct)) {
            Ast_ગાંઠ_પદાવલિ* અચલ valuep = consp->memberValuep(nodep->name());
            જો (અચલ AstConst* અચલ constp = VN_CAST(valuep, Const)) {
                VL_DO_DANGLING(replace_અંક(nodep, constp->અંક()), nodep);
            } નહીંતર {
                nodep->replaceWith(valuep->clone_વૃક્ષ(ખોટું));
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        }
    }

    શૂન્ય visit(AstCAwait* nodep) override {
        m_hasJumpDelay = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Not linked");
        તાર્કિક did = ખોટું;
        જો (m_doV && (!nodep->varp()->constPoolEntry() || m_selp) && nodep->varp()->valuep()
            && !m_attrp) {
            // UINFOTREE(1, valuep, "", "visitvaref");
            iterateAndNextNull(nodep->varp()->valuep());  // May change nodep->varp()->valuep()
            Ast_ગાંઠ* અચલ valuep = nodep->varp()->valuep();
            // The selected element, when under a constant select of an array
            અચલ AstConsArray* અચલ consp = VN_CAST(valuep, ConsArray);
            અચલ Ast_ગાંઠ* અચલ selItemp
                = (m_selp && consp) ? consp->valuep(m_selp->bitConst()) : nullptr;
            જો (nodep->access().isReadOnly() && valuep
                && ((!m_params  // Can reduce constant wires into equations
                     && m_doNConst && v3Global.opt.fConst() && nodep->varp()->isConst())
                    || nodep->varp()->isParam())) {
                જો (operandConst(valuep)) {
                    અચલ V3_સંખ્યા& અંક = VN_AS(valuep, Const)->અંક();
                    // UINFO(2, "constVisit " << cvtToHex(valuep) << " " << num);
                    અચલ string origParamName
                        = nodep->varp()->isParam() ? nodep->varp()->name() : "";
                    VL_DO_DANGLING(replace_અંક(nodep, અંક, origParamName), nodep);
                    did = સાચું;
                } નહીંતર જો (અચલ AstConst* અચલ itemp = VN_CAST(selItemp, Const)) {
                    VL_DO_DANGLING(replace_અંક(nodep, itemp->અંક()), nodep);
                    did = સાચું;
                } નહીંતર જો (m_params && consp) {
                    // Allow parameters to pass arrays
                    // Earlier recursion of ConsArray made sure each array value is constant
                    // This exception is fairly fragile, i.e. doesn't
                    // support arrays of arrays or other stuff
                    Ast_ગાંઠ* અચલ newp = valuep->clone_વૃક્ષ(ખોટું);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    did = સાચું;
                } નહીંતર જો (m_params && VN_IS(valuep, ConsStruct)) {
                    Ast_ગાંઠ* અચલ newp = valuep->clone_વૃક્ષ(ખોટું);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    did = સાચું;
                } નહીંતર જો (nodep->varp()->isParam() && VN_IS(valuep, Unbounded)) {
                    Ast_ગાંઠ* અચલ newp = valuep->clone_વૃક્ષ(ખોટું);
                    nodep->replaceWithKeepDType(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    did = સાચું;
                }
            }
        }
        જો (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but variable isn't const: "
                           << nodep->varp()->prettyNameQ());
        }
    }
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        iterateChildren(nodep);
        જો (!Ast_ગાંઠ::afterCommentp(nodep->stmtsp())) {
            deleteVarScopesUnder(nodep->stmtsp());
            UINFO(8, "ExprStmt(...) " << nodep << " " << nodep->resultp());
            nodep->replaceWith(nodep->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            // Removing the ExprStmt might have made something impure above now pure
        }
    }
    શૂન્ય visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        તાર્કિક did = ખોટું;
        જો (nodep->itemp()->valuep()) {
            // UINFOTREE(1, nodep->itemp()->valuep(), "", "visitvaref");
            અચલ AstEnumItem* અચલ itemp = nodep->itemp();
            જો (m_recursing_Enum_વસ્તુઓ.ગણતરી(itemp)) {
                nodep->v3error("Recursive enum value: " << nodep->itemp()->prettyNameQ());
            } નહીંતર {
                m_recursing_Enum_વસ્તુઓ.emplace(itemp);
                iterateAndNextNull(nodep->itemp()->valuep());
                m_recursing_Enum_વસ્તુઓ.erase(itemp);
            }
            જો (AstConst* અચલ valuep = VN_CAST(nodep->itemp()->valuep(), Const)) {
                અચલ V3_સંખ્યા& અંક = valuep->અંક();
                VL_DO_DANGLING(replace_અંક(nodep, અંક), nodep);
                did = સાચું;
            }
        }
        જો (!did && m_required) {
            nodep->v3error("Expecting expression to be constant, but enum value isn't const: "
                           << nodep->itemp()->prettyNameQ());
        }
    }

    તાર્કિક only_Sen_Item_In_Sen_વૃક્ષ(અચલ AstSenItem* nodep) {
        // Only one if it's not in a list
        પાછા (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }
    શૂન્ય visit(AstSenItem* nodep) override {
        iterateChildren(nodep);
        જો (m_doNConst
            && !v3Global.opt.timing().isSetTrue()  // If --timing, V3Sched would turn this into an
                                                   // infinite loop. See #5080
            && (VN_IS(nodep->sensp(), Const) || VN_IS(nodep->sensp(), EnumItemRef)
                || (nodep->varrefp() && nodep->varrefp()->varp()->isParam()))) {
            // Constants in sensitivity lists may be removed (we'll simplify later)
            જો (nodep->isClocked()) {  // A constant can never get a pos/negedge
                જો (only_Sen_Item_In_Sen_વૃક્ષ(nodep)) {
                    જો (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                        // TODO: This really is dodgy, as strictly compliant simulators will not
                        //       execute this block, but but t_func_check relies on it
                        nodep->replaceWith(
                            નવું AstSenItem{nodep->fileline(), AstSenItem::Initial{}});
                    } નહીંતર {
                        nodep->replaceWith(નવું AstSenItem{nodep->fileline(), AstSenItem::Never{}});
                    }
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                } નહીંતર {
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } નહીંતર {  // Otherwise it may compute a result that needs to settle out
                nodep->replaceWith(નવું AstSenItem{nodep->fileline(), AstSenItem::Combo{}});
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        } નહીંતર જો (m_doNConst && VN_IS(nodep->sensp(), Not)) {
            // V3Gate may propagate NOTs into clocks... Just deal with it
            Ast_ગાંઠ* અચલ sensp = nodep->sensp();
            Ast_ગાંઠ* lastSensp = sensp;
            તાર્કિક invert = ખોટું;
            જ્યારે (VN_IS(lastSensp, Not)) {
                lastSensp = VN_AS(lastSensp, Not)->lhsp();
                invert = !invert;
            }
            UINFO(8, "senItem(NOT...) " << nodep << " " << invert);
            જો (invert) nodep->edgeType(nodep->edgeType().invert());
            sensp->replaceWith(lastSensp->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(sensp), sensp);
        }
    }

    વર્ગ SenItemCmp final {
        સ્થિર પૂર્ણાંક cmp(અચલ Ast_ગાંઠ_પદાવલિ* ap, અચલ Ast_ગાંઠ_પદાવલિ* bp) {
            અચલ VNType aType = ap->type();
            અચલ VNType bType = bp->type();
            જો (aType != bType) પાછા static_cast<પૂર્ણાંક>(bType) - static_cast<પૂર્ણાંક>(aType);

            જો (અચલ AstVarRef* અચલ aRefp = VN_CAST(ap, VarRef)) {
                અચલ AstVarRef* અચલ bRefp = VN_AS(bp, VarRef);
                // Looks visually better if we keep sorted by name
                જો (aRefp->name() < bRefp->name()) પાછા -1;
                જો (aRefp->name() > bRefp->name()) પાછા 1;
                // But might be same name with different scopes
                જો (aRefp->varScopep() < bRefp->varScopep()) પાછા -1;
                જો (aRefp->varScopep() > bRefp->varScopep()) પાછા 1;
                // Or rarely, different data types
                જો (aRefp->dtypep() < bRefp->dtypep()) પાછા -1;
                જો (aRefp->dtypep() > bRefp->dtypep()) પાછા 1;
                પાછા 0;
            }

            જો (અચલ AstConst* અચલ aConstp = VN_CAST(ap, Const)) {
                અચલ AstConst* અચલ bConstp = VN_AS(bp, Const);
                જો (aConstp->અંક().isLtXZ(bConstp->અંક())) પાછા -1;
                જો (bConstp->અંક().isLtXZ(aConstp->અંક())) પાછા 1;
                પાછા 0;
            }

            જો (અચલ Ast_ગાંઠ_Biop* અચલ aBiOpp = VN_CAST(ap, ગાંઠ_Biop)) {
                અચલ Ast_ગાંઠ_Biop* અચલ bBiOpp = VN_AS(bp, ગાંઠ_Biop);
                // Compare RHSs first, as LHS might be const
                જો (અચલ પૂર્ણાંક c = cmp(aBiOpp->rhsp(), bBiOpp->rhsp())) પાછા c;
                પાછા cmp(aBiOpp->lhsp(), bBiOpp->lhsp());
            }

            જો (અચલ AstCMethodHard* અચલ aCallp = VN_CAST(ap, CMethodHard)) {
                અચલ AstCMethodHard* અચલ bCallp = VN_AS(bp, CMethodHard);
                જો (aCallp->method() < bCallp->method()) પાછા -1;
                જો (aCallp->method() > bCallp->method()) પાછા 1;
                જો (અચલ પૂર્ણાંક c = cmp(aCallp->fromp(), bCallp->fromp())) પાછા c;
                અચલ Ast_ગાંઠ_પદાવલિ* aPinsp = aCallp->pinsp();
                અચલ Ast_ગાંઠ_પદાવલિ* bPinsp = bCallp->pinsp();
                જ્યારે (aPinsp && bPinsp) {
                    જો (અચલ પૂર્ણાંક c = cmp(aPinsp, bPinsp)) પાછા c;
                    aPinsp = VN_AS(aPinsp->nextp(), ગાંઠ_પદાવલિ);
                    bPinsp = VN_AS(bPinsp->nextp(), ગાંઠ_પદાવલિ);
                }
                પાછા aPinsp ? -1 : bPinsp ? 1 : 0;
            }

            પાછા 0;
        }

    જાહેર:
        તાર્કિક operator()(અચલ AstSenItem* lhsp, અચલ AstSenItem* rhsp) અચલ {
            અચલ Ast_ગાંઠ_પદાવલિ* અચલ lSensp = lhsp->sensp();
            અચલ Ast_ગાંઠ_પદાવલિ* અચલ rSensp = rhsp->sensp();
            જો (lSensp && rSensp) {
                // If both terms have sensitivity expressions, recursively compare them
                જો (અચલ પૂર્ણાંક c = cmp(lSensp, rSensp)) પાછા c < 0;
            } નહીંતર જો (lSensp || rSensp) {
                // Terms with sensitivity expressions come after those without
                પાછા rSensp;
            }
            // Finally sort by edge, AFTER variable, as we want multiple edges for same var
            // adjacent. note the SenTree optimizer requires this order (more general first,
            // less general last)
            પાછા lhsp->edgeType() < rhsp->edgeType();
        }
    };

    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override {
        iterateChildren(nodep);
        જો (m_doExpensive) {
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
            માટે (AstSenItem *nextp, *senp = nodep->sensesp(); senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                // cppcheck-suppress unassignedVariable  // cppcheck bug
                અચલ SenItemCmp cmp;
                જો (nextp && !cmp(senp, nextp)) {
                    // Something's out of order, sort it
                    senp = nullptr;
                    std::vector<AstSenItem*> vec;
                    માટે (AstSenItem* senp = nodep->sensesp(); senp;
                         senp = VN_AS(senp->nextp(), SenItem)) {
                        vec.ધકેલો_back(senp);
                    }
                    stable_sort(vec.begin(), vec.end(), SenItemCmp());
                    માટે (અચલ auto& ip : vec) ip->unlinkFrBack();
                    માટે (અચલ auto& ip : vec) nodep->addSensesp(ip);
                    તોડો;
                }
            }

            // Pass 2, remove duplicates and simplify adjacent terms if possible
            માટે (AstSenItem *senp = nodep->sensesp(), *nextp; senp; senp = nextp) {
                nextp = VN_AS(senp->nextp(), SenItem);
                જો (!nextp) તોડો;
                AstSenItem* અચલ lItemp = senp;
                AstSenItem* અચલ rItemp = nextp;
                અચલ Ast_ગાંઠ_પદાવલિ* અચલ lSenp = lItemp->sensp();
                અચલ Ast_ગાંઠ_પદાવલિ* અચલ rSenp = rItemp->sensp();
                જો (!lSenp || !rSenp) ચાલુ;

                જો (lSenp->same_Gate_વૃક્ષ(rSenp)) {
                    // POSEDGE or NEGEDGE -> BOTHEDGE. (We've sorted POSEDGE, before NEGEDGE, so we
                    // do not need to test for the opposite orders.)
                    જો (lItemp->edgeType() == VEdgeType::ET_POSEDGE
                        && rItemp->edgeType() == VEdgeType::ET_NEGEDGE) {
                        // Make both terms BOTHEDGE, the second will be removed below
                        lItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                        rItemp->edgeType(VEdgeType::ET_BOTHEDGE);
                    }

                    // Remove identical expressions
                    જો (lItemp->edgeType() == rItemp->edgeType()) {
                        VL_DO_DANGLING(ધકેલો_Deletep(rItemp->unlinkFrBack()), rItemp);
                        nextp = lItemp;
                    }
                }
            }
        }
    }

    //-----
    // Zero elimination
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        iterateChildren(nodep);
        જો (nodep->timingControlp()) m_hasJumpDelay = સાચું;
        જો (m_doNConst && replace_ગાંઠ_Assign(nodep)) પાછા;
    }
    શૂન્ય visit(AstAlias* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    શૂન્ય visit(AstAliasScope* nodep) override {
        // Don't perform any optimizations, keep the alias around
    }
    શૂન્ય visit(AstAssignW* nodep) override {
        iterateChildren(nodep);
        જો (m_doNConst && replace_ગાંઠ_Assign(nodep)) પાછા;
        // Process containing this AssignW as single body statement
        AstAlways* અચલ procp = VN_CAST(nodep->backp(), Always);
        જો (!procp || procp->stmtsp() != nodep || nodep->nextp()) પાછા;
        // Not VarXRef, as different refs may set different values to each hierarchy
        Ast_ગાંઠ_Var_Ref* અચલ varrefp = VN_CAST(nodep->lhsp(), VarRef);
        જો (m_wremove && !m_params && m_doNConst && m_modp && operandConst(nodep->rhsp())
            && !VN_AS(nodep->rhsp(), Const)->અંક().isFourState()
            && varrefp  // Don't do messes with BITREFs/ARRAYREFs
            && !varrefp->varp()->hasStrengthAssignment()  // Strengths are resolved in V3Tristate
            && !varrefp->varp()->valuep()  // Not already constified
            && !varrefp->varScopep()  // Not scoped (or each scope may have different initial val.)
            && !varrefp->varp()->isForced()  // Not forced (not really a constant)
        ) {
            // ASSIGNW (VARREF, const) -> INITIAL ( ASSIGN (VARREF, const) )
            UINFO(4, "constAssignW " << nodep);
            // Make a initial assignment
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->rhsp()->unlinkFrBack();
            varrefp->unlinkFrBack();
            File_લીટી* અચલ flp = nodep->fileline();
            AstInitial* અચલ newinitp = નવું AstInitial{flp, નવું AstAssign{flp, varrefp, exprp}};
            procp->replaceWith(newinitp);
            VL_DO_DANGLING(ધકેલો_Deletep(procp), procp);
            // Set the initial value right in the variable so we can constant propagate
            Ast_ગાંઠ* અચલ initvaluep = exprp->clone_વૃક્ષ(ખોટું);
            varrefp->varp()->isConst(સાચું);
            varrefp->varp()->valuep(initvaluep);
        }
    }
    શૂન્ય visit(AstCReset* nodep) override {
        iterateChildren(nodep);
        જો (!m_doNConst) પાછા;
        અચલ AstBasicDType* અચલ bdtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType);
        જો (!bdtypep) પાછા;
        જો (!bdtypep->isZeroInit()) પાછા;
        AstConst* અચલ newp = નવું AstConst{nodep->fileline(), V3_સંખ્યા{nodep, bdtypep}};
        UINFO(9, "CRESET(0) => CONST(0) " << nodep);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstCvtUnpackToUnpack* nodep) override {
        iterateChildren(nodep);
        AstConsArray* અચલ fromp = VN_CAST(nodep->fromp(), ConsArray);
        જો (!fromp) પાછા;
        // Fold the outermost dimension, converting the elements for any dimension under it
        અચલ AstUnpackArrayDType* અચલ toArrayp
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        અચલ AstUnpackArrayDType* અચલ fromArrayp
            = VN_AS(fromp->dtypep()->skipRefp(), UnpackArrayDType);
        // This dimension may not be reversed, only one under it
        અચલ તાર્કિક reverse
            = toArrayp->declRange().ascending() != fromArrayp->declRange().ascending();
        અચલ uint64_t lastIndex = toArrayp->elementsConst() - 1;
        Ast_ગાંઠ_DType* અચલ subDTypep = toArrayp->subDTypep();
        અચલ AstUnpackArrayDType* અચલ subArrayp
            = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
        અચલ auto convert = [&](Ast_ગાંઠ_પદાવલિ* valuep) -> Ast_ગાંઠ_પદાવલિ* {
            valuep->unlinkFrBack();
            જો (!subArrayp || !subArrayp->isReverseOf(valuep->dtypep())) પાછા valuep;
            પાછા નવું AstCvtUnpackToUnpack{valuep->fileline(), valuep, subDTypep};
        };
        AstConsArray* અચલ newp
            = નવું AstConsArray{fromp->fileline(), nodep->dtypep(),
                               fromp->defaultp() ? convert(fromp->defaultp()) : nullptr};
        માટે (અચલ AstConsArrayItem* itemp = fromp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            newp->valuep(reverse ? lastIndex - itemp->index() : itemp->index(),
                         convert(itemp->valuep()));
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        iterate(newp);  // Fold the conversions of the elements
    }
    શૂન્ય visit(AstCvtArrayToArray* nodep) override {
        iterateChildren(nodep);
        // Handle the case where we have a stream operation inside a cast conversion
        // Check for both StreamL and StreamR operations
        Ast_ગાંઠ_Stream* streamp = nullptr;
        તાર્કિક isReverse = ખોટું;
        જો (AstStreamL* અચલ streamLp = VN_CAST(nodep->fromp(), StreamL)) {
            streamp = streamLp;
            isReverse = સાચું;  // StreamL reverses the operation
        } નહીંતર જો (AstStreamR* અચલ streamRp = VN_CAST(nodep->fromp(), StreamR)) {
            streamp = streamRp;
            isReverse = ખોટું;  // StreamR doesn't reverse the operation
        }
        જો (streamp) {
            Ast_ગાંઠ_પદાવલિ* srcp = streamp->lhsp();
            અચલ Ast_ગાંઠ_DType* અચલ srcDTypep = srcp->dtypep()->skipRefp();
            Ast_ગાંઠ_DType* અચલ dstDTypep = nodep->dtypep()->skipRefp();
            જો (VN_IS(srcDTypep, QueueDType) && VN_IS(dstDTypep, QueueDType)) {
                પૂર્ણાંક blockSize = 1;
                જો (અચલ AstConst* અચલ constp = VN_CAST(streamp->rhsp(), Const)) {
                    blockSize = constp->toSInt();
                    જો (VL_UNLIKELY(blockSize <= 0)) {
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
                પૂર્ણાંક srcElementBits = 0;
                જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = srcDTypep->subDTypep()) {
                    srcElementBits = elemDtp->width();
                }
                પૂર્ણાંક dstElementBits = 0;
                જો (અચલ Ast_ગાંઠ_DType* અચલ elemDtp = dstDTypep->subDTypep()) {
                    dstElementBits = elemDtp->width();
                }
                streamp->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* newp = નવું AstCvtArrayToArray{
                    srcp->fileline(), srcp->unlinkFrBack(), dstDTypep,     isReverse,
                    blockSize,        dstElementBits,       srcElementBits};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(streamp), streamp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            }
        }
    }
    શૂન્ય visit(AstRelease* nodep) override {
        // cppcheck-suppress constVariablePointer // children unlinked below
        જો (AstConcat* અચલ concatp = VN_CAST(nodep->lhsp(), Concat)) {
            File_લીટી* અચલ flp = nodep->fileline();
            AstRelease* અચલ newLp = નવું AstRelease{flp, concatp->lhsp()->unlinkFrBack()};
            AstRelease* અચલ newRp = નવું AstRelease{flp, concatp->rhsp()->unlinkFrBack()};
            nodep->replaceWith(newLp);
            newLp->addNextHere(newRp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            visit(newLp);
            visit(newRp);
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        iterateChildren(nodep);
        જો (m_doNConst) {
            જો (અચલ AstConst* અચલ constp = VN_CAST(nodep->condp(), Const)) {
                Ast_ગાંઠ* keepp = nullptr;
                Ast_ગાંઠ* delp = nullptr;
                જો (constp->isZero()) {
                    UINFO(4, "IF(0,{any},{x}) => {x}: " << nodep);
                    keepp = nodep->elsesp();
                    delp = nodep->thensp();
                } નહીંતર જો (!m_doV || constp->isNeqZero()) {  // Might be X in Verilog
                    UINFO(4, "IF(!0,{x},{any}) => {x}: " << nodep);
                    keepp = nodep->thensp();
                    delp = nodep->elsesp();
                } નહીંતર {
                    UINFO(4, "IF condition is X, retaining: " << nodep);
                    પાછા;
                }
                જો (!canDeleteStatements()) {
                    UINFO(4, "IF retained until statics lifted: " << nodep);
                    પાછા;
                }

                // If we delete a branch that contains variable declarations, also delete the
                // corresponding varscopes so we don't leave dangling AstVarScope::m_varp pointers.
                deleteVarScopesUnder(delp);

                જો (keepp) {
                    keepp->unlinkFrBackWithNext();
                    nodep->replaceWith(keepp);
                } નહીંતર {
                    nodep->unlinkFrBack();
                    deleteVarScopesUnder(nodep->thensp());
                    deleteVarScopesUnder(nodep->elsesp());
                }
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);

                // Removed branch could contain only impurity within a function and the
                // function could be purified
                VIsCached::clear_Cache_વૃક્ષ();
            } નહીંતર જો (!Ast_ગાંઠ::afterCommentp(nodep->thensp())
                       && !Ast_ગાંઠ::afterCommentp(nodep->elsesp())) {
                જો (!nodep->condp()->isPure()) {
                    // Condition has side effect - leave, but don't need return value
                    UINFO(4, "IF({x}) ; => STMTEXPR({x}): " << nodep);
                    nodep->fileline()->warnOff(V3ErrorCode::IGNOREDRETURN, સાચું);
                    Ast_ગાંઠ_Stmt* અચલ newp
                        = નવું Ast_Stmt_પદાવલિ{nodep->fileline(), nodep->condp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                } નહીંતર {
                    // Empty block, remove it
                    UINFO(4, "IF({x}) ; => ; : " << nodep);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                }
            } નહીંતર જો (!Ast_ગાંઠ::afterCommentp(nodep->thensp())) {
                UINFO(4, "IF({x}) nullptr {...} => IF(NOT{x}}: " << nodep);
                Ast_ગાંઠ_પદાવલિ* અચલ condp = nodep->condp();
                Ast_ગાંઠ* અચલ elsesp = nodep->elsesp();
                condp->unlinkFrBackWithNext();
                elsesp->unlinkFrBackWithNext();
                જો (nodep->thensp()) {  // Must have been comment
                    ધકેલો_Deletep(nodep->thensp()->unlinkFrBackWithNext());
                }
                nodep->condp(નવું AstLogNot{condp->fileline(),
                                           condp});  // LogNot, as C++ optimization also possible
                nodep->addThensp(elsesp);
                nodep->branchPred(nodep->branchPred().invert());
            } નહીંતર જો (((VN_IS(nodep->condp(), Not) && nodep->condp()->width() == 1)
                        || VN_IS(nodep->condp(), LogNot))
                       && nodep->thensp() && nodep->elsesp()) {
                UINFO(4, "IF(NOT {x})  => IF(x) swapped if/else" << nodep);
                Ast_ગાંઠ_પદાવલિ* અચલ condp
                    = VN_AS(nodep->condp(), ગાંઠ_Uniop)->lhsp()->unlinkFrBackWithNext();
                Ast_ગાંઠ* અચલ thensp = nodep->thensp()->unlinkFrBackWithNext();
                Ast_ગાંઠ* અચલ elsesp = nodep->elsesp()->unlinkFrBackWithNext();
                AstIf* અચલ ifp = નવું AstIf{nodep->fileline(), condp, elsesp, thensp};
                ifp->isBoundsCheck(nodep->isBoundsCheck());  // Copy bounds check info
                ifp->branchPred(nodep->branchPred().invert());
                nodep->replaceWith(ifp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            } નહીંતર જો (ifSameAssign(nodep)) {
                UINFO(4,
                      "IF({a}) ASSIGN({b},{c}) else ASSIGN({b},{d}) => ASSIGN({b}, {a}?{c}:{d})");
                Ast_ગાંઠ_Assign* અચલ thensp = VN_AS(nodep->thensp(), ગાંઠ_Assign);
                // cppcheck-suppress constVariablePointer // children unlinked below
                Ast_ગાંઠ_Assign* અચલ elsesp = VN_AS(nodep->elsesp(), ગાંઠ_Assign);
                thensp->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ condp = nodep->condp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ truep = thensp->rhsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ falsep = elsesp->rhsp()->unlinkFrBack();
                thensp->rhsp(નવું AstCond{truep->fileline(), condp, truep, falsep});
                nodep->replaceWith(thensp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            } નહીંતર જો (ખોટું  // Disabled, as vpm assertions are faster
                              // without due to short-circuiting
                       && operandIfIf(nodep)) {
                UINFO(9, "IF({a}) IF({b}) => IF({a} && {b})");
                Ast_ગાંઠ_If* અચલ lowerIfp = VN_AS(nodep->thensp(), ગાંઠ_If);
                Ast_ગાંઠ_પદાવલિ* અચલ condp = nodep->condp()->unlinkFrBack();
                Ast_ગાંઠ* અચલ lowerThensp = lowerIfp->thensp()->unlinkFrBackWithNext();
                Ast_ગાંઠ_પદાવલિ* અચલ lowerCondp = lowerIfp->condp()->unlinkFrBackWithNext();
                nodep->condp(નવું AstLogAnd{lowerIfp->fileline(), condp, lowerCondp});
                lowerIfp->replaceWith(lowerThensp);
                VL_DO_DANGLING(ધકેલો_Deletep(lowerIfp), lowerIfp);
            } નહીંતર {
                // Optimizations that don't reform the IF itself
                જો (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
                matchIfSameLast(nodep);
                matchIfCondCond(nodep);
            }
        }
    }

    શૂન્ય visit(AstDisplay* nodep) override {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        iterateChildren(nodep);
        જો (stmtDisplayDisplay(nodep)) પાછા;
    }
    તાર્કિક stmtDisplayDisplay(AstDisplay* nodep) {
        // DISPLAY(SFORMAT(text1)),DISPLAY(SFORMAT(text2)) -> DISPLAY(SFORMAT(text1+text2))
        જો (!m_modp) પાછા ખોટું;  // Don't optimize under single statement
        AstDisplay* અચલ prevp = VN_CAST(nodep->backp(), Display);
        જો (!prevp) પાછા ખોટું;
        જો (!((prevp->displayType() == nodep->displayType())
              || (prevp->displayType() == VDisplayType::DT_WRITE
                  && nodep->displayType() == VDisplayType::DT_DISPLAY)
              || (prevp->displayType() == VDisplayType::DT_DISPLAY
                  && nodep->displayType() == VDisplayType::DT_WRITE)))
            પાછા ખોટું;
        જો ((prevp->filep() && !nodep->filep()) || (!prevp->filep() && nodep->filep())
            || (prevp->filep() && nodep->filep() && !prevp->filep()->same_વૃક્ષ(nodep->filep())))
            પાછા ખોટું;
        જો (!prevp->fmtp() || prevp->fmtp()->nextp() || !nodep->fmtp() || nodep->fmtp()->nextp())
            પાછા ખોટું;
        AstSFormatF* અચલ pformatp = prevp->fmtp();
        // cppcheck-suppress constVariablePointer // children unlinked below
        AstSFormatF* અચલ nformatp = nodep->fmtp();
        // We don't merge scopeNames as can have only one and might be different scopes (late in
        // process) Also rare for real code to print %m multiple times in same message
        જો (nformatp->scopeNamep() && pformatp->scopeNamep()) પાછા ખોટું;
        // We don't early merge arguments as might need to later print warnings with
        // right line numbers, nor scopeNames as might be different scopes (late in process)
        જો (!m_doCpp && pformatp->exprsp()) પાછા ખોટું;
        જો (!m_doCpp && nformatp->exprsp()) પાછા ખોટું;
        જો (pformatp->exprsp() && !pformatp->exprsp()->isPureAndNext()) પાછા ખોટું;
        જો (nformatp->exprsp() && !nformatp->exprsp()->isPureAndNext()) પાછા ખોટું;
        // Avoid huge merges
        સ્થિર constexpr પૂર્ણાંક DISPLAY_MAX_MERGE_LENGTH = 500;
        જો (pformatp->text().length() + nformatp->text().length() > DISPLAY_MAX_MERGE_LENGTH)
            પાછા ખોટું;
        //
        UINFO(9, "DISPLAY(SF({a})) DISPLAY(SF({b})) -> DISPLAY(SF({a}+{b}))");
        // Convert DT_DISPLAY to DT_WRITE as may allow later optimizations
        જો (prevp->displayType() == VDisplayType::DT_DISPLAY) {
            prevp->displayType(VDisplayType::DT_WRITE);
            pformatp->text(pformatp->text() + "\n");
        }
        // We can't replace prev() as the edit tracking iterators will get confused.
        // So instead we edit the prev note itself.
        જો (prevp->addNewline()) pformatp->text(pformatp->text() + "\n");
        pformatp->text(pformatp->text() + nformatp->text());
        જો (!prevp->addNewline() && nodep->addNewline()) pformatp->text(pformatp->text() + "\n");
        જો (nformatp->exprsp()) pformatp->addExprsp(nformatp->exprsp()->unlinkFrBackWithNext());
        જો (AstScopeName* અચલ scopeNamep = nformatp->scopeNamep()) {
            scopeNamep->unlinkFrBackWithNext();
            pformatp->scopeNamep(scopeNamep);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        પાછા સાચું;
    }
    શૂન્ય visit(AstSFormatArg* nodep) override {
        // Skip namep(): requiring its runtime lookup to be constant can reject valid
        // enum-valued constant-function calls in parameters. displayedEnum() resolves
        // the name from the folded exprp() value and enum dtype instead.
        iterateAndNextNull(nodep->exprp());
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        // Substitute constants into displays.  The main point of this is to
        // simplify assertion methodologies which call functions with display's.
        // This eliminates a pile of wide temps, and makes the C a whole lot more readable.
        iterateChildren(nodep);
        જો (nodep->પદાવલિ_Format_2()) {
            // Upconvert to text-based format?
            // Similar code in V3LinkResolve::visit(AstSFormatF)
            UASSERT_OBJ(nodep->text() == "", nodep,
                        "Non-format $sformatf should have text format == \"\"");
            જો (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->અંક().isFromString()) {
                AstConst* અચલ fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->અંક().toString());
                nodep->પદાવલિ_Format_2(ખોટું);
                VL_DO_DANGLING(ધકેલો_Deletep(fmtp), fmtp);
            }
        }
        જો (nodep->પદાવલિ_Format_2()) પાછા;  // Runtime, unfortunately  (unless constify format later)
        તાર્કિક anyconst = ખોટું;
        માટે (Ast_ગાંઠ* argp = nodep->exprsp(); argp; argp = argp->nextp()) {
            AstSFormatArg* અચલ fargp = VN_CAST(argp, SFormatArg);
            Ast_ગાંઠ* અચલ subargp = fargp ? fargp->exprp() : argp;
            જો (VN_IS(subargp, Const)) {
                anyconst = સાચું;
                તોડો;
            }
        }
        જો (m_doNConst && anyconst) {
            UINFO(9, "  Display in  " << nodep->text());
            string newFormat;
            string fmt;
            તાર્કિક inPct = ખોટું;
            Ast_ગાંઠ* argp = nodep->exprsp();
            અચલ string text = nodep->text();
            માટે (અક્ષર ch : text) {
                જો (!inPct && ch == '%') {
                    inPct = સાચું;
                    fmt = ch;
                } નહીંતર જો (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } નહીંતર જો (inPct) {
                    inPct = ખોટું;
                    fmt += ch;
                    ch = std::tolower(ch);
                    પસંદગી (ch) {
                    સ્થિતિ '%': તોડો;  // %% - still %%
                    સ્થિતિ 'm': તોડો;  // %m - still %m - auto insert "name"
                    સ્થિતિ 'l': તોડો;  // %l - still %l - auto insert "library"
                    સ્થિતિ 't':  // %t - don't know $timeformat so can't constify
                        જો (argp) argp = argp->nextp();
                        તોડો;
                    મૂળભૂત:  // Most operators, just move to next argument
                        જો (argp) {
                            Ast_ગાંઠ* અચલ nextp = argp->nextp();
                            AstSFormatArg* અચલ fargp = VN_CAST(argp, SFormatArg);
                            Ast_ગાંઠ* અચલ subargp = fargp ? fargp->exprp() : argp;
                            અચલ VFormatAttr formatAttr
                                = argp
                                      ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                                      : VFormatAttr{};
                            જો (VN_IS(subargp, Const)) {  // Convert it
                                અચલ string out
                                    = formatAttr.isEnum()
                                          ? const_અંક_V(subargp).displayedEnum(fargp, fmt)
                                          : const_અંક_V(subargp).displayed(nodep, fmt, formatAttr);
                                UINFO(9, "     DispConst: " << fmt << " -> " << out << "  for "
                                                            << subargp);
                                // fmt = out w/ replace % with %% as it must later when
                                // used as a format output as literal.
                                fmt = VString::quotePercent(out);
                                VL_DO_DANGLING(ધકેલો_Deletep(argp->unlinkFrBack()), argp);
                            }
                            argp = nextp;
                        }
                        તોડો;
                    }  // switch
                    newFormat += fmt;
                } નહીંતર {
                    newFormat += ch;
                }
            }
            જો (newFormat != nodep->text()) {
                nodep->text(newFormat);
                UINFO(9, "  Display out " << nodep);
            }
        }
        જો (!nodep->exprsp() && nodep->text().find('%') == string::npos && !nodep->hidden()) {
            // Just a simple constant string - the formatting is pointless
            UINFO(9, "replaceConstStr");
            VL_DO_DANGLING(replaceConstString(nodep, nodep->text()), nodep);
            પાછા;
        }
        જો (VN_IS(nodep->backp(), ગાંઠ_Assign)  // Can't remove under Display etc
            && nodep->text() == "%s" && VN_IS(nodep->exprsp(), SFormatArg)
            && VN_AS(nodep->exprsp(), SFormatArg)->formatAttr().isString()
            && !nodep->exprsp()->nextp()) {
            UINFO(9, "  Display(%p, expr) -> constant expr " << nodep);
            nodep->replaceWith(VN_AS(nodep->exprsp(), SFormatArg)->exprp()->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
    }
    શૂન્ય visit(AstSFormat* nodep) override {
        iterateChildren(nodep);
        જો (!m_doNConst) પાછા;
        જો (m_doNConst) {
            // If it's a constant string written to a string variable, inline it as an assignment
            AstSFormatF* અચલ fmtp = nodep->fmtp();
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp();
            AstBasicDType* અચલ basicp = VN_CAST(lhsp->dtypep()->skipRefp(), BasicDType);
            જો (basicp && basicp->isString() && !fmtp->exprsp()
                && fmtp->text().find('%') == string::npos) {
                AstConst* અચલ strp
                    = નવું AstConst{fmtp->fileline(), AstConst::String{}, fmtp->text()};
                lhsp->unlinkFrBack();
                nodep->replaceWith(નવું AstAssign{nodep->fileline(), lhsp, strp});
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_underRecFunc);
        જો (nodep->recursive()) m_underRecFunc = સાચું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        iterateChildren(nodep);
        m_hasJumpDelay = સાચું;  // As don't analyze inside tasks for timing controls
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        // Note excludes AstFuncRef as other visitor below
        iterateChildren(nodep);
        m_hasJumpDelay = સાચું;  // As don't analyze inside tasks for timing controls
    }
    શૂન્ય visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_ગાંઠ_FTask_Ref*>(nodep));
        જો (m_params) {  // Only parameters force us to do constant function call propagation
            replaceWithSimulation(nodep);
        }
    }
    શૂન્ય visit(AstArg* nodep) override {
        // replaceWithSimulation on the Arg's parent FuncRef replaces these
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstLoop* nodep) override {
        VL_RESTORER(m_hasLoopTest);
        અચલ તાર્કિક oldHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = ખોટું;
        m_hasLoopTest = ખોટું;
        iterateChildren(nodep);
        અચલ તાર્કિક thisLoopHasJumpDelay = m_hasJumpDelay;
        m_hasJumpDelay = thisLoopHasJumpDelay || oldHasJumpDelay;
        // If the first statement always break, the loop is useless
        જો (અચલ AstLoopTest* અચલ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            જો (testp->condp()->isZero() && (!testp->nextp() || canDeleteStatements())) {
                nodep->v3warn(UNUSEDLOOP, "Loop condition is always false");
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
        }
        // If last statement always breaks, repalce loop with body
        જો (Ast_ગાંઠ* lastp = nodep->stmtsp()) {
            જ્યારે (Ast_ગાંઠ* અચલ nextp = lastp->nextp()) lastp = nextp;
            જો (AstLoopTest* અચલ testp = VN_CAST(lastp, LoopTest)) {
                જો (testp->condp()->isZero()) {
                    VL_DO_DANGLING2(ધકેલો_Deletep(testp->unlinkFrBack()), testp, lastp);
                    nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
            }
        }
        // Warn on infinite loop
        જો (!m_hasLoopTest && !thisLoopHasJumpDelay) {
            nodep->v3warn(INFINITELOOP, "Infinite loop (condition always true)");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::INFINITELOOP, સાચું);  // Complain once
        }
    }
    શૂન્ય visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);

        // If never breaks, remove
        જો (nodep->condp()->isNeqZero()) {
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }

        m_hasLoopTest = સાચું;

        // If always breaks, subsequent statements are dead code, delete them
        જો (nodep->condp()->isZero()) {
            Ast_ગાંઠ* અચલ nextp = nodep->nextp();
            જો (nextp && canDeleteStatements()) {
                VL_DO_DANGLING(ધકેલો_Deletep(nextp->unlinkFrBackWithNext()), nodep);
            }
            પાછા;
        }

        જો (operandBoolShift(nodep->condp())) replaceBoolShift(nodep->condp());
    }

    શૂન્ય visit(AstConsArray* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstConsStruct* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstConsStructItem* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstConsArrayItem* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstUnbounded* nodep) override { iterateChildren(nodep); }
    // These are converted by V3Param.  Don't constify as we don't want the
    // from() VARREF to disappear, if any.
    // If output of a presel didn't get consted, chances are V3Param didn't visit properly
    શૂન્ય visit(Ast_ગાંઠ_Pre_Sel*) override {}

    // Ignored, can eliminate early
    શૂન્ય visit(AstSysIgnore* nodep) override {
        iterateChildren(nodep);
        જો (m_doNConst) VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        iterateChildren(nodep);
        // Malformed due to child being deleted, remove here
        જો (!nodep->exprp()) {
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        // Remove if expression is trivially pure
        જો (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), VarRef)) {
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        // Remove calls to empty functions
        જો (અચલ AstCCall* અચલ callp = VN_CAST(nodep->exprp(), CCall)) {
            અચલ AstCFunc* અચલ funcp = callp->funcp();
            જો (!callp->argsp() && funcp->emptyBody()) {
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
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
    શૂન્ય visit(AstBasicDType* nodep) override {
        iterateChildren(nodep);
        nodep->cvtRangeConst();
    }

    //-----
    // Jump elimination

    શૂન્ય visit(AstJumpGo* nodep) override {
        // Any statements following the JumpGo (at this statement level) never execute, delete
        જો (nodep->nextp() && canDeleteStatements()) {
            ધકેલો_Deletep(nodep->nextp()->unlinkFrBackWithNext());
        }

        // JumpGo as last statement in target JumpBlock (including last in a last sub-list),
        // is a no-op, remove it.
        જો (!nodep->nextp()) {
            માટે (Ast_ગાંઠ* abovep = nodep->abovep(); abovep; abovep = abovep->abovep()) {
                જો (abovep == nodep->blockp()) {
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                    પાછા;
                }
                // Stop if not doing expensive, or if the above node is not the last in its list,
                // ... or if it's not an 'if' TODO: it would be enough if it was not a branch.
                જો (!m_doExpensive || abovep->nextp() || !VN_IS(abovep, If)) તોડો;
            }
        }
        // Mark JumpBlock as used
        m_usedJumpBlocks.emplace(nodep->blockp());
        m_hasJumpDelay = સાચું;
    }

    શૂન્ય visit(AstJumpBlock* nodep) override {
        iterateChildren(nodep);

        // If first statement is an AstLoopTest, pull it before the jump block
        જો (AstLoopTest* અચલ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            nodep->addHereThisAsNext(testp->unlinkFrBack());
        }

        // Remove if empty
        જો (!nodep->stmtsp()) {
            UINFO(4, "JUMPLABEL => empty " << nodep);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }

        // If no JumpGo points to this node, replace it with its body
        જો (!m_usedJumpBlocks.ગણતરી(nodep)) {
            UINFO(4, "JUMPLABEL => unused " << nodep);
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }

    //-----
    // Below lines are magic expressions processed by astgen
    //  TREE_SKIP_VISIT("AstNODETYPE")    # Rename normal visit to visitGen and don't iterate
    //-----
    // clang-format off
    વૃક્ષ_SKIP_VISIT("ArraySel");
    વૃક્ષ_SKIP_VISIT("CAwait");

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
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        // Default: Just iterate
        જો (m_required) {
            જો (VN_IS(nodep, ગાંઠ_DType) || VN_IS(nodep, Range) || VN_IS(nodep, SliceSel)
                || VN_IS(nodep, Dot) || VN_IS(nodep, Text)) {
                // ignore
            } નહીંતર જો (VN_IS(nodep, Pattern) || VN_IS(nodep, PatMember)) {
                // A parameter override pattern is typed later, so only fold its members
                iterateChildren(nodep);
            } નહીંતર જો (AstCellRef* અચલ crp = VN_CAST(nodep, CellRef)) {
                iterate(crp->exprp());
                જો (Ast_ગાંઠ* અચલ newp = crp->exprp()) {
                    crp->replaceWithKeepDType(newp->unlinkFrBack());
                    VL_DO_DANGLING(ધકેલો_Deletep(crp), crp);
                }
                પાછા;
            } નહીંતર {
                nodep->v3error("Expecting expression to be constant, but can't convert a "
                               << nodep->prettyTypeName() << " to constant.");
            }
        } નહીંતર {
            જો (nodep->isTimingControl()) m_hasJumpDelay = સાચું;
            // Calculate the width of this operation
            જો (m_params && !nodep->width()) nodep = V3Width::widthParamsEdit(nodep);
            iterateChildren(nodep);
        }
    }

જાહેર:
    // Processing Mode Enum
    ગણના ProcMode : uint8_t {
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
    ConstVisitor(ProcMode pmode, તાર્કિક globalPass)
        : m_globalPass{globalPass}
        , m_concswapNames{globalPass ? ("__Vconcswap_" + cvtToStr(s_global_Pass_અંક++)) : ""} {
        // clang-format off
        પસંદગી (pmode) {
        સ્થિતિ PROC_PARAMS_NOWARN:  m_doV = સાચું;  m_doNConst = સાચું; m_params = સાચું;
                                  m_required = ખોટું; તોડો;
        સ્થિતિ PROC_PARAMS:         m_doV = સાચું;  m_doNConst = સાચું; m_params = સાચું;
                                  m_required = સાચું; તોડો;
        સ્થિતિ PROC_GENERATE:       m_doV = સાચું;  m_doNConst = સાચું; m_params = સાચું;
                                  m_required = સાચું; m_doGenerate = સાચું; તોડો;
        સ્થિતિ PROC_LIVE:           તોડો;
        સ્થિતિ PROC_V_WARN:         m_doV = સાચું;  m_doNConst = સાચું; m_warn = સાચું; m_convertLogicToBit = સાચું; તોડો;
        સ્થિતિ PROC_V_NOWARN:       m_doV = સાચું;  m_doNConst = સાચું; તોડો;
        સ્થિતિ PROC_V_EXPENSIVE:    m_doV = સાચું;  m_doNConst = સાચું; m_doExpensive = સાચું; તોડો;
        સ્થિતિ PROC_CPP:            m_doV = ખોટું; m_doNConst = સાચું; m_doCpp = સાચું; તોડો;
        મૂળભૂત:                  v3fatalSrc("Bad case"); તોડો;
        }
        // clang-format on
    }
    ~ConstVisitor() override {
        જો (m_doCpp) {
            જો (m_globalPass) {
                V3Stats::addStat("Optimizations, Const bit op reduction", m_statBitOpReduction);
            } નહીંતર {
                V3Stats::addStatSum("Optimizations, Const bit op reduction", m_statBitOpReduction);
            }
        }
        V3Stats::addStatSum("Optimizations, Cond redundant expressions", m_stat_Cond_પદાવલિ_Redundant);
        V3Stats::addStatSum("Optimizations, If cond redundant expressions",
                            m_stat_If_Cond_પદાવલિ_Redundant);
        V3Stats::addStatSum("Optimizations, Concat merges", m_statConcatMerge);
        V3Stats::addStatSum("Optimizations, Member access predicate node visits",
                            m_statMemberAccessVisits);
    }

    Ast_ગાંઠ* mainAcceptEdit(Ast_ગાંઠ* nodep) {
        VIsCached::clear_Cache_વૃક્ષ();  // Avoid using any stale isPure
        // Operate starting at a random place
        પાછા iterateSubtreeReturnEdits(nodep);
    }
};

uint32_t ConstVisitor::s_global_Pass_અંક = 0;

//######################################################################
// Const class functions

//! Force this cell node's parameter list to become a constant
શૂન્ય V3Const::constifyParamsEdit(Ast_ગાંઠ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS, /* globalPass: */ ખોટું};
    // cppcheck-suppress constVariablePointer // edited below
    જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        જો (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } નહીંતર {
        (શૂન્ય)visitor.mainAcceptEdit(nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump if have next line
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

//! Constify this cell node's parameter list if possible
શૂન્ય V3Const::constifyParamsNoWarnEdit(Ast_ગાંઠ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_PARAMS_NOWARN, /* globalPass: */ ખોટું};
    // cppcheck-suppress constVariablePointer // edited below
    જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        જો (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } નહીંતર {
        (શૂન્ય)visitor.mainAcceptEdit(nodep);
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
શૂન્ય V3Const::constifyGenerateParamsEdit(Ast_ગાંઠ* nodep) {
    // UINFOTREE(1, nodep, "", "forceConPRE:");
    // Resize even if the node already has a width, because buried in the tree
    // we may have a node we just created with signing, etc, that isn't sized
    // yet.

    // Make sure we've sized everything first
    nodep = V3Width::widthGenerateParamsEdit(nodep);
    ConstVisitor visitor{ConstVisitor::PROC_GENERATE, /* globalPass: */ ખોટું};
    // cppcheck-suppress constVariablePointer // edited below
    જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
        // If a var wants to be constified, it's really a param, and
        // we want the value to be constant.  We aren't passed just the
        // init value because we need widthing above to handle the var's type.
        જો (varp->valuep()) visitor.mainAcceptEdit(varp->valuep());
    } નહીંતર {
        VL_DO_DANGLING(visitor.mainAcceptEdit(nodep), nodep);
    }
    // Because we do edits, nodep links may get trashed and core dump this.
    // UINFOTREE(1, nodep, "", "forceConDONE");
}

શૂન્ય V3Const::constifyAllLint(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_WARN, /* globalPass: */ સાચું};
        (શૂન્ય)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("const", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3Const::constifyCpp(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ સાચું};
        (શૂન્ય)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("const_cpp", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

Ast_ગાંઠ* V3Const::constifyEdit(Ast_ગાંઠ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_NOWARN, /* globalPass: */ ખોટું};
    nodep = visitor.mainAcceptEdit(nodep);
    પાછા nodep;
}

Ast_ગાંઠ* V3Const::constifyEditCpp(Ast_ગાંઠ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_CPP, /* globalPass: */ ખોટું};
    nodep = visitor.mainAcceptEdit(nodep);
    પાછા nodep;
}

શૂન્ય V3Const::constifyAllLive(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    // This only pushes constants up, doesn't make any other edits
    // IE doesn't prune dead statements, as we need to do some usability checks after this
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_LIVE, /* globalPass: */ સાચું};
        (શૂન્ય)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("const", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3Const::constifyAll(AstNetlist* nodep) {
    // Only call from Verilator.cpp, as it uses user#'s
    UINFO(2, __FUNCTION__ << ":");
    {
        ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ સાચું};
        (શૂન્ય)visitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("const", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

Ast_ગાંઠ* V3Const::constifyExpensiveEdit(Ast_ગાંઠ* nodep) {
    ConstVisitor visitor{ConstVisitor::PROC_V_EXPENSIVE, /* globalPass: */ ખોટું};
    nodep = visitor.mainAcceptEdit(nodep);
    પાછા nodep;
}
